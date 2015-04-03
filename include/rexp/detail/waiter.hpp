//
// detail/waiter.hpp
// ~~~~~~~~~~~~~~~~~
// Representation of a resumable function that can await.
//
// Copyright (c) 2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef RESUMABLE_EXPRESSIONS_DETAIL_WAITER_HPP
#define RESUMABLE_EXPRESSIONS_DETAIL_WAITER_HPP

#include <memory>
#include <mutex>
#include "rexp/resumable.hpp"

namespace rexp {
namespace detail {

inline class waiter*& active_waiter()
{
  static __thread class waiter* c;
  return c;
}

class waiter : public std::enable_shared_from_this<waiter>
{
public:
  void prepare()
  {
    state_ = wait_pending;
  }

  resumable void wait()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    if (state_ != running)
    {
      state_ = waiting;
      lock.unlock();
      active_waiter() = nullptr;
      break_resumable;
    }
  }

  void notify()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    bool is_waiting = (state_ == waiting);
    state_ = running;
    lock.unlock();
    if (is_waiting)
      resume();
  }

protected:
  ~waiter() {}

private:
  virtual void resume() = 0;

  std::mutex mutex_;
  enum { running, wait_pending, waiting } state_ = running;
};

template <class F>
class waiter_impl : public waiter
{
public:
  explicit waiter_impl(F f) :
    f_(std::move(f)),
    r_([f = &f_]{ (*f)(); })
  {
  }

  virtual void resume()
  {
    waiter* prev_active_waiter = active_waiter();
    active_waiter() = this;
    try
    {
      while (active_waiter() == this && !r_.ready())
        r_.resume();
      active_waiter() = prev_active_waiter;
    }
    catch (...)
    {
      active_waiter() = prev_active_waiter;
      throw;
    }
  }

private:
  F f_;
  resumable_object<void> r_;
};

template <class F>
void launch_waiter(F f)
{
  std::make_shared<detail::waiter_impl<F>>(std::move(f))->resume();
}

} // namespace detail
} // namespace rexp

#endif // RESUMABLE_EXPRESSIONS_DETAIL_WAITER_HPP
