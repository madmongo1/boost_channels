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
#include <boost/asio/defer.hpp>
#include <boost/channels/error_code.hpp>

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

template < class ValueType, class Handler >
struct basic_channel_send_op final : channel_send_op_concept< ValueType >
{
    basic_channel_send_op(ValueType value, Handler handler)
    : channel_send_op_concept< ValueType >(std::move(value))
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

    Handler handler_;
};

template < class ValueType, class Handler >
auto
create_channel_send_op(ValueType value, Handler &&handler)
{
    using op_type = basic_channel_send_op< ValueType, std::decay_t< Handler > >;
    return new op_type(std::move(value), std::forward< Handler >(handler));
}

template < class ValueType, class Handler >
auto
basic_channel_send_op< ValueType, Handler >::consume() -> ValueType
{
    // move the result value to the local scope
    auto result = std::move(this->value_);

    // move the handler to local scope and transform it to be associated with
    // the correct executor.
    auto ex      = asio::get_associated_executor(handler_);
    auto handler = ::boost::asio::bind_executor(
        ex,
        [handler = std::move(handler_)]() mutable { handler(error_code()); });

    // then destroy this object (equivalent to delete this)
    destroy();

    // post the modified handler to its associated executor
    asio::defer(std::move(handler));

    // return the value from the local scope to the caller (but note that NRVO
    // will guarantee that there is not actually a second move)
    return result;
}

template < class ValueType, class Handler >
auto
basic_channel_send_op< ValueType, Handler >::notify_error(error_code ec) -> void
{
    auto ex      = asio::get_associated_executor(handler_);
    auto handler = asio::bind_executor(
        ex, [handler = std::move(handler_), ec]() mutable { handler(ec); });
    destroy();
    asio::defer(std::move(handler));
}

template < class ValueType, class Handler >
auto
basic_channel_send_op< ValueType, Handler >::destroy() -> void
{
    delete this;
}

}   // namespace boost::channels::detail
#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_CHANNEL_SENDER_HPP
