//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_CONSUME_OP_INTERFACE_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_CONSUME_OP_INTERFACE_HPP

#include <boost/channels/detail/io_op_interface_base.hpp>
#include <boost/channels/error_code.hpp>

namespace boost::channels::detail {
template < class ValueType, concepts::Lockable Mutex = std::mutex >
struct basic_consume_op_interface : basic_io_op_interface_base< Mutex >
{
    using value_type = std::tuple< error_code, ValueType >;

    virtual ~basic_consume_op_interface() = default;

    /// @brief Commit a value to a prepared consumer.
    /// @pre completed() == false
    /// @post completed() == true
    /// @param source An r-value reference to the object that will be committed.
    virtual void
    commit(value_type &&source) = 0;
};

template < class ValueType , concepts::Lockable Mutex>
using basic_consumer_ptr = std::shared_ptr< basic_consume_op_interface< ValueType, Mutex > >;

template<class ValueType>
using consume_op_interface = basic_consume_op_interface<ValueType>;

template < class ValueType >
using consumer_ptr = std::shared_ptr< consume_op_interface< ValueType > >;
}   // namespace boost::channels::detail
#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_CONSUME_OP_INTERFACE_HPP
