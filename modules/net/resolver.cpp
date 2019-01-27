// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/net.h>
#include <gsl/gsl>

char buf[NI_MAXHOST]{};
auto host_name() noexcept -> gsl::czstring<NI_MAXHOST>
{
    ::gethostname(buf, NI_MAXHOST);
    return buf;
}

uint32_t nameof(const sockaddr_in6& ep, gsl::zstring<NI_MAXHOST> name) noexcept
{
    constexpr int flag = NI_NUMERICHOST | NI_NUMERICSERV;
    const sockaddr* ptr = reinterpret_cast<const sockaddr*>(&ep);
    // Success : zero
    // Failure : non-zero uint32_t code
    return ::getnameinfo(ptr, sizeof(sockaddr_in6), name, NI_MAXHOST, nullptr,
                         0, flag);
}

uint32_t nameof(const sockaddr_in6& ep, //
                gsl::zstring<NI_MAXHOST> name,
                gsl::zstring<NI_MAXSERV> serv) noexcept
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

auto resolve(const addrinfo& hint, //
             gsl::czstring<NI_MAXHOST> name,
             gsl::czstring<NI_MAXSERV> serv) noexcept
    -> enumerable<sockaddr_in6>
{
    addrinfo* list = nullptr;

    if (auto ec = ::getaddrinfo(name, serv, //
                                std::addressof(hint), &list))
    {
        fputs(gai_strerror(ec), stderr);
        co_return;
    }

    // RAII clean up for the assigned addrinfo
    // This holder guarantees clean up
    //      when the generator is destroyed
    auto d1 = gsl::finally([list]() noexcept { ::freeaddrinfo(list); });

    for (addrinfo* iter = list; iter; iter = iter->ai_next)
    {
        sockaddr_in6& ep = *reinterpret_cast<sockaddr_in6*>(iter->ai_addr);
        // yield and proceed
        co_yield ep;
    }
}
