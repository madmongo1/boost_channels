//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_CONSUMER_OP_FUNCTION_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_CONSUMER_OP_FUNCTION_HPP

#include <boost/channels/detail/consume_op_interface.hpp>

namespace boost::channels::detail {
/// @brief A consumer op specialisation which calls a function when done
/// @tparam ValueType
/// @tparam Mutex
/// @tparam CompletionFunction A function object with signature void(error_code,
/// ValueType)
///
template < class ValueType, concepts::Lockable Mutex, class CompletionFunction >
struct consumer_op_function final
: detail::basic_consume_op_interface< ValueType, Mutex >
{
    using value_type =
        typename detail::basic_consume_op_interface< ValueType,
                                                     Mutex >::value_type;

    template <
        class CompletionArg,
        std::enable_if_t< !std::is_base_of_v< consumer_op_function,
                                              std::decay_t< CompletionArg > > >
            * = nullptr >
    consumer_op_function(CompletionArg&& completion)
    : completion_(std::forward<CompletionArg>(completion))
    {
    }

    virtual bool
    completed() const override
    {
        return completed_;
    };

    virtual typename detail::basic_produce_op_interface< ValueType,
                                                         Mutex >::mutex_type &
    get_mutex() override
    {
        return mutex_;
    }

    virtual void
    commit(value_type &&val) override
    {
        BOOST_CHANNELS_ASSERT(!completed_);

        auto completion = std::move(completion_);

        // destroy here
        completed_ = true;

        std::apply(completion, std::move(val));
    }

    CompletionFunction completion_;
    Mutex              mutex_;
    bool               completed_ = false;
};

template < class ValueType, concepts::Lockable Mutex, class CompletionFunction >
auto
make_consumer_op_function(CompletionFunction &&completion)
{
    using type = consumer_op_function< ValueType,
                                       Mutex,
                                       std::decay_t< CompletionFunction > >;
    return std::make_shared< type >(
        std::forward< CompletionFunction >(completion));
}

}   // namespace boost::channels::detail
#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_CONSUMER_OP_FUNCTION_HPP
