//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_DETAIL_SELECT_WAIT_OP_HPP
#define BOOST_CHANNELS_DETAIL_SELECT_WAIT_OP_HPP

#include <cstddef>
#include <memory>

namespace boost::channels::detail {

struct select_wait_op : std::enable_shared_from_this< select_wait_op >
{
    virtual void
    notify(std::size_t which) = 0;

    virtual ~select_wait_op() = default;
};

}   // namespace boost::channels::detail
#endif   // BOOST_CHANNELS_DETAIL_SELECT_WAIT_OP_HPP
