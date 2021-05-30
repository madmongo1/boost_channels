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

#include <forward_list>

#include <boost/channels/detail/channel_consume_op.hpp>
#include <boost/channels/detail/channel_send_op.hpp>

#include <boost/assert.hpp>

#include <cstddef>

namespace boost::channels::detail {

template < class ValueType, class Executor >
struct alignas(std::max_align_t) channel_impl
{
    using value_type = ValueType;

    using executor_type = Executor;

    channel_impl(Executor exec, std::size_t capacity)
    : exec_(std::move(exec))
    , capacity_(capacity)
    , size_(0)
    , first_(0)
    , last_(0)
    {
    }

    channel_impl(channel_impl const &) = delete;

    channel_impl &
    operator=(channel_impl const &) = delete;

    channel_impl(channel_impl &&) = delete;

    channel_impl &
    operator=(channel_impl &&) = delete;

    ~channel_impl()
    {
        while (size_)
        {
            auto p = mem(first_);
            p->~ValueType();
            inc(first_);
            --size_;
        }
    }

    executor_type const &
    get_executor() const
    {
        return exec_;
    }

    void close()
    {
        state_ = state_closed;

        while(!senders_.empty())
        {
            BOOST_ASSERT(free() == 0);
            senders_.front()->notify_error(errors::channel_closed);
            senders_.pop();
        }

        while(!consumers_.empty())
        {
            BOOST_ASSERT(free() == capacity_);
            consumers_.front()->notify_error(errors::channel_closed);
            consumers_.pop();
        }

    }

    void
    notify_send(detail::channel_send_op_concept< ValueType > *send_op)
    {
        switch (state_)
        {
        case state_running:
            [[likely]] if (consumers_.empty())
            {
                if (free())
                    push(send_op->consume());
                else
                    senders_.push(send_op);
            }
            else
            {
                auto my_receiver = std::move(consumers_.front());
                consumers_.pop();
                my_receiver->notify_value(send_op->consume());
            }
            break;
        case state_closed:
            [[unlikely]] send_op->notify_error(errors::channel_closed);
            break;
        }
    }

    void
    notify_consume(detail::channel_consume_op_concept< ValueType > *consume_op)
    {
        if (size_)
        {
            // there are values waiting in the circular buffer so immediately
            // deliver the first available
            consume_op->notify_value(pop());

            // there will now be space in the queue so if there any senders
            // pending delivery, allow one to delliver to the circular buffer
            if (!senders_.empty())
            {
                push(senders_.front()->consume());
                senders_.pop();
            }
        }
        else if (!senders_.empty())
        {
            // nothing in the circular buffer but there is a sender pending
            // delivery.
            /// Consume the sender' value directly into the consumer
            consume_op->notify_value(senders_.front()->consume());
            senders_.pop();
        }
        else
        {
            // nothing in circular buffer and no senders pending.
            switch (state_)
            {
            case state_running:
                consumers_.push(std::move(consume_op));
                break;
            case state_closed:
                consume_op->notify_error(errors::channel_closed);
                break;
            }
        }
    }

    std::optional< value_type >
    consume_if(error_code &ec);

    std::size_t
    free() const
    {
        return capacity_ - size_;
    }

    ValueType *
    mem(std::size_t pos)
    {
        BOOST_ASSERT(pos < capacity_);
        return storage() + pos;
    }

    void
    inc(std::size_t &pos) noexcept
    {
        pos = (pos + 1) % capacity_;
    }

    ValueType
    pop()
    {
        BOOST_ASSERT(size_);
        auto p      = mem(first_);
        auto result = std::move(*p);
        p->~ValueType();
        inc(first_);
        --size_;
        return result;
    }

    void
    push(ValueType v)
    {
        BOOST_ASSERT(free());
        auto p = mem(last_);
        new (p) ValueType(std::move(v));
        ++size_;
        inc(last_);
    }

    ValueType *
    storage()
    {
        return reinterpret_cast< ValueType * >(this + 1);
    }

    /// Internal work is executed on this executor. It is also the default
    /// executor for any completion handlers.
    Executor exec_;

    /// Monitor capacity and state of the value buffer
    std::size_t capacity_, size_, first_, last_;

    /// A list of receivers waiting to receive a value
    std::queue<
        detail::channel_consume_op_concept< ValueType > *,
        std::deque< detail::channel_consume_op_concept< ValueType > * > >
        consumers_;

    /// A list of senders waiting to send a value
    std::queue< channel_send_op_concept< ValueType > *,
                std::deque< channel_send_op_concept< ValueType > * > >
        senders_;

    enum state_code
    {
        state_running,
        state_closed,
    } state_ = state_running;
};

//
//
//

template < class ValueType, class Executor >
auto
channel_impl< ValueType, Executor >::consume_if(error_code &ec)
    -> std::optional< value_type >
{
    std::optional< value_type > result;

    if (size_)
    {
        result = pop();
    }
    else
    {
        if (senders_.empty())
        {
            if (state_ == state_closed) [[unlikely]]
                ec = errors::channel_errors::channel_closed;
        }
        else
        {
            auto sender = std::move(senders_.front());
            senders_.pop();
            result = sender->consume();
        }
    }

    return result;
}

}   // namespace boost::channels::detail
#endif   // BOOST_CHANNELS_DETAIL_CHANNEL_IMPL_HPP
