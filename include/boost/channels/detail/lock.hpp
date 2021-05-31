//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_LOCK_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_LOCK_HPP

#include <boost/channels/concepts/std_lockable.hpp>

#include <mutex>

namespace boost::channels::detail {

template < concepts::Lockable Mutex = std::mutex >
struct basic_dual_lock
{
    basic_dual_lock(Mutex &m1, Mutex &m2)
    : basic_dual_lock(lockem(m1, m2))
    {
    }

    void
    unlock()
    {
        get< 0 >(locks_).unlock();
        get< 1 >(locks_).unlock();
    }

  private:
    using lock_type = std::unique_lock< Mutex >;

    basic_dual_lock(std::tuple< lock_type, lock_type > &&locks)
    : locks_(std::move(locks))
    {
    }

    static std::tuple< lock_type, lock_type >
    lockem(Mutex &m1, Mutex &m2)
    {
        std::lock(m1, m2);
        return std::make_tuple(lock_type(m1, std::adopt_lock),
                               lock_type(m2, std::adopt_lock));
    }

    std::tuple< lock_type, lock_type > locks_;
};

using dual_lock = basic_dual_lock<>;

}   // namespace boost::channels::detail

#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_LOCK_HPP
