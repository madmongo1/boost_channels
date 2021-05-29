//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//
#include <doctest/doctest.h>

#include <boost/channels/channel.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/as_single.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/redirect_error.hpp>

namespace asio     = boost::asio;
namespace channels = boost::channels;

using namespace std::literals;

TEST_CASE("consume_if")
{
    auto ioc = asio::io_context();

    auto                 c = channels::channel< int >(ioc.get_executor());
    channels::error_code ec;

    auto val = c.consume_if(ec);
    CHECK(!val);

    asio::co_spawn(
        ioc,
        [&c]() -> asio::awaitable< void > {
            channels::error_code ec;
            co_await c.async_send(
                42, asio::redirect_error(asio::use_awaitable, ec));
        },
        asio::detached);

    asio::co_spawn(
        ioc,
        [&c]() -> asio::awaitable< void > {
            auto ec  = channels::error_code();
            auto val = co_await c.async_consume(
                asio::redirect_error(asio::use_awaitable, ec));
            CHECK(val == 42);
            CHECK(!ec);
        },
        asio::detached);

    ioc.run();
}

TEST_CASE("size 0 produce before consume")
{
    auto ioc = asio::io_context();

    auto c = channels::channel< int >(ioc.get_executor());

    bool sent     = false;
    bool consumed = false;

    c.async_send(43, [&](channels::error_code ec) {
        CHECK(!ec);
        sent = true;
    });
    c.async_consume([&](channels::error_code ec, int x) {
        CHECK(!ec);
        CHECK(x == 43);
        consumed = true;
    });

    ioc.run_for(50ms);
    CHECK(sent);
    CHECK(consumed);
}