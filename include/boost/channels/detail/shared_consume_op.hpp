//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_SHARED_CONSUME_OP_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_SHARED_CONSUME_OP_HPP

#include <boost/channels/concepts/std_lockable.hpp>
#include <boost/channels/detail/consume_op_interface.hpp>
#include <boost/channels/detail/select_state_base.hpp>

namespace boost::channels::detail {
template < class ValueType, concepts::Lockable Mutex >
struct shared_consume_op final
: detail::basic_consume_op_interface< ValueType, Mutex >
{
    using value_type =
        typename detail::basic_consume_op_interface< ValueType,
                                                     Mutex >::value_type;

    using mutex_type =
        typename detail::basic_consume_op_interface< ValueType,
                                                     Mutex >::mutex_type;

    shared_consume_op(
        std::shared_ptr< detail::select_state_base< Mutex > > sbase,
        std::reference_wrapper< ValueType >                   sink,
        int                                                   which)
    : sbase_(std::move(sbase))
    , sink_(sink)
    , which_(which)
    {
    }

    bool
    completed() const override
    {
        return sbase_->completed();
    }

    void
    commit(value_type &&value) override
    {
        BOOST_CHANNELS_ASSERT(!sbase_->completed());
        sink_.get() = std::move(get< 1 >(std::move(value)));
        sbase_->complete(std::make_tuple(get< 0 >(value), which_));
        BOOST_CHANNELS_ASSERT(sbase_->completed());
    }

    mutex_type &
    get_mutex() override
    {
        return sbase_->get_mutex();
    }

    std::shared_ptr< detail::select_state_base< Mutex > > sbase_;
    std::reference_wrapper< ValueType >                   sink_;
    int                                                   which_;
};

template < class ValueType, concepts::Lockable Mutex >
auto
make_shared_consume_op(
    std::shared_ptr< detail::select_state_base< Mutex > > sbase,
    std::reference_wrapper< ValueType >                   sink,
    int which) -> std::shared_ptr< shared_consume_op< ValueType, Mutex > >
{
    return std::make_shared< shared_consume_op< ValueType, Mutex > >(
        std::move(sbase), sink, which);
}

}   // namespace boost::channels::detail

#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_SHARED_CONSUME_OP_HPP
