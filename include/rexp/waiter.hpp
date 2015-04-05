//
// waiter.hpp
// ~~~~~~~~~~
// Representation of a resumable function that can await.
//
// Copyright (c) 2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef RESUMABLE_EXPRESSIONS_WAITER_HPP
#define RESUMABLE_EXPRESSIONS_WAITER_HPP

#include <cassert>
#include <memory>
#include <mutex>
#include "rexp/resumable.hpp"

namespace rexp {

class waiter :
  public std::enable_shared_from_this<waiter>
{
public:
  virtual ~waiter() {}

  void run()
  {
    struct state_saver
    {
      waiter* prev = active_waiter_;
      ~state_saver() { active_waiter_ = prev; }
    } saver;
    active_waiter_ = this;

    std::lock_guard<std::mutex> lock(mutex_);
    nested_resumption_ = false;
    do_run();
  }

  resumable void suspend()
  {
    assert(active_waiter_ == this);
    if (!nested_resumption_)
    {
      active_waiter_ = nullptr;
      break_resumable;
    }
  }

  void resume()
  {
    if (active_waiter_ == this)
      nested_resumption_ = true;
    else
      run();
  }

  static waiter* active()
  {
    return active_waiter_;
  }

private:
  virtual void do_run() = 0;

  std::mutex mutex_;
  bool nested_resumption_ = false;
  static __thread waiter* active_waiter_;
};

namespace detail
{
  template <class F>
  class waiter_impl : public waiter
  {
  public:
    explicit waiter_impl(F f) :
      f_(std::move(f)),
      r_([f = &f_]{ (*f)(); })
    {
    }

  private:
    virtual void do_run()
    {
      while (active() == this && !r_.ready())
        r_.resume();
    }

    F f_;
    resumable_object<void> r_;
  };
} // namespace detail

template <class F>
void launch_waiter(F f)
{
  std::make_shared<detail::waiter_impl<F>>(std::move(f))->run();
}

} // namespace rexp

#endif // RESUMABLE_EXPRESSIONS_WAITER_HPP
