#include <iostream>
#include "rexp/resumable.hpp"

template <class T>
struct yielder
{
  T& out;

  resumable void operator()(T t)
  {
    out = t;
    break_resumable;
  };
};

resumable void fib(yielder<int> yield, int n)
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

int main()
{
  int out;
  resumable_expression(r, fib(yielder<int>{out}, 10));
  while (!r.ready())
  {
    r.resume();
    if (!r.ready())
      std::cout << out << std::endl;
  }
}
