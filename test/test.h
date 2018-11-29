// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include <array>
// #include <functional>
#include <numeric>

template<typename Fn>
auto defer(Fn&& todo)
{
    struct caller
    {
      private:
        Fn func;

      private:
        caller(const caller&) = delete;
        caller(caller&&) = delete;
        caller& operator=(const caller&) = delete;
        caller& operator=(caller&&) = delete;

      public:
        caller(Fn&& todo) : func{todo} {}
        ~caller() { func(); }
    };
    return caller{std::move(todo)};
}

#endif // TEST_HELPER_H
