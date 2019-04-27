// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/net.h>
#include <gsl/gsl>

using namespace std;
using namespace coro;

array<char, NI_MAXHOST> hnbuf{};

auto host_name() noexcept -> gsl::czstring<NI_MAXHOST> {
    ::gethostname(hnbuf.data(), hnbuf.size());
    return hnbuf.data();
}

GSL_SUPPRESS(type .1)
int get_name(const endpoint_t& ep, gsl::zstring<NI_MAXHOST> name) noexcept {
    socklen_t addrlen = 0;
    if (ep.storage.ss_family == AF_INET)
        addrlen = sizeof(ep.in4);
    else if (ep.storage.ss_family == AF_INET6)
        addrlen = sizeof(ep.in6);

    return ::getnameinfo(&ep.addr, addrlen, name, NI_MAXHOST, nullptr, 0,
                         NI_NUMERICHOST | NI_NUMERICSERV);
}

GSL_SUPPRESS(type .1)
int get_name(const endpoint_t& ep, //
             gsl::zstring<NI_MAXHOST> name,
             gsl::zstring<NI_MAXSERV> serv) noexcept {

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
    return ::getnameinfo(&ep.addr, addrlen, name, NI_MAXHOST, serv, NI_MAXSERV,
                         NI_NUMERICHOST | NI_NUMERICSERV);
}

GSL_SUPPRESS(es .76)
GSL_SUPPRESS(type .1)
GSL_SUPPRESS(gsl.util)
auto resolve(const addrinfo& hint, //
             gsl::czstring<NI_MAXHOST> name,
             gsl::czstring<NI_MAXSERV> serv) noexcept
    -> coro::enumerable<endpoint_t> {
    addrinfo* list = nullptr;
    if (::getaddrinfo(name, serv, addressof(hint), &list))
        co_return;

    // RAII clean up for the assigned addrinfo
    // This holder guarantees clean up
    //      when the generator is destroyed
    auto d1 = gsl::finally([list]() noexcept { ::freeaddrinfo(list); });
    endpoint_t* ptr = nullptr;
    for (addrinfo* iter = list; nullptr != iter; iter = iter->ai_next) {
        if (iter->ai_family != AF_INET6)
            continue;

        ptr = reinterpret_cast<endpoint_t*>(iter->ai_addr);
        co_yield* ptr; // yield and proceed
    }
}
