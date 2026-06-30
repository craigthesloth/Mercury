#pragma once
/**
 * ╔══════════════════════════════════════════════════════════════════════════╗
 * ║                          Mercury v1.2                                    ║
 * ║              Multithreaded Task Scheduler & Async Toolkit                ║
 * ╚══════════════════════════════════════════════════════════════════════════╝
 *
 * A modern C++20 header-only library for parallel and asynchronous programming,
 * providing three integrated layers:
 *
 *   1.  ThreadPool – a work-stealing, priority-aware thread pool with a fast
 *       channel for urgent tasks and built-in monitoring.
 *
 *   2.  Signal / Slot – a type-safe, priority-ordered observer pattern that can
 *       deliver events synchronously or asynchronously via the thread pool.
 *
 *   3.  Coroutine Support – C++20 coroutine primitives (CoAwaitable, Task<R>)
 *       that allow writing asynchronous code that suspends and resumes on the
 *       Mercury thread pool.
 *
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *  FEATURES
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *  - Priority-based task execution (0-255, lower = higher priority).
 *  - Fast queue for urgent tasks, bypassing the priority queue.
 *  - Exception logging to timestamped files with automatic rethrow.
 *  - Full move semantics and perfect forwarding.
 *  - Thread-safe signal/slot with per-slot priority.
 *  - C++20 coroutines: co_await std::future<> on the pool, Task<R> return type.
 *  - Header-only – single #include, zero dependencies beyond the C++ standard library.
 *
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *  QUICK START
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *
 *   #include "Mercury.hpp"
 *   using namespace Mercury;
 *
 *   // 1. Batch parallel execution with TaskController
 *   TaskController<double> areaCalc(4);
 *   for (auto& shape : shapes) {
 *       areaCalc.addTask(Priority::Normal, [&shape] { return shape.getArea(); });
 *   }
 *   auto areas = areaCalc.execute();  // std::vector<double>
 *
 *   // 2. Asynchronous events with Signal
 *   ThreadPool pool(2);
 *   Signal<std::string, double> onDone(pool, Priority::High);
 *   onDone.connect([](std::string name, double sum) {
 *       std::cout << "Chunk " << name << " done: " << sum << '\n';
 *   });
 *   onDone.emit("First", 500.0);
 *   
 *   WARN
 *   The slot remains connected only as long as the returned std::shared_ptr<Connection>
 *   is alive.If the shared_ptr is destroyed, the slot is automatically disconnected.
 *   For persistent connections, store the returned shared_ptr in a member variable.
 *
 *   // 3. Coroutine that waits for futures on the pool
 *   Task<double> computeTotal(std::vector<Shape>& shapes, ThreadPool& pool) {
 *       double total = 0.0;
 *       for (auto& s : shapes) {
 *           auto fut = pool.enqueue(Priority::Normal, [&s]{ return s.getArea(); });
 *           total += co_await awaitable(std::move(fut), pool);
 *       }
 *       co_return total;
 *   }
 *
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *  PRIORITY LEVELS
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *  Priority::Urgent    (0)   - Real-time critical
 *  Priority::VeryHigh  (50)  - High importance
 *  Priority::High      (100) - Above normal
 *  Priority::Normal    (150) - Default level
 *  Priority::Low       (200) - Background tasks
 *  Priority::VeryLow   (250) - Nice-to-have
 *  Priority::Deferred  (255) - Lowest priority
 *
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *  API REFERENCE (simplified)
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *
 *  class ThreadPool:
 *      explicit ThreadPool(size_t numThreads)
 *      auto enqueue(priority, callable, args...) -> std::future<ReturnType>
 *      auto enqueueFast(callable, args...) -> std::future<ReturnType>
 *      size_t getPendingTaskCount() const
 *      size_t getIdleThreadCount() const
 *      size_t getTotalThreadCount() const
 *
 *  template<typename ReturnType> class TaskController:
 *      explicit TaskController(size_t numThreads)
 *      void addTask(priority, std::function<ReturnType()>)
 *      std::vector<ReturnType> execute()
 *      std::future<ReturnType> submitFast(std::function<ReturnType()>)
 *
 *  template<typename... Args> class Signal:
 *      Signal()                           // synchronous
 *      Signal(ThreadPool&, priority)      // asynchronous
 *      std::shared_ptr<Connection> connect(SlotType, priority)
 *      void emit(Args...)
 *
 *  // Coroutine support (C++20)
 *  template<typename R> struct Task<R>       // coroutine return type
 *  template<typename R> CoAwaitable<R> awaitable(std::future<R>, ThreadPool&)
 *
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *  THREAD SAFETY
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *  All public methods are thread-safe unless noted otherwise. Adding tasks
 *  from multiple threads is safe; calling execute() concurrently from
 *  several threads is not recommended (it will block each caller).
 *
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *  EXCEPTION HANDLING
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *  Exceptions thrown by tasks are caught and logged to
 *  "error_YYYYMMDD_HHMMSS_ms.log". The first exception is rethrown after
 *  all tasks have completed (or after the first batch error).
 *
 * ╔══════════════════════════════════════════════════════════════════════════╗
 * ║  License: MIT (feel free to use, modify, and distribute)                 ║
 * ║  Author:  P.Sobin                                                        ║
 * ║  Email:   craigthesloth@gmail.com                                        ║
 * ║  GitHub:  !EDIT AFTER PUSH!                                              ║
 * ╚══════════════════════════════════════════════════════════════════════════╝
 */

