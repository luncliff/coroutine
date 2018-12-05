// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include "./vstest.h"

#include <coroutine/unplug.hpp>

class UnplugTest //
    : public TestClass<UnplugTest>
{
  public:
    TEST_METHOD(InvokeOrder)
    {
        int status = 0;
        {
            auto try_plugging = [=](await_point& point,
                                    int& status) -> unplug {
                auto a = defer([&]() {
                    // ensure final action
                    status = 3;
                });
                //println("try_plugging: %p \n", std::addressof(a));

                status = 1;
                co_await std::experimental::suspend_never{};
                co_await point;
                status = 2;
                co_await point;
                co_return;
            };

            await_point point{};
            try_plugging(point, status);
            Assert::IsTrue(status == 1);
            point.resume();
            Assert::IsTrue(status == 2);
            point.resume();
        }
        Assert::IsTrue(status == 3);
    }
};
