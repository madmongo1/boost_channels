//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#include <boost/channels/detail/consume_op_interface.hpp>
#include <boost/channels/detail/produce_op_interface.hpp>
#include <boost/channels/detail/value_buffer.hpp>

#include <deque>
#include <queue>

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_IMPLEMENT_CHANNEL_QUEUE_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_IMPLEMENT_CHANNEL_QUEUE_HPP

namespace boost::channels::detail {
template < class ValueType, concepts::Lockable Mutex = std::mutex >
using basic_producer_queue =
    std::queue< basic_producer_ptr< ValueType, Mutex >,
                std::deque< basic_producer_ptr< ValueType, Mutex > > >;

template < class ValueType, concepts::Lockable Mutex = std::mutex >
using basic_consumer_queue =
    std::queue< basic_consumer_ptr< ValueType, Mutex >,
                std::deque< basic_consumer_ptr< ValueType, Mutex > > >;

template < class ValueType, concepts::Lockable Mutex >
void
flush_closed(value_buffer_ref< ValueType >             values,
             basic_consumer_queue< ValueType, Mutex > &consumers_pending,
             basic_producer_queue< ValueType, Mutex > &producers_pending)
{
    //
    // any producers that are pending delivery are simply cancelled with an
    // error
    //
    while (!producers_pending.empty())
    {
        auto lck = channels::detail::lock(*producers_pending.front());
        if (!producers_pending.front()->completed())
            producers_pending.front()->fail(channels::errors::channel_closed);
        lck.unlock();
        producers_pending.pop();
    }

    //
    // consumers are satisfied if there is a value in the queue, otherwise
    // they are notified with an error
    //
    while (!consumers_pending.empty())
    {
        auto &consumer = *consumers_pending.front();
        auto  lck      = channels::detail::lock(consumer);
        if (!consumer.completed())
        {
            if (values.empty())
            {
                consumer.commit(std::make_tuple(
                    error_code(channels::errors::channel_closed), ValueType()));
            }
            else
            {
                consumer.commit(
                    std::make_tuple(error_code(), std::move(values.front())));
                values.pop();
            }
        }
        lck.unlock();
        consumers_pending.pop();
    }
}

template < class ValueType, concepts::Lockable Mutex >
void
flush_not_closed(value_buffer_ref< ValueType >             values,
                 basic_consumer_queue< ValueType, Mutex > &consumers_pending,
                 basic_producer_queue< ValueType, Mutex > &producers_pending)
{
    for (;;)
    {
        // try to consume into ring buffer
        if (values.size() < values.capacity() && producers_pending.size())
        {
            auto &producer = *producers_pending.front();
            auto  lck      = lock(producer);
            if (!producer.completed())
                values.push(producer.consume());
            lck.unlock();
            producers_pending.pop();
            continue;
        }

        // try to transfer from ring buffer to consumers
        if (values.size() && consumers_pending.size())
        {
            auto &consumer = *consumers_pending.front();
            auto  lck      = lock(consumer);
            if (!consumer.completed())
            {
                consumer.commit(std::make_tuple(channels::error_code(),
                                                std::move(values.front())));
                values.pop();
            }
            lck.unlock();
            consumers_pending.pop();
            continue;
        }

        // if the ring buffer is empty and there is a matched consumer and
        // producer, perform a direct transfer
        while (values.empty() && consumers_pending.size() &&
               producers_pending.size())
        {
            auto &consumer = *consumers_pending.front();
            auto &producer = *producers_pending.front();
            auto  locks    = channels::detail::lock(consumer, producer);
            auto  pc       = producer.completed();
            auto  cc       = consumer.completed();
            if (!pc && !cc)
            {
                consumer.commit(std::make_tuple(channels::error_code(),
                                                producer.consume()));
                pc = cc = true;
            }
            locks.unlock();
            if (cc)
                consumers_pending.pop();
            if (pc)
                producers_pending.pop();
            BOOST_CHANNELS_ASSERT(pc || cc);
        }

        break;
    }
}

template < class ValueType, concepts::Lockable Mutex >
void
process_producer(basic_producer_ptr< ValueType, Mutex >    producer_op,
                 value_buffer_ref< ValueType >             values,
                 basic_consumer_queue< ValueType, Mutex > &consumers_pending,
                 basic_producer_queue< ValueType, Mutex > &producers_pending,
                 bool                                      closed)
{
    producers_pending.push(std::move(producer_op));
    if (closed) [[unlikely]]
        flush_closed(values, consumers_pending, producers_pending);
    else
        flush_not_closed(values, consumers_pending, producers_pending);
}

template < class ValueType, concepts::Lockable Mutex >
void
process_consumer(basic_consumer_ptr< ValueType, Mutex >    consumer_op,
                 value_buffer_ref< ValueType >             values,
                 basic_consumer_queue< ValueType, Mutex > &consumers_pending,
                 basic_producer_queue< ValueType, Mutex > &producers_pending,
                 bool                                      closed)
{
    consumers_pending.push(std::move(consumer_op));
    if (closed) [[unlikely]]
        flush_closed(values, consumers_pending, producers_pending);
    else
        flush_not_closed(values, consumers_pending, producers_pending);
}

template < class ValueType >
using producer_queue = basic_producer_queue< ValueType >;
template < class ValueType >
using consumer_queue = basic_consumer_queue< ValueType >;

}   // namespace boost::channels::detail
#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_IMPLEMENT_CHANNEL_QUEUE_HPP
