//
// generator.hpp
// ~~~~~~~~~~~~~
// Implementation of a type-erased generator.
//
// Copyright (c) 2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef RESUMABLE_EXPRESSIONS_GENERATOR_HPP
#define RESUMABLE_EXPRESSIONS_GENERATOR_HPP

#include <cassert>
#include <exception>
#include <memory>
#include <type_traits>
#include "rexp/resumable.hpp"

namespace rexp {

class stop_generation : std::exception {};

namespace detail
{
  template <class T>
  struct generator_impl_base
  {
    virtual ~generator_impl_base() {}
    virtual T next() = 0;
  };

  template <class T, class G>
  struct generator_impl
    : generator_impl_base<T>
  {
    generator_impl(G g) :
      generator_(g),
      r_([this]
          {
            generator_(
              [v = &value_](T next)
              {
                *v = next;
                break_resumable;
              });
          })
    {
    }

    virtual T next()
    {
      r_.resume();
      if (r_.ready())
        throw stop_generation();
      return value_;
    }

    G generator_;
    T value_;
    resumable_object<void> r_;
  };
}

template <class T>
class generator
{
public:
  template <class G>
  generator(G g)
    : impl_(std::make_shared<
        detail::generator_impl<T, G>>(std::move(g))) {}

  template <class G, class Allocator>
    generator(std::allocator_arg_t, const Allocator& a, G g)
      : impl_(std::allocate_shared<
          detail::generator_impl<T, G>>(a, std::move(g)))
    {
    }

  T next() { return impl_->next(); }

private:
  std::shared_ptr<detail::generator_impl_base<T>> impl_;
};

} // namespace rexp

#endif // RESUMABLE_EXPRESSIONS_GENERATOR_HPP
