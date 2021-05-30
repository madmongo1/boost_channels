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
#include <boost/asio/strand.hpp>

#include <iostream>
#include <string>

#include <string_view>

using namespace boost;

auto
produce(channels::channel< std::string > &c) -> asio::awaitable< void >
{
    constexpr auto wait = asio::use_awaitable;
    co_await c.async_send("The", wait);
    co_await c.async_send("cat", wait);
    co_await c.async_send("sat", wait);
    co_await c.async_send("on", wait);
    co_await c.async_send("the", wait);
    co_await c.async_send("mat", wait);
    c.close();
}

auto
consume(std::string_view name, channels::channel< std::string > &c)
    -> asio::awaitable< void >
{
    auto ec  = channels::error_code();
    auto tok = asio::redirect_error(asio::use_awaitable, ec);
    for (;;)
    {
        auto s = co_await c.async_consume(tok);
        if (ec)
        {
            std::cout << name << " : " << ec.message() << "\n";
            break;
        }
        else
            std::cout << name << " : " << s << "\n";
    }
}

int
main()
{
    auto ioc = asio::io_context();
    auto e   = ioc.get_executor();
    auto c   = channels::channel< std::string >(e);

    asio::co_spawn(e, consume("a", c), asio::detached);
    asio::co_spawn(e, consume("b", c), asio::detached);
    asio::co_spawn(e, consume("c", c), asio::detached);
    asio::co_spawn(e, produce(c), asio::detached);

    ioc.run();
}