#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <future>
#include <functional>
#include <condition_variable>
#include <atomic>
#include <type_traits>
#include <cstdint>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>

 // C++20 coroutine support
#if defined(__cpp_impl_coroutine) && __has_include(<coroutine>)
#include <coroutine>
#define MERCURY_HAS_COROUTINES 1
#else
#define MERCURY_HAS_COROUTINES 0
#endif


namespace Mercury {
    inline namespace v1 {

        // PRIORITY ENUM
        enum class Priority : uint8_t {
            Urgent = 0,
            VeryHigh = 50,
            High = 100,
            Normal = 150,
            Low = 200,
            VeryLow = 250,
            Deferred = 255
        };

        // EXCEPTION LOGGER
        class ExceptionLogger {
        public:
            static void logException(const std::exception_ptr& ex_ptr) {
                static std::mutex mtx;
                std::lock_guard lock(mtx);

                auto timestamp = getCurrentTimestamp();
                std::string filename = "error_" + timestamp + ".log";

                std::ofstream logFile(filename, std::ios::app);
                if (!logFile.is_open()) {
                    std::cerr << "[Mercury::ExceptionLogger] Failed to open log file: " << filename << "\n";
                    return;
                }

                logFile << "=== Exception Report ===\n";
                logFile << "Timestamp: " << timestamp << "\n";

                try {
                    if (ex_ptr) std::rethrow_exception(ex_ptr);
                }
                catch (const std::exception& e) {
                    logFile << "Exception Type: " << typeid(e).name() << "\n";
                    logFile << "Message: " << e.what() << "\n";
                }
                catch (...) {
                    logFile << "Exception Type: Unknown\n";
                    logFile << "Message: Unknown exception occurred\n";
                }

                logFile << "========================\n\n";
                logFile.flush();
            }

        private:
            static std::string getCurrentTimestamp() {
                auto now = std::chrono::system_clock::now();
                auto time = std::chrono::system_clock::to_time_t(now);
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()) % 1000;

                std::tm tm{};
#if defined(_WIN32) || defined(_MSC_VER)
                localtime_s(&tm, &time);
#else
                localtime_r(&time, &tm);
#endif
                std::stringstream ss;
                ss << std::put_time(&tm, "%Y%m%d_%H%M%S");
                ss << "_" << std::setfill('0') << std::setw(3) << ms.count();
                return ss.str();
            }
        };

        // THREAD POOL
        struct PrioritizedTask {
            uint8_t priority;
            std::function<void()> func;
            bool operator<(const PrioritizedTask& other) const { return priority > other.priority; }
        };

        class ThreadPool {
        public:
            /**
             * @brief Constructs a thread pool with a fixed number of worker threads.
             * @param num Number of threads (must be > 0).
             * @throws std::invalid_argument if num == 0.
             */
            explicit ThreadPool(size_t num) : m_numThreads(num) {
                if (num == 0) {
                    throw std::invalid_argument("ThreadPool requires at least 1 thread");
                }
                for (size_t i = 0; i < num; ++i) {
                    m_threads.emplace_back([this] {
                        while (true) {
                            std::function<void()> task;
                            {
                                std::unique_lock lock(m_mutex);

                                if (!m_fastQueue.empty()) {
                                    task = std::move(m_fastQueue.front());
                                    m_fastQueue.pop();
                                }
                                else {
                                    ++m_idleThreads;
                                    m_condition.wait(lock, [this] {
                                        return m_stop || !m_tasks.empty() || !m_fastQueue.empty();
                                        });
                                    --m_idleThreads;

                                    if (m_stop && m_tasks.empty() && m_fastQueue.empty())
                                        return;

                                    if (!m_fastQueue.empty()) {
                                        task = std::move(m_fastQueue.front());
                                        m_fastQueue.pop();
                                    }
                                    else if (!m_tasks.empty()) {
                                        task = std::move(m_tasks.top().func);
                                        m_tasks.pop();
                                    }
                                }
                            }
                            if (task) task();
                        }
                        });
                }
            }

