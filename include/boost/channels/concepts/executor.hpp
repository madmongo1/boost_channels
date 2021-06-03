//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CONCEPTS_IS_EXECUTOR_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CONCEPTS_IS_EXECUTOR_HPP

#include <boost/channels/concepts/boolean_testable.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/is_executor.hpp>

namespace boost::channels::concepts {

template < class T >
concept executor_model =
    asio::is_executor< T >::value || std::is_same_v< T, asio::any_io_executor >;

// clang-format off
template<executor_model A, executor_model B>
struct are_comparable_executors
{
static auto test(...) -> std::false_type;
template<executor_model X, executor_model Y>
static auto test(X a, Y b) -> decltype((a == b), void(), std::true_type());

using type = decltype(test(std::declval<A>(), std::declval<B>()));
};

template<executor_model A, executor_model B>
constexpr bool are_comparable_executors_v =  are_comparable_executors<A, B>::type::value;

template<class A, class B>
concept comparable_executors = are_comparable_executors_v<A, B>;
// clang-format on
}   // namespace boost::channels::concepts
#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CONCEPTS_IS_EXECUTOR_HPP
