//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CONCEPTS_BOOLEAN_TESTABLE_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CONCEPTS_BOOLEAN_TESTABLE_HPP

#include <boost/channels/concepts/convertible_to.hpp>

#include <type_traits>

namespace boost::channels::concepts {
namespace detail {
// clang-format off
template<class B>
concept boolean_testable_impl =
    convertible_to<B, bool>;
// clang-format on
}   // namespace detail

// clang-format off
template<class B>
concept boolean_testable =
    detail::boolean_testable_impl<B> &&
    requires (B&& b) {
        { !std::forward<B>(b) } -> detail::boolean_testable_impl;
    };
// clang-format on
}   // namespace boost::channels::concepts

#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CONCEPTS_BOOLEAN_TESTABLE_HPP
