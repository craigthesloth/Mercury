#include <filesystem>
#include <fstream>
#include <string>
#include <algorithm>
#include "catch_amalgamated.hpp"
#include "mercury.hpp"


TEST_CASE("TaskController logs exceptions to file", "[TaskController][exception]") {
    namespace fs = std::filesystem;
    for (const auto& entry : fs::directory_iterator(fs::current_path())) {
        if (entry.is_regular_file() && entry.path().filename().string().find("error_") == 0) {
            try { fs::remove(entry.path()); }
            catch (...) {}
        }
    }

    Mercury::TaskController<int> ctrl(2);
    ctrl.addTask(1, []() -> int { throw std::runtime_error("Test log message 42"); });
    REQUIRE_THROWS_AS(ctrl.execute(), std::runtime_error);

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
            break;
        }
    }

    REQUIRE(found);
    REQUIRE(logContent.find("Test log message 42") != std::string::npos);
}