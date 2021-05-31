//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_SCOPE_EXIT_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_SCOPE_EXIT_HPP

namespace boost::channels {

/// @brief Provide a means of executing a noexcept nullary function upon object
/// destruction.
/// @tparam F A type which is callable with signature void() noexcept
template < class F >
struct scope_exit
{
    scope_exit(F f) noexcept
    : f(static_cast<F&&>(f))
    {
    }
    scope_exit(scope_exit const &) = delete;
    scope_exit &
    operator=(scope_exit const &) = delete;
    ~scope_exit()
    {
        f();
    }

  private:
    F f;
};

}   // namespace boost::channels
#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_SCOPE_EXIT_HPP
