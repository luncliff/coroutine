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

#include <CppUnitTest.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

template <typename... Args>
auto println(const char* format, Args&&... args)
{
    std::string reserve{};
    reserve.resize(1024u);

    sprintf_s(reserve.data(), reserve.size(), format,
              std::forward<Args>(args)...);
    Logger::WriteMessage(reserve.c_str());
}

// - Note
//      Mock gsl::finally until it becomes available
template<typename Fn>
auto final_action(Fn&& todo)
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
        caller(Fn&& todo) : func{ todo } {}
        ~caller() { func(); }
    };
    return caller{ std::move(todo) };
}
