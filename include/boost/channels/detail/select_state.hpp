//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_SELECT_STATE_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_SELECT_STATE_HPP

#include <boost/channels/concepts/select_handler.hpp>
#include <boost/channels/detail/select_state_base.hpp>

namespace boost::channels::detail {
template < concepts::Lockable Mutex, concepts::select_handler Handler >
struct select_state final : select_state_base< Mutex >
{
    using value_type =
        typename detail::select_state_base< Mutex >::value_type;

    template < concepts::select_handler HandlerArg >
    select_state(HandlerArg &&arg)
    : handler_(std::forward< HandlerArg >(arg))
    {
    }

    void
    complete(value_type value) override
    {
        auto handler = std::move(handler_);
        this->set_completed();
        std::apply(handler, std::move(value));
    }

  private:
    Handler handler_;
};

template < concepts::Lockable Mutex, concepts::select_handler Handler >
auto
make_select_state(Handler &&handler)
    -> std::shared_ptr< select_state< Mutex, std ::decay_t< Handler > > >
{
    return std::make_shared< select_state< Mutex, std ::decay_t< Handler > > >(
        std::forward< Handler >(handler));
}

}   // namespace boost::channels::detail
#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_SELECT_STATE_HPP
