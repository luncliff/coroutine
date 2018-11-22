// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------

#ifdef _WIN32
#define PROCEDURE
#else
#define PROCEDURE __attribute__((constructor))
#endif

#include <array>
#include <cassert>
#include <numeric>

#include <coroutine/enumerable.hpp>
#include <coroutine/frame.h>
#include <coroutine/sync.h>
#include <coroutine/unplug.hpp>

namespace coroutine
{
auto check_coroutine_available() noexcept -> unplug
{
    co_await std::experimental::suspend_never{};
}

auto check_generator_available() noexcept -> enumerable<uint16_t>
{
    uint16_t value = 4;
    co_yield value;
    co_yield value;
    co_return;
}

// this function is reserved for library initialization
// for now, it just checks some coroutine expressions
PROCEDURE void on_load_test(void*) noexcept(false)
{
    check_coroutine_available();
    auto g = check_generator_available();
    auto sum = std::accumulate(g.begin(), g.end(), 0u);
    assert(sum == 4 * 2);
}

} // namespace coroutine

#ifdef _WIN32
#include <sdkddkver.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

thread_id_t current_thread_id() noexcept
{
    return static_cast<thread_id_t>(GetCurrentThreadId());
}

extern void setup_indices() noexcept;
extern void teardown_indices() noexcept;
extern void register_thread(thread_id_t thread_id) noexcept(false);
extern void forget_thread(thread_id_t thread_id) noexcept(false);

// https://msdn.microsoft.com/en-us/library/windows/desktop/ms682583(v=vs.85).aspx
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    const auto tid = current_thread_id();
    try
    {
        if (reason == DLL_THREAD_ATTACH)
        {
            coroutine::on_load_test(instance);
            register_thread(tid);
        }
        if (reason == DLL_THREAD_DETACH)
        {
            forget_thread(tid);
        }
        if (reason == DLL_PROCESS_ATTACH)
        {
            setup_indices();
            register_thread(tid);
        }
        if (reason == DLL_PROCESS_DETACH)
        {
            forget_thread(tid);
            teardown_indices();
        }
        return TRUE;
    }
    catch (const std::exception& e)
    {
        ::perror(e.what());
    }
    return FALSE;
}

#endif // _WIN32
