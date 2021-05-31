//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_POSTIT_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_POSTIT_HPP

#include <boost/asio/post.hpp>

#include <tuple>
#include <utility>

namespace boost::channels::detail {

template < class Handler, class... Args >
struct handler_bound_to_args
{
    using executor_type  = asio::associated_executor_t< Handler >;
    using allocator_type = asio::associated_allocator_t< Handler >;

    template < class Handler_, class... Args_ >
    handler_bound_to_args(Handler_ &&h, Args_ &&...args)
    : handler_(std::forward< Handler_ >(h))
    , args_(std::forward< Args_ >(args)...)
    {
    }

    void
    operator()()
    {
        std::apply(handler_, std::move(args_));
    }

    executor_type
    get_executor() const
    {
        return asio::get_associated_executor(handler_);
    }

    allocator_type
    get_allocator() const
    {
        return asio::get_associated_allocator(handler_);
    }

  private:
    Handler               handler_;
    std::tuple< Args... > args_;
};

template < class Handler, class... Args >
handler_bound_to_args(Handler &&, Args &&...)
    -> handler_bound_to_args< std::decay_t< Handler >,
                              std::decay_t< Args >... >;

//

template < class Executor, class Handler >
struct postit
{
    template < class ExecutorArg, class HandlerArg >
    postit(ExecutorArg &&e, HandlerArg &&h)
    : exec_(std::forward< ExecutorArg >(e))
    , handler_(std::forward< HandlerArg >(h))
    {
    }

    template < class... Args >
    void
    operator()(Args &&...args)
    {
        asio::post(exec_,
                   handler_bound_to_args(std::move(handler_),
                                         std::forward< Args >(args)...));
    }

    Executor exec_;
    Handler  handler_;
};

template < class Executor, class Handler >
postit(Executor &&, Handler &&)
    -> postit< std::decay_t< Executor >,
                              std::decay_t< Handler > >;

}   // namespace boost::channels::detail

#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_POSTIT_HPP
