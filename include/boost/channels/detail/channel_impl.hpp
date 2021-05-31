//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_DETAIL_CHANNEL_IMPL_HPP
#define BOOST_CHANNELS_DETAIL_CHANNEL_IMPL_HPP

#include <boost/channels/concepts/std_lockable.hpp>
#include <boost/channels/detail/channel_consume_op.hpp>
#include <boost/channels/detail/channel_send_op.hpp>
#include <boost/channels/detail/implement_channel_queue.hpp>
#include <boost/channels/detail/value_buffer.hpp>

#include <boost/assert.hpp>

#include <cstddef>
#include <deque>
#include <queue>

namespace boost::channels::detail {

template < class ValueType, concepts::Lockable Mutex >
struct alignas(std::max_align_t) channel_impl final
{
    using value_type = ValueType;

    channel_impl(std::size_t capacity);

    channel_impl(channel_impl const &) = delete;

    channel_impl &
    operator=(channel_impl const &) = delete;

    channel_impl(channel_impl &&) = delete;

    channel_impl &
    operator=(channel_impl &&) = delete;

    ~channel_impl();

    void
    close();

    void
    submit_consume_op(basic_consumer_ptr< ValueType, Mutex > consume_op);

    void
    submit_produce_op(basic_producer_ptr< ValueType, Mutex > produce_op);

    std::optional< value_type >
    consume_if(error_code &ec);

  private:
    std::aligned_storage_t< sizeof(ValueType) > *
    storage()
    {
        return reinterpret_cast<
            std::aligned_storage_t< sizeof(ValueType) > * >(this + 1);
    }

    value_buffer_ref< ValueType >
    buffer()
    {
        return value_buffer_ref< ValueType > { &buffer_data_, storage() };
    }

    Mutex mutex_;

    value_buffer_data buffer_data_;

    /// A list of receivers waiting to receive a value
    basic_consumer_queue< ValueType, Mutex > consumers_;

    /// A list of senders waiting to send a value
    basic_producer_queue< ValueType, Mutex > producers_;

    // current state of the implementation

    enum state_code
    {
        state_running,
        state_closed,
    } state_ = state_running;
};

//
//
//

template < class ValueType, concepts::Lockable Mutex >
channel_impl< ValueType, Mutex >::channel_impl(std::size_t capacity)
: buffer_data_ { .capacity = capacity }
{
}

template < class ValueType, concepts::Lockable Mutex >
channel_impl< ValueType, Mutex >::~channel_impl()
{
    auto ring_buffer = buffer();
    switch (state_)
    {
    case state_running:
        state_ = state_closed;
        flush_closed(ring_buffer, consumers_, producers_);
        break;
    case state_closed:
        break;
    }
    ring_buffer.destroy();
}

template < class ValueType, concepts::Lockable Mutex >
void
channel_impl< ValueType, Mutex >::close()
{
    auto lock = std::unique_lock(mutex_);
    switch (state_)
    {
    case state_running:
        state_ = state_closed;
        flush_closed(buffer(), consumers_, producers_);
        break;
    case state_closed:
        break;
    }
}

template < class ValueType, concepts::Lockable Mutex >
void
channel_impl< ValueType, Mutex >::submit_consume_op(
    basic_consumer_ptr< ValueType, Mutex > consume_op)
{
    auto lck = std::lock_guard(mutex_);

    consumers_.push(std::move(consume_op));

    switch (state_)
    {
    case state_closed:
        flush_closed(buffer(), consumers_, producers_);
        break;
    case state_running:
        flush_not_closed(buffer(), consumers_, producers_);
        break;
    }
}

template < class ValueType, concepts::Lockable Mutex >
void
channel_impl< ValueType, Mutex >::submit_produce_op(
    basic_producer_ptr< ValueType, Mutex > produce_op)
{
    auto lck = std::lock_guard(mutex_);

    producers_.push(std::move(produce_op));

    switch (state_)
    {
    case state_closed:
        flush_closed(buffer(), consumers_, producers_);
        break;
    case state_running:
        flush_not_closed(buffer(), consumers_, producers_);
        break;
    }
}

template < class ValueType, concepts::Lockable Mutex >
auto
channel_impl< ValueType, Mutex >::consume_if(error_code &ec)
    -> std::optional< value_type >
{
    auto lock = std::unique_lock(mutex_);

    std::optional< value_type > result;

    if (auto ring_buffer = buffer(); ring_buffer.size())
    {
        result.emplace(std::move(ring_buffer.front()));
        ring_buffer.pop();
    }
    else
    {
        switch (state_)
        {
        case state_closed:
            ec = errors::channel_closed;
            break;
        case state_running:
            if (!producers_.empty())
            {
                result.emplace(producers_.front()->consume());
                producers_.pop();
            }
            break;
        }
    }

    return result;
}

}   // namespace boost::channels::detail
#endif   // BOOST_CHANNELS_DETAIL_CHANNEL_IMPL_HPP
