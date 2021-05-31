//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_NULL_MUTEX_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_NULL_MUTEX_HPP

#include <boost/channels/concepts/std_lockable.hpp>

#ifdef BOOST_CHANNELS_DEBUG_BUILD
#include <boost/assert>

#include <thread>
#endif

namespace boost::channels {
/// @brief A class that models the
struct null_mutex
{
    void
    lock()
    {
#ifdef BOOST_CHANNELS_DEBUG_BUILD
        BOOST_ASSERT(locked_ = std::thread::id());
        locked_ = std::this_thread::get_id();
#endif
    }

    void
    unlock()
    {
#ifdef BOOST_CHANNELS_DEBUG_BUILD
        BOOST_ASSERT(locked_ == std::thread::id());
        locked_ = std::thread::id();
#endif
    }

    bool
    try_lock()
    {
#ifdef BOOST_CHANNELS_DEBUG_BUILD
        if (locked_ = std::thread::id())
        {
            locked_ = std::this_thread::get_id();
            return true;
        }
        else
        {
            return false;
        }
#else
        return true;
#endif
    }

#ifdef BOOST_CHANNELS_DEBUG_BUILD
    std::thread::id locked_;
#endif
};
}   // namespace boost::channels
#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_NULL_MUTEX_HPP
