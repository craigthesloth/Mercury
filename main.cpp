/*
 * Mercury v1.2 Full Demo
 * Shows:
 *   - parallel execution of heavy CPU tasks,
 *   - use of signals synchronous and asynchrpnous,
 *   - exception handling with logging,
 *   - a competititve benchmark of single-threaded and multi-thread execution.
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "Mercury.hpp"



 // ---------- Protection against aggressive optimizations ----------
volatile double g_sink = 0.0;

// ---------- Random Number Generators ----------
double getRandDouble(double low, double high) {
    static thread_local std::mt19937_64 mt(std::random_device{}());
    std::uniform_real_distribution<double> dis(low, high);
    return dis(mt);
}

int getRandInt(int low, int high) {
    static thread_local std::mt19937_64 mt(std::random_device{}());
    std::uniform_int_distribution<int> dis(low, high);
    return dis(mt);
}

// ---------- Heavy CPU load ----------
double heavyCPUWork(size_t iterations) {
    double result = 0.0;
    for (size_t i = 0; i < iterations; ++i) {
        result += std::sin(static_cast<double>(i)) * std::cos(static_cast<double>(i));
    }
    return result;
}

// ---------- Single-threaded processing of a set of tasks ----------
double singleThreadHeavy(size_t tasksCount, size_t iterationsPerTask) {
    double sum = 0.0;
    for (size_t i = 0; i < tasksCount; ++i) {
        sum += heavyCPUWork(iterationsPerTask);
    }
    return sum;
}

// ---------- Multi-threaded processing via Mercury (with chunks) ----------
double multiThreadHeavy(Mercury::TaskController<double>& controller,
    size_t tasksCount,
    size_t iterationsPerTask,
    size_t chunkCount) {
    if (tasksCount == 0) {
        return 0.0;
    }

    const size_t chunks = std::min(chunkCount, tasksCount);
    const size_t tasksPerChunk = (tasksCount + chunks - 1) / chunks;

    for (size_t start = 0; start < tasksCount; start += tasksPerChunk) {
        const size_t end = std::min(start + tasksPerChunk, tasksCount);

        controller.addTask(Mercury::Priority::Normal, [start, end, iterationsPerTask]() {
            double sum = 0.0;
            for (size_t i = start; i < end; ++i) {
                sum += heavyCPUWork(iterationsPerTask);
            }
            return sum;
            });
    }

    auto results = controller.execute();
    return std::accumulate(results.begin(), results.end(), 0.0);
}

// ---------- Signal demonstration ----------
void demonstrateSignals() {
    std::cout << "\n=== Signal Demo ===\n";

    Mercury::Signal<double> onAreaComputed;
    auto conn1 = onAreaComputed.connect([](double area) {
        std::cout << "  [Sync] Area computed: " << area << '\n';
        }, Mercury::Priority::Urgent);

    Mercury::ThreadPool pool(2);
    Mercury::Signal<std::string, double> onChunkDone(pool, Mercury::Priority::High);
    auto conn2 = onChunkDone.connect([](std::string name, double sum) {
        std::cout << "  [Async] Chunk '" << name << "' sum: " << sum << '\n';
        });

    onAreaComputed.emit(123.45);
    onChunkDone.emit("First", 500.0);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::cout << "Signal demo finished.\n";
}

// ---------- Demonstration of exceptions with logging ----------
void demonstrateExceptions() {
    std::cout << "\n=== Exception Demo ===\n";

    Mercury::TaskController<double> ctrl(4);
    for (int i = 0; i < 10; ++i) {
        ctrl.addTask(Mercury::Priority::Normal, [i]() -> double {
            if (i == 4) {
                throw std::runtime_error("Oops! Task 4 failed.");
            }
            return i * 10.0;
            });
    }

    try {
        auto results = ctrl.execute();
        (void)results;
        std::cout << "Unexpected: all tasks succeeded.\n";
    }
    catch (const std::exception& e) {
        std::cout << "Caught exception after execute: " << e.what() << "\n";
        std::cout << "Check error_*.log file for full report.\n";
    }
}

using Clock = std::chrono::steady_clock;

struct BenchResult {
    double timeMs = 0.0;
    double speedup = 0.0;
    double efficiency = 0.0;
};

double runSingle(size_t N, size_t iterationsPerTask) {
    const auto start = Clock::now();
    double r = singleThreadHeavy(N, iterationsPerTask);
    g_sink += r;
    const auto end = Clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

double runMulti(size_t threads, size_t N, size_t iterationsPerTask, size_t chunkCount) {
    Mercury::TaskController<double> ctrl(threads);
    const auto start = Clock::now();
    double r = multiThreadHeavy(ctrl, N, iterationsPerTask, chunkCount);
    g_sink += r;
    const auto end = Clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

double median(std::vector<double> v) {
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

void runBenchmark() {
    std::cout << "\n=== Benchmark (CPU-bound, median of runs) ===\n";

    const std::vector<size_t> taskCounts = { 100, 500, 1000, 2000, 5000 };
    const size_t iterationsPerTask = 20000;
    const size_t chunkCount = 100;
    const int repeats = 5;

    const std::vector<size_t> threadCounts = { 0, 1, 2, 4, 8, 16 };
    const size_t maxThreads = std::thread::hardware_concurrency();

    std::vector<size_t> allowedThreads;
    for (size_t t : threadCounts) {
        if (t == 0 || maxThreads == 0 || t <= maxThreads) {
            allowedThreads.push_back(t);
        }
    }

    std::cout << std::setw(12) << "Tasks" << " | "
        << std::setw(14) << "Single (ms)" << " | ";

    for (size_t t : allowedThreads) {
        if (t == 0) continue;
        std::ostringstream ss;
        ss << "T=" << t << " (ms)";
        std::cout << std::setw(14) << ss.str() << " | "
            << std::setw(10) << "Speedup" << " | "
            << std::setw(10) << "Eff." << " | ";
    }
    std::cout << "\n";

    std::cout << std::string(13 + allowedThreads.size() * 31, '-') << "\n";

    // Прогрев
    {
        double warm = singleThreadHeavy(10, 1000);
        g_sink += warm;
        Mercury::TaskController<double> warmCtrl(2);
        double warm2 = multiThreadHeavy(warmCtrl, 10, 1000, 4);
        g_sink += warm2;
    }

    for (size_t N : taskCounts) {
        std::cout << std::setw(12) << N << " | ";

        std::vector<double> singleRuns;
        singleRuns.reserve(repeats);
        for (int i = 0; i < repeats; ++i) {
            singleRuns.push_back(runSingle(N, iterationsPerTask));
        }
        const double singleTime = median(singleRuns);

        std::cout << std::setw(14) << std::fixed << std::setprecision(2) << singleTime << " | ";

        for (size_t t : allowedThreads) {
            if (t == 0) continue;

            std::vector<double> multiRuns;
            multiRuns.reserve(repeats);
            for (int i = 0; i < repeats; ++i) {
                multiRuns.push_back(runMulti(t, N, iterationsPerTask, chunkCount));
            }
            const double multiTime = median(multiRuns);
            const double speedup = singleTime / multiTime;
            const double eff = speedup / static_cast<double>(t);

            std::cout << std::setw(14) << std::fixed << std::setprecision(2) << multiTime << " | "
                << std::setw(10) << std::fixed << std::setprecision(2) << speedup << " | "
                << std::setw(10) << std::fixed << std::setprecision(2) << eff << " | ";
        }

        std::cout << "\n";
    }
}

// ---------- Enter point ----------
int main() {
    try {
        demonstrateSignals();
        demonstrateExceptions();
        runBenchmark();

        std::cout << "\nChecksum: " << g_sink << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
