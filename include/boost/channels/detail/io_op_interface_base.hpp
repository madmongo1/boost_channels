//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_IO_OP_INTERFACE_BASE_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_IO_OP_INTERFACE_BASE_HPP

#include "lock.hpp"

#include <boost/channels/detail/lock.hpp>

namespace boost::channels::detail {

template < concepts::Lockable Mutex = std::mutex >
struct basic_io_op_interface_base;

template < concepts::Lockable Mutex >
struct basic_io_op_interface_base
{
    /// @brief Test whether the op has already been completed
    /// @return true if the op has already been completed, otherwise false
    virtual bool
    completed() const = 0;

    using mutex_type = Mutex;

    virtual mutex_type &
    get_mutex() = 0;

};

template < concepts::Lockable Mutex >
basic_dual_lock< Mutex >
lock(basic_io_op_interface_base< Mutex > &consumer,
     basic_io_op_interface_base< Mutex > &producer)
{
    return basic_dual_lock< Mutex >(consumer.get_mutex(), producer.get_mutex());
}

template < concepts::Lockable Mutex >
std::unique_lock< Mutex >
lock(basic_io_op_interface_base< Mutex > &c_or_p)
{
    return std::unique_lock< Mutex >(c_or_p.get_mutex());
}

using io_op_interface_base = basic_io_op_interface_base<>;

}   // namespace boost::channels::detail
#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_IO_OP_INTERFACE_BASE_HPP
