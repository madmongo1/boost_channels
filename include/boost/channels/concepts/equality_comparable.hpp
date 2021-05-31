//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CONCEPTS_EQUALITY_COMPARABLE_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CONCEPTS_EQUALITY_COMPARABLE_HPP

#include <type_traits>
#include <boost/channels/concepts/boolean_testable.hpp>

namespace boost::channels::concepts
{
// clang-format off
template <class T, class U>
concept equality_comparable_with = requires(T const& l, U const& r)
{
    { l == r } -> boolean_testable;
};

template < class T >
concept equality_comparable = equality_comparable_with<T, T>;

// clang-format on
}
#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CONCEPTS_EQUALITY_COMPARABLE_HPP
