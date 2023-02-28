#undef NDEBUG


#include <array>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <iterator>
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
#define PI (3.141592654)

constexpr size_t N = 100;

double dummy_work(size_t d) {
    // std::this_thread::sleep_for(std::chrono::milliseconds(d));
    return static_cast<double>(d) * PI;
}


int main() {
    const int d = 20;

    std::array<double, N> arr = {0};
    std::vector<std::future<double>> futures;
    futures.reserve(N);

    std::cout << std::fixed << std::setprecision(3);

    ThreadPool pool(QUEUE_MAX_SIZE, THREADS);
    pool.start();

    auto inserter = std::inserter(futures, futures.begin());
    for (size_t i = 0; i < N; ++i) {
        auto future = pool.submit(dummy_work, i);
        inserter = std::move(future);
    }

    std::transform(futures.begin(), futures.end(), arr.begin(),
                   [](auto& f) {
                       std::future_status status = f.wait_for(std::chrono::milliseconds(500));
                       assert(status == std::future_status::ready);
                       return f.get();
                   });

    std::for_each(arr.begin(), arr.end(), [i = static_cast<size_t>(0)](const auto& res) mutable {
        assert(static_cast<double>(i) * PI == res);
        ++i;
    });

    std::cout << "Finished. Result is: " << endl;
    std::copy(arr.begin(), arr.end(), std::ostream_iterator<double>(std::cout, " "));

    return 0;
}