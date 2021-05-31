//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_VALUE_BUFFER_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_VALUE_BUFFER_HPP

#include <boost/channels/config.hpp>

#include <cstddef>

#include <type_traits>

namespace boost::channels::detail {
struct value_buffer_data
{
    std::size_t capacity = 0;
    std::size_t begin    = 0;
    std::size_t end      = 0;
    std::size_t size     = 0;

    void
    reduce()
    {
        BOOST_CHANNELS_ASSERT(size);
        if (++begin == capacity)
            begin = 0;
        --size;
    }

    void
    increase()
    {
        BOOST_CHANNELS_ASSERT(size < capacity);
        if (++end == capacity)
            end = 0;
        ++size;
    }
};

template < class ValueType >
struct value_buffer_ref
{
    value_buffer_data *                          pdata;
    std::aligned_storage_t< sizeof(ValueType) > *storage;

    ValueType *
    mem() const
    {
        return reinterpret_cast< ValueType * >(storage);
    }

    std::size_t
    size() const
    {
        return pdata->size;
    }

    std::size_t
    capacity() const
    {
        return pdata->capacity;
    }

    ValueType &
    front()
    {
        BOOST_CHANNELS_ASSERT(mem());
        return *(mem() + pdata->begin);
    }

    bool
    empty() const
    {
        return pdata->size == 0;
    }

    void
    pop()
    {
        BOOST_CHANNELS_ASSERT(mem());
        BOOST_CHANNELS_ASSERT(pdata->size);
        (mem() + pdata->begin)->~ValueType();
        pdata->reduce();
    }

    void
    push(ValueType &&v)
    {
        BOOST_CHANNELS_ASSERT(mem());
        BOOST_CHANNELS_ASSERT(pdata->size < pdata->capacity);
        auto pback = mem() + pdata->end;
        new (pback) ValueType(std::move(v));
        pdata->increase();
    }

    void
    destroy()
    {
        if (auto pmem = mem())
        {
            while (pdata->size)
            {
                (pmem + pdata->begin)->~ValueType();
                pdata->reduce();
            }
        }
    }
};

}   // namespace boost::channels::detail
#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_VALUE_BUFFER_HPP
