// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/enumerable.hpp>
#include <coroutine/unplug.hpp>

#include "net.h"
// #include <gsl/gsl_util>

using namespace std;
/*
const char* host_name() noexcept
{
    static char buf[NI_MAXHOST]{};
    static bool ready = false;
    // if not initialized...
    if (!ready)
    {
        ::gethostname(buf, NI_MAXHOST);
        ready = true;
    }
    return buf;
}
*/

// GSL_SUPPRESS(type .1)
uint32_t nameof(const endpoint& ep, char* name) noexcept
{
    constexpr int flag = NI_NUMERICHOST | NI_NUMERICSERV;
    const sockaddr* ptr = reinterpret_cast<const sockaddr*>(&ep);
    // Success : zero
    // Failure : non-zero uint32_t code
    return ::getnameinfo(ptr, sizeof(sockaddr_in6), name, NI_MAXHOST, nullptr,
                         0, flag);
}

// GSL_SUPPRESS(type .1)
uint32_t nameof(const endpoint& ep, char* name, char* serv) noexcept
{
    //      NI_NAMEREQD
    //      NI_DGRAM
    //      NI_NOFQDN
    //      NI_NUMERICHOST
    //      NI_NUMERICSERV
    constexpr int flag = NI_NUMERICHOST | NI_NUMERICSERV;
    const sockaddr* ptr = reinterpret_cast<const sockaddr*>(&ep);
    // Success : zero
    // Failure : non-zero uint32_t code
    return ::getnameinfo(ptr, sizeof(sockaddr_in6), name, NI_MAXHOST, serv,
                         NI_MAXSERV, flag);
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

constexpr addrinfo make_hint(int flags, int sock) noexcept
{
    addrinfo h{};
    h.ai_flags = flags;
    h.ai_family = AF_INET6;
    h.ai_socktype = sock;
    return h;
}

// GSL_SUPPRESS(type .1)
auto resolve(const addrinfo& hints, const char* name, const char* serv) noexcept
    -> enumerable<endpoint>
{
    addrinfo* list = nullptr;
    // Success : zero
    // Failure : non-zero uint32_t code
    if (::getaddrinfo(name, serv, std::addressof(hints), &list) == NO_ERROR)
    {
        // RAII clean up for the assigned addrinfo
        // This holder guarantees clean up
        //      when the generator is destroyed
        // auto h = gsl::finally([list]() noexcept { ::freeaddrinfo(list); });

        addrinfo* iter = list;
        while (iter != nullptr)
        {
            endpoint* ptr = reinterpret_cast<endpoint*>(iter->ai_addr);
            // yield and proceed
            co_yield* ptr;
            iter = iter->ai_next;
        }
    }
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

auto resolve(const char* name, const char* serv) noexcept
    -> enumerable<endpoint>
{
    // for `connect()`    // TCP + IPv6
    const auto hints = make_hint(AI_ALL | AI_V4MAPPED, SOCK_STREAM);
    // Delegate to another generator coroutine
    return resolve(hints, name, serv);
}

auto resolve(const char* serv) noexcept -> enumerable<endpoint>
{
    // for `bind()`    // TCP + IPv6
    const auto hints = make_hint(AI_PASSIVE | AI_V4MAPPED, SOCK_STREAM);
    // Delegate to another generator coroutine
    return resolve(hints, nullptr, serv);
}

//
// auto resolve(const char *name, const char *serv) noexcept
//    -> enumerable<endpoint>
//{
//    // UDP + IPv6
//    addrinfo hints = hint(AI_ALL | AI_V4MAPPED, SOCK_DGRAM);
//    // Delegate to another generator coroutine
//    return  resolve(hints, name, serv);
//}

//
// auto resolve(const char *serv) noexcept
//    -> enumerable<endpoint>
//{
//    // UDP + IPv6
//    addrinfo hints = hint(AI_PASSIVE | AI_V4MAPPED, SOCK_DGRAM);
//    // Delegate to another generator coroutine
//    return  resolve(hints, nullptr, serv);
//}
