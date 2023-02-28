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
using std::bind;
using std::size_t;
#define THREADS 4ULL
#define QUEUE_MAX_SIZE 8500

std::size_t fibo_runs{};
size_t task_delay_us{20};
std::mutex cnt_mtx{};

auto fibo = [a = static_cast<std::size_t>(0), b = static_cast<std::size_t>(1)](int steps = 1) mutable -> std::size_t {
    struct Results {
        std::size_t& a;
        std::size_t& b;

        Results getNextFibo(int num = 1) {
            while (num > 0) {
                a = std::exchange(b, b + a);
                --num;
            }

            return *this;
        }

        std::size_t operator()() {
            return a;
        }

        operator std::size_t() {
            return a;
        }

        Results printFibos() {
            // std::cout << "a: " << a << " b: " << b << std::endl;
            return *this;
        }
    };
    Results{a, b}.getNextFibo(steps).printFibos();

    cnt_mtx.lock();
    fibo_runs++;
    cnt_mtx.unlock();
    //std::this_thread::sleep_for(std::chrono::microseconds(task_delay_us));

    return a;
};


int main() {

    std::random_device rd;
    std::seed_seq ss{rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd()};
    std::mt19937 random_generator(ss);

    std::uniform_int_distribution<int> random_dist(120, 1400);
    ThreadPool pool(QUEUE_MAX_SIZE, THREADS);

    const size_t FIBO_N = 600000;

    //task_delay_us = 620 + 5 * THREADS * 1'000'000ULL / FIBO_N;

    std::cout << "Task Delay in Microseconds: " << task_delay_us << endl;

    for (size_t i = 0; i < FIBO_N; ++i) {
        pool.submit(fibo, random_dist(random_generator));
        //cout << i << " ";
        if (i == QUEUE_MAX_SIZE - 1) {
            pool.start();
            cout << "Pool is running now.." << endl;
            assert(pool.getTasksPending() > 0);
        }
    }

    std::cout << "All tasks has been submitted to the work queue." << endl;
    std::cout << "Waiting for tasks to finish... " << endl;

    WaitStatus ret = pool.wait();
    std::cout << "Pending Tasks: " << pool.getTasksPending() << endl;

    assert(ret == WaitStatus::DONE);
    assert(pool.getTasksPending() == 0);
    assert(pool.getTasksQueued() == 0);
    assert(fibo_runs == FIBO_N);

    std::cout << "Finished. " << endl;

    return 0;
}