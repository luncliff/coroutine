// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/return.h>
#include <coroutine/sync.h>

#include <Windows.h>
#include <sdkddkver.h>

#include <CppUnitTest.h>
#include <TlHelp32.h>
#include <threadpoolapiset.h>

using namespace std::literals;
using namespace std::experimental;

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using Microsoft::VisualStudio::CppUnitTestFramework::TestClass;

class unplug_frame_test : public TestClass<unplug_frame_test>
{
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

  public:
    TEST_METHOD(unplug_destroy_frame_after_return)
    {
        int status = 0;
        {
            auto try_plugging
                = [=](suspend_hook& point, int& status) -> unplug {
                // ensure final action
                auto f = final_action([&]() { status = 3; });

                status = 1;
                co_await point;

                status = 2;
                co_await point;

                co_return;
            };

            suspend_hook sp{};
            try_plugging(sp, status);
            Assert::IsTrue(status == 1);

            sp.resume();
            Assert::IsTrue(status == 2);

            sp.resume();
        }
        // did we pass through final action?
        Assert::IsTrue(status == 3);
    }
};
