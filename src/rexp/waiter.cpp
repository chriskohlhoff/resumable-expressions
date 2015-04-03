//
// waiter.cpp
// ~~~~~~~~~~
// Representation of a resumable function that can await.
//
// Copyright (c) 2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "rexp/waiter.hpp"

namespace rexp {

__thread waiter* waiter::active_waiter_;

} // namespace rexp
