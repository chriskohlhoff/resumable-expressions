//
// resumable.hpp
// ~~~~~~~~~~~~~
// Emulation of resumable expressions using Boost.Coroutine.
//
// Copyright (c) 2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef RESUMABLE_EXPRESSIONS_RESUMABLE_HPP
#define RESUMABLE_EXPRESSIONS_RESUMABLE_HPP

#include <boost/coroutine/asymmetric_coroutine.hpp>
#include <boost/optional.hpp>
#include <cassert>
#include <exception>

namespace rexp {

namespace detail
{
  typedef ::boost::coroutines::push_coroutine<void> push_coroutine;
  typedef ::boost::coroutines::pull_coroutine<void> pull_coroutine;

  inline pull_coroutine*& current_resumable()
  {
    static __thread pull_coroutine* r;
    return r;
  }
}

template <class R>
class resumable_object
{
public:
  typedef R result_type;

  template <class F>
  explicit resumable_object(F f)
    : push_(
        [this, f](auto& pull)
        {
          this->pull_ = &pull;
          (*this->pull_)();
          try
          {
            this->result_.reset(f());
            this->ready_ = true;
          }
          catch (...)
          {
            this->exception_ = std::current_exception();
            this->ready_ = true;
          }
        })
  {
    push_();
  }

  resumable_object(const resumable_object&) = delete;
  resumable_object& operator=(const resumable_object&) = delete;

  void resume()
  {
    detail::pull_coroutine* prev = detail::current_resumable();
    try
    {
      detail::current_resumable() = pull_;
      push_();
      detail::current_resumable() = prev;
      if (exception_)
        std::rethrow_exception(exception_);
    }
    catch (...)
    {
      detail::current_resumable() = prev;
      throw;
    }
  }

  bool ready() const noexcept
  {
    return ready_;
  }

  R result()
  {
    return std::move(result_.get());
  }

private:
  detail::push_coroutine push_;
  detail::pull_coroutine* pull_;
  bool ready_ = false;
  std::exception_ptr exception_;
  boost::optional<R> result_;
};

template <>
class resumable_object<void>
{
public:
  typedef void result_type;

  template <class F>
  explicit resumable_object(F f)
    : push_(
        [this, f](auto& pull)
        {
          this->pull_ = &pull;
          (*this->pull_)();
          try
          {
            f();
            this->ready_ = true;
          }
          catch (...)
          {
            this->exception_ = std::current_exception();
            this->ready_ = true;
          }
        })
  {
    push_();
  }

  resumable_object(const resumable_object&) = delete;
  resumable_object& operator=(const resumable_object&) = delete;

  void resume()
  {
    detail::pull_coroutine* prev = detail::current_resumable();
    try
    {
      detail::current_resumable() = pull_;
      push_();
      detail::current_resumable() = prev;
      if (exception_)
        std::rethrow_exception(exception_);
    }
    catch (...)
    {
      detail::current_resumable() = prev;
      throw;
    }
  }

  bool ready() const noexcept
  {
    return ready_;
  }

  void result()
  {
  }

private:
  detail::push_coroutine push_;
  detail::pull_coroutine* pull_;
  bool ready_ = false;
  std::exception_ptr exception_;
};

inline void break_current_resumable()
{
  assert(detail::current_resumable());
  (*detail::current_resumable())();
}

} // namespace rexp

// Emulation of "resumable" keyword for marking functions.
#define resumable inline

// Emulation of "break resumable".
#define break_resumable do { ::rexp::break_current_resumable(); } while (0)

// Emulation of "resumable" keyword for resumable expressions.
#define resumable_expression(obj, ...) \
  ::rexp::resumable_object<decltype(__VA_ARGS__)> obj{[&]{ __VA_ARGS__; }}

#endif // RESUMABLE_EXPRESSIONS_RESUMABLE_HPP
