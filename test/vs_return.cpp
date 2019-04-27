//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/concrt.h>
#include <coroutine/return.h>

#include <gsl/gsl>

// clang-format off
#include <sdkddkver.h>
#include <CppUnitTest.h>
// clang-format on

using namespace std::literals;
using namespace std::experimental;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace coro;

class return_type_test : public TestClass<return_type_test>
{
    TEST_METHOD(no_return_nothrow)
    {
        // user won't care about coroutine life cycle.
        // the routine will be resumed(continued) properly,
        //   and co_return will destroy the frame
        auto routine = []() -> no_return {
            co_await suspend_never{};
            co_return;
        };
        routine();
    }

    TEST_METHOD(return_frame_to_coroutine)
    {
        // when the coroutine frame destuction need to be controlled manually,
        //   `return_frame` can do the work
        auto routine = []() -> frame {
            // no initial suspend
            co_await suspend_never{};
            co_return;
        };

        // invoke of coroutine will create a new frame
        auto fm = routine();

        // now the frame is 'final suspend'ed, so it can be deleted.
        auto coro = static_cast<coroutine_handle<void>>(fm);
        Assert::IsTrue(static_cast<bool>(coro));

        Assert::IsTrue(coro.done()); // 'final suspend'ed?
        coro.destroy();              // destroy it
    }
};
