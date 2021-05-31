//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#include <boost/channels/channel.hpp>
#include <boost/channels/concepts/std_lockable.hpp>
#include <boost/channels/config.hpp>
#include <boost/channels/detail/postit.hpp>
#include <boost/channels/detail/produce_op_interface.hpp>
#include <boost/channels/detail/producer_op_function.hpp>

#include <boost/asio/io_context.hpp>

#include <doctest/doctest.h>

namespace boost::channels {

template < class ValueType, class Executor, concepts::Lockable Mutex >
struct basic_produce_op
{
    using executor_type = Executor;

    basic_produce_op(channel< ValueType, Executor, Mutex > &chan,
                     ValueType &                            source)
    : pchannel_(&chan)
    , source_(source)
    {
    }

    template < BOOST_ASIO_COMPLETION_TOKEN_FOR(void(error_code)) WaitHandler
                   BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type) >
    BOOST_ASIO_INITFN_RESULT_TYPE(WaitHandler, void(error_code))
    async_wait(
        WaitHandler &&token BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(executor_type))
    {
        return asio::async_initiate< WaitHandler, void(error_code) >(
            [default_executor = pchannel_->get_executor(),
             impl1            = pchannel_->get_implementation(),
             source1 = source_]< class Handler1 >(Handler1 &&handler1) mutable {
                //
                if (impl1) [[likely]]
                {
                    auto e1 =
                        asio::prefer(asio::get_associated_executor(
                                         handler1, default_executor),
                                     asio::execution::outstanding_work.tracked);

                    auto handler2 = detail::postit(
                        std::move(e1), std::forward< Handler1 >(handler1));
                    auto op = detail::make_producer_op_function< Mutex >(
                        source1, std::move(handler2));

                    impl1->submit_produce_op(op);
                }
                else
                {
                    auto e1 = asio::get_associated_executor(handler1,
                                                            default_executor);

                    auto handler2 = detail::postit(
                        std::move(e1), std::forward< Handler1 >(handler1));
                    handler2(error_code(errors::channel_null));
                }
            },
            token);
    }

  private:
    channel< ValueType, Executor, Mutex > *pchannel_;
    std::reference_wrapper< ValueType >    source_;
};

template < class ValueType, class Executor, concepts::Lockable Mutex >
basic_produce_op< ValueType, Executor, Mutex >
operator>>(ValueType &source, channel< ValueType, Executor, Mutex > &chan)
{
    return basic_produce_op< ValueType, Executor, Mutex >(chan, source);
}

template < class ValueType >
struct consume_op
{
};

}   // namespace boost::channels

using namespace boost;
using namespace std::literals;

TEST_CASE("producer")
{
    auto ioc = asio::io_context();
    auto e   = ioc.get_executor();

    auto chan = channels::channel< std::string >(e);

    auto const original = "0123456789012345678901234567890123456789"s;
    auto       source   = original;

    auto prod = source >> chan;

    prod.async_wait([&](channels::error_code ec) {
        CHECK(!ec);
        CHECK(source != original);
    });

    chan.async_consume([&](channels::error_code ec, std::string s) {
        CHECK(!ec);
        CHECK(s == original);
    });

    ioc.run();
}