#undef NDEBUG

#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>

#include <array>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>
#include <utility>

#include "ThreadPool-CPP.h"

using std::cout;
using std::endl;
using std::mutex;
using millis = std::chrono::milliseconds;
using std::bind;
using std::lock_guard;
using std::ostringstream;

#define THREADS 8ULL
#define QUEUE_MAX_SIZE 2500

constexpr size_t N = 100;

class ThreadSafePrinter {
    static mutex m;
    static thread_local ostringstream oss;

   public:
    ThreadSafePrinter() = default;
    ~ThreadSafePrinter() {
        m.lock();
        std::cout << oss.str();
        m.unlock();
        oss.str(std::string());
        oss.clear();
    }

    template <typename T>
    ThreadSafePrinter& operator<<(const T& c) {
        oss << c;
        return *this;
    }

    using manip_fn = std::ostream& (*)(std::ostream&);  // function signature of std::endl
    ThreadSafePrinter& operator<<(manip_fn manip) {
        manip(oss);
        return *this;
    }
};

mutex ThreadSafePrinter::m{};
thread_local ostringstream ThreadSafePrinter::oss{};

#define tsprint ThreadSafePrinter()


double dummy_work(int d, std::future<double>& future) {
    // std::this_thread::sleep_for(std::chrono::milliseconds(d));

    tsprint << "waiting for input...\n";
    std::future_status status;

    do {
        status = future.wait_for(std::chrono::seconds(1));
        // std::lock_guard<mutex> cout_locker(mu_cout);
        switch (status) {
            case std::future_status::ready:
                tsprint << "waiting 1sec -> Ready " << std::endl;
                break;
            case std::future_status::timeout:
                tsprint << "waiting 1sec -> Timeout " << std::endl;
                break;
            case std::future_status::deferred:
                tsprint << "waiting 1sec -> Deferred " << std::endl;
                break;
        }

    } while (status != std::future_status::ready);

    double num{0};
    num = future.get();
    return static_cast<double>(d) * num;
}


TEST(Exceptions, PromiseException) {
    ThreadPool pool(QUEUE_MAX_SIZE, THREADS);

    std::promise<double> p;
    std::future<double> promise_future = p.get_future();

    auto future = pool.submit(dummy_work, 50, std::ref(promise_future));

    pool.start();

    tsprint << "Breaking promise in 3 secounds:" << endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    p.set_exception(std::make_exception_ptr(std::runtime_error("Breaking Promise")));

    try {
        EXPECT_THROW(future.get(), std::runtime_error);
    } catch (...) {
        tsprint << "Exception captured. " << endl;
    }

}