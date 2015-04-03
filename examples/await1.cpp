#include <chrono>
#include <iostream>
#include <thread>
#include "rexp/await.hpp"
#include "rexp/future.hpp"

using rexp::await;
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

resumable void print_1_to(int n)
{
  for (int i = 1;;)
  {
    std::cout << i << std::endl;
    if (++i > n) break;
    await(sleep_for(std::chrono::milliseconds(500)));
  }
}

int main()
{
  spawn([]{ print_1_to(10); }).get();
}
