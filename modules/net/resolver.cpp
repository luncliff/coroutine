//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/net.h>
#include <gsl/gsl>

using namespace std;
using namespace coro;

GSL_SUPPRESS(type .1)
int get_name(const endpoint_t& ep, //
             zstring_host name, zstring_serv serv, int flags) noexcept {

    socklen_t addrlen = 0;
    if (ep.storage.ss_family == AF_INET)
        addrlen = sizeof(ep.in4);
    else if (ep.storage.ss_family == AF_INET6)
        addrlen = sizeof(ep.in6);

    // available flags...
    //      NI_NAMEREQD
    //      NI_DGRAM
    //      NI_NOFQDN
    //      NI_NUMERICHOST
    //      NI_NUMERICSERV
    // non-zero if failed
    return ::getnameinfo(addressof(ep.addr), addrlen, name, NI_MAXHOST, serv,
                         (serv == nullptr) ? 0 : NI_MAXSERV, flags);
}

auto enumerate_addrinfo(gsl::not_null<addrinfo*> list)
    -> coro::enumerable<endpoint_t> {
    // RAII clean up for the assigned addrinfo
    // This holder guarantees clean up
    //      when the generator is destroyed
    auto d1 = gsl::finally([list]() noexcept { ::freeaddrinfo(list); });

    endpoint_t* ptr = nullptr;
    for (addrinfo* it = list; it != nullptr; it = it->ai_next) {
        ptr = reinterpret_cast<endpoint_t*>(it->ai_addr);
        if (ptr == nullptr)
            continue;

        endpoint_t& ep = *ptr;
        co_yield ep;
    }
}

int resolve(coro::enumerable<endpoint_t>& g,
            const addrinfo& hint, //
            czstring_host name, czstring_serv serv) noexcept {
    addrinfo* list = nullptr;
    if (const auto ec = ::getaddrinfo(name, serv, addressof(hint), &list))
        return ec;

    g = enumerate_addrinfo(list);
    return 0;
}

errc peer_name(uint64_t sd, sockaddr_in6& ep) noexcept {
    socklen_t len = sizeof(sockaddr_in6);
    errc ec{};

    if (getpeername(static_cast<int64_t>(sd), reinterpret_cast<sockaddr*>(&ep),
                    &len))
        ec = errc{errno};
    return ec;
}

errc sock_name(uint64_t sd, sockaddr_in6& ep) noexcept {
    socklen_t len = sizeof(sockaddr_in6);
    errc ec{};

    if (getsockname(static_cast<int64_t>(sd), reinterpret_cast<sockaddr*>(&ep),
                    &len))
        ec = errc{errno};
    return ec;
}
