#include <filesystem>
#include <fstream>
#include <string>
#include <algorithm>
#include "catch_amalgamated.hpp"
#include "mercury.hpp"


TEST_CASE("TaskController executes all tasks and collects results", "[TaskController]") {
    Mercury::TaskController<int> ctrl(4);
    ctrl.addTask(Mercury::Priority::Normal, [] { return 10; });
    ctrl.addTask(Mercury::Priority::Normal, [] { return 20; });
    ctrl.addTask(Mercury::Priority::Normal, [] { return 30; });

    auto results = ctrl.execute();
    REQUIRE(results.size() == 3);
    REQUIRE(results[0] == 10);
    REQUIRE(results[1] == 20);
    REQUIRE(results[2] == 30);
}
static void cleanErrorLogs() {
    namespace fs = std::filesystem;
    for (const auto& entry : fs::directory_iterator(fs::current_path())) {
        if (entry.is_regular_file() && entry.path().filename().string().find("error_") == 0) {
            fs::remove(entry.path());
        }
    }
}

TEST_CASE("TaskController logs exceptions to file", "[TaskController][exception]") {
    cleanErrorLogs();

    Mercury::TaskController<int> ctrl(2);
    ctrl.addTask(1, []() -> int { throw std::runtime_error("Test log message 42"); });
    REQUIRE_THROWS_AS(ctrl.execute(), std::runtime_error);

    namespace fs = std::filesystem;
    std::string logContent;
    bool found = false;
    for (const auto& entry : fs::directory_iterator(fs::current_path())) {
        if (entry.is_regular_file() && entry.path().filename().string().find("error_") == 0) {
            {
                std::ifstream ifs(entry.path());
                REQUIRE(ifs.is_open());
                std::string line;
                while (std::getline(ifs, line)) {
                    logContent += line + "\n";
                }
               
            }
            found = true;
            REQUIRE(logContent.find("Test log message 42") != std::string::npos);
            fs::remove(entry.path()); // now it is safe
            break;
        }
    }
    REQUIRE(found);
}

TEST_CASE("TaskController clearPendingTasks", "[TaskController]") {
    Mercury::TaskController<int> ctrl(2);
    ctrl.addTask(1, [] { return 1; });
    ctrl.addTask(1, [] { return 2; });
    ctrl.clearPendingTasks();
    auto results = ctrl.execute();
    REQUIRE(results.empty());
}

TEST_CASE("TaskController submitFast returns future", "[TaskController]") {
    Mercury::TaskController<int> ctrl(2);
    auto future = ctrl.submitFast([] { return 42; });
    REQUIRE(future.get() == 42);
}