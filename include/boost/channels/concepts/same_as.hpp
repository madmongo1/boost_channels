//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CONCEPTS_COMMON_REFERENCE_WITH_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CONCEPTS_COMMON_REFERENCE_WITH_HPP

#include <boost/channels/concepts/convertible_to.hpp>
#include <boost/channels/concepts/same_as.hpp>

#include <type_traits>

namespace boost::channels::concepts {
namespace detail {
// clang-format off
template < class T, class U >
concept SameHelper =
    std::is_same_v<T, U>;
// clang-format on
}   // namespace detail

// clang-format off
template < class T, class U >
concept same_as =
    detail::SameHelper<T, U> &&
    detail::SameHelper<U, T>;
// clang-format on

}   // namespace boost::channels::concepts

#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CONCEPTS_COMMON_REFERENCE_WITH_HPP
