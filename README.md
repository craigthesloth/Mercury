# Mercury v1.2 – Multithreaded Task Scheduler & Async Toolkit

A modern C++20 header-only library for parallel and asynchronous programming,
providing three integrated layers:

1. **ThreadPool** – a priority-aware thread pool with a fast channel for urgent tasks and built-in monitoring.
2. **Signal / Slot** – a type-safe, priority-ordered observer pattern that can deliver events synchronously or asynchronously via the thread pool.
3. **Coroutine Support** – C++20 coroutine primitives (`CoAwaitable`, `Task<R>`) that allow writing asynchronous code that suspends and resumes on the Mercury thread pool.

## Features

- Priority-based task execution (0-255, lower = higher priority).
- Fast queue for urgent tasks, bypassing the priority queue.
- Exception logging to timestamped files with automatic rethrow.
- Full move semantics and perfect forwarding.
- Thread-safe signal/slot with per-slot priority.
- C++20 coroutines: `co_await std::future<>` on the pool, `Task<R>` return type.
- Header-only – single `#include`, zero dependencies beyond the C++ standard library.

## Quick Start

```cpp
#include "Mercury.hpp"
using namespace Mercury;

// 1. Batch parallel execution with TaskController
TaskController<double> areaCalc(4);
for (auto& shape : shapes) {
    areaCalc.addTask(Priority::Normal, [&shape] { return shape.getArea(); });
}
auto areas = areaCalc.execute();  // std::vector<double>

// 2. Asynchronous events with Signal
ThreadPool pool(2);
Signal<std::string, double> onDone(pool, Priority::High);
auto conn = onDone.connect([](std::string name, double sum) {
    std::cout << "Chunk " << name << " done: " << sum << '\n';
});
onDone.emit("First", 500.0);

// WARNING: The slot remains connected only as long as the returned
// std::shared_ptr<Connection> is alive. If the shared_ptr is destroyed,
// the slot is automatically disconnected. For persistent connections,
// store the returned shared_ptr in a member variable.

// 3. Coroutine that waits for futures on the pool
Task<double> computeTotal(std::vector<Shape>& shapes, ThreadPool& pool) {
    double total = 0.0;
    for (auto& s : shapes) {
        auto fut = pool.enqueue(Priority::Normal, [&s]{ return s.getArea(); });
        total += co_await awaitable(std::move(fut), pool);
    }
    co_return total;
}
```

## Priority Levels

| Priority            | Value | Usage                  |
|---------------------|-------|------------------------|
| `Priority::Urgent`  | 0     | Real-time critical     |
| `Priority::VeryHigh`| 50    | High importance        |
| `Priority::High`    | 100   | Above normal           |
| `Priority::Normal`  | 150   | Default level          |
| `Priority::Low`     | 200   | Background tasks       |
| `Priority::VeryLow` | 250   | Nice-to-have           |
| `Priority::Deferred`| 255   | Lowest priority        |

## API Reference (simplified)

**ThreadPool**
- `explicit ThreadPool(size_t numThreads)`
- `auto enqueue(priority, callable, args...) -> std::future<ReturnType>`
- `auto enqueueFast(callable, args...) -> std::future<ReturnType>`
- `size_t getPendingTaskCount() const`
- `size_t getIdleThreadCount() const`
- `size_t getTotalThreadCount() const`

**TaskController<ReturnType>**
- `explicit TaskController(size_t numThreads)`
- `void addTask(priority, std::function<ReturnType()>)`
- `std::vector<ReturnType> execute()`
- `std::future<ReturnType> submitFast(std::function<ReturnType()>)`

**Signal<Args...>**
- `Signal()` – synchronous
- `Signal(ThreadPool&, priority)` – asynchronous
- `std::shared_ptr<Connection> connect(SlotType, priority)`
- `void emit(Args...)`

**Coroutines (C++20)**
- `template<typename R> struct Task<R>` – coroutine return type
- `template<typename R> CoAwaitable<R> awaitable(std::future<R>, ThreadPool&)`

## Thread Safety

All public methods are thread-safe unless noted otherwise. Adding tasks from multiple threads is safe; calling `execute()` concurrently from several threads is not recommended (it will block each caller).

## Exception Handling

Exceptions thrown by tasks are caught and logged to `"error_YYYYMMDD_HHMMSS_ms.log"`. The first exception is rethrown after all tasks have completed (or after the first batch error).

## Building Tests

```bash
cmake -B build -DMERCURY_BUILD_TESTS=ON
cmake --build build
cd build && ctest
```

## License

MIT © P.Sobin

Email:  craigthesloth@gmail.com

GitHub: [https://github.com/craigthesloth/Mercury](https://github.com/craigthesloth/Mercury)
