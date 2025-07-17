
#include "queue.h"

#include <iostream>
#include <thread>

inline constexpr std::chrono::seconds interval(3);

void threadsafe_queue::push(std::string value) {
    {
        std::scoped_lock lock(mtx);
        q.push(std::move(value));
    }
    cond_var.notify_one();
}
void threadsafe_queue::wait_pop(std::string &value) {
    std::unique_lock ulock(mtx);
    cond_var.wait(ulock, [this] { return !q.empty(); });
    value = std::move(q.front());
    q.pop();
}
