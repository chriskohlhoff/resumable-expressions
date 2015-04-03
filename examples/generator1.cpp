#include <iostream>
#include "rexp/resumable.hpp"

resumable void fib(int& out, int n)
{
  int a = 0;
  int b = 1;
  while (n-- > 0)
  {
    out = a;
    break_resumable;
    auto next = a + b;
    a = b;
    b = next;
  }
}

int main()
{
  int out;
  resumable_expression(r, fib(out, 10));
  while (!r.ready())
  {
    r.resume();
    if (!r.ready())
      std::cout << out << std::endl;
  }
}
