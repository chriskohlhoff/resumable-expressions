#include <iostream>
#include <memory>
#include "rexp/resumable.hpp"
#include "rexp/generator.hpp"

using rexp::generator;
using rexp::stop_generation;

std::allocator<char> my_allocator_obj;

generator<int> fib(int n)
{
  return {
    std::allocator_arg, my_allocator_obj,
    [=](auto yield) mutable
    {
      int a = 0;
      int b = 1;
      while (n-- > 0)
      {
        yield(a);
        auto next = a + b;
        a = b;
        b = next;
      }
    }
  };
}

int main()
{
  generator<int> g = fib(10);
  try
  {
    for (;;)
      std::cout << g.next() << std::endl;
  }
  catch (stop_generation&)
  {
  }
}
