//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CHANNEL_PRODUCER_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CHANNEL_PRODUCER_HPP

#include <boost/channels/channel.hpp>
#include <boost/channels/concepts/executor.hpp>
#include <boost/channels/concepts/selectable_op.hpp>
#include <boost/channels/concepts/std_lockable.hpp>
#include <boost/channels/detail/select_state_base.hpp>
#include <boost/channels/detail/shared_produce_op.hpp>

namespace boost::channels {
/// @brief An object that is associated with both a channel and a value
/// reference. The object coordinates the conditional transfer of the value into
/// the channel when instructed.
/// @tparam ValueType is the type of value to be produced into the channel.
/// @tparam Executor is the default executor which will be associated with
/// asynchronous completion handlers.
template < class ValueType,
           concepts::executor_model Executor,
           concepts::Lockable       Mutex >
struct basic_channel_producer
{
    using executor_type = Executor;
    using mutex_type    = Mutex;

    basic_channel_producer(channel< ValueType, Executor, Mutex > &chan,
                           ValueType &                            source)
    : impl_(chan.get_implementation())
    , exec_(chan.get_executor())
    , source_(source)
    {
    }

    executor_type
    get_executor() const
    {
        return exec_;
    }

    auto
    get_implementation() const
        -> std::shared_ptr< detail::channel_impl< ValueType, Mutex > > const &
    {
        return impl_;
    }

    template < BOOST_ASIO_COMPLETION_TOKEN_FOR(void(error_code)) WaitHandler
                   BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type) >
    BOOST_ASIO_INITFN_RESULT_TYPE(WaitHandler, void(error_code))
    async_wait(
        WaitHandler &&token BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(executor_type))
    {
        return asio::async_initiate< WaitHandler, void(error_code) >(
            [default_executor = exec_,
             impl1            = impl_,
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

    void
    submit_shared_op(
        std::shared_ptr< detail::select_state_base< Mutex > > shared_op,
        int                                                   which) const
    {
        BOOST_CHANNELS_ASSERT(impl_);
        impl_->submit_produce_op(
            detail::make_shared_produce_op(shared_op, source_, which));
    }

  private:
    std::shared_ptr< detail::channel_impl< ValueType, Mutex > > impl_;
    Executor                                                    exec_;
    std::reference_wrapper< ValueType >                         source_;
};

namespace detail {
struct producer_value_archetype
{
};
}   // namespace detail
static_assert(concepts::selectable_op<
              basic_channel_producer< detail::producer_value_archetype,
                                      asio::any_io_executor,
                                      std::mutex > >);

/// @brief Construct a basic_channel_producer which assocaiates the provided
/// channel and value reference.
/// @tparam ValueType is the type of value to be produced to the channel.
/// @tparam Executor is the type of default executor associated with the
/// channel.
/// @param source is a reference to the object to be produced to the channel.
/// @param chan
/// @note The source may be used for multiple transfers to the channel but only
/// one such transfer may be in progress at any one time.
/// @note The source parameter is treated as a reference and is not copied or
/// moved unless there is a produce operation completes. The value object must
/// not be destroyed before any outstanding async_wait operations have
/// completed.
/// @return a basic_channel_producer
template < class ValueType, class Executor, concepts::Lockable Mutex >
basic_channel_producer< ValueType, Executor, Mutex >
operator>>(ValueType &source, channel< ValueType, Executor, Mutex > &chan)
{
    return basic_channel_producer< ValueType, Executor, Mutex >(chan, source);
}

}   // namespace boost::channels
#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CHANNEL_PRODUCER_HPP
