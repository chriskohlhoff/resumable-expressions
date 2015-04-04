#include <iostream>
#include <iterator>
#include <vector>
#include "rexp/resumable.hpp"

template <class T>
struct resumable_iterator
  : std::iterator<std::output_iterator_tag, void, void, void, void>
{
  T& out;

  explicit resumable_iterator(int& i) : out(i) {}

  resumable_iterator& operator*() { return *this; }
  resumable_iterator& operator++() { return *this; }
  resumable_iterator& operator++(int) { return *this; }

  resumable resumable_iterator& operator=(T t)
  {
    out = t;
    break_resumable;
    return *this;
  }
};

template <class OutputIterator>
void fib(OutputIterator out, int n)
{
  int a = 0;
  int b = 1;
  while (n-- > 0)
  {
    *out++ = a;
    auto next = a + b;
    a = b;
    b = next;
  }
}

int main()
{
  std::vector<int> v;
  fib(std::back_inserter(v), 10);
  for (int i: v)
    std::cout << i << std::endl;

  int out;
  resumable_expression(r, fib(resumable_iterator<int>{out}, 10));
  while (!r.ready())
  {
    r.resume();
    if (!r.ready())
      std::cout << out << std::endl;
  }
}
