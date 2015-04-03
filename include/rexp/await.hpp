//
// await.hpp
// ~~~~~~~~~
// Implementation of await as a library function.
//
// Copyright (c) 2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef RESUMABLE_EXPRESSIONS_AWAIT_HPP
#define RESUMABLE_EXPRESSIONS_AWAIT_HPP

#include <cassert>
#include <memory>
#include <type_traits>
#include "future.hpp"
#include "resumable.hpp"

namespace rexp {

namespace detail
{
  struct waiter : std::enable_shared_from_this<waiter>
  {
    virtual ~waiter() {}
    virtual void resume() = 0;

    std::mutex mutex_;
    enum { running, wait_pending, waiting } state_ = running;
  };

  inline waiter*& active_waiter()
  {
    static __thread waiter* c;
    return c;
  }

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
}

template <class Resumable>
auto spawn(Resumable r,
    typename std::enable_if<
      !std::is_same<
        typename std::result_of<Resumable()>::type,
        void
      >::value
    >::type* = 0)
{
  promise<typename std::result_of<Resumable()>::type> p;
  auto f = p.get_future();

  detail::launch_waiter(
      [r = std::move(r), p = std::move(p)]() mutable
      {
        try
        {
          p.set_value(r());
        }
        catch (...)
        {
          p.set_exception(std::current_exception());
        }
      });

  return f;
}

template <class Resumable>
auto spawn(Resumable r,
    typename std::enable_if<
      std::is_same<
        typename std::result_of<Resumable()>::type,
        void
      >::value
    >::type* = 0)
{
  promise<typename std::result_of<Resumable()>::type> p;
  auto f = p.get_future();

  detail::launch_waiter(
      [r = std::move(r), p = std::move(p)]() mutable
      {
        try
        {
          r();
          p.set_value();
        }
        catch (...)
        {
          p.set_exception(std::current_exception());
        }
      });

  return f;
}

template <class T>
resumable T await(future<T> f)
{
  detail::waiter* this_waiter = detail::active_waiter();
  assert(this_waiter != nullptr);

  future<T> result;
  this_waiter->state_ = detail::waiter::wait_pending;

  f.then([w = this_waiter->shared_from_this(), &result](auto f)
      {
        w->mutex_.lock();
        bool is_waiting = (w->state_ == detail::waiter::waiting);
        w->state_ = detail::waiter::running;
        w->mutex_.unlock();
        result = std::move(f);
        if (is_waiting)
          w->resume();
      });

  this_waiter->mutex_.lock();
  if (this_waiter->state_ != detail::waiter::running)
  {
    this_waiter->state_ = detail::waiter::waiting;
    this_waiter->mutex_.unlock();
    detail::active_waiter() = nullptr;
    break_resumable;
  }
  else
  {
    this_waiter->mutex_.unlock();
  }

  return result.get();
}

} // namespace rexp

#endif // AWAIT_HPP
