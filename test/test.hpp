// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef CORO_TEST_HPP
#define CORO_TEST_HPP

#include <coroutine>

#include <coro/main.hpp>

namespace coro
{

struct test_case_promise;
struct test_case { test_case_promise * promise; };


struct test_case_promise : promise_cancellation_base<asio::cancellation_slot, asio::enable_total_cancellation>,
                      promise_throw_if_cancelled_base,
                      enable_awaitables<test_case_promise>,
                      enable_async_operation
{
    using promise_cancellation_base<asio::cancellation_slot, asio::enable_total_cancellation>::await_transform;
    using promise_throw_if_cancelled_base::await_transform;
    using enable_awaitables<test_case_promise>::await_transform;
    using enable_async_operation::await_transform;

    std::suspend_always initial_suspend()          { return {}; }
    std::suspend_never    final_suspend() noexcept { return {}; }

    test_case get_return_object()
    {
        return {this};
    }

    void unhandled_exception() {throw;}
    void return_void() {}

    using executor_type = asio::io_context::executor_type;
    executor_type get_executor() const {return exec->context().get_executor();}

    auto await_transform(this_coro::executor_t) const
    {
        struct exec_helper
        {
            executor_type value;

            constexpr static bool await_ready() noexcept
            {
                return true;
            }

            constexpr static void await_suspend(std::coroutine_handle<>) noexcept
            {
            }

            executor_type await_resume() const noexcept
            {
                return value;
            }
        };

        return exec_helper{get_executor()};
    }
    std::optional<typename asio::require_result<executor_type, asio::execution::outstanding_work_t ::tracked_t>::type> exec;

};

}

namespace std
{

template<>
struct coroutine_traits<coro::test_case>
{
    using promise_type = coro::test_case_promise;
};

}


#define CO_TEST_CASE_IMPL(Function, ...)                                                                               \
static ::coro::test_case Function();                                                                                   \
TEST_CASE(__VA_ARGS__)                                                                                                 \
{                                                                                                                      \
    asio::io_context ctx;                                                                                              \
    auto tc = Function();                                                                                              \
    tc.promise->exec = asio::require(ctx.get_executor(), asio::execution::outstanding_work.tracked);                   \
    auto p = std::coroutine_handle<coro::test_case_promise>::from_promise(*tc.promise);                                \
    asio::post(ctx.get_executor(), [p]{p.resume();});                                                                  \
    ctx.run();                                                                                                         \
}                                                                                                                      \
static ::coro::test_case Function()


#define CO_TEST_CASE(...) CO_TEST_CASE_IMPL(DOCTEST_ANONYMOUS(CO_DOCTEST_ANON_FUNC_), __VA_ARGS__)


#endif //CORO_TEST_HPP
