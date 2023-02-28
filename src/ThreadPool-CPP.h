#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdlib>
#include <deque>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <random>
#include <stdexcept>
#include <thread>
#include <vector>

using std::cout;
using std::endl;
using std::mutex;
using std::size_t;
using millis = std::chrono::milliseconds;
using std::make_shared;
using std::packaged_task;

enum class WaitStatus : int {
    DONE = 0,
    TIMEOUT = 1,
    DONE_BEFORE_TIMEOUT = 2,
};

class ThreadPool {
   private:
    std::deque<std::function<void(void)>> queue;
    std::unique_ptr<std::thread[]> threads = nullptr;

    std::mutex queue_mtx;                 // mutex for the queue
    std::mutex pool_mtx;                  // mutex for the counters
    std::condition_variable cond;         // CondVar for queue not empty and not full signals
    std::condition_variable cv_all_done;  // CondVar for all work done

    std::size_t max_size{0};             // max size of task queue
    std::size_t thread_cnt{0};           // paralell threads in the thread pool
    std::size_t pending_task_cnt{0};     // number of remaining tasks (in the queue + running)
    std::size_t num_threads_alive{0};    // number of threads alive
    std::size_t num_threads_working{0};  // number of threads running a task

    std::atomic_bool shutdown = false;

   public:
    explicit ThreadPool(std::size_t _max_size, std::size_t _thread_cnt);

    ~ThreadPool();

    template <class F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>;

    std::size_t getTasksQueued();
    std::size_t getTasksPending();
    std::size_t getTasksRunning();

    void start();
    void worker();
    void destroy();

    WaitStatus wait();  // wait for tasks to be finished
    WaitStatus wait_for(size_t timeout_ms);

   private:
    void push_task(std::function<void()>&& f);
};


ThreadPool::ThreadPool(std::size_t _max_size, std::size_t _thread_cnt = 1)
    : max_size(_max_size), thread_cnt(_thread_cnt) {
    threads = std::make_unique<std::thread[]>(thread_cnt);
    shutdown = false;
}

ThreadPool::~ThreadPool() {
    destroy();
}

// immediate shutdown of worker threads, do not wait for tasks to finish
void ThreadPool::destroy() {
    if (shutdown) {
        return;
    }

    shutdown = true;
    cond.notify_all();

    // Poll threads until all finished
    while (num_threads_alive != 0) {
        cond.notify_one();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // try {
    for (std::size_t i = 0; i < thread_cnt; ++i) {
        if (threads[i].joinable()) {
            threads[i].join();
        }
    }
    queue.clear();
    threads.reset();
    
    // std::cout <<"# of alive threads: "<< num_threads_alive << endl;

    // } catch (const std::exception& e) {
    //     std::cerr << "Join exception: " << e.what() << endl;
    // }
}

void ThreadPool::push_task(std::function<void()>&& f) {
    std::unique_lock<std::mutex> lock(queue_mtx);
    cond.wait(lock, [this]() { return queue.size() < max_size; });
    queue.emplace_back(std::move(f));

    pool_mtx.lock();
    pending_task_cnt++;
    pool_mtx.unlock();

    lock.unlock();
    cond.notify_one();
}


template <class F, typename... Args>
auto ThreadPool::submit(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type> {
    using R = std::invoke_result_t<F, Args...>;  // return type of f is R
    // using R = decltype(f(args...));
    std::function<R()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    auto task_ptr = std::make_shared<std::packaged_task<R()>>(std::move(func));

    // wrapping them with a generic void function so that it can be stored in the queue
    std::function<void()> wrapper = [task_ptr]() { (*task_ptr)(); };
    push_task(std::move(wrapper));
    return task_ptr->get_future();
}

std::size_t ThreadPool::getTasksQueued() {
    std::lock_guard<mutex> guard(queue_mtx);
    return queue.size();
}

std::size_t ThreadPool::getTasksPending() {
    std::lock_guard<mutex> guard(pool_mtx);
    return pending_task_cnt;
}

std::size_t ThreadPool::getTasksRunning() {
    std::lock_guard<std::mutex> l(pool_mtx);
    return num_threads_working;
}

// start N threads to consume from the thread pool.
void ThreadPool::start() {
    try {
        for (std::size_t i = 0; i < thread_cnt; ++i) {
            threads[i] = std::thread(&ThreadPool::worker, this);
        }
    } catch (...) {
        std::cerr << "Error launching threadpool." << endl;
        shutdown = true;
        throw;
    }
}

void ThreadPool::worker() {
    std::function<void()> task{};
    {
        std::lock_guard<std::mutex> l(pool_mtx);
        num_threads_alive++;
    }

    while (!shutdown) {
        {
            std::unique_lock<mutex> lock(queue_mtx);
            cond.wait(lock, [this]() { return !queue.empty() || shutdown; });

            if (!shutdown) {
                {
                    std::lock_guard<std::mutex> l(pool_mtx);
                    num_threads_working++;
                }

                task = std::move(queue.front());
                queue.pop_front();
                lock.unlock();      // unlock queue_mtx
                cond.notify_one();  // Queue is not full any more (if it was)
                if (task) {
                    task();  // Run the task
                    {
                        std::unique_lock<std::mutex> l(pool_mtx);
                        num_threads_working--;
                        pending_task_cnt--;

                        if (num_threads_working == 0 && queue.size() == 0) {
                            l.unlock();
                            cv_all_done.notify_one();
                        }
                    }
                } else {
                    cout << "invalid task" << endl;
                    throw std::runtime_error("Invalid Task Function. Cannot execute.");
                }
            }

            task = nullptr;

        }  // loop until shutdown != true
    }

    {
        std::lock_guard<std::mutex> l(pool_mtx);
        num_threads_alive--;
    }
}

WaitStatus ThreadPool::wait() {
    std::unique_lock<std::mutex> l(pool_mtx);
    // wait until task queue is empty and all worker threads are finished
    cv_all_done.wait(l, [this]() { return num_threads_working == 0 && queue.size() == 0; });
    return WaitStatus::DONE;
}


WaitStatus ThreadPool::wait_for(size_t timeout_ms) {
    std::unique_lock<std::mutex> lock(pool_mtx);
    bool status = cv_all_done.wait_for(
        lock,
        std::chrono::milliseconds(timeout_ms),
        [this]() { return num_threads_working == 0 && queue.size() == 0; });

    if (status == false) {
        return WaitStatus::TIMEOUT;  // timed out
    } else {
        return WaitStatus::DONE_BEFORE_TIMEOUT;  // finished within timeout
    }
}
