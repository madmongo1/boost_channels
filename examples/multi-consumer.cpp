//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#include <boost/channels/channel.hpp>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/redirect_error.hpp>

#include <iostream>
#include <string>

#include <string_view>

using namespace boost;

auto
produce(channels::channel< std::string > &c) -> asio::awaitable< void >
{
    auto nowait = asio::detached;
    auto wait   = asio::use_awaitable;

    c.async_send("The", nowait);
    c.async_send("cat", nowait);
    c.async_send("sat", nowait);
    c.async_send("on", nowait);
    c.async_send("the", nowait);
    co_await c.async_send("mat", wait);
    c.close();
    co_return;
}

auto
consume(std::string_view name, channels::channel< std::string > &c)
    -> asio::awaitable< void >
{
    channels::error_code ec;

    auto tok = asio::redirect_error(asio::use_awaitable, ec);

    while (!ec)
    {
        auto s = co_await c.async_consume(tok);
        if (!ec)
            std::cout << name << " : " << s << "\n";
    }
    std::cout << name << " : " << ec.message() << "\n";
}

int
main()
{
    auto ioc = asio::io_context();
    auto c   = channels::channel< std::string >(ioc.get_executor());

    asio::co_spawn(ioc, consume("a", c), asio::detached);
    asio::co_spawn(ioc, consume("b", c), asio::detached);
    asio::co_spawn(ioc, consume("c", c), asio::detached);
    asio::co_spawn(ioc, produce(c), asio::detached);

    ioc.run();
}