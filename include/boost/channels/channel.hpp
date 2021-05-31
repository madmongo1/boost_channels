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

#include <boost/channels/concepts/std_lockable.hpp>
#include <boost/channels/detail/free_deleter.hpp>
#include <boost/channels/detail/select_wait_op.hpp>
#include <boost/channels/error_code.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/post.hpp>
#include <boost/assert.hpp>
#include <boost/variant2/variant.hpp>

#include <deque>
#include <optional>
#include <queue>

namespace boost::channels {

namespace detail {
template < class ValueType, concepts::Lockable Mutex >
struct channel_impl;
};

// clang-format off
template < class Executor >
concept constructible_with_system_executor =
requires(asio::system_executor se)
{
    static_cast<Executor>(se);
};
// clang-format on

/// @brief Provides a communications channel between asynchronous coroutines.
///
/// Based on golang's channel idiom.
/// @tparam ValueType is the type of value passed through the channel.
/// @tparam Executor is the type of executor associated with the channel.
template < class ValueType,
           class Executor           = asio::any_io_executor,
           concepts::Lockable Mutex = std::mutex >
struct channel
{
    using executor_type = Executor;

    /// @brief T type of value handled by this channel
    using value_type = ValueType;

    /// @brief Construct a channel associated with the system_executor
    /// @param capacity
    template < class T = Executor >
    requires constructible_with_system_executor< T >
    channel(std::size_t capacity = 0);

    /// @brief Construct an executor associated with the given executor
    /// @param exec
    /// @param capacity
    channel(Executor exec, std::size_t capacity = 0);

    ~channel()
    {
        close();
    }

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
        return exec_;
    }

    using impl_type = detail::channel_impl< ValueType, Mutex >;
    using impl_ptr  = std::shared_ptr< impl_type >;

    impl_ptr const &
    get_implementation() const
    {
        return impl_;
    }

  private:
    impl_ptr
    create_impl(std::size_t capacity);

  private:
    Executor exec_;
    impl_ptr impl_;
};

}   // namespace boost::channels

#include <boost/channels/concepts/equality_comparable.hpp>
#include <boost/channels/concepts/executor.hpp>
#include <boost/channels/detail/channel_impl.hpp>
#include <boost/channels/detail/channel_send_op.hpp>
#include <boost/channels/detail/consumer_op_function.hpp>
#include <boost/channels/detail/postit.hpp>
#include <boost/channels/detail/producer_op_function.hpp>

#include <cstdlib>
#include <new>
#include <utility>

namespace boost::channels {

template < class ValueType, class Executor, concepts::Lockable Mutex >
channel< ValueType, Executor, Mutex >::channel(Executor    exec,
                                               std::size_t capacity)
: exec_(std::move(exec))
, impl_(create_impl(capacity))
{
}

template < class ValueType, class Executor, concepts::Lockable Mutex >
auto
channel< ValueType, Executor, Mutex >::consume_if(error_code &ec)
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

template < class ValueType, class Executor, concepts::Lockable Mutex >
auto
channel< ValueType, Executor, Mutex >::create_impl(std::size_t capacity)
    -> impl_ptr
{
    auto extra  = (sizeof(ValueType) * capacity) + (sizeof(impl_type) - 1);
    auto blocks = 1 + (extra / sizeof(impl_type));

    auto pmem = std::calloc(blocks, sizeof(impl_type));
    if (!pmem)
        BOOST_THROW_EXCEPTION(std::bad_alloc());

    try
    {
        return impl_ptr(new (pmem) impl_type(capacity), detail::free_deleter());
    }
    catch (...)
    {
        std::free(pmem);
        throw;
    }
}

/// @brief Rebind a completion handler to a given executor and call a function
/// with the resulting handler
///
/// The implementation is free to modify the handler's type or use the same
/// type.
/// @tparam Executor
/// @tparam Handler
/// @tparam F
/// @param enew
/// @param handler
/// @param f
template < class Executor, class Handler, class F >
void
rebind_and_call(Executor const &enew, Handler &&handler, F &&f)
{
    f(asio::bind_executor(
        enew,
        [handler1 = std::forward< Handler >(handler)]< class... Args >(
            Args && ...args) mutable {
            handler1(std::forward< Args >(args)...);
        }));
}

template < class ValueType, class Executor, concepts::Lockable Mutex >
template < BOOST_ASIO_COMPLETION_TOKEN_FOR(void(error_code)) SendHandler >
BOOST_ASIO_INITFN_RESULT_TYPE(SendHandler, void(error_code))
channel< ValueType, Executor, Mutex >::async_send(value_type    value,
                                                  SendHandler &&token)
{
    return asio::async_initiate< SendHandler, void(error_code) >(
        [value1 = std::move(value),
         impl1  = impl_,
         default_executor =
             get_executor()]< class Handler1 >(Handler1 &&handler1) mutable {
            if (impl1) [[likely]]
            {
                auto exec1 = asio::prefer(
                    asio::get_associated_executor(handler1, default_executor),
                    asio::execution::outstanding_work.tracked);
                auto op = detail::make_producer_op_function< Mutex >(
                    std::move(value1),
                    detail::postit(std::move(exec1),
                                   std::forward< Handler1 >(handler1)));
                impl1->submit_produce_op(op);
            }
            else [[unlikely]]
            {
                auto exec1 =
                    asio::get_associated_executor(handler1, default_executor);

                auto completion = detail::postit(
                    std::move(exec1), std::forward< Handler1 >(handler1));
                completion(error_code(errors::channel_null));
            }
        },
        token);
}

template < class ValueType, class Executor, concepts::Lockable Mutex >
template < BOOST_ASIO_COMPLETION_TOKEN_FOR(void(error_code, ValueType))
               ConsumeHandler >
BOOST_ASIO_INITFN_RESULT_TYPE(ConsumeHandler, void(error_code, ValueType))
channel< ValueType, Executor, Mutex >::async_consume(ConsumeHandler &&token)
{
    if (!impl_) [[unlikely]]
        BOOST_THROW_EXCEPTION(std::logic_error("channel is null"));

    return asio::async_initiate< ConsumeHandler, void(error_code, ValueType) >(
        [impl1 = impl_, default_executor = get_executor()]< class Handler1 >(
            Handler1 &&handler1) {
            if (impl1) [[likely]]
            {
                auto exec1 = asio::prefer(
                    asio::get_associated_executor(handler1, default_executor),
                    asio::execution::outstanding_work.tracked);
                auto handler2 = detail::postit(
                    std::move(exec1), std::forward< Handler1 >(handler1));
                impl1->submit_consume_op(
                    detail::make_consumer_op_function< ValueType, Mutex >(std::move(handler2)));
            }
            else [[unlikely]]
            {
                auto exec1 =
                    asio::get_associated_executor(handler1, default_executor);
                auto handler2 = detail::postit(
                    std::move(exec1), std::forward< Handler1 >(handler1));
                handler2(error_code(errors::channel_null), ValueType {});
            }
        },
        token);
}

template < class ValueType, class Executor, concepts::Lockable Mutex >
void
channel< ValueType, Executor, Mutex >::close() noexcept
{
    if (impl_) [[likely]]
    {
        asio::dispatch(get_executor(), [impl = impl_] { impl->close(); });
    }
}

}   // namespace boost::channels

#endif   // BOOST_CHANNELS_CHANNEL_HPP
