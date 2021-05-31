//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_PRODUCE_OP_INTERFACE_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_PRODUCE_OP_INTERFACE_HPP

#include <boost/channels/detail/io_op_interface_base.hpp>
#include <boost/channels/error_code.hpp>

namespace boost::channels::detail {
template < class ValueType, concepts::Lockable Mutex = std::mutex >
struct basic_produce_op_interface : basic_io_op_interface_base< Mutex >
{
    virtual ~basic_produce_op_interface() = default;

    /// @brief Consume the value from the produce_op_interface.
    /// @pre completed() == false
    /// @post completed() == true
    /// @return the object held within the produce_op_interface, having been
    /// moved out of its temporary storeag
    virtual ValueType
    consume() = 0;

    /// @brief Complete the operation with an error code.
    ///
    /// Does not take the value from the producer.
    /// @pre completed() == false
    /// @post completed() == true
    virtual void
    fail(error_code ec) = 0;
};

template < class ValueType, concepts::Lockable Mutex >
using basic_producer_ptr =
    std::shared_ptr< basic_produce_op_interface< ValueType, Mutex > >;

// common specialisations

template < class ValueType >
using produce_op_interface = basic_produce_op_interface< ValueType >;

template < class ValueType >
using producer_ptr = std::shared_ptr< produce_op_interface< ValueType > >;
}   // namespace boost::channels::detail

#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_PRODUCE_OP_INTERFACE_HPP
