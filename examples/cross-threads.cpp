//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#include <boost/channels/channel.hpp>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/as_single.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/system_executor.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/use_future.hpp>

#include <iostream>

using namespace boost;
using namespace asio;
using namespace std::literals;

auto mintime = std::chrono::steady_clock::time_point::max();
auto maxtime = std::chrono::steady_clock::time_point::min();

channels::channel< std::string > *logger_ = nullptr;
template < class... Ts >
auto
println(Ts const &...ts)
{
    using std::tie;
    using std::tuple;
    using std::tuple_cat;

    auto now = std::chrono::steady_clock::now();
    mintime  = std::min(mintime, now);
    maxtime  = std::max(mintime, now);
    dispatch(logger_->get_executor(), [now, ts = std::tuple(ts...)] {
        std::ostringstream ss;
        const char *       sep = " : ";
        ss << now.time_since_epoch().count();
        mp11::tuple_for_each(ts, [&](auto const &x) {
            ss << sep << x;
            sep = " ";
        });
        logger_->async_send(ss.str(), detached);
    });
}

void
thread_producer(std::string name, channels::channel< std::string > &chan)
{
    int count = 0;
    for (;;)
    {
        try
        {
            chan.async_send("producer "s + name + " : message "s +
                                std::to_string(++count),
                            asio::use_future)
                .get();
        }
        catch (std::exception &e)
        {
            println("producer", name, "stopping with error", e.what());
            break;
        }
    }
}

asio::awaitable< void >
coro_producer(std::string name, channels::channel< std::string > &chan)
{
    int count = 0;
    for (;;)
    {
        try
        {
            co_await chan.async_send("producer "s + name + " : message "s +
                                         std::to_string(++count),
                                     asio::use_awaitable);
        }
        catch (std::exception &e)
        {
            println("producer", name, "stopping with error", e.what());
            break;
        }
    }
}

asio::awaitable< void >
coro_consumer(std::string name, channels::channel< std::string > &chan)
{
    for (;;)
    {
        auto [ec, msg] =
            co_await chan.async_consume(experimental::as_single(use_awaitable));
        println("consumer", name, ec ? ec.message() : msg);
        if (ec)
            break;
    }
}

awaitable< void >
log_task(channels::channel< std::string > &logs)
{
    for (;;)
    {
        auto [ec, str] =
            co_await logs.async_consume(experimental::as_single(use_awaitable));
        if (ec)
        {
            std::cout << "logger done: " << ec.message() << '\n';
            break;
        }
        else
        {
            std::cout << str << '\n';
        }
    }
}

int
main()
{
    std::mutex              om;
    std::condition_variable cv;
    std::size_t             outstanding = 0;

    auto notify = [&](auto &&...) {
        auto l = std::lock_guard { om };
        if (--outstanding == 0)
            cv.notify_all();
    };

    auto spawn = [&](auto a) {
        auto l = std::lock_guard(om);
        ++outstanding;
        co_spawn(make_strand(system_executor()), std::move(a), notify);
    };

    auto logs = channels::channel< std::string >(
        make_strand(system_executor(), 100000));
    logger_ = &logs;
    std::promise< void > logger_promise;
    co_spawn(logs.get_executor(), log_task(logs), [&](auto &&...) {
        logger_promise.set_value();
    });

    auto messages =
        channels::channel< std::string >(make_strand(system_executor()), 1000);

    std::thread t1 { [&] { thread_producer("thread 1", messages); } };
    std::thread t2 { [&] { thread_producer("thread 2", messages); } };
    std::thread t3 { [&] { thread_producer("thread 3", messages); } };
    std::thread timeout { [&] {
        std::this_thread::sleep_for(1s);
        messages.close();
    } };

    spawn(coro_producer("coro 1", messages));
    spawn(coro_producer("coro 2", messages));
    spawn(coro_producer("coro 3", messages));
    spawn([&]() -> awaitable< void > {
        co_await messages.async_send("ping", use_awaitable);
    });

    spawn(coro_consumer("coro a", messages));
    spawn(coro_consumer("coro b", messages));
    spawn(coro_consumer("coro c", messages));

    t1.join();
    t2.join();
    t3.join();
    timeout.join();
    println("about to wait");
    auto l = std::unique_lock(om);
    cv.wait(l, [&] {
        println("outstanding=", outstanding);
        return outstanding == 0;
    });

    logs.close();
    logger_promise.get_future().wait();

    std::cout << "elapsed time: " << (maxtime-mintime).count() << std::endl;
}