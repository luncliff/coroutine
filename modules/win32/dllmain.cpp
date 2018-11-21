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

#include <coroutine/sequence.hpp>
#include <coroutine/sync.h>
#include <coroutine/unplug.hpp>

namespace coroutine
{

auto check_await_available(size_t& status) noexcept -> unplug
{
    co_await std::experimental::suspend_never{};
    status = 0;
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
    size_t code = 20;
    {
        check_await_available(code);
        //assert(code == 0);
    }
    {
        auto g = check_generator_available();
        for (auto v : g)
            code = v;
        //assert(code == 4 * 2);
    }
}

} // namespace coroutine

#ifdef _WIN32
#include <sdkddkver.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

//
// extern void setup_indices() noexcept;
// extern void teardown_indices() noexcept;
// extern void register_thread(uint32_t thread_id) noexcept(false);
// extern void forget_thread(uint32_t thread_id) noexcept(false);
//

// https://docs.microsoft.com/en-us/windows/desktop/ToolHelp/traversing-the-thread-list
// https://msdn.microsoft.com/en-us/library/windows/desktop/ms682583(v=vs.85).aspx
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    // const auto thread_id = GetCurrentThreadId();
    try
    {
        if (reason == DLL_THREAD_ATTACH)
        {
            coroutine::on_load_test(instance);
            // register_thread(thread_id);
        }
        if (reason == DLL_THREAD_DETACH)
        {
            // forget_thread(thread_id);
        }
        if (reason == DLL_PROCESS_ATTACH)
        {
            // setup_indices();
            // register_thread(thread_id);
        }
        if (reason == DLL_PROCESS_DETACH)
        {
            // forget_thread(thread_id);
            // teardown_indices();
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
