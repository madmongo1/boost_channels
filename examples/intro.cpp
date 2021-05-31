#include <boost/channels/channel.hpp>

#include <boost/asio.hpp>
#include <boost/asio/experimental/as_single.hpp>
#include <boost/mp11/tuple.hpp>
#include <boost/variant2/variant.hpp>

#include <concepts>
#include <deque>
#include <iostream>
#include <queue>

namespace boost { namespace channels {

namespace detail {
// clang-format off
template < class From, class To >
concept convertible_to =
std::is_convertible_v< From, To > &&
requires(std::add_rvalue_reference_t< From > (&f)())
{
    static_cast< To >(f());
};

template< class T, class U >
concept SameHelper =
std::is_same_v<T, U>;

template < class T, class U >
concept same_as =
SameHelper<T, U> &&
SameHelper<U, T>;
// clang-format on
}   // namespace detail

// clang-format off
template < class T >
concept selectable =
requires(T const &c, T& m, std::shared_ptr<detail::select_wait_op> const& pwo, std::size_t i)
{
    { c.count() } -> detail::convertible_to< std::size_t >;
    { m.consume() } -> detail::same_as< typename T::value_type >;
    { m.notify_wait(pwo, i) };
    { m.cancel_wait() };
};
// clang-format on

template < class Executor, class Handler >
struct implement_select_wait_op final : detail::select_wait_op
{
    implement_select_wait_op(Executor exec, Handler handler)
    : exec_(std::move(exec))
    , handler_(std::move(handler))
    {
    }

    using executor_type = Executor;

    executor_type const &
    get_executor() const
    {
        return exec_;
    }

    void
    notify(std::size_t which) override
    {
        asio::post(get_executor(),
                   [self = std::shared_ptr< implement_select_wait_op >(
                        shared_from_this(), this),
                    which]() mutable {
                       if (std::exchange(self->armed_, false))
                       {
                           auto handler = std::move(self->handler_);
                           self.reset();
                           handler(system::error_code(), which);
                       }
                   });
    }

  private:
    Executor exec_;
    Handler  handler_;
    bool     armed_ = true;
};

template < selectable... Ts >
struct result_of_select
{
    using type = variant2::variant< typename Ts::value_type... >;
};

template < selectable... Ts >
using result_of_select_t = typename result_of_select< Ts... >::type;

namespace detail {
template < selectable... Ts, std::size_t... Is >
void
setup_notify(std::tuple< Ts &... > ts,
             std::index_sequence< Is... >,
             std::shared_ptr< detail::select_wait_op > const &op)
{
    (get< Is >(ts).notify_wait(op, Is), ...);
}

template < selectable... Ts, std::size_t... Is >
void
assign_result(std::optional< result_of_select_t< Ts... > > &result,
              std::tuple< Ts &... >                         ts,
              std::size_t                                   which,
              std::index_sequence< Is... >)
{
    auto op = [&]< std::size_t I >(std::integral_constant< std::size_t, I >) {
        if (which == I)
        {
            BOOST_ASSERT(!result);
            BOOST_ASSERT(get< I >(ts).count());
            result = result_of_select_t< Ts... >(variant2::in_place_index< I >,
                                                 get< I >(ts).consume());
        }
    };
    (op(std::integral_constant< std::size_t, Is >()), ...);
}

}   // namespace detail

template <
    selectable... Ts,
    class Executor,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void(system::error_code, std::size_t))
        CompletionToken BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(Executor) >
auto
async_wait_select(std::tuple< Ts &... > ts,
                  Executor const &      exec,
                  CompletionToken &&token
                      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(Executor))
    -> BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken,
                                          void(system::error_code, std::size_t))
{
    return asio::async_initiate< CompletionToken,
                                 void(system::error_code, std::size_t) >(
        [exec, ts](auto &&handler) {
            auto my_exec =
                asio::prefer(asio::get_associated_executor(handler, exec),
                             asio::execution::outstanding_work.tracked);
            using handler_type = std::decay_t< decltype(handler) >;
            using op_type =
                implement_select_wait_op< decltype(my_exec), handler_type >;
            auto op = std::make_shared< op_type >(
                exec, std::forward< handler_type >(handler));
            detail::setup_notify(
                ts, std::make_index_sequence< sizeof...(Ts) >(), op);
        },
        token);
}

template < selectable... Ts >
asio::awaitable< result_of_select_t< Ts... > >
select(Ts &...ts)
{
    std::optional< result_of_select_t< Ts... > > result;
    auto pre_scan = [&result]< selectable T >(T &item) {
        if (!result && item.count())
            result = item.consume();
    };
    mp11::tuple_for_each(std::tie(ts...), pre_scan);

    if (!result)
    {
        auto which =
            co_await async_wait_select(std::tie(ts...),
                                       co_await asio::this_coro::executor,
                                       asio::use_awaitable);
        detail::assign_result(
            result, std::tie(ts...), which, std::index_sequence_for< Ts... >());
        BOOST_ASSERT(result);
        BOOST_ASSERT(result->index() == which);
        mp11::tuple_for_each(std::tie(ts...),
                             []< selectable T >(T &sel) { sel.cancel_wait(); });
    }

    co_return *std::move(result);
}

}}   // namespace boost::channels

using namespace boost;
using namespace std::literals;

using asio::use_awaitable;
using asio::experimental::as_single;

template < class... Ts >
auto
println(Ts const &...ts)
{
    const char *sep = "";
    auto        op  = [&](auto const &x) {
        std::cout << sep << x;
        sep = " ";
    };
    mp11::tuple_for_each(std::tie(ts...), op);
    std::cout << '\n';
}

asio::awaitable< void >
pull(boost::channels::channel< std::string > &c1)
{
    for (;;)
    {
        auto [ec, s] = co_await c1.async_consume(as_single(use_awaitable));
        if (ec)
        {
            println("c1 error:", ec.message());
            break;
        }
        else
            println("c1 says:", s);
    }
}

template < class Duration >
asio::awaitable< void >
wait(asio::steady_timer &t, Duration d)
{
    t.expires_after(d);
    co_await t.async_wait(asio::use_awaitable);
}

boost::asio::awaitable< void >
push(boost::channels::channel< std::string > &c1)
{
    auto t = asio::steady_timer(co_await asio::this_coro::executor);
    co_await wait(t, 500ms);
    co_await c1.async_send("Hello", use_awaitable);
    co_await wait(t, 500ms);
    co_await c1.async_send("World", use_awaitable);
    co_await wait(t, 500ms);
    c1.close();
}

int
main()
{
    auto ioc = asio::io_context();
    auto e   = ioc.get_executor();

    channels::channel< std::string > c1(e);

    asio::co_spawn(e, pull(c1), asio::detached);
    asio::co_spawn(e, push(c1), asio::detached);

    ioc.run();
}
