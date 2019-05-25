//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#pragma once
#include <coroutine/frame.h>

#include <coroutine/channel.hpp>
#include <coroutine/concrt.h>
#include <coroutine/return.h>
#include <coroutine/yield.hpp>

#include <gsl/gsl>

#include <array>
#include <atomic>
#include <future>

using namespace std;
using namespace std::experimental;
using namespace std::literals;

#if __has_include(<catch2/catch.hpp>) // for clang, use catch2
#define CATCH_CONFIG_FAST_COMPILE
#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
#elif defined(_WINDOWS_) || defined(_MSC_VER) // Windows
#define CATCH_CONFIG_WINDOWS_CRTDBG
#endif
#include <catch2/catch.hpp>
#define FAIL_WITH_MESSAGE(msg) FAIL(msg)
#define PRINT_MESSAGE(msg) CAPTURE(msg)
#define FAIL_WITH_CODE(ec) FAIL(system_category().message(ec));

#elif __has_include(<CppUnitTest.h>) // for msvc, use visual studio test
#include <CppUnitTest.h>
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#define REQUIRE(cond) Assert::IsTrue(cond)
#define PRINT_MESSAGE(msg) Logger::WriteMessage(msg.c_str());
#define FAIL_WITH_MESSAGE(msg)                                                 \
    Logger::WriteMessage(msg.c_str());                                         \
    Assert::Fail();
#define FAIL_WITH_CODE(ec)                                                     \
    Logger::WriteMessage(system_category().message(ec).c_str());               \
    Assert::Fail();

#endif
