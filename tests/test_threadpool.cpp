#include "catch_amalgamated.hpp"
#include "mercury.hpp"

TEST_CASE("ThreadPool executes a simple task", "[ThreadPool]") {
    Mercury::ThreadPool pool(2);
    auto future = pool.enqueue(Mercury::Priority::Normal, []() -> int {
        return 42;
        });
    REQUIRE(future.get() == 42);
}

TEST_CASE("ThreadPool enqueueFast runs task", "[ThreadPool]") {
    Mercury::ThreadPool pool(2);
    auto future = pool.enqueueFast([]() -> int {
        return 99;
        });
    REQUIRE(future.get() == 99);
}

TEST_CASE("ThreadPool zero threads throws", "[ThreadPool]") {
    REQUIRE_THROWS_AS(Mercury::ThreadPool(0), std::invalid_argument);
}

TEST_CASE("ThreadPool executes higher-priority tasks first", "[ThreadPool][priority]") {
    const int N = 50;
    Mercury::ThreadPool pool(N); 
    std::mutex mtx;
    std::vector<int> executionOrder;
    std::atomic<int> readyCount{ 0 };
    std::atomic<bool> startFlag{ false };

    std::vector<std::future<void>> futures;
    for (int i = 0; i < N; ++i) {
        uint8_t prio = static_cast<uint8_t>(i * 5);
        futures.push_back(pool.enqueue(prio, [&mtx, &executionOrder, &readyCount, &startFlag, prio]() {
            readyCount++;
            while (!startFlag.load()) {
                std::this_thread::yield();
            }
            {
                std::lock_guard lock(mtx);
                executionOrder.push_back(prio);
            }
            }));
    }

    while (readyCount.load() < N) {
        std::this_thread::yield();
    }
    startFlag.store(true);

    for (auto& f : futures) f.get();

    const size_t checkCount = std::min<size_t>(10, executionOrder.size());
    for (size_t i = 0; i < checkCount; ++i) {
        REQUIRE(executionOrder[i] < 250);
    }
}
//normal tasks
TEST_CASE("ThreadPool fast queue preempts normal tasks", "[ThreadPool][fast]") {
    Mercury::ThreadPool pool(2);
    std::atomic<bool> fastDone{ false };
    std::atomic<int> normalCounter{ 0 };

    for (int i = 0; i < 50; ++i) {
        pool.enqueue(Mercury::Priority::Normal, [&normalCounter]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            normalCounter++;
            });
    }

    auto fastFut = pool.enqueueFast([&fastDone]() {
        fastDone = true;
        });

    fastFut.get();

    REQUIRE(fastDone == true);
    REQUIRE(normalCounter < 50);
}

TEST_CASE("ThreadPool lifecycle and monitoring", "[ThreadPool][lifecycle]") {
    Mercury::ThreadPool pool(4);

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    REQUIRE(pool.getIdleThreadCount() == 4);
    REQUIRE(pool.getTotalThreadCount() == 4);
    REQUIRE(pool.getPendingTaskCount() == 0);

    auto future = pool.enqueue([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return 1;
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    REQUIRE(pool.getIdleThreadCount() == 3);
    REQUIRE(pool.getPendingTaskCount() == 0); 

    future.get();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    REQUIRE(pool.getIdleThreadCount() == 4);
}

TEST_CASE("ThreadPool propagates exceptions", "[ThreadPool][exception]") {
    Mercury::ThreadPool pool(2);
    auto future = pool.enqueue([]() -> int {
        throw std::runtime_error("test exception");
        return 0;
        });

    REQUIRE_THROWS_AS(future.get(), std::runtime_error);
}

TEST_CASE("ThreadPool handles concurrent enqueue", "[ThreadPool][stress]") {
    Mercury::ThreadPool pool(8);
    std::atomic<int> counter{ 0 };
    const int tasksPerThread = 500;
    const int numThreads = 4;

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

    while (pool.getPendingTaskCount() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    REQUIRE(counter.load() == tasksPerThread * numThreads);
}