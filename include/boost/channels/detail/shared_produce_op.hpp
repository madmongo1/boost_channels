//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_SHARED_PRODUCE_OP_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_SHARED_PRODUCE_OP_HPP

#include <boost/channels/concepts/std_lockable.hpp>
#include <boost/channels/detail/produce_op_interface.hpp>
#include <boost/channels/detail/select_state_base.hpp>

namespace boost::channels::detail {
/// @brief Models the state of an asynchronous produce operation occuring in a
/// first-past-the-post operation on a set of channels, such as during @see
/// select.
/// @tparam ValueType is the type of value being produced to the associated
/// channel
template < class ValueType, concepts::Lockable Mutex >
struct shared_produce_op
: detail::basic_produce_op_interface< ValueType, Mutex >
{
    using mutex_type =
        typename detail::basic_produce_op_interface< ValueType,
                                                     Mutex >::mutex_type;

    shared_produce_op(
        std::shared_ptr< detail::select_state_base< Mutex > > sbase,
        std::reference_wrapper< ValueType >                          source,
        int                                                          which)
    : sbase_(std::move(sbase))
    , source_(source)
    , which_(which)
    {
    }

    bool
    completed() const override
    {
        return sbase_->completed();
    }
    mutex_type &
    get_mutex() override
    {
        return sbase_->get_mutex();
    }
    ValueType
    consume() override
    {
        BOOST_CHANNELS_ASSERT(!sbase_->completed());
        auto v = std::move(source_.get());
        sbase_->complete(std::make_tuple(error_code(), which_));
        BOOST_CHANNELS_ASSERT(sbase_->completed());
        return v;
    }

    void
    fail(error_code ec) override
    {
        BOOST_CHANNELS_ASSERT(!sbase_->completed());
        sbase_->complete(std::make_tuple(ec, which_));
        BOOST_CHANNELS_ASSERT(sbase_->completed());
    }

    std::shared_ptr< detail::select_state_base< Mutex > > sbase_;
    std::reference_wrapper< ValueType >                          source_;
    int                                                          which_;
};

template < class ValueType, concepts::Lockable Mutex >
auto
make_shared_produce_op(
    std::shared_ptr< detail::select_state_base< Mutex > > sbase,
    std::reference_wrapper< ValueType >                          source,
    int which) -> std::shared_ptr< shared_produce_op< ValueType, Mutex > >
{
    return std::make_shared< shared_produce_op< ValueType, Mutex > >(
        std::move(sbase), source, which);
}

}   // namespace boost::channels::detail
#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_SHARED_PRODUCE_OP_HPP
