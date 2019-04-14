//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <atomic>
#include <coroutine/frame.h>
#include <cstddef>

using namespace std;
using namespace std::experimental;

TEST_CASE("coroutine_handle", "[primitive][semantics]")
{
    // helper object to use address
    std::byte blob[64]{};

    coroutine_handle<void> c1{}, c2{};

    SECTION("move")
    {
        c1 = coroutine_handle<void>::from_address(blob + 2);
        c2 = coroutine_handle<void>::from_address(blob + 3);

        REQUIRE(c1.address() == blob + 2);
        REQUIRE(c2.address() == blob + 3);
        c2 = move(c1);
        // expect the address is moved to c2
        REQUIRE(c1.address() == blob + 2);
        REQUIRE(c2.address() == blob + 2);
    }

    SECTION("swap")
    {
        c1 = coroutine_handle<void>::from_address(blob + 1);
        c2 = coroutine_handle<void>::from_address(blob + 7);

        REQUIRE(c1.address() != nullptr);
        REQUIRE(c2.address() != nullptr);

        swap(c1, c2);
        REQUIRE(c1.address() == blob + 7);
        REQUIRE(c2.address() == blob + 1);
    }
}

enum thread_id_t : uint64_t;

#if defined(_MSC_VER)
#include <Windows.h>
#include <sdkddkver.h>

auto get_current_thread_id() noexcept -> thread_id_t
{
    return static_cast<thread_id_t>(GetCurrentThreadId());
}
void get_current_thread_id(std::atomic<thread_id_t>& tid) noexcept
{
    tid.store(get_current_thread_id());
}

#elif defined(__unix__) || defined(__linux__) || defined(__APPLE__)
#include <pthread.h>

void get_current_thread_id(std::atomic<thread_id_t>& tid) noexcept
{
    auto id = reinterpret_cast<uint64_t>((void*)pthread_self());
    tid.store(thread_id_t{id});
}
auto get_current_thread_id() noexcept -> thread_id_t
{
    auto id = reinterpret_cast<uint64_t>((void*)pthread_self());
    return thread_id_t{id};
}

#endif