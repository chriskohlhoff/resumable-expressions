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

#include <boost/optional.hpp>
#include <cassert>
#include <memory>
#include <type_traits>
#include "future.hpp"
#include "resumable.hpp"

namespace rexp {

namespace detail
{
  template <class T>
  using optional = boost::optional<T>;

  struct awaiter : std::enable_shared_from_this<awaiter>
  {
    virtual ~awaiter() {}
    virtual void resume() = 0;
  };

  inline awaiter*& active_awaiter()
  {
    static __thread awaiter* c;
    return c;
  }

  template <class F>
  class awaiter_impl : public awaiter
  {
  public:
    explicit awaiter_impl(F f) :
      f_(std::move(f)),
      r_([f = &f_]{ (*f)(); })
    {
    }

    virtual void resume()
    {
      awaiter* prev_active_awaiter = active_awaiter();
      active_awaiter() = this;
      try
      {
        while (active_awaiter() == this && !r_.ready())
          r_.resume();
        active_awaiter() = prev_active_awaiter;
      }
      catch (...)
      {
        active_awaiter() = prev_active_awaiter;
        throw;
      }
    }

  private:
    F f_;
    resumable_expression<void> r_;
  };

  template <class F>
  void launch_awaiter(F f)
  {
    std::make_shared<detail::awaiter_impl<F>>(std::move(f))->resume();
  }

  enum class await_state { starting, suspended, complete };
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

  detail::launch_awaiter(
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

  detail::launch_awaiter(
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
  assert(detail::active_awaiter() != nullptr);

  std::mutex mutex;
  detail::await_state state = detail::await_state::starting;
  future<T> result;

  f.then([chain = detail::active_awaiter()->shared_from_this(), &mutex, &state, &result](auto f)
      {
        mutex.lock();
        bool is_suspended = (state == detail::await_state::suspended);
        state = detail::await_state::complete;
        mutex.unlock();
        result = std::move(f);
        if (is_suspended)
          chain->resume();
      });

  mutex.lock();
  if (state != detail::await_state::complete)
  {
    state = detail::await_state::suspended;
    detail::active_awaiter() = nullptr;
    mutex.unlock();
    break_resumable;
  }
  else
  {
    mutex.unlock();
  }

  return result.get();
}

} // namespace rexp

#endif // AWAIT_HPP
