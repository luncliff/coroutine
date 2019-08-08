//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
//  Note
//      Set of concepts for the C++ Coroutines
//
#pragma once
#ifndef COROUTINE_CONCEPTS_V1_H
#define COROUTINE_CONCEPTS_V1_H

#include <type_traits>

#if defined(__cpp_concepts) // clang-format off

template<typename T, typename R = void>
concept bool awaitable = requires(T a, coroutine_handle<void> h) {
    { a.await_ready() } -> bool;
    { a.await_suspend(h) } -> void;
    { a.await_resume() } -> R;
};

template<typename P>
concept bool promise_requirement_basic = requires(P p) {
    { p.initial_suspend() } -> awaitable;
    { p.final_suspend() } -> awaitable;
    { p.unhandled_exception() } -> void;
};

template<typename T>
concept bool non_void = !std::is_same_v<T, void>;

template<promise_requirement_basic P>
concept bool promise_return_void = requires(P p) {
    { p.return_void() } -> void;
    { p.get_return_object() } -> non_void;
};

template<promise_requirement_basic P, non_void T>
concept bool promise_return_value = requires(P p, T&& v) {
    { p.return_value(v) } -> void;
    { p.get_return_object() } -> non_void;
};

// todo: combination with `std::enable_if`
template<promise_requirement_basic P, non_void T>
concept bool promise_requirement_return = 
    promise_return_void<P> || promise_return_value<P,T>;

template<typename T>
concept bool has_promise_type = requires(){
    typename T::promise_type;
};

#endif //clang-format on 
#endif // COROUTINE_CONCEPTS_V1_H