            ~ThreadPool() noexcept {
                {
                    std::lock_guard lock(m_mutex);
                    m_stop = true;
                }
                m_condition.notify_all();
                for (auto& t : m_threads) {
                    if (t.joinable()) t.join();
                }
            }

            ThreadPool(const ThreadPool&) = delete;
            ThreadPool& operator=(const ThreadPool&) = delete;
            ThreadPool(ThreadPool&&) = delete;
            ThreadPool& operator=(ThreadPool&&) = delete;


            /**
            * @brief Enqueues a callable with a given priority.
            * @tparam Func Callable type.
            * @tparam Args Argument types.
            * @param priority Task priority (0 = highest, 255 = lowest).
            * @param f Callable object.
            * @param args Arguments to forward to the callable.
            * @return std::future<R> where R is the return type of f.
            *
            * The task is executed by an idle worker thread, respecting priority order.
            */
            template<typename Func, typename... Args>
            auto enqueue(uint8_t priority, Func&& f, Args&&... args)
                -> std::future<std::invoke_result_t<Func, Args...>>
            {
                using ReturnType = std::invoke_result_t<Func, Args...>;
                auto task = std::make_shared<std::packaged_task<ReturnType()>>(
                    [f = std::forward<Func>(f), ...args = std::forward<Args>(args)]() mutable {
                        return std::invoke(f, args...);
                    }
                );
                std::future<ReturnType> result = task->get_future();
                {
                    std::lock_guard lock(m_mutex);
                    m_tasks.emplace(PrioritizedTask{ priority, [task] { (*task)(); } });
                }
                m_condition.notify_one();
                return result;
            }

            template<typename Func, typename... Args>
            auto enqueue(Func&& f, Args&&... args)
                -> std::future<std::invoke_result_t<Func, Args...>>
            {
                return enqueue(static_cast<uint8_t>(Priority::Normal),
                    std::forward<Func>(f), std::forward<Args>(args)...);
            }

            template<typename Func, typename... Args>
            auto enqueue(Priority p,Func&& f, Args&&... args)
                -> std::future<std::invoke_result_t<Func, Args...>>
            {
                return enqueue(static_cast<uint8_t>(p),
                    std::forward<Func>(f), std::forward<Args>(args)...);
            }


            /**
            * @brief Enqueues a callable into the fast lane, bypassing the priority queue.
            * @details If a worker is idle, the task runs immediately; otherwise it waits
            *          at the front of the fast queue.
            * @return std::future<R> for the task's result.
            */
            template<typename Func, typename... Args>
            auto enqueueFast(Func&& f, Args&&... args)
                -> std::future<std::invoke_result_t<Func, Args...>>
            {
                using ReturnType = std::invoke_result_t<Func, Args...>;
                auto task = std::make_shared<std::packaged_task<ReturnType()>>(
                    [f = std::forward<Func>(f), ...args = std::forward<Args>(args)]() mutable {
                        return std::invoke(f, args...);
                    }
                );
                std::future<ReturnType> result = task->get_future();

                {
                    std::lock_guard lock(m_mutex);
                    m_fastQueue.push([task] { (*task)(); });
                }
                m_condition.notify_one();
                return result;
            }

            size_t getPendingTaskCount() const {
                std::lock_guard lock(m_mutex);
                return m_tasks.size() + m_fastQueue.size();
            }

            size_t getIdleThreadCount() const {
                std::lock_guard lock(m_mutex);
                return m_idleThreads;
            }

            size_t getTotalThreadCount() const {
                return m_numThreads;
            }

        private:
            std::vector<std::thread> m_threads;
            std::priority_queue<PrioritizedTask> m_tasks;
            std::queue<std::function<void()>> m_fastQueue;
            mutable std::mutex m_mutex;
            std::condition_variable m_condition;
            std::atomic<bool> m_stop{ false };
            size_t m_idleThreads = 0;
            size_t m_numThreads = 0;
        };

