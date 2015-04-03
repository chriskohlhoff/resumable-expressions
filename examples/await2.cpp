#include <chrono>
#include <iostream>
#include <thread>
#include "rexp/await.hpp"
#include "rexp/future.hpp"

using rexp::future;
using rexp::promise;
using rexp::spawn;

future<void> sleep_for(std::chrono::milliseconds ms)
{
  promise<void> p;
  future<void> f = p.get_future();

  std::thread([ms, p = std::move(p)]() mutable
      {
        std::this_thread::sleep_for(ms);
        p.set_value();
      }).detach();

  return f;
}

future<void> print_1_to(int n)
{
  return spawn([n]
      {
        for (int i = 1;;)
        {
          std::cout << i << std::endl;
          if (++i > n) break;
          await(sleep_for(std::chrono::milliseconds(500)));
        }
      });
}

resumable void print_numbers()
{
  await(print_1_to(10));
  await(print_1_to(5));
}

int main()
{
  spawn([]{ print_numbers(); }).get();
}
