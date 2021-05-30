//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_CHANNEL_CONSUME_OP_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_CHANNEL_CONSUME_OP_HPP

#include <boost/asio/associated_executor.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/defer.hpp>

namespace boost::channels::detail {
template < class ValueType >
struct channel_consume_op_concept
{
    virtual void
    notify(error_code ec, ValueType v) = 0;

    virtual ~channel_consume_op_concept() = default;
};

template < class ValueType, class Executor, class Handler >
struct basic_channel_consume_op final : channel_consume_op_concept< ValueType >
{
    basic_channel_consume_op(Executor exec, Handler handler)
    : exec_(std::move(exec))
    , handler_(std::move(handler))
    {
    }

    virtual void
    notify(error_code ec, ValueType v) override
    {
        auto handler = asio::bind_executor(
            std::move(exec_),
            [handler = std::move(handler_), ec, v = std::move(v)]() mutable {
                handler(ec, std::move(v));
            });
        destroy();
        asio::defer(std::move(handler));
    }

  private:
    void
    destroy() noexcept
    {
        delete this;
    }

    Executor exec_;
    Handler  handler_;
};

template < class ValueType, class DefaultExecutor, class Handler >
auto
create_channel_consume_op(DefaultExecutor const &dflt_exec, Handler &&handler)
{
    auto exec = asio::prefer(asio::get_associated_executor(handler, dflt_exec),
                             asio::execution::outstanding_work.tracked);

    using op_type = basic_channel_consume_op< ValueType,
                                              decltype(exec),
                                              std::decay_t< Handler > >;
    return new op_type(std::move(exec), std::forward< Handler >(handler));
}

}   // namespace boost::channels::detail
#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_CHANNEL_CONSUME_OP_HPP
