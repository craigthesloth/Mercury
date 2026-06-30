#include "catch_amalgamated.hpp"

#if MERCURY_HAS_COROUTINES
#include "mercury.hpp"
#include <thread>
#include <chrono>
#include <atomic>

//simpliest coroutine
Mercury::Task<int> simpleTask(int value) {
    co_return value;
}

//coroutine co_awaits future
Mercury::Task<int> awaitFutureTask(std::future<int> fut, Mercury::ThreadPool& pool) {
    int result = co_await Mercury::awaitable(std::move(fut), pool);
    co_return result;
}

//croutine with exception
Mercury::Task<int> throwingTask() {
    throw std::runtime_error("coroutine error");
    co_return 0;
}

//void couroutine 
Mercury::Task<void> voidTask(std::atomic<int>& counter) {
    counter++;
    co_return;
}

TEST_CASE("CoAwaitable with ready future returns immediately", "[Coroutine]") {
    Mercury::ThreadPool pool(1);
    std::promise<int> p;
    p.set_value(42);
    std::future<int> fut = p.get_future();

    auto task = awaitFutureTask(std::move(fut), pool);
    REQUIRE(task.get() == 42);
}

TEST_CASE("CoAwaitable suspends and resumes when future becomes ready", "[Coroutine]") {
    Mercury::ThreadPool pool(1);
    std::promise<int> p;
    std::future<int> fut = p.get_future();

    auto task = awaitFutureTask(std::move(fut), pool);

    //give it some time
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // ake future ready
    p.set_value(99);

    REQUIRE(task.get() == 99);
}

TEST_CASE("Task<int> returns correct value", "[Coroutine]") {
    auto task = simpleTask(7);
    REQUIRE(task.get() == 7);
}

TEST_CASE("Task<void> executes without error", "[Coroutine]") {
    std::atomic<int> counter{ 0 };
    auto task = voidTask(counter);
    task.get();
    REQUIRE(counter.load() == 1);
}

TEST_CASE("Task<int> propagates exception", "[Coroutine]") {
    auto task = throwingTask();
    REQUIRE_THROWS_AS(task.get(), std::runtime_error);
}

#else
TEST_CASE("Coroutines are not supported by this compiler", "[Coroutine]") {
    SUCCEED("Coroutines not available, skipping tests.");
}
#endif // MERCURY_HAS_COROUTINES