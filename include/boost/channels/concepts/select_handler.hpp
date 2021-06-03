//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CONCEPTS_SELECT_HANDLER_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CONCEPTS_SELECT_HANDLER_HPP

#include <boost/channels/error_code.hpp>

namespace boost::channels::concepts {

/// @brief Describe the concept of a completion handler for a select()
/// operation.
///
/// @see boost::channels::select
/// @tparam F A type which is callable with the signature (error_code, int)
// clang-format off
template < class F >
concept select_handler =
    requires(F &f, error_code const &ec, int i)
    {
        f(ec, i);
    };
// clang-format on

}   // namespace boost::channels::concepts
#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CONCEPTS_SELECT_HANDLER_HPP
