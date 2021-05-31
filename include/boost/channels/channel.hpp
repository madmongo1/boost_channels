//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_CHANNEL_HPP
#define BOOST_CHANNELS_CHANNEL_HPP

#include <boost/channels/detail/free_deleter.hpp>
#include <boost/channels/detail/select_wait_op.hpp>
#include <boost/channels/error_code.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/assert.hpp>
#include <boost/variant2/variant.hpp>

#include <deque>
#include <optional>
#include <queue>

namespace boost::channels {

namespace detail {
template < class ValueType, class Executor >
struct channel_impl;
};

/// @brief Provides a communications channel between asynchronous coroutines.
///
/// Based on golang's channel idiom.
/// @tparam ValueType is the type of value passed through the channel.
/// @tparam Executor is the type of executor associated with the channel.
template < class ValueType, class Executor = asio::any_io_executor >
struct channel
{
    using executor_type = Executor;

    /// @brief T type of value handled by this channel
    using value_type = ValueType;

    channel(Executor exec, std::size_t capacity = 0);
    ~channel()
    {
        close();
    }

    /// @brief Return a boolean indicating whether a call to consume may be
    /// made.
    ///
    /// @note The return value of this function is only valid until the next
    /// suspension point as after that, other coroutines could have sent or
    /// consumed values.
    /// @return A boolean indicating whether a call to @see consume may be made
    std::size_t
    ready() const;

    /// @brief Consume one value from the channel if there is one available.
    /// @param ec is an out parameter referencing an error_code which will be
    /// overwritten by this function. If a value is available, ec will be
    /// cleared. Otherwise, ec will contain an error code.
    /// @return A value, which will be default-constructed if ec is assigned an
    /// error code.
    ValueType
    consume(error_code &ec);

    /// @brief Consume one value if there is a value available to be consumed
    /// immediately.
    /// @param ec is a reference to an error_code. The value of this variable
    /// shall be set to error_code() unless :
    /// - The channel has no pending values in its internal queue, and
    /// - The channel has no waiting senders, and
    /// - The channel is in the closed state as a result of a call to close()
    /// @return An optional @see value_type which shall have a value if a value
    /// was available to be consumed, otherwise empty.
    std::optional< value_type >
    consume_if(error_code &ec);

    /// @brief Initiate an asynchronous send of a value to the channel
    ///
    /// If the channel is closed, the completion handler will be invoked with
    /// error_code errors::channel_closed. The completion handler will always be
    /// invoked as if by a call to post(handler).
    /// @tparam SendHandler is the type of completion token used to
    /// configure the initiation function
    /// @param value is the value to send into the channel
    /// @param token is the completion token
    /// @return depends on CompletionToken
    template < BOOST_ASIO_COMPLETION_TOKEN_FOR(void(error_code)) SendHandler
                   BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type) >
    BOOST_ASIO_INITFN_RESULT_TYPE(SendHandler, void(error_code))
    async_send(value_type value,
               SendHandler &&token
                   BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(executor_type));

    template < BOOST_ASIO_COMPLETION_TOKEN_FOR(void(error_code, ValueType))
                   ConsumeHandler BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(
                       executor_type) >
    BOOST_ASIO_INITFN_RESULT_TYPE(ConsumeHandler, void(error_code, ValueType))
    async_consume(ConsumeHandler &&token
                      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(executor_type));

    /// @brief Cause the channel to be closed.
    ///
    /// All values already buffered will be delivered to consumers.
    /// All pending async_send operations will be cancelled with
    /// errors::channel_closed. The values associated with those operations will
    /// be destroyed.
    /// Subsequent async_send operations will wait with errors::channel_closed
    /// Any async_consume operation which does not match an already-stored value
    /// will wait with errors::channel_closed and a default value will be
    /// returned.
    /// @post closed() == true
    /// @note If called on an already closed channel, no action is taken.
    void
    close() noexcept;

    executor_type const &
    get_executor() const
    {
        BOOST_ASSERT(impl_);
        return impl_->get_executor();
    }

  private:
    using impl_type = detail::channel_impl< ValueType, Executor >;
    using impl_ptr  = std::shared_ptr< impl_type >;

