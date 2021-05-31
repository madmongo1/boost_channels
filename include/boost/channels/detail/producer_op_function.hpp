//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_PRODUCER_OP_FUNCTION_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_PRODUCER_OP_FUNCTION_HPP

#include <boost/channels/config.hpp>
#include <boost/channels/detail/produce_op_interface.hpp>

namespace boost::channels::detail {

/// @brief A producer op specialisation which calls a function when done
/// @tparam ValueType
/// @tparam Mutex
/// @tparam CompletionFunction A function object with signature void(error_code)
///
template < class ValueType, concepts::Lockable Mutex, class CompletionFunction >
struct producer_op_function final
: detail::basic_produce_op_interface< ValueType, Mutex >
{
    template < class ValueArg, class CompletionArg >
    producer_op_function(ValueArg &&source, CompletionArg &&completion)
    : source_(std::forward< ValueArg >(source))
    , completion_(std::forward< CompletionArg >(completion))
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

    virtual ValueType
    consume() override
    {
        auto result     = std::move(source_);
        auto completion = std::move(completion_);

        // a normal comepletion handler would destroy itself here
        completed_ = true;

        completion(error_code());
        return result;
    }

    virtual void
    fail(error_code ec) override
    {
        BOOST_CHANNELS_ASSERT(!completed_);

        auto completion = std::move(completion_);

        // a normal completion handler would destroy itself here
        completed_ = true;

        completion(ec);
    }

    ValueType          source_;
    CompletionFunction completion_;
    Mutex              mutex_;
    bool               completed_ = false;
};

template < concepts::Lockable Mutex, class ValueType, class CompletionFunction >
auto
make_producer_op_function(ValueType &&value, CompletionFunction &&completion)
{
    using type = producer_op_function< std::decay_t< ValueType >,
                                       Mutex,
                                       std::decay_t< CompletionFunction > >;
    return std::make_shared< type >(
        std::forward< ValueType >(value),
        std::forward< CompletionFunction >(completion));
}

//
// Specialisation to deliver through a reference
template < class ValueType, concepts::Lockable Mutex, class CompletionFunction >
struct producer_op_function< std::reference_wrapper< ValueType >,
                             Mutex,
                             CompletionFunction >
    final : detail::basic_produce_op_interface< ValueType, Mutex >
{
    template < class ValueArg, class CompletionArg >
    producer_op_function(ValueArg &&source, CompletionArg &&completion)
    : source_(std::forward< ValueArg >(source))
    , completion_(std::forward< CompletionArg >(completion))
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

    virtual ValueType
    consume() override
    {
        auto result     = std::move(source_.get());
        auto completion = std::move(completion_);

        // a normal comepletion handler would destroy itself here
        completed_ = true;

        completion(error_code());
        return result;
    }

    virtual void
    fail(error_code ec) override
    {
        BOOST_CHANNELS_ASSERT(!completed_);

        auto completion = completion_;

        // a normal completion handler would destroy itself here
        completed_ = true;

        completion(ec);
    }

    std::reference_wrapper< ValueType > source_;
    CompletionFunction                  completion_;
    Mutex                               mutex_;
    bool                                completed_ = false;
};

}   // namespace boost::channels::detail

#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_PRODUCER_OP_FUNCTION_HPP
