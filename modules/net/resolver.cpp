// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/net.h>
#include <gsl/gsl>

using namespace coro;

std::array<char, NI_MAXHOST> buf{};
auto host_name() noexcept -> gsl::czstring<NI_MAXHOST>
{
    ::gethostname(buf.data(), buf.size());
    return buf.data();
}

GSL_SUPPRESS(type .1)
std::errc nameof(const sockaddr_in6& ep, gsl::zstring<NI_MAXHOST> name) noexcept
{
    constexpr int flag = NI_NUMERICHOST | NI_NUMERICSERV;
    const sockaddr* ptr = reinterpret_cast<const sockaddr*>(&ep);
    const auto ec = ::getnameinfo(ptr, sizeof(sockaddr_in6), name, NI_MAXHOST,
                                  nullptr, 0, flag);
    return static_cast<std::errc>(ec);
}

GSL_SUPPRESS(type .1)
std::errc nameof(const sockaddr_in& ep, //
                 gsl::zstring<NI_MAXHOST> name) noexcept
{
    constexpr int flag = NI_NUMERICHOST | NI_NUMERICSERV;
    const sockaddr* ptr = reinterpret_cast<const sockaddr*>(&ep);
    const auto ec = ::getnameinfo(ptr, sizeof(sockaddr_in), name, NI_MAXHOST,
                                  nullptr, 0, flag);
    return static_cast<std::errc>(ec);
}

GSL_SUPPRESS(type .1)
std::errc nameof(const sockaddr_in6& ep, //
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
    const auto ec = ::getnameinfo(ptr, sizeof(sockaddr_in6), name, NI_MAXHOST,
                                  serv, NI_MAXSERV, flag);
    return static_cast<std::errc>(ec);
}

GSL_SUPPRESS(es .76)
GSL_SUPPRESS(type .1)
GSL_SUPPRESS(gsl.util)
auto resolve(const addrinfo& hint, //
             gsl::czstring<NI_MAXHOST> name,
             gsl::czstring<NI_MAXSERV> serv) noexcept
    -> coro::enumerable<sockaddr_in6>
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

    for (addrinfo* iter = list; nullptr != iter; iter = iter->ai_next)
    {
        if (iter->ai_family != AF_INET6)
            continue;

        sockaddr_in6& ep = *reinterpret_cast<sockaddr_in6*>(iter->ai_addr);
        // yield and proceed
        co_yield ep;
    }
}
