//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#include <boost/channels/concepts/executor.hpp>
#include <boost/channels/concepts/same_as.hpp>
#include <boost/channels/detail/select_state_base.hpp>

#include <memory>

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CONCEPTS_SELECTABLE_OP_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CONCEPTS_SELECTABLE_OP_HPP

namespace boost::channels::concepts {

// clang-format off
template<class T>
concept selectable_op =
    requires(
        T& op,
        T const& cop,
        std::shared_ptr<
            ::boost::channels::detail::select_state_base<typename
                T::mutex_type> > const&
                    sbase,
        int which)
    {
        concepts::executor_model<typename T::executor_type>;
        concepts::Lockable<typename T::mutex_type>;

        { op.get_executor() } -> concepts::same_as<typename T::executor_type>;
        { cop.submit_shared_op(sbase, which) };
        { cop.get_implementation() }; // ->
            // concepts::convertible_to<std::shared_ptr<
                // detail::channel_impl<ValueType, Mutex>>>;
    };
// clang-format on

}   // namespace boost::channels::concepts
#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_CONCEPTS_SELECTABLE_OP_HPP
