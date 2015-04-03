#include <chrono>
#include <iostream>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>
#include "rexp/spawn.hpp"
#include "rexp/use_await.hpp"

using rexp::spawn;
using rexp::use_await;
using boost::asio::ip::tcp;

resumable void echo(tcp::socket socket)
{
  for (;;)
  {
    char data[1024];
    std::size_t n = socket.async_read_some(boost::asio::buffer(data), use_await);
    boost::asio::async_write(socket, boost::asio::buffer(data, n), use_await);
  }
}

resumable void listen(tcp::acceptor acceptor)
{
  for (;;)
  {
    try
    {
      tcp::socket socket(acceptor.get_io_service());
      acceptor.async_accept(socket, use_await);
      spawn([s = std::move(socket)]() mutable { echo(std::move(s)); });
    }
    catch (...)
    {
    }
  }
}

int main()
{
  boost::asio::io_service io_service;
  spawn([&]{ listen({io_service, {tcp::v4(), 55555}}); });
  io_service.run();
}
