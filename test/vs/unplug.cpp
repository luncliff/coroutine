// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include "./vstest.h"

#include <coroutine/return.h>

class unplug_frame_test : public TestClass<unplug_frame_test>
{
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
