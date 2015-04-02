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
#include <cassert>

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
class resumable_expression
{
public:
  typedef R result_type;

  template <class F>
  explicit resumable_expression(F f)
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
            this->ready_ = true;
            throw;
          }
        })
  {
    push_();
  }

  resumable_expression(const resumable_expression&) = delete;
  resumable_expression& operator=(const resumable_expression&) = delete;

  void resume()
  {
    detail::pull_coroutine* prev = detail::current_resumable();
    try
    {
      detail::current_resumable() = pull_;
      push_();
      detail::current_resumable() = prev;
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

private:
  detail::push_coroutine push_;
  detail::pull_coroutine* pull_;
  bool ready_ = false;
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

#endif // RESUMABLE_EXPRESSIONS_RESUMABLE_HPP
