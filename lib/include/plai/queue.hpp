#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <plai/util/defer.hpp>

namespace plai {

template <class T, class Container = std::deque<T>>
class Queue {
 public:
    using container_type = Container;
    constexpr Queue() = default;

    template <class... Ts>
    void emplace(Ts&&... ts) {
        std::unique_lock lk{m_mut};
        m_v.emplace_back(std::forward<Ts>(ts)...);
        lk.unlock();
        m_cv.notify_one();
    }

    void push(const T& t) {
        std::unique_lock lk{m_mut};
        m_v.push_back(t);
        lk.unlock();
        m_cv.notify_one();
    }

    void push(T&& t) {
        std::unique_lock lk{m_mut};
        m_v.push_back(std::move(t));
        lk.unlock();
        m_cv.notify_one();
    }

    T pop() {
        std::unique_lock lk{m_mut};
        m_cv.wait(lk, [&] { return !m_v.empty(); });
        Defer defer{[&] { m_v.pop_front(); }};
        return std::move(m_v.front());
    }

 private:
    std::mutex m_mut{};
    std::condition_variable m_cv{};
    container_type m_v{};
};

}  // namespace plai
