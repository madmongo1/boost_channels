//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//
#include <boost/channels/channel.hpp>
#include <boost/channels/channel_consumer.hpp>
#include <boost/channels/channel_producer.hpp>
#include <boost/channels/tie.hpp>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_future.hpp>

#include <iostream>
#include <string>

#include <string_view>

using namespace boost;
using namespace std::literals;

auto
produce(channels::channel< std::string > &c) -> asio::awaitable< void >
{
    auto [rng, dist] = [] {
        std::random_device rd;
        std::mt19937       g(rd());
        return std::make_tuple(g, std::uniform_int_distribution< int >(0, 100));
    }();
    constexpr auto wait = asio::use_awaitable;
    auto           t = asio::steady_timer(co_await asio::this_coro::executor);
    std::string    words[] = { "The", "cat", "sat", "on", "the", "mat" };
    for (auto &word : words)
    {
        co_await(word >> c).async_wait(wait);
        t.expires_after(std::chrono::milliseconds(50 + dist(rng)));
        co_await t.async_wait(wait);
    }
    c.close();
}

auto
consume(channels::channel< std::string > &c1,
        channels::channel< std::string > &c2,
        channels::channel< std::string > &c3) -> asio::awaitable< void >
{
    auto [rng, dist] = [] {
        std::random_device rd;
        std::mt19937       g(rd());
        return std::make_tuple(g, std::uniform_int_distribution< int >(0, 100));
    }();
    auto t        = asio::steady_timer(co_await asio::this_coro::executor);
    auto ec       = channels::error_code();
    auto tok      = asio::redirect_error(asio::use_awaitable, ec);
    bool done[]   = { false, false, false };
    auto not_done = [](bool done) { return !done; };

    std::string s;
    // consume as fast as we can until one of the channels is closed
    while (std::all_of(std::begin(done), std::end(done), not_done))
    {
        t.expires_after(std::chrono::milliseconds(50 + dist(rng)));
        co_await t.async_wait(asio::use_awaitable);
        auto which =
            co_await channels::tie(s << c1, s << c2, s << c3).async_wait(tok);
        if (ec)
        {
            std::cout << which << " : " << ec.message() << "\n";
            done[which] = true;
            break;
        }
        else
            std::cout << which << " : " << s << "\n";
    }

    // now clean up remaining channels
    channels::channel< std::string > *chans[] = { &c1, &c2, &c3 };
    for (int i = 0; i < 3; ++i)
    {
        while (!done[i])
        {
            ec.clear();
            s = co_await chans[i]->async_consume(tok);
            if (ec)
            {
                std::cout << i << " : " << ec.message() << "\n";
                done[i] = true;
            }
            else
                std::cout << i << " : " << s << "\n";
        }
    }
}

int
main()
{
    auto e = asio::system_executor();

    auto c1 = channels::channel< std::string >(e),
         c2 = channels::channel< std::string >(e),
         c3 = channels::channel< std::string >(e);

    auto fc  = asio::co_spawn(e, consume(c1, c2, c3), asio::use_future);
    auto fp1 = asio::co_spawn(e, produce(c1), asio::use_future);
    auto fp2 = asio::co_spawn(e, produce(c2), asio::use_future);
    auto fp3 = asio::co_spawn(e, produce(c3), asio::use_future);

    fc.get();
    fp1.get();
    fp2.get();
    fp3.get();
}
