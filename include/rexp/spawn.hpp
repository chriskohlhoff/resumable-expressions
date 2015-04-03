//
// spawn.hpp
// ~~~~~~~~~
// Function used to start a new resumable function that can await.
//
// Copyright (c) 2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef RESUMABLE_EXPRESSIONS_SPAWN_HPP
#define RESUMABLE_EXPRESSIONS_SPAWN_HPP

#include <type_traits>
#include "rexp/future.hpp"
#include "rexp/waiter.hpp"

namespace rexp {

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

  launch_waiter(
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

  launch_waiter(
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

} // namespace rexp

#endif // RESUMABLE_EXPRESSIONS_SPAWN_HPP
