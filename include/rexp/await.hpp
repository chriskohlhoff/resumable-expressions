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
#include "rexp/detail/waiter.hpp"
#include "rexp/future.hpp"

namespace rexp {

template <class T>
resumable T await(future<T> f)
{
  detail::waiter* this_waiter = detail::active_waiter();
  assert(this_waiter != nullptr);
  this_waiter->prepare();

  future<T> result;
  f.then([w = this_waiter->shared_from_this(), &result](auto f)
      {
        result = std::move(f);
        w->notify();
      });

  this_waiter->wait();
  return result.get();
}

} // namespace rexp

#endif // RESUMABLE_EXPRESSIONS_AWAIT_HPP
