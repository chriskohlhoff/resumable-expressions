//
// use_await.hpp
// ~~~~~~~~~~~~~
// Completion token to use await-enabled functions with asynchronous operations.
//
// Copyright (c) 2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef RESUMABLE_EXPRESSIONS_USE_AWAIT_HPP
#define RESUMABLE_EXPRESSIONS_USE_AWAIT_HPP

#include <boost/optional.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/handler_type.hpp>
#include <cassert>
#include <memory>
#include <tuple>
#include <type_traits>
#include "rexp/waiter.hpp"

namespace rexp {

constexpr struct use_await_t
{
  constexpr use_await_t() {}
} use_await;

namespace detail
{
  using boost::system::error_code;
  using boost::system::system_error;

  template <class... Args>
  struct await_handler_base
  {
    typedef std::tuple<typename std::decay<Args>::type...> tuple_type;

    std::shared_ptr<waiter> waiter_;
    boost::optional<tuple_type>* result_ = nullptr;
    std::exception_ptr* exception_ = nullptr;
  };

  template <class... Args>
  struct await_handler : await_handler_base<Args...>
  {
    await_handler(use_await_t) {}

    void operator()(Args... args)
    {
      this->result_->reset(std::make_tuple(std::forward<Args>(args)...));
      this->waiter_->resume();
    }
  };

  template <class... Args>
  struct await_handler<error_code, Args...> : await_handler_base<Args...>
  {
    await_handler(use_await_t) {}

    void operator()(const error_code& ec, Args... args)
    {
      if (ec)
        *this->exception_ = std::make_exception_ptr(system_error(ec));
      else
        this->result_->reset(std::make_tuple(std::forward<Args>(args)...));
      this->waiter_->resume();
    }
  };

  template <class... Args>
  struct await_handler<std::exception_ptr, Args...> : await_handler_base<Args...>
  {
    await_handler(use_await_t) {}

    void operator()(const std::exception_ptr& e, Args... args)
    {
      if (e)
        *this->exception_ = e;
      else
        this->result_->reset(std::make_tuple(std::forward<Args>(args)...));
      this->waiter_->resume();
    }
  };

  template <class... T>
  inline std::tuple<T...> get_await_result(std::tuple<T...>& result)
  {
    return std::move(result);
  }

  template <class T>
  inline T get_await_result(std::tuple<T>& result)
  {
    return std::move(std::get<0>(result));
  }

  inline void get_await_result(std::tuple<>& result)
  {
  }
} // namespace detail

} // namespace rexp

namespace boost {
namespace asio {

template <class R, class... Args>
struct handler_type<rexp::use_await_t, R(Args...)>
{
  typedef rexp::detail::await_handler<Args...> type;
};

template <class... Args>
class async_result<rexp::detail::await_handler<Args...>>
{
public:
  typedef typename rexp::detail::await_handler<Args...>::tuple_type tuple_type;
  typedef decltype(rexp::detail::get_await_result(std::declval<tuple_type&>())) type;

  explicit async_result(rexp::detail::await_handler<Args...>& handler)
  {
    assert(rexp::waiter::active() != nullptr);
    handler.waiter_ = rexp::waiter::active()->shared_from_this();
    handler.result_ = &result_;
    handler.exception_ = &exception_;
  }

  resumable type get()
  {
    rexp::waiter::active()->suspend();
    if (exception_)
      std::rethrow_exception(exception_);
    return rexp::detail::get_await_result(result_.get());
  }

private:
  boost::optional<tuple_type> result_;
  std::exception_ptr exception_;
};

} // namespace asio
} // namespace boost

#endif // AWAIT_HPP
