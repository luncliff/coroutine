// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  License
//      CC BY 4.0
//
//  Reference
//      -
//      https://docs.microsoft.com/en-us/windows/desktop/ProcThread/using-the-thread-pool-functions
//
// ---------------------------------------------------------------------------

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <CppUnitTest.h>
#include <Windows.h>
#include <sdkddkver.h>
#include <threadpoolapiset.h>

#include <array>

#include <coroutine/sync.h>
#include <coroutine/unplug.hpp>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std::literals;

using namespace std::experimental;

// ---- ---- ---- ----

TEST_CLASS(ThreadMessageTest)
{
    static constexpr auto repeat_count = 100'000;
    struct context_t
    {
        DWORD anton, bruno, ceaser, dora;
        wait_group start, end;
        wait_group a, b, c, d;
    };

    static void CALLBACK Anton(PTP_CALLBACK_INSTANCE,
                               context_t & context,
                               PTP_WORK work) noexcept(false)
    {
        try
        {
            context.anton = GetCurrentThreadId();
            context.start.done();
            context.a.wait();

            size_t repeat = repeat_count;
            while (repeat--)
            {
                message_t msg{};
                msg.ptr = work;

                post_message(context.bruno, msg);
                post_message(context.ceaser, msg);
                post_message(context.dora, msg);
            }
            context.end.done();
        }
        catch (const std::exception& ex)
        {
            Logger::WriteMessage(ex.what());
            context.end.done();
        }
    }
    static void CALLBACK Bruno(PTP_CALLBACK_INSTANCE,
                               context_t & context,
                               PTP_WORK work) noexcept(false)
    {
        try
        {
            UNREFERENCED_PARAMETER(work);

            context.bruno = GetCurrentThreadId();
            context.start.done();
            context.b.wait();

            size_t repeat = 0;
            do
            {
                message_t msg{};

                if (peek_message(msg) == true) repeat += 1;

            } while (repeat < repeat_count * 4);
            context.end.done();
        }
        catch (const std::exception& ex)
        {
            Logger::WriteMessage(ex.what());
            context.end.done();
        }
    }
    static void CALLBACK Ceaser(PTP_CALLBACK_INSTANCE,
                                context_t & context,
                                PTP_WORK work) noexcept(false)
    {
        try
        {
            UNREFERENCED_PARAMETER(work);

            context.ceaser = GetCurrentThreadId();
            context.start.done();
            context.c.wait();

            size_t repeat = 0;
            do
            {
                message_t msg{};

                if (peek_message(msg) == true)
                {
                    repeat += 1;

                    post_message(context.bruno, msg);
                    post_message(context.dora, msg);
                }
            } while (repeat < repeat_count);
            context.end.done();
        }
        catch (const std::exception& ex)
        {
            Logger::WriteMessage(ex.what());
            context.end.done();
        }
    }

    static void CALLBACK Dora(PTP_CALLBACK_INSTANCE,
                              context_t & context,
                              PTP_WORK work) noexcept(false)
    {
        try
        {
            UNREFERENCED_PARAMETER(work);

            context.dora = GetCurrentThreadId();
            context.start.done();
            context.d.wait();

            size_t repeat = 0;
            do
            {
                message_t msg{};

                if (peek_message(msg) == true)
                {
                    repeat += 1;
                    post_message(context.bruno, msg);
                }

            } while (repeat < repeat_count * 2);
            context.end.done();
        }
        catch (const std::exception& ex)
        {
            Logger::WriteMessage(ex.what());
            context.end.done();
        }
    }

    context_t context{};
    std::array<PTP_WORK, 4> works{};

    TEST_METHOD_INITIALIZE(Initialize)
    {
        works[0] =
            ::CreateThreadpoolWork(reinterpret_cast<PTP_WORK_CALLBACK>(Anton),
                                   std::addressof(context),
                                   nullptr);
        works[1] =
            ::CreateThreadpoolWork(reinterpret_cast<PTP_WORK_CALLBACK>(Bruno),
                                   std::addressof(context),
                                   nullptr);
        works[2] =
            ::CreateThreadpoolWork(reinterpret_cast<PTP_WORK_CALLBACK>(Ceaser),
                                   std::addressof(context),
                                   nullptr);
        works[3] =
            ::CreateThreadpoolWork(reinterpret_cast<PTP_WORK_CALLBACK>(Dora),
                                   std::addressof(context),
                                   nullptr);

        for (auto work : works)
            Assert::IsNotNull(work);
    }

    TEST_METHOD_CLEANUP(CleanUp)
    {
        SleepEx(1'000, TRUE);

        for (auto work : works)
            ::CloseThreadpoolWork(work);
    }

    TEST_METHOD(MessageFlow)
    {
        const auto num_workers = static_cast<uint32_t>(works.size());
        context.start.add(num_workers);
        context.end.add(num_workers);
        context.a.add(1);
        context.b.add(1);
        context.c.add(1);
        context.d.add(1);

        for (auto work : works)
            ::SubmitThreadpoolWork(work);

        context.start.wait();
        context.b.done();
        context.d.done();
        context.c.done();
        context.a.done();
        context.end.wait();
    }
};

TEST_CLASS(ThreadPoolTest)
{
    static void CALLBACK onWork1(PTP_CALLBACK_INSTANCE instance,
                                 wait_group & group,
                                 PTP_WORK work) noexcept
    {
        UNREFERENCED_PARAMETER(instance);
        UNREFERENCED_PARAMETER(work);
        group.done();
    }

    wait_group group{};
    PTP_WORK work{};

    TEST_METHOD_INITIALIZE(Initialize)
    {
        work =
            ::CreateThreadpoolWork(reinterpret_cast<PTP_WORK_CALLBACK>(onWork1),
                                   std::addressof(group),
                                   nullptr);
        Assert::IsNotNull(work);
    }
    TEST_METHOD_CLEANUP(CleanUp) { ::CloseThreadpoolWork(work); }

    TEST_METHOD(WaitWorkers)
    {
        constexpr auto num_of_work = 1000;

        // fork - join
        group.add(num_of_work);
        for (auto i = 0u; i < num_of_work; ++i)
        {
            ::SubmitThreadpoolWork(work);
        }
        group.wait();
    }

    TEST_METHOD(WaitGroupSelfWait)
    {
        group.add(1);
        group.done();
        group.wait(200);
    }

    TEST_METHOD(WaitGroupMultipleDone)
    {
        group.done();
        group.done();
        group.wait(200);
    }
};

TEST_CLASS(CriticalSectionTest)
{
    section cs[4]{};

    TEST_METHOD(WaitGroupMultipleDone)
    {
        std::lock_guard<section> lck1{cs[0]};
        std::lock_guard<section> lck2{cs[1]};
        std::lock_guard<section> lck3{cs[2]};
        std::lock_guard<section> lck4{cs[3]};
    }
};