    impl_ptr
    create_impl(Executor exec, std::size_t capacity);

  private:
    impl_ptr impl_;
};

}   // namespace boost::channels

#include <boost/channels/detail/channel_impl.hpp>
#include <boost/channels/detail/channel_send_op.hpp>

#include <cstdlib>
#include <new>
#include <utility>

namespace boost::channels {

template < class ValueType, class Executor >
channel< ValueType, Executor >::channel(Executor exec, std::size_t capacity)
: impl_(create_impl(std::move(exec), capacity))
{
}

template < class ValueType, class Executor >
auto
channel< ValueType, Executor >::consume_if(error_code &ec)
    -> std::optional< value_type >
{
    ec.clear();

    if (!impl_) [[unlikely]]
    {
        ec = make_error_code(errors::channel_errors::channel_null);
        return {};
    }
    else
    {
        return impl_->consume_if(ec);
    }
}

template < class ValueType, class Executor >
auto
channel< ValueType, Executor >::create_impl(Executor exec, std::size_t capacity)
    -> impl_ptr
{
    auto extra  = (sizeof(ValueType) * capacity) + (sizeof(impl_type) - 1);
    auto blocks = 1 + (extra / sizeof(impl_type));

    auto pmem = std::calloc(blocks, sizeof(impl_type));
    if (!pmem)
        BOOST_THROW_EXCEPTION(std::bad_alloc());

    try
    {
        return impl_ptr(new (pmem) impl_type(std::move(exec), capacity),
                        detail::free_deleter());
    }
    catch (...)
    {
        std::free(pmem);
        throw;
    }
}

template < class ValueType, class Executor >
template < BOOST_ASIO_COMPLETION_TOKEN_FOR(void(error_code)) SendHandler >
BOOST_ASIO_INITFN_RESULT_TYPE(SendHandler, void(error_code))
channel< ValueType, Executor >::async_send(value_type    value,
                                           SendHandler &&token)
{
    if (!impl_) [[unlikely]]
        BOOST_THROW_EXCEPTION(std::logic_error("channel is null"));

    return asio::async_initiate< SendHandler, void(error_code) >(
        [value1 = std::move(value), impl1 = impl_](auto &&handler) {
            asio::dispatch(
                impl1->get_executor(),
                [impl2    = impl1,
                 handler2 = std::forward< decltype(handler) >(handler),
                 value2   = std::move(value1)]() mutable {
                    impl2->notify_send(
                        detail::create_channel_send_op(std::move(value2),
                                                       impl2->get_executor(),
                                                       std::move(handler2)));
                });
        },
        token);
}

template < class ValueType, class Executor >
template < BOOST_ASIO_COMPLETION_TOKEN_FOR(void(error_code, ValueType))
               ConsumeHandler >
BOOST_ASIO_INITFN_RESULT_TYPE(ConsumeHandler, void(error_code, ValueType))
channel< ValueType, Executor >::async_consume(ConsumeHandler &&token)
{
    if (!impl_) [[unlikely]]
        BOOST_THROW_EXCEPTION(std::logic_error("channel is null"));

    return asio::async_initiate< ConsumeHandler, void(error_code, ValueType) >(
        [impl1 = impl_](auto &&handler) {
            asio::dispatch(
                impl1->get_executor(),
                [impl2 = impl1,
                 handler2 =
                     std::forward< decltype(handler) >(handler)]() mutable {
                    impl2->notify_consume(
                        detail::create_channel_consume_op< ValueType >(
                            impl2->get_executor(), std::move(handler2)));
                });
        },
        token);
}

template < class ValueType, class Executor >
void
channel< ValueType, Executor >::close() noexcept
{
    if (impl_) [[likely]]
        asio::dispatch(impl_->get_executor(),
                       [impl = impl_] { impl->close(); });
}

template < class ValueType, class Executor >
std::size_t
channel< ValueType, Executor >::ready() const
{
    if (impl_) [[likely]]
        return impl_->ready();
    else
        // in the case where the channel is null, we return true so that the
        // next call to consume will report the error
        return true;
}

}   // namespace boost::channels

#endif   // BOOST_CHANNELS_CHANNEL_HPP