        // TASK ITEM
        template<typename ReturnType>
        class TaskItem {
        public:
            TaskItem(uint8_t priority, std::function<ReturnType()> func)
                : m_priority(priority), m_func(std::move(func)) {
            }

            uint8_t getPriority() const { return m_priority; }
            std::function<ReturnType()> getFunction() const& { return m_func; }
            std::function<ReturnType()> getFunction()&& { return std::move(m_func); }

        private:
            uint8_t m_priority;
            std::function<ReturnType()> m_func;
        };

        // TASK ITEN CONTROLLER (typological)
        template<typename ReturnType>
        class TaskController {
        public:
            explicit TaskController(size_t numThreads)
                : m_threadPool(std::make_unique<ThreadPool>(numThreads)) {
            }

            ~TaskController() noexcept = default;


            /**
             * @brief Adds a task to the controller's internal queue.
             * @param priority Task priority (lower = more urgent).
             * @param func A callable that returns ReturnType.
             *
             * The task is not executed until execute() is called.
             */
            void addTask(uint8_t priority, std::function<ReturnType()> func) {
                m_tasks.emplace_back(priority, std::move(func));
            }

            void addTask(Priority priority, std::function<ReturnType()> func) {
                addTask(static_cast<uint8_t>(priority), std::move(func));
            }


            /**
             * @brief Runs all queued tasks in parallel and collects their results.
             * @return std::vector<ReturnType> Results in the order tasks were added.
             * @throws The first exception thrown by any task, after all tasks have finished.
             *
             * Exceptions are logged via ExceptionLogger. The internal task queue is cleared
             * after execution.
             */
            std::vector<ReturnType> execute() {
                if (m_tasks.empty()) return {};

                std::vector<std::future<ReturnType>> futures;
                futures.reserve(m_tasks.size());

                for (auto& task : m_tasks) {
                    try {
                        futures.emplace_back(
                            m_threadPool->enqueue(task.getPriority(), std::move(task).getFunction())
                        );
                    }
                    catch (const std::exception&) {
                        ExceptionLogger::logException(std::current_exception());
                        throw;
                    }
                }

                std::vector<ReturnType> results;
                results.reserve(futures.size());
                std::exception_ptr first_exception;

                for (auto& f : futures) {
                    try {
                        results.push_back(f.get());
                    }
                    catch (...) {
                        if (!first_exception) {
                            first_exception = std::current_exception();
                            ExceptionLogger::logException(first_exception);
                        }
                    }
                }

                m_tasks.clear();
                if (first_exception) std::rethrow_exception(first_exception);
                return results;
            }



            /**
             * @brief Submits a single function for immediate execution on an available worker.
             * @param func A callable returning ReturnType.
             * @return std::future<ReturnType> that will hold the result.
             *
             * The task bypasses the priority queue and uses the fast lane of the underlying
             * ThreadPool.
             */
            std::future<ReturnType> submitFast(std::function<ReturnType()> func) {
                return m_threadPool->enqueueFast(std::move(func));
            }

            size_t getPendingTaskCount() const {
                return m_threadPool->getPendingTaskCount() + m_tasks.size();
            }

            bool isEmpty() const {
                return m_tasks.empty() && m_threadPool->getPendingTaskCount() == 0;
            }

            void clearPendingTasks() {
                m_tasks.clear();
            }

        private:
            std::unique_ptr<ThreadPool> m_threadPool;
            std::vector<TaskItem<ReturnType>> m_tasks;
        };

        // SIGNAL with per-slot priority (sync/async)
        template<typename... Args>
        class Signal {
        public:
            using SlotType = std::function<void(Args...)>;


            /**
             * @brief Creates a Signal that can deliver events to subscribers.
             * @param pool ThreadPool to use for asynchronous delivery (optional).
             * @param defaultPriority Default priority for slots if not specified.
             *
             * If no pool is provided, the signal operates in synchronous mode: slots are
             * called directly in emit().
             */
            Signal() = default;
            Signal(ThreadPool& pool, uint8_t defaultPriority = static_cast<uint8_t>(Priority::Normal))
                : m_pool(pool), m_defaultPriority(defaultPriority) {
            }

            Signal(ThreadPool& pool, Priority priority)
                : Signal(pool, static_cast<uint8_t>(priority)) {
            }


            struct Connection {
                explicit Connection(Signal* sig) : signal(sig) {}
                void disconnect() { disconnected.store(true); }
                bool isDisconnected() const { return disconnected.load(); }
            private:
                std::atomic<bool> disconnected{ false };
                Signal* signal;
            };


