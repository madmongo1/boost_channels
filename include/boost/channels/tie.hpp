//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_TIE_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_TIE_HPP

#include <boost/channels/concepts/selectable_op.hpp>
#include <boost/channels/detail/postit.hpp>
#include <boost/channels/detail/select_state.hpp>

#include <boost/asio/associated_executor.hpp>
#include <boost/mp11/tuple.hpp>

#include <algorithm>
#include <random>

namespace boost::channels {
template < concepts::selectable_op... >
struct tied_channel_op;

template < concepts::selectable_op PoC1, concepts::selectable_op... PoCRest >
struct tied_channel_op< PoC1, PoCRest... >
{
    using executor_type = typename PoC1::executor_type;
    using mutex_type    = typename PoC1::mutex_type;

    template < class... PoCArgs >
    tied_channel_op(PoCArgs &&...args)
    : ops_(std::forward< PoCArgs >(args)...)
    {
    }

    template < BOOST_ASIO_COMPLETION_TOKEN_FOR(void(error_code, int))
                   SelectHandler BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(
                       executor_type) >
    BOOST_ASIO_INITFN_RESULT_TYPE(SelectHandler, void(error_code, int))
    async_wait(SelectHandler &&token
                   BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(executor_type))
    {
        return asio::async_initiate< SelectHandler, void(error_code, int) >(
            [ops = ops_]< class Handler >(Handler &&handler) mutable {
                auto [ec, which] = check_for_null(ops);
                if (ec)
                {
                    // early completion if one of the ops has no state
                    auto exec = asio::get_associated_executor(
                        handler, get< 0 >(ops).get_executor());
                    auto fin =
                        detail::postit(std::move(exec), std::move(handler));
                    fin(ec, which);
                }
                else
                {
                    auto exec =
                        asio::prefer(asio::get_associated_executor(
                                         handler, get< 0 >(ops).get_executor()),
                                     asio::execution::outstanding_work.tracked);
                    auto ss =
                        detail::make_select_state< mutex_type >(detail::postit(
                            std::move(exec), std::forward< Handler >(handler)));

                    static thread_local auto rng = [] {
                        std::random_device rd;
                        std::mt19937       g(rd());
                        return g;
                    }();

                    std::function< void() > inits[sizeof...(PoCRest) + 1];
                    int which = 0;
                    mp11::tuple_for_each(ops, [&](auto const &op) {
                        inits[which] = [ss, which, op] {
                            op.submit_shared_op(ss, which);
                        };
                        ++which;
                    });
                    std::shuffle(std::begin(inits), std::end(inits), rng);
                    for(auto&& init : inits)
                        init();
                }
            },
            token);
    }

    static std::tuple< error_code, int >
    check_for_null(std::tuple< PoC1, PoCRest... > const &ops)
    {
        int        which = -1, i = -1;
        error_code ec;
        auto       check =
            [&i, &which, &ec]< concepts::selectable_op Op >(Op const &op) {
                ++i;
                if (which != -1 && !op.get_implementation())
                {
                    ec    = errors::channel_null;
                    which = i;
                }
            };
        mp11::tuple_for_each(ops, check);
        return std::tuple(ec, which);
    }

    std::tuple< PoC1, PoCRest... > ops_;
};

template < concepts::selectable_op... ProduceOrConsume >
tied_channel_op< std::decay_t< ProduceOrConsume >... >
tie(ProduceOrConsume &&...pods)
{
    return tied_channel_op< std::decay_t< ProduceOrConsume >... >(
        std::forward< ProduceOrConsume >(pods)...);
}

}   // namespace boost::channels
#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_TIE_HPP
