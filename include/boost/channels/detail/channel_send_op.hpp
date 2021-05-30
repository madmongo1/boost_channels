//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_CHANNEL_SENDER_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_CHANNEL_SENDER_HPP

#include <boost/asio/associated_executor.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/post.hpp>
namespace boost::channels::detail {

template < class ValueType >
struct channel_send_op_concept
{
    channel_send_op_concept(ValueType value)
    : value_(std::move(value))
    {
    }
    /// @brief Consume the value in the send op, destroy the op and schedule the
    /// completion handler on the correct executor.

    /// @return Return the value taken from the channel_send_op_concept
    virtual ValueType
    consume() = 0;

    virtual void
    notify_error(error_code ec) = 0;

  protected:
    ValueType value_;
};

template < class ValueType, class Executor, class Handler >
struct basic_channel_send_op final : channel_send_op_concept< ValueType >
{
    basic_channel_send_op(ValueType value, Executor exec, Handler handler)
    : channel_send_op_concept< ValueType >(std::move(value))
    , exec_(std::move(exec))
    , handler_(std::move(handler))
    {
    }

    /// @brief Consume the value in the send op, destroy the op and schedule the
    /// completion handler on the correct executor
    /// @return
    virtual ValueType
    consume() override;

    virtual void
    notify_error(error_code ec) override;

  private:
    void
    destroy();

    Executor exec_;
    Handler  handler_;
};

template < class ValueType, class DefaultExecutor, class Handler >
auto
create_channel_send_op(ValueType              value,
                       DefaultExecutor const &dflt_exec,
                       Handler &&             handler)
{
    auto exec =
        asio::prefer(::boost::asio::get_associated_executor(handler, dflt_exec),
                     ::boost::asio::execution::outstanding_work.tracked);

    using op_type = basic_channel_send_op< ValueType,
                                           decltype(exec),
                                           std::decay_t< Handler > >;
    return new op_type(
        std::move(value), std::move(exec), std::forward< Handler >(handler));
}

template < class ValueType, class Executor, class Handler >
auto
basic_channel_send_op< ValueType, Executor, Handler >::consume() -> ValueType
{
    auto result  = std::move(this->value_);
    auto handler = ::boost::asio::bind_executor(
        std::move(exec_),
        [handler = std::move(handler_)]() mutable { handler(error_code()); });
    destroy();
    asio::post(std::move(handler));
    return result;
}

template < class ValueType, class Executor, class Handler >
auto
basic_channel_send_op< ValueType, Executor, Handler >::notify_error(
    error_code ec) -> void
{
    auto handler = asio::bind_executor(
        std::move(exec_),
        [handler = std::move(handler_), ec]() mutable { handler(ec); });
    destroy();
    asio::post(std::move(handler));
}

template < class ValueType, class Executor, class Handler >
auto
basic_channel_send_op< ValueType, Executor, Handler >::destroy() -> void
{
    delete this;
}

}   // namespace boost::channels::detail
#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_CHANNEL_SENDER_HPP
