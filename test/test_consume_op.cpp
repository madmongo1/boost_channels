//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#include <boost/channels/detail/implement_channel_queue.hpp>
#include <boost/channels/scope_exit.hpp>

#include <boost/assert.hpp>

#include <atomic>
#include <deque>
#include <iostream>
#include <optional>
#include <queue>
#include <string>

#include <doctest/doctest.h>


using namespace boost;

namespace {

struct test_consumer final
: channels::detail::consume_op_interface< std::string >
{
    void
    commit(value_type &&source) override
    {
        REQUIRE(!mutex_.try_lock());
        REQUIRE(!target.has_value());
        target.emplace(std::move(source));
    }

    bool
    completed() const override
    {
        return target.has_value();
    }

    mutex_type &
    get_mutex() override
    {
        return mutex_;
    }

    mutex_type                  mutex_;
    std::optional< value_type > target;
};

struct test_producer final
: channels::detail::produce_op_interface< std::string >
{
    test_producer(std::string source)
    : source(std::move(source))
    {
    }

    std::string
    consume() override
    {
        REQUIRE(source.has_value());
        REQUIRE(!completed_);
        REQUIRE(!mutex_.try_lock());
        auto result = std::move(*source);
        source.reset();
        completed_ = true;
        return result;
    }

    void
    fail(channels::error_code ec) override
    {
        REQUIRE(source.has_value());
        REQUIRE(!completed_);
        REQUIRE(!mutex_.try_lock());
        this->ec   = ec;
        completed_ = true;
    }

    bool
    completed() const override
    {
        return completed_;
    }

    mutex_type &
    get_mutex() override
    {
        return mutex_;
    }

    mutex_type                   mutex_;
    channels::error_code         ec;
    std::optional< std::string > source;
    bool                         completed_ = false;
};

}   // namespace
TEST_CASE("flush values(0) consumers(0) producers(0)")
{
    channels::detail::value_buffer_data data { 0 };
    auto values = channels::detail::value_buffer_ref< std::string > {
        .pdata = &data, .storage = nullptr
    };
    auto _ = channels::scope_exit([&] { values.destroy(); });

    channels::detail::consumer_queue< std::string > consumers;
    channels::detail::producer_queue< std::string > producers;

    SUBCASE("not closed")
    {
        channels::detail::flush_not_closed(values, consumers, producers);

        CHECK(values.size() == 0);
        CHECK(producers.size() == 0);
        CHECK(consumers.size() == 0);
    }

    SUBCASE("closed")
    {
        channels::detail::flush_closed(values, consumers, producers);

        CHECK(values.size() == 0);
        CHECK(producers.size() == 0);
        CHECK(consumers.size() == 0);
    }
}

TEST_CASE("flush values(0) consumers(0) producers(1)")
{
    channels::detail::value_buffer_data data { 0 };
    auto values = channels::detail::value_buffer_ref< std::string > {
        .pdata = &data, .storage = nullptr
    };
    auto _ = channels::scope_exit([&] { values.destroy(); });

    channels::detail::consumer_queue< std::string > consumers;
    channels::detail::producer_queue< std::string > producers;

    std::string const original0 = "0123456789012345678901234567890123456789";
    auto              p0        = std::make_shared< test_producer >(original0);
    producers.push(p0);

    SUBCASE("not closed")
    {
        channels::detail::flush_not_closed(values, consumers, producers);

        CHECK(values.size() == 0);
        CHECK(producers.size() == 1);
        CHECK(consumers.size() == 0);
        CHECK(!p0->completed());
        CHECK(!p0->ec);
        CHECK(p0->source.has_value());
        CHECK(*(p0->source) == original0);
    }

    SUBCASE("closed")
    {
        channels::detail::flush_closed(values, consumers, producers);

        CHECK(values.size() == 0);
        CHECK(producers.size() == 0);
        CHECK(consumers.size() == 0);
        CHECK(p0->completed());
        CHECK(p0->ec == channels::errors::channel_closed);
        CHECK(p0->source.has_value());
        CHECK(*(p0->source) == original0);
    }
}

TEST_CASE("flush values(0) consumers(1) producers(0)")
{
    channels::detail::value_buffer_data data { 0 };
    auto values = channels::detail::value_buffer_ref< std::string > {
        .pdata = &data, .storage = nullptr
    };
    auto _ = channels::scope_exit([&] { values.destroy(); });

    channels::detail::consumer_queue< std::string > consumers;
    channels::detail::producer_queue< std::string > producers;

    auto c0 = std::make_shared< test_consumer >();
    consumers.push(c0);

    SUBCASE("not closed")
    {
        channels::detail::flush_not_closed(values, consumers, producers);

        CHECK(values.size() == 0);
        CHECK(producers.size() == 0);
        CHECK(consumers.size() == 1);
        CHECK(!c0->completed());
        CHECK(!c0->target);
    }

    SUBCASE("closed")
    {
        channels::detail::flush_closed(values, consumers, producers);

        CHECK(values.size() == 0);
        CHECK(producers.size() == 0);
        CHECK(consumers.size() == 0);
        CHECK(c0->completed());
        REQUIRE(c0->target);
        auto &[ec, s] = *(c0->target);
        CHECK(ec == channels::errors::channel_closed);
        CHECK(s == std::string());
    }
}

TEST_CASE("flush values(0) consumers(1) producers(1)")
{
    channels::detail::value_buffer_data data { 0 };
    auto values = channels::detail::value_buffer_ref< std::string > {
        .pdata = &data, .storage = nullptr
    };
    auto _ = channels::scope_exit([&] { values.destroy(); });

    channels::detail::consumer_queue< std::string > consumers;
    channels::detail::producer_queue< std::string > producers;

    std::string const original0 = "0123456789012345678901234567890123456789";
    auto              p0        = std::make_shared< test_producer >(original0);
    producers.push(p0);
    auto c0 = std::make_shared< test_consumer >();
    consumers.push(c0);

    SUBCASE("not closed")
    {
        channels::detail::flush_not_closed(values, consumers, producers);

        CHECK(values.size() == 0);
        CHECK(producers.size() == 0);
        CHECK(consumers.size() == 0);
        CHECK(p0->completed());
        CHECK(c0->completed());
        CHECK(!p0->ec);
        CHECK(!p0->source);
        REQUIRE(c0->target);
        auto &[ec, s] = *(c0->target);
        CHECK(!ec);
        CHECK(s == original0);
    }

    SUBCASE("closed")
    {
        channels::detail::flush_closed(values, consumers, producers);

        CHECK(values.size() == 0);
        CHECK(producers.size() == 0);
        CHECK(consumers.size() == 0);
        CHECK(p0->completed());
        CHECK(c0->completed());
        CHECK(p0->ec);
        CHECK(p0->source);
        REQUIRE(c0->target);
        auto &[ec, s] = *(c0->target);
        CHECK(ec == channels::errors::channel_closed);
        CHECK(s.empty());
    }
}

TEST_CASE("flush 1 0 0")
{
}

TEST_CASE("flush 1 0 1")
{
}

TEST_CASE("flush 1 1 0")
{
}

TEST_CASE("flush 1 1 1")
{
}
