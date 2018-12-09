// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include "./vstest.h"

#include <coroutine/return.h>

class UnplugTest : public TestClass<UnplugTest>
{
  public:
    TEST_METHOD(InvokeFinalAction)
    {
        int status = 0;

        {
            auto try_plugging
                = [=](suspend_hook& point, int& status) -> unplug {
                // ensure final action
                auto a = defer([&]() { status = 3; });

                status = 1;
                co_await point;

                status = 2;
                co_await point;

                co_return;
            };

            suspend_hook point{};
            try_plugging(point, status);
            Assert::IsTrue(status == 1);

            point.resume();
            Assert::IsTrue(status == 2);

            point.resume();
        }
        // did we pass through final action?
        Assert::IsTrue(status == 3);
    }
};
