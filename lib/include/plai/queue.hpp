#pragma once

#include <deque>

namespace plai {

template <class T>
class Queue {
 public:
 private:
  std::deque<T> m_q{};
};
}  // namespace plai
