// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <sdkddkver.h>
#include <threadpoolapiset.h>

#include <TlHelp32.h>

#include <CppUnitTest.h>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using Microsoft::VisualStudio::CppUnitTestFramework::TestClass;

// - Note
//      Mock gsl::finally until it becomes available
template <typename Fn>
auto final_action(Fn&& todo)
{
    class caller final
    {
      private:
        Fn func;

      private:
        caller(const caller&) = delete;
        caller(caller&&) = delete;
        caller& operator=(const caller&) = delete;
        caller& operator=(caller&&) = delete;

      public:
        explicit caller(Fn&& todo) : func{todo}
        {
        }
        ~caller()
        {
            func();
        }
    };
    return caller{std::move(todo)};
}
