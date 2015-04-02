//
// future.hpp
// ~~~~~~~~~~
// Reimplementation of standard futures/promises including .then().
//
// Copyright (c) 2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef RESUMABLE_EXPRESSION_FUTURE_HPP
#define RESUMABLE_EXPRESSION_FUTURE_HPP

#include <condition_variable>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>

namespace rexp {

template <class R> class future;
template <class R> class promise;

class future_error : std::runtime_error
{
public:
  explicit future_error(const char* msg) :
    std::runtime_error(msg)
  {
  }
};

namespace detail
{
  template <class R> class future_state
  {
    friend class future<R>;
    friend class promise<R>;

    std::mutex mutex_;
    std::condition_variable condition_;
    std::unique_ptr<R> value_;
    std::exception_ptr exception_;
    std::function<void(future<R>)> continuation_;
    bool ready_ = false;
  };
}

template <class R> class future
{
public:
  future() noexcept
  {
  }

  future(future&& rhs) noexcept
    : state_(std::move(rhs.state_))
  {
  }

  future(const future&& rhs) = delete;

  ~future()
  {
  }

  future& operator=(const future& rhs) = delete;

  future& operator=(future&& rhs) noexcept
  {
    state_ = std::move(rhs.state_);
    return *this;
  }

  R get()
  {
    if (!state_)
      throw future_error("future is empty");

    std::unique_lock<std::mutex> lock(state_->mutex_);

    while (!state_->ready_)
      state_->condition_.wait(lock);

    std::shared_ptr<detail::future_state<R>> state(std::move(state_));
    if (state->exception_)
      std::rethrow_exception(state->exception_);
    return std::move(*state->value_.get());
  }

  bool valid() const noexcept
  {
    return state_ != nullptr;
  }

  void wait() const
  {
    if (!state_)
      throw future_error("future is empty");

    std::unique_lock<std::mutex> lock(state_->mutex_);

    while (!state_->ready_)
      state_->condition_.wait(lock);
  }

  template <class F>
  void then(F f)
  {
    std::function<void(future)> continuation(std::move(f));

    std::unique_lock<std::mutex> lock(state_->mutex_);

    if (state_->ready_)
    {
      lock.unlock();
      continuation(future(std::move(*this)));
    }
    else
    {
      state_->continuation_ = std::move(continuation);
      state_.reset();
    }
  }

private:
  friend class promise<R>;

  explicit future(std::shared_ptr<detail::future_state<R>> state) :
    state_(std::move(state))
  {
  }

  std::shared_ptr<detail::future_state<R>> state_;
};

template <class R> class promise
{
public:
  promise() :
    state_(std::make_shared<detail::future_state<R>>()),
    future_already_retrieved_(false)
  {
  }

  template <class Allocator>
  promise(std::allocator_arg_t, const Allocator& a) :
    state_(std::allocate_shared<detail::future_state<R>>(a)),
    future_already_retrieved_(false)
  {
  }

  promise(promise&& rhs) noexcept
    : state_(std::move(rhs.state_)),
      future_already_retrieved_(rhs.future_already_retrieved_)
  {
    rhs.future_already_retrieved_ = false;
  }

  promise(const promise& rhs) = delete;

  promise& operator=(promise&& rhs) noexcept
  {
    state_ = std::move(rhs.state_);
    future_already_retrieved_ = rhs.future_already_retrieved_;
    rhs.future_already_retrieved_ = false;

    return *this;
  }

  promise& operator=(const promise& rhs) = delete;

  void swap(promise& other) noexcept
  {
    std::swap(state_, other.state_);
    std::swap(future_already_retrieved_, other.future_already_retrieved_);
  }

  future<R> get_future()
  {
    if (!state_)
      throw future_error("no future shared state");

    if (future_already_retrieved_)
      throw future_error("future already retrieved");

    future_already_retrieved_ = true;

    return future<R>(state_);
  }

  void set_value(R r)
  {
    if (!state_)
      throw future_error("no future shared state");

    std::unique_ptr<R> value(new R(std::move(r)));

    std::unique_lock<std::mutex> lock(state_->mutex_);

    if (state_->ready_)
      throw future_error("promise already satisfied");

    state_->value_ = std::move(value);
    state_->ready_ = true;
    state_->condition_.notify_all();

    std::function<void(future<R>)> continuation(std::move(state_->continuation_));
    lock.unlock();
    if (continuation)
      continuation(future<R>(state_));
  }

  void set_exception(std::exception_ptr e)
  {
    if (!state_)
      throw future_error("no future shared state");

    std::unique_lock<std::mutex> lock(state_->mutex_);

    if (state_->ready_)
      throw future_error("promise already satisfied");

    state_->exception_ = std::move(e);
    state_->ready_ = true;
    state_->condition_.notify_all();

    std::function<void(future<R>)> continuation(std::move(state_->continuation_));
    lock.unlock();
    if (continuation)
      continuation(future<R>(state_));
  }

private:
  std::shared_ptr<detail::future_state<R>> state_;
  bool future_already_retrieved_;
};

namespace detail
{
  template <> class future_state<void>
  {
    friend class future<void>;
    friend class promise<void>;

    std::mutex mutex_;
    std::condition_variable condition_;
    std::exception_ptr exception_;
    std::function<void(future<void>)> continuation_;
    bool ready_ = false;
  };
}

template <> class future<void>
{
public:
  future() noexcept
  {
  }

  future(future&& rhs) noexcept
    : state_(std::move(rhs.state_))
  {
  }

  future(const future& rhs) = delete;

  ~future()
  {
  }

  future& operator=(const future& rhs) = delete;

  future& operator=(future&& rhs) noexcept
  {
    state_ = std::move(rhs.state_);
    return *this;
  }

  void get()
  {
    if (!state_)
      throw future_error("no future shared state");

    std::unique_lock<std::mutex> lock(state_->mutex_);

    while (!state_->ready_)
      state_->condition_.wait(lock);

    std::shared_ptr<detail::future_state<void>> state(std::move(state_));
    if (state->exception_)
      std::rethrow_exception(state->exception_);
  }

  bool valid() const noexcept
  {
    return state_ != nullptr;
  }

  void wait() const
  {
    if (!state_)
      throw future_error("future is empty");

    std::unique_lock<std::mutex> lock(state_->mutex_);

    while (!state_->ready_)
      state_->condition_.wait(lock);
  }

  template <class F>
  void then(F f)
  {
    if (!state_)
      throw future_error("no future shared state");

    std::function<void(future)> continuation(std::move(f));

    std::unique_lock<std::mutex> lock(state_->mutex_);

    if (state_->ready_)
    {
      lock.unlock();
      continuation(future(std::move(*this)));
    }
    else
    {
      state_->continuation_ = std::move(continuation);
      state_.reset();
    }
  }

private:
  friend class promise<void>;

  explicit future(std::shared_ptr<detail::future_state<void>> state) :
    state_(std::move(state))
  {
  }

  std::shared_ptr<detail::future_state<void>> state_;
};

template <> class promise<void>
{
public:
  promise()
    : state_(std::make_shared<detail::future_state<void>>()),
      future_already_retrieved_(false)
  {
  }

  template <class Allocator>
  promise(std::allocator_arg_t, const Allocator& a) :
    state_(std::allocate_shared<detail::future_state<void>>(a)),
    future_already_retrieved_(false)
  {
  }

  promise(promise&& rhs) noexcept
    : state_(std::move(rhs.state_)),
      future_already_retrieved_(rhs.future_already_retrieved_)
  {
    rhs.future_already_retrieved_ = false;
  }

  promise(const promise& rhs) = delete;

  promise& operator=(promise&& rhs) noexcept
  {
    state_ = std::move(rhs.state_);
    future_already_retrieved_ = rhs.future_already_retrieved_;
    rhs.future_already_retrieved_ = false;

    return *this;
  }

  promise& operator=(const promise& rhs) = delete;

  void swap(promise& other) noexcept
  {
    std::swap(state_, other.state_);
    std::swap(future_already_retrieved_, other.future_already_retrieved_);
  }

  future<void> get_future()
  {
    if (!state_)
      throw future_error("no future shared state");

    if (future_already_retrieved_)
      throw future_error("future already retrieved");

    future_already_retrieved_ = true;

    return future<void>(state_);
  }

  void set_value()
  {
    if (!state_)
      throw future_error("no future shared state");

    std::unique_lock<std::mutex> lock(state_->mutex_);

    if (state_->ready_)
      throw future_error("promise already satisfied");

    state_->ready_ = true;
    state_->condition_.notify_all();

    std::function<void(future<void>)> continuation(std::move(state_->continuation_));
    lock.unlock();
    if (continuation)
      continuation(future<void>(state_));
  }

  void set_exception(std::exception_ptr e)
  {
    if (!state_)
      throw future_error("no future shared state");

    std::unique_lock<std::mutex> lock(state_->mutex_);

    if (state_->ready_)
      throw future_error("promise already satisfied");

    state_->exception_ = std::move(e);
    state_->ready_ = true;
    state_->condition_.notify_all();

    std::function<void(future<void>)> continuation(std::move(state_->continuation_));
    lock.unlock();
    if (continuation)
      continuation(future<void>(state_));
  }

private:
  std::shared_ptr<detail::future_state<void>> state_;
  bool future_already_retrieved_;
};

} // namespace rexp

#endif // RESUMABLE_EXPRESSION_FUTURE_HPP
