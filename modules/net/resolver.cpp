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

    socklen_t slen = NI_MAXSERV;
    if (serv == nullptr)
        slen = 0;

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
                         slen, flags);
}

GSL_SUPPRESS(es .76)
GSL_SUPPRESS(type .1)
GSL_SUPPRESS(gsl.util)
auto resolve(const addrinfo& hint, //
             czstring_host name, czstring_serv serv) noexcept(false)
    -> coro::enumerable<endpoint_t> {

    addrinfo* list = nullptr;
    if (const auto ec = ::getaddrinfo(name, serv, addressof(hint), &list))
        // instead of `runtime_error`, use `system_error`
        throw system_error{ec, system_category(), gai_strerror(ec)};

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
