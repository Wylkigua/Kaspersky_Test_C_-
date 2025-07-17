#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>

class threadsafe_queue {
    std::mutex mtx;
    std::queue<std::string> q;
    std::condition_variable cond_var;

   public:
    threadsafe_queue() = default;
    void push(std::string value);
    void wait_pop(std::string& value);
    threadsafe_queue& operator=(threadsafe_queue const &) = delete;
    threadsafe_queue(threadsafe_queue const &) = delete;
};