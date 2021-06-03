//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CHANNEL_CONSUMER_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CHANNEL_CONSUMER_HPP

#include <boost/channels/channel.hpp>
#include <boost/channels/concepts/executor.hpp>
#include <boost/channels/concepts/selectable_op.hpp>
#include <boost/channels/concepts/std_lockable.hpp>
#include <boost/channels/detail/select_state_base.hpp>
#include <boost/channels/detail/shared_consume_op.hpp>

namespace boost::channels {
template < class ValueType,
           concepts::executor_model Executor,
           concepts::Lockable       Mutex >
struct basic_channel_consumer
{
    using executor_type = Executor;
    using mutex_type    = Mutex;

    basic_channel_consumer(channel< ValueType, Executor, Mutex > &chan,
                           ValueType &                            sink)
    : impl_(chan.get_implementation())
    , exec_(chan.get_executor())
    , sink_(sink)
    {
    }

    executor_type
    get_executor() const
    {
        return exec_;
    }

    std::shared_ptr< detail::channel_impl< ValueType, Mutex > > const &
    get_implementation() const
    {
        return impl_;
    }

    void
    submit_shared_op(
        std::shared_ptr< detail::select_state_base< Mutex > > shared_op,
        int                                                   which) const
    {
        BOOST_CHANNELS_ASSERT(impl_);
        impl_->submit_consume_op(
            detail::make_shared_consume_op(shared_op, sink_, which));
    }

  private:
    std::shared_ptr< detail::channel_impl< ValueType, Mutex > > impl_;
    Executor                                                    exec_;
    std::reference_wrapper< ValueType >                         sink_;
};

static_assert(
    concepts::selectable_op< basic_channel_consumer< std::string,
                                                     asio::any_io_executor,
                                                     std::mutex > >);

template < class ValueType, class Executor, concepts::Lockable Mutex >
basic_channel_consumer< ValueType, Executor, Mutex >
operator<<(ValueType &target, channel< ValueType, Executor, Mutex > &chan)
{
    return basic_channel_consumer< ValueType, Executor, Mutex >(chan, target);
}

}   // namespace boost::channels
#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CHANNEL_CONSUMER_HPP