            /**
             * @brief Subscribes a slot with a given priority.
             * @param slot Callable to invoke when the signal is emitted.
             * @param priority Slot priority (lower = called earlier).
             * @return std::shared_ptr<Connection> that can be used to disconnect.
             *
             * Slots with the same priority are called in the order they were connected.
             */
            /**@note The slot remains connected only as long as the returned std::shared_ptr<Connection>
            * is alive.If the shared_ptr is destroyed, the slot is automatically disconnected.
            * For persistent connections, store the returned shared_ptr in a member variable.
            */
            std::shared_ptr<Connection> connect(SlotType slot,
                uint8_t priority = static_cast<uint8_t>(Priority::Normal)) {
                auto conn = std::make_shared<Connection>(this);
                std::lock_guard lock(m_mutex);
                m_slots.emplace_back(priority, std::move(slot), conn);
                return conn;
            }


            std::shared_ptr<Connection> connect(SlotType slot, Priority priority) {
                return connect(std::move(slot), static_cast<uint8_t>(priority));
            }


            /**
             * @brief Emits the signal, calling all active slots.
             * @param args Arguments to pass to each slot.
             *
             * In synchronous mode, slots are called sequentially in priority order.
             * In asynchronous mode, each slot is scheduled as a task in the associated
             * ThreadPool with its own priority.
             */
            void emit(Args... args) const {
                //cобираем активные слоты
                std::vector<std::tuple<uint8_t, SlotType, std::weak_ptr<Connection>>> active;
                {
                    std::unique_lock lock(m_mutex);
                    active.reserve(m_slots.size());
                    for (const auto& [prio, slot, weak_conn] : m_slots) {
                        active.emplace_back(prio, slot, weak_conn);
                    }
                }

                // sort the lesser the higher
                std::sort(active.begin(), active.end(), [](const auto& a, const auto& b) {
                    return std::get<0>(a) < std::get<0>(b);
                    });

                if (m_pool) {
                    // shared_ptr for security
                    auto shared_args = std::make_shared<std::tuple<Args...>>(args...);
                    for (auto& [prio, slot, weak_conn] : active) {
                        auto conn = weak_conn.lock();
                        if (!conn || conn->isDisconnected()) continue;
                        m_pool->get().enqueue(prio, [slot, shared_args] {
                            std::apply(slot, *shared_args);
                            });
                    }
                }
                else {
                    //straight call for slots
                    for (auto& [prio, slot, weak_conn] : active) {
                        auto conn = weak_conn.lock();
                        if (!conn || conn->isDisconnected()) continue;
                        slot(args...);
                    }
                }
            }


            void cleanup() {
                std::lock_guard lock(m_mutex);
                std::erase_if(m_slots, [](const auto& t) {
                    return std::get<2>(t).expired();
                    });
            }

            bool hasSubscribers() const {
                std::lock_guard lock(m_mutex);
                return std::any_of(m_slots.begin(), m_slots.end(), [](const auto& t) {
                    return !std::get<2>(t).expired();
                    });
            }

        private:

            std::optional<std::reference_wrapper<ThreadPool>> m_pool;
            uint8_t m_defaultPriority = static_cast<uint8_t>(Priority::Normal);
            mutable std::mutex m_mutex;
            mutable std::vector<std::tuple<uint8_t, SlotType, std::weak_ptr<Connection>>> m_slots;
        };


#if MERCURY_HAS_COROUTINES
        // COROUTINE SUPPORT (C++20)
        template<typename ReturnType>
        struct CoWaitable {
            std::future<ReturnType> future;
            ThreadPool* pool{ nullptr };

            bool await_ready() const noexcept {
                return future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready;
            }

            void await_suspend(std::coroutine_handle<> handle) noexcept {
                if (future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                    return;
                }

                std::thread([this, handle]() {
                    future.wait();
                    handle.resume();
                    }).detach();
            }

            ReturnType await_resume() {
                return future.get();
            }
        };


        /**
         * @brief Creates an awaitable wrapper for a std::future that resumes on the given ThreadPool.
         * @details The coroutine is suspended and a background thread waits for the future.
         *          Once the future is ready, the coroutine is resumed (currently on a separate thread).
         * @param future The std::future to await.
         * @param pool   The ThreadPool (reserved for future use where resumption could happen on the pool).
         * @return CoAwaitable<R> object that can be co_awaited.
         *
         * Usage:
         *   auto result = co_await Mercury::awaitable(pool.enqueue(...), pool);
         */
        template<typename ReturnType>
        CoWaitable<ReturnType> awaitable(std::future<ReturnType>&& future, ThreadPool& pool) {
            return CoWaitable<ReturnType>{std::move(future), & pool};
        }

