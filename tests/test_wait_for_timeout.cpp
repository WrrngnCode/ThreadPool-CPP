#undef NDEBUG


#include <cassert>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include <utility>

#include "ThreadPool-CPP.h"

using std::cout;
using std::endl;
using std::mutex;
using millis = std::chrono::milliseconds;

#define THREADS 12ULL
#define QUEUE_MAX_SIZE 8500
#define TASK_DURATION_MS 100

auto task = []() {
    millis delay(TASK_DURATION_MS);
    std::this_thread::sleep_for(delay);
};

int main() {

    const size_t N = 500;

    ThreadPool pool(QUEUE_MAX_SIZE, THREADS);
    for (size_t i = 0; i < N; ++i) {
        pool.submit(task);
    }
    pool.start();

    const size_t TIME_NEEDED_EST = N * TASK_DURATION_MS / THREADS;
    std::cout << "All tasks has been submitted to the work queue." << endl;
    std::cout << "Waiting for tasks to finish... Timeout in " << TIME_NEEDED_EST/2 << " ms. " << endl;


    WaitStatus ret = pool.wait_for(TIME_NEEDED_EST / 2);

    assert(ret == WaitStatus::TIMEOUT);  // expect timeout
    assert(pool.getTasksPending() > 0);
    assert(pool.getTasksQueued() > 0);

    if (ret == WaitStatus::DONE_BEFORE_TIMEOUT) {
        std::cout << "Error: Timeout should have happened." << endl;
    } else if (ret == WaitStatus::TIMEOUT) {
        std::cout << "Success: Timeout happened." << endl;
    }

    std::cout << "Pending Tasks: " << pool.getTasksPending() << endl;
    std::cout << "Tasks in the queue: " << pool.getTasksQueued() << endl;
    std::cout << "Finished. " << endl;

    return 0;
}