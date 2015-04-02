#include <asio/post.hpp>
#include <cstdio>
#include "rexp/await.hpp"
#include "rexp/future.hpp"

using rexp::future;
using rexp::promise;
using rexp::spawn;

future<void> async_foo()
{
  promise<void> p;
  future<void> f = p.get_future();
  p.set_value();
  return f;
}

future<int> async_bar()
{
  promise<int> p;
  future<int> f = p.get_future();
  asio::post([p = std::move(p)]() mutable{ usleep(500000); p.set_value(42); });
  return f;
}

void foo()
{
  for (int i = 0; i < 10; ++i)
  {
    std::printf("i = %d\n", i);
    await(async_foo());
    std::printf("after async_foo\n");
    int j = await(async_bar());
    std::printf("after async_bar j = %d\n", j);
  }
}

int main()
{
  spawn(foo).get();
}
