#include <chrono>
#include <iostream>
#include <thread>
#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#include "rexp/await.hpp"
#include "rexp/use_await.hpp"

using rexp::spawn;
using rexp::use_await;

boost::asio::io_service io_service;

resumable void print_1_to(int n)
{
  for (int i = 1;;)
  {
    std::cout << i << std::endl;
    if (++i > n) break;

    boost::asio::steady_timer timer(io_service, std::chrono::milliseconds(500));
    timer.async_wait(use_await);
  }
}

int main()
{
  spawn([]{ print_1_to(10); });

  io_service.run();
}
