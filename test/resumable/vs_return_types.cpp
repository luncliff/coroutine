// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/return.h>
#include <coroutine/sync.h>

#include <gsl/gsl>

// clang-format off
#include <sdkddkver.h>
#include <CppUnitTest.h>
// clang-format on

using namespace std::literals;
using namespace std::experimental;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

class return_ignore_test : public TestClass<return_ignore_test>
{
};

class return_frame_test : public TestClass<return_frame_test>
{
};

class suspend_hook_test : public TestClass<suspend_hook_test>
{
};