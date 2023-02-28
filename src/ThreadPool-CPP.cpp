


// #include "ThreadPool-CPP.h"

// #include <cassert>
// #include <chrono>
// #include <cmath>
// #include <condition_variable>
// #include <cstdlib>
// #include <deque>
// #include <functional>
// #include <future>
// #include <iostream>
// #include <mutex>
// #include <random>
// #include <thread>
// #include <vector>

// using std::cout;
// using std::endl;
// using std::mutex;
// using millis = std::chrono::milliseconds;

// static std::mutex g_mtx;


// ThreadPool::ThreadPool(std::size_t _max_size, std::size_t _thread_cnt = 1)
//     : max_size(_max_size), thread_cnt(_thread_cnt) {
//     // threads.reserve(_max_size);
//     threads = std::make_unique<std::thread[]>(thread_cnt);
//     shutdown = false;
// }


// ThreadPool::~ThreadPool() noexcept {
//     shutdown = true;
//     cond.notify_all();
//     for (std::size_t i = 0; i < thread_cnt; ++i) {
//         if (threads[i].joinable()) {
//             threads[i].join();
//         }
//     }
// }

// void ThreadPool::push_task(std::function<void()>&& f) {
//     std::unique_lock<std::mutex> lock(queue_mtx);
//     cond.wait(lock, [this]() { return queue.size() < max_size; });
//     queue.push_back(f);
//     lock.unlock();
//     cond.notify_one();
// }

// void ThreadPool::submit_function(std::function<void()>&& f) {
//     this->push_task(std::forward<std::function<void()>>(f));
// }

// template <class F, typename... Args>
// void ThreadPool::submit(F&& f, Args&&... args) {
//     cout << "submit lambda" << endl;
//     std::function<void()> task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
//     this->push_task(std::move(task));
// }


// std::size_t ThreadPool::size() {
//     std::lock_guard<mutex> guard(queue_mtx);
//     return queue.size();
// }

// // start N threads to consume from the thread pool.
// void ThreadPool::start() {
//     try {
//         for (std::size_t i = 0; i < thread_cnt; ++i) {
//             threads[i] = std::thread(&ThreadPool::worker, this);
//         }
//     } catch (...) {
//         cout << "Error" << endl;
//         shutdown = true;
//         throw;
//     }
// }

// void ThreadPool::worker() {
//     func_type task;


//     num_thread_alive++;


//     while (!shutdown) {
//         {
//             std::unique_lock<mutex> lock(queue_mtx);
//             cond.wait(lock, [this]() { return !queue.empty() || shutdown; });
//             if (!shutdown) {
//                 task = std::move(queue.front());
//                 queue.pop_front();
//                 lock.unlock();
//                 cond.notify_one();

//                 if (task) {
//                     task();
//                     pending_task_cnt--;
//                 } else {
//                     cout << "invalid task" << endl;
//                 }
//             }
//         }
//     }

//     num_thread_alive--;
// }
