// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef CORO_UTIL_HPP
#define CORO_UTIL_HPP

#include <limits>
#include <type_traits>
#include <coroutine>

namespace coro
{

template<typename T>
constexpr std::size_t variadic_first(std::size_t pos = 0u)
{
    return std::numeric_limits<std::size_t>::max();
}


template<typename T, typename First, typename ... Args>
constexpr std::size_t variadic_first(std::size_t pos = 0u)
{
    if constexpr (std::is_same_v<std::decay_t<First>, T>)
        return pos;
    else
        return variadic_first<T, Args...>(pos+1);
}


template<typename T, typename ... Args>
constexpr bool variadic_has = variadic_first<T, Args...>() < sizeof...(Args);

template<std::size_t Idx, typename First, typename ... Args>
    requires (Idx <= sizeof...(Args))
constexpr decltype(auto) get_variadic(First && first, Args  && ... args)
{
    if constexpr (Idx == 0u)
        return static_cast<First>(first);
    else
        return get_variadic<Idx-1u>(static_cast<Args>(args)...);
}

template<std::size_t Idx, typename ... Args>
struct variadic_element;

template<std::size_t Idx, typename First, typename ...Tail>
struct variadic_element<Idx, First, Tail...>
{
    using type = typename variadic_element<Idx-1, Tail...>::type;
};

template<typename First, typename ...Tail>
struct variadic_element<0u, First, Tail...>
{
    using type = First;
};

template<std::size_t Idx, typename ... Args>
using variadic_element_t = typename variadic_element<Idx, Args...>::type;


template<typename ... Args>
struct variadic_last
{
    using type = variadic_element_t<sizeof...(Args) - 1, Args...>;
};

template<>
struct variadic_last<>
{
    using type = void;
};

template<typename ... Args>
using variadic_last_t = typename variadic_last<Args...>::type;


template<typename First>
constexpr decltype(auto) get_last_variadic(First && first)
{
    return first;
}

template<typename First, typename ... Args>
constexpr decltype(auto) get_last_variadic(First && first, Args  && ... args)
{
    return get_last_variadic(static_cast<Args>(args)...);
}

template<typename Promise = void>
struct coro_deleter
{
    coro_deleter() = default;
    void operator()(Promise * c)
    {
        std::coroutine_handle<Promise>::from_address(c).destroy();
    }
};


template<>
struct coro_deleter<void>
{
    coro_deleter() = default;

    template<typename Promise>
    coro_deleter(std::coroutine_handle<Promise> h)
        : deleter(+[](void * h) {std::coroutine_handle<Promise>::from_address(h).destroy();})
    {}
    void(*deleter)(void*) = nullptr;

    void operator()(void* c)
    {
        if (deleter)
            deleter(c);
        else
            std::coroutine_handle<void>::from_address(c).destroy();
    }
};

template<std::size_t N>
struct rank : rank<N-1> {};

template<>
struct rank<0u> {};

}

#endif //CORO_UTIL_HPP
