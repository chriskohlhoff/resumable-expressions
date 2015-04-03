#include <iostream>
#include "rexp/resumable.hpp"

resumable void print_1_to(int n)
{
  for (int i = 1;;)
  {
    std::cout << i << std::endl;
    if (++i > n) break;
    break_resumable;
  }
}

int main()
{
  resumable_expression(r, print_1_to(5));
  while (!r.ready())
  {
    std::cout << "resuming ... ";
    r.resume();
  }
}
