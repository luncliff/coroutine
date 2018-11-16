// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
// ---------------------------------------------------------------------------
#include "net.h"

#include <coroutine/unplug.hpp>
#include <coroutine/enumerable.hpp>

using namespace std;

template<typename Fn>
auto defer(Fn&& todo)
{
    struct caller
    {
        Fn func;
        caller(Fn&& todo) : func{todo} {}
        ~caller() { func(); }
    };
    return caller{std::move(todo)};
}

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

uint32_t nameof(const endpoint& ep, char* name) noexcept
{
    constexpr int flag = NI_NUMERICHOST | NI_NUMERICSERV;
    const sockaddr* ptr = reinterpret_cast<const sockaddr*>(&ep);
    // Success : zero
    // Failure : non-zero uint32_t code
    return ::getnameinfo(
        ptr, sizeof(sockaddr_in6), name, NI_MAXHOST, nullptr, 0, flag);
}

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
    return ::getnameinfo(
        ptr, sizeof(sockaddr_in6), name, NI_MAXHOST, serv, NI_MAXSERV, flag);
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

addrinfo hint(int flags, int sock) noexcept
{
    addrinfo h{};
    h.ai_flags = flags;
    h.ai_family = AF_INET6;
    h.ai_socktype = sock;
    return h;
}

// - Note
//      Coroutine resolver
auto resolve(addrinfo hints, const char* name, const char* serv) noexcept
    -> enumerable<endpoint>
{
    addrinfo* list = nullptr;
    // Success : zero
    // Failure : non-zero uint32_t code
    uint32_t ec = ::getaddrinfo(name, serv, std::addressof(hints), &list);
    if (ec != NO_ERROR) co_return;

    // RAII clean up for the assigned addrinfo
    // This holder guarantees clean up
    //      when the generator is destroyed
    auto h = defer([list]() { ::freeaddrinfo(list); });

    addrinfo* iter = list;
    while (iter != nullptr)
    {
        endpoint* ptr = reinterpret_cast<endpoint*>(iter->ai_addr);
        // yield and proceed
        co_yield* ptr;
        iter = iter->ai_next;
    }
}

// - Note
//      Coroutine resolver
auto resolve(addrinfo hints, const char* name, const uint16_t port) noexcept
    -> enumerable<endpoint>
{
    // Convert to numeric string
    char serv[8]{};
    _itoa_s(port, serv, 7, 10);

    addrinfo* list = nullptr;
    // Success : zero
    // Failure : non-zero uint32_t code
    uint32_t ec = ::getaddrinfo(name, serv, std::addressof(hints), &list);
    if (ec != NO_ERROR) co_return;

    // RAII clean up for the assigned addrinfo
    // This holder guarantees clean up
    //      when the generator is destroyed
    auto h = defer([list]() { ::freeaddrinfo(list); });

    addrinfo* iter = list;
    // endpoint res{};
    while (iter != nullptr)
    {
        endpoint* ptr = reinterpret_cast<endpoint*>(iter->ai_addr);
        // yield and proceed
        co_yield* ptr;
        iter = iter->ai_next;
    }
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

auto resolve(const char* name, const char* serv) noexcept
    -> enumerable<endpoint>
{
    // for `connect()`    // TCP + IPv6
    addrinfo hints = hint(AI_ALL | AI_V4MAPPED, SOCK_STREAM);
    // Delegate to another generator coroutine
    return resolve(hints, name, serv);
}

auto resolve(const char* name, uint16_t port) noexcept
    -> enumerable<endpoint>
{
    addrinfo hints = hint(AI_ALL | AI_V4MAPPED | AI_NUMERICSERV, SOCK_STREAM);
    return resolve(hints, name, port);
}

auto resolve(const char* serv) noexcept
    -> enumerable<endpoint>
{
    // for `bind()`    // TCP + IPv6
    addrinfo hints = hint(AI_PASSIVE | AI_V4MAPPED, SOCK_STREAM);
    // Delegate to another generator coroutine
    return resolve(hints, nullptr, serv);
}
auto resolve(uint16_t port) noexcept -> enumerable<endpoint>
{
    addrinfo hints = hint(AI_ALL | AI_V4MAPPED | AI_NUMERICSERV, SOCK_STREAM);
    return resolve(hints, nullptr, port);
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
