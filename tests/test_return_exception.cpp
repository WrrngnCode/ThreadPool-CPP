#undef NDEBUG

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
#include <exception>
#include "ThreadPool-CPP.h"

using std::cout;
using std::endl;
using std::mutex;
using millis = std::chrono::milliseconds;
using std::bind;
using std::lock_guard;
using std::ostringstream;

#define THREADS 8ULL
#define QUEUE_MAX_SIZE 1500

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

    std::cout << "waiting for input...\n";
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
    try {
        num = future.get();
    } catch (const std::exception& e) {
        tsprint << "Rethrow exception: " << e.what() << std::endl;
        throw;
    }
    return static_cast<double>(d) * num;
}


int main() {
    ThreadPool pool(QUEUE_MAX_SIZE, THREADS);

    std::promise<double> p;
    std::future<double> promise_future = p.get_future();
    //std::shared_future<double> sh_future = promise_future.share();

    auto future = pool.submit(dummy_work, 500, std::ref(promise_future));

    pool.start();
    tsprint << "Breaking promise in 3 secounds:" << endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    p.set_exception(std::make_exception_ptr(std::runtime_error("Breaking Promise")));

    bool exception_caught{false};
    try {
        future.get();
    } catch (const std::exception& e) {
        tsprint << "Exception: " << e.what() << endl;
        if (e.what() == std::string("Breaking Promise")) {
            tsprint << "Success. Exception caught " << endl;
            exception_caught = true;
        }
    }

    assert(exception_caught == true);

    std::vector<std::future<size_t>> futures;
    futures.reserve(N);
    for (size_t i = 0; i < N; ++i) {
        auto fut = pool.submit([](size_t n) {
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            if (n > N / 2) {
                throw std::runtime_error("This task has thrown.");
            }
            return n;
        },
                               i);
        futures.push_back(std::move(fut));
    }
    size_t thrown_cnt{0};
    size_t finished_cnt{0};
    std::for_each(futures.begin(), futures.end(),
                  [&, i = static_cast<size_t>(0)](std::future<size_t>& f) mutable {
                      std::future_status status = f.wait_for(std::chrono::milliseconds(500));
                      assert(status == std::future_status::ready);
                      try {
                          auto retval = f.get();
                          finished_cnt++;
                      } catch (const std::exception& e) {
                          assert(i > N / 2);
                          assert(e.what() == std::string("This task has thrown."));
                          thrown_cnt++;
                      }
                      i++;
                  });


    assert(finished_cnt == N / 2 + 1);
    assert(thrown_cnt == N / 2 - 1);

    tsprint << "Thrown: " << thrown_cnt << endl;
    tsprint << "Finished: " << finished_cnt << endl;

    pool.destroy();

    tsprint << "Finished. " << endl;

    return EXIT_SUCCESS;
}