#include "catch_amalgamated.hpp"
#include "mercury.hpp"
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <string>

TEST_CASE("Signal calls slots in priority order (synchronous)", "[Signal]") {
    Mercury::Signal<int> signal;
    std::vector<int> callOrder;

    auto conn1 = signal.connect([&](int) { callOrder.push_back(1); }, Mercury::Priority::Low);
    auto conn2 = signal.connect([&](int) { callOrder.push_back(2); }, Mercury::Priority::Urgent);
    auto conn3 = signal.connect([&](int) { callOrder.push_back(3); }, Mercury::Priority::High);

    signal.emit(0);

    REQUIRE(callOrder.size() == 3);
    REQUIRE(callOrder[0] == 2);
    REQUIRE(callOrder[1] == 3);
    REQUIRE(callOrder[2] == 1);
}

TEST_CASE("Signal async executes all slots", "[Signal]") {
    Mercury::ThreadPool pool(4);
    Mercury::Signal<int> signal(pool);
    std::atomic<int> counter{ 0 };
    std::atomic<int> done{ 0 };
    const int numSlots = 3;

    auto conn1 = signal.connect([&](int) { counter++; done++; }, Mercury::Priority::Normal);
    auto conn2 = signal.connect([&](int) { counter++; done++; }, Mercury::Priority::Normal);
    auto conn3 = signal.connect([&](int) { counter++; done++; }, Mercury::Priority::Normal);

    signal.emit(0);

    auto start = std::chrono::steady_clock::now();
    while (done.load() < numSlots) {
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(2)) {
            FAIL("Timeout waiting for async slots");
        }
        std::this_thread::yield();
    }
    REQUIRE(counter.load() == numSlots);
}

TEST_CASE("Signal disconnect prevents slot from being called", "[Signal]") {
    Mercury::Signal<> signal;
    std::atomic<int> counter{ 0 };

    auto conn = signal.connect([&]() { counter++; });
    conn->disconnect();
    signal.emit();
    REQUIRE(counter.load() == 0);
}

TEST_CASE("Signal cleanup removes expired connections", "[Signal]") {
    Mercury::Signal<> signal;
    {
        auto conn = signal.connect([]() {});
        REQUIRE(signal.hasSubscribers());
    }
    signal.cleanup();
    REQUIRE_FALSE(signal.hasSubscribers());
}

TEST_CASE("Signal passes arguments correctly (synchronous)", "[Signal]") {
    Mercury::Signal<std::string, int> signal;
    std::string resultStr;
    int resultInt = 0;

    auto conn = signal.connect([&](std::string s, int i) {
        resultStr = s;
        resultInt = i;
        });

    signal.emit("hello", 42);
    REQUIRE(resultStr == "hello");
    REQUIRE(resultInt == 42);
}