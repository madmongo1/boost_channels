//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CONCEPTS_CONVERTIBLE_TO_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CONCEPTS_CONVERTIBLE_TO_HPP

#include <type_traits>

namespace boost::channels::concepts
{
// clang-format off
template <class From, class To>
concept convertible_to =
    std::is_convertible_v<From, To> &&
    requires(std::add_rvalue_reference_t<From> (&f)())
    {
        static_cast<To>(f());
    };
// clang-format on
}
#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CONCEPTS_CONVERTIBLE_TO_HPP
