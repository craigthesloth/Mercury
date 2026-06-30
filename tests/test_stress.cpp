#include "catch_amalgamated.hpp"
#include "mercury.hpp"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <random>

//Many snall jobs
TEST_CASE("Stress: many small tasks via enqueue", "[stress]") {
    Mercury::ThreadPool pool(std::thread::hardware_concurrency());
    std::atomic<int> counter{ 0 };
    const int numTasks = 10000;

    for (int i = 0; i < numTasks; ++i) {
        pool.enqueue([&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
            });
    }

    // wait till all done
    while (pool.getPendingTaskCount() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    REQUIRE(counter.load() == numTasks);
}

TEST_CASE("Stress: concurrent enqueue from multiple threads", "[stress]") {
    Mercury::ThreadPool pool(std::thread::hardware_concurrency());
    std::atomic<int> counter{ 0 };
    const int tasksPerThread = 1000;
    const int numThreads = 8;

    auto worker = [&]() {
        for (int i = 0; i < tasksPerThread; ++i) {
            pool.enqueue([&counter]() {
                counter.fetch_add(1, std::memory_order_relaxed);
                });
        }
        };

    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(worker);
    }
    for (auto& t : threads) t.join();

    // wait till all done
    while (pool.getPendingTaskCount() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    REQUIRE(counter.load() == tasksPerThread * numThreads);
}

// fast create & destroy
TEST_CASE("Stress: rapid pool creation and destruction", "[stress]") {
    for (int i = 0; i < 50; ++i) {
        Mercury::ThreadPool pool(4);
        pool.enqueue([]() { std::this_thread::sleep_for(std::chrono::milliseconds(1)); });
        pool.enqueue([]() { std::this_thread::sleep_for(std::chrono::milliseconds(1)); });
        // destructor is automatical
    }
    //passed if nothing is dropped dead or stuck
    REQUIRE(true);
}

// mixed: cpu-bound and small tasks
TEST_CASE("Stress: mixed CPU-bound and fast tasks", "[stress]") {
    Mercury::ThreadPool pool(4);
    std::atomic<int> counter{ 0 };
    const int cpuTasks = 100;
    const int fastTasks = 500;

    // heavy tasks
    for (int i = 0; i < cpuTasks; ++i) {
        pool.enqueue(Mercury::Priority::Low, [&counter]() {
            double x = 0.0;
            for (int j = 0; j < 10000; ++j) x += std::sin(static_cast<double>(j));
            counter.fetch_add(1, std::memory_order_relaxed);
            });
    }

    // priritized small tasks
    for (int i = 0; i < fastTasks; ++i) {
        pool.enqueue(Mercury::Priority::Urgent, [&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
            });
    }

    while (pool.getPendingTaskCount() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    REQUIRE(counter.load() == cpuTasks + fastTasks);
}

// many sleeping tasks
TEST_CASE("Stress: many sleeping tasks", "[stress]") {
    Mercury::ThreadPool pool(8);
    std::atomic<int> counter{ 0 };
    const int tasks = 200;
    const int sleepMs = 5;

    for (int i = 0; i < tasks; ++i) {
        pool.enqueue([&counter, sleepMs]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
            counter.fetch_add(1, std::memory_order_relaxed);
            });
    }

    // awaiting 
    auto start = std::chrono::steady_clock::now();
    while (counter.load() < tasks) {
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(5)) {
            FAIL("Timeout waiting for sleeping tasks");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    REQUIRE(counter.load() == tasks);
}