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

#include <boost/asio/io_context.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_future.hpp>

#include <doctest/doctest.h>

using namespace boost;
using namespace std::literals;

TEST_CASE("producer")
{
    auto ioc = asio::io_context();
    auto e   = ioc.get_executor();

    auto chan = channels::channel< std::string >(e);

    auto const original = "0123456789012345678901234567890123456789"s;
    auto       source   = original;

    (source >> chan).async_wait([&](channels::error_code ec) {
        CHECK(e.running_in_this_thread());
        CHECK(!ec);
        CHECK(source != original);
    });

    chan.async_consume([&](channels::error_code ec, std::string s) {
        CHECK(e.running_in_this_thread());
        CHECK(!ec);
        CHECK(s == original);
    });

    ioc.run();
}

TEST_CASE("2 producers")
{
    auto ioc = asio::io_context();
    auto e   = ioc.get_executor();

    auto c1 = channels::channel< std::string >(e);
    auto c2 = channels::channel< std::string >(e);

    auto const o1   = "9876543210987654321098765432109876543210"s;
    auto const o2   = "0123456789012345678901234567890123456789"s;
    auto       src1 = o1;
    auto       src2 = o2;

    std::string s1, s2;
    channels::tie(s1 << c1, s2 << c2)
        .async_wait([&](channels::error_code ec, int which) {
            CHECK(e.running_in_this_thread());
            CHECK(!ec);
            CHECK(s1 == o1);
            CHECK(which == 0);
        });

    (src1 >> c1).async_wait([&](channels::error_code ec) {
        CHECK(e.running_in_this_thread());
        CHECK(!ec);
        CHECK(src1 != o1);
    });

    ioc.run();
}

TEST_CASE("2 producers, 2 consumer, threads")
{
    auto e = asio::system_executor();

    auto c1 = channels::channel< std::string >(e);
    auto c2 = channels::channel< std::string >(e);

    auto const o1   = "9876543210987654321098765432109876543210"s;
    auto const o2   = "0123456789012345678901234567890123456789"s;
    auto       src1 = o1;
    auto       src2 = o2;

    std::string s1, s2;

    auto f1 = std::async(std::launch::async, [&]() {
        int i = 0;
        for (;; ++i)
        {
            auto ec = channels::error_code();
            auto which1 =
                channels::tie(s1 << c1, s2 << c2)
                    .async_wait(asio::redirect_error(asio::use_future, ec))
                    .get();
            if (ec)
            {
                CHECK(i == 1000);
                break;
            }
            CHECK(!ec);
            auto in_range1 = which1 >= 0 && which1 <= 1;
            INFO("which1=", which1);
            CHECK(in_range1);

            auto which2 =
                channels::tie(s1 << c1, s2 << c2)
                    .async_wait(asio::redirect_error(asio::use_future, ec))
                    .get();
            CHECK(!ec);
            auto in_range2 = which2 >= 0 && which2 <= 1;
            CHECK(in_range2);
            CHECK(which2 != which1);
        }
    });

    auto f2 = std::async(std::launch::async, [&]() {
        for (int i = 0; i < 1000; ++i)
        {
            auto ec = channels::error_code();
            auto which1 =
                channels::tie(src1 >> c1, src2 >> c2)
                    .async_wait(asio::redirect_error(asio::use_future, ec))
                    .get();
            CHECK(!ec);
            auto in_range1 = which1 >= 0 && which1 <= 1;
            CHECK(in_range1);

            auto which2 =
                which1 == 0
                    ? channels::tie(src2 >> c2)
                          .async_wait(redirect_error(asio::use_future, ec))
                          .get()
                    : channels::tie(src1 >> c1)
                          .async_wait(redirect_error(asio::use_future, ec))
                          .get();
            CHECK(!ec);
            auto in_range2 = which2 >= 0 && which2 <= 1;
            CHECK(in_range2);

            CHECK(which2 == 0);
            CHECK(src1 != o1);
            CHECK(src2 != o2);

            src1 = o1;
            src2 = o2;
        }
        c1.close();
        c2.close();
    });

    f1.get();
    f2.get();
}
