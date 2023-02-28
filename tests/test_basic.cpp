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

#define THREADS 12ULL
#define QUEUE_MAX_SIZE 1500


int finished;

std::mutex cnt_mtx{};
std::deque<std::function<void()>> q;

void dummy_work(int d) {
    std::this_thread::sleep_for(std::chrono::milliseconds(d));
    cnt_mtx.lock();
    finished++;
    cnt_mtx.unlock();
}


int main() {


    const int N = 20;
    const int duration_ms = 100;
    ThreadPool pool(QUEUE_MAX_SIZE, THREADS);


    {
        for (int i = 0; i < N; ++i) {
            pool.submit(dummy_work, duration_ms);
        }

        pool.start();

        const int a = 5;
        const int b = 5;
        int c = 0;

        for (int i = 0; i < N; ++i) {
            pool.submit([&c, duration_ms](const int _a, const int _b) {
                cnt_mtx.lock();
                c = c + _a + _b;
                finished++;
                cnt_mtx.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
            },
                        a, b);
        }

        for (int i = 0; i < N; ++i) {
            pool.submit(dummy_work, duration_ms);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(N * 3 * duration_ms / THREADS + 2000));
        assert(finished == N * 3);
        assert(c == N * (a + b));
        std::cout << "Success: " << N * 3 << " tasks done." << endl;
    }

    {
        std::size_t counter{0};
        auto increment = [&counter](std::size_t a) {
            counter += a;
            // std::cout << "Counter incremented: " << counter << endl;
        };

        ThreadPool pool2(QUEUE_MAX_SIZE, 1);
        pool2.start();

        for (int i = 0; i < N; ++i) {
            pool2.submit(increment, 1);
        }

        WaitStatus ret = pool2.wait();
        assert(ret == WaitStatus::DONE);
        assert(counter == N);
        std::cout << "Success: counter = " << N  << " as expected." << endl;
    }

    std::cout << "Finished." << endl;

    return 0;
}