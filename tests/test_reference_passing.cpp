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

void dummy_work(int& sum, int& d) {
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    cnt_mtx.lock();
    finished++;
    sum += d;
    cnt_mtx.unlock();
}


int main() {
    const int N = 20;
    const int duration_ms = 100;
    ThreadPool pool(QUEUE_MAX_SIZE, THREADS);

    int sum{0};

    for (int i = 0; i < N; ++i) {
        pool.submit(dummy_work, std::ref(sum), i);
    }

    pool.start();
    pool.wait();
    assert(sum == 190); //190 = 1 + 2 + 3 + ... + N-1;
    std::cout << "Success: " << sum << endl;

    std::cout << "Finished." << endl;

    return 0;
}