        template<>
        struct CoWaitable<void> {
            std::future<void> future;
            ThreadPool* pool{ nullptr };

            bool await_ready() const noexcept {
                return future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready;
            }

            void await_suspend(std::coroutine_handle<> handle) noexcept {
                if (future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                    return;
                }

                std::thread([this, handle]() {
                    future.wait();
                    handle.resume();
                    }).detach();
            }


            void await_resume() { future.get(); }
        };

        inline CoWaitable<void> awaitable(std::future<void>&& future, ThreadPool& pool) {
            return CoWaitable<void>{std::move(future), & pool};
        }


        /**
         * @brief A C++20 coroutine return type that represents an asynchronous task.
         * @tparam ReturnType The type of the value produced by the coroutine.
         *
         * The coroutine starts suspended (suspend_always) and must be explicitly resumed
         * (or awaited). The result can be retrieved via get() or by co_awaiting the Task.
         */
        template<typename ReturnType>
        struct Task {
            struct promise_type {
                std::suspend_always initial_suspend() noexcept { return {}; }
                std::suspend_always final_suspend() noexcept { return {}; }
                Task get_return_object() {
                    return Task{ std::coroutine_handle<promise_type>::from_promise(*this) };
                }
                void return_value(ReturnType value) { result = value; }
                void unhandled_exception() { exception = std::current_exception(); }
                ReturnType result{};
                std::exception_ptr exception;
            };

            using handle_type = std::coroutine_handle<promise_type>;

            Task() = default;
            explicit Task(handle_type h) : handle(h) {}
            ~Task() { if (handle) handle.destroy(); }
            Task(const Task&) = delete;
            Task& operator=(const Task&) = delete;
            Task(Task&& other) noexcept : handle(other.handle) { other.handle = nullptr; }
            Task& operator=(Task&& other) noexcept {
                if (this != &other) { if (handle) handle.destroy(); handle = other.handle; other.handle = nullptr; }
                return *this;
            }

            ReturnType get() {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (!handle.done()) handle.resume();
                if (handle.promise().exception) std::rethrow_exception(handle.promise().exception);
                return handle.promise().result;
            }

            bool await_ready() noexcept { return false; }
            void await_suspend(std::coroutine_handle<> caller) {
                handle.resume();
            }

            ReturnType await_resume() {
                {
                    std::lock_guard lock(m_mutex);
                    if (handle.promise().exception)
                        std::rethrow_exception(handle.promise().exception);
                }
                return handle.promise().result;
            }

            handle_type handle{};

        private:
            mutable std::mutex m_mutex;
        };

        template<>
        struct Task<void> {
            struct promise_type {
                std::suspend_always initial_suspend() noexcept { return {}; }
                std::suspend_always final_suspend() noexcept { return {}; }
                Task get_return_object() {
                    return Task{ std::coroutine_handle<promise_type>::from_promise(*this) };
                }
                void return_void() {}
                void unhandled_exception() { exception = std::current_exception(); }
                std::exception_ptr exception;
            };

            using handle_type = std::coroutine_handle<promise_type>;

            Task() = default;
            explicit Task(handle_type h) : handle(h) {}
            ~Task() { if (handle) handle.destroy(); }
            Task(const Task&) = delete;
            Task& operator=(const Task&) = delete;
            Task(Task&& other) noexcept : handle(other.handle) { other.handle = nullptr; }
            Task& operator=(Task&& other) noexcept {
                if (this != &other) { if (handle) handle.destroy(); handle = other.handle; other.handle = nullptr; }
                return *this;
            }

            void get() {
                std::lock_guard lock(m_mutex);
                if (!handle.done()) handle.resume();
                if (handle.promise().exception) std::rethrow_exception(handle.promise().exception);
            }

            bool await_ready() noexcept { return false; }
            void await_suspend(std::coroutine_handle<> caller) {
                handle.resume();
            }
            void await_resume() {
                {
                    std::lock_guard lock(m_mutex);
                    if (handle.promise().exception)
                        std::rethrow_exception(handle.promise().exception);
                }
            }

            handle_type handle{};


        private:
            mutable std::mutex m_mutex;
        };
#endif // COROUTINE SUPPORT (C++20)
    } // namespace v1
}    // namespace Mercury

