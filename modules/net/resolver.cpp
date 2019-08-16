//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <coroutine/net.h>
#include <gsl/gsl>

using namespace std;
namespace coro {

int32_t get_name(const sockaddr_in& addr, zstring_host name, zstring_serv serv,
                 int32_t flags) noexcept {
    const auto* ptr = reinterpret_cast<const sockaddr*>(addressof(addr));
    return ::getnameinfo(ptr, sizeof(sockaddr_in),                 //
                         name, NI_MAXHOST,                         //
                         serv, (serv == nullptr) ? 0 : NI_MAXSERV, //
                         flags);
}

int32_t get_name(const sockaddr_in6& addr, zstring_host name, zstring_serv serv,
                 int32_t flags) noexcept {
    const auto* ptr = reinterpret_cast<const sockaddr*>(addressof(addr));
    return ::getnameinfo(ptr, sizeof(sockaddr_in6),                //
                         name, NI_MAXHOST,                         //
                         serv, (serv == nullptr) ? 0 : NI_MAXSERV, //
                         flags);
}

auto resolve_error(int32_t ec) noexcept -> system_error {
    return std::system_error{ec, system_category(), ::gai_strerror(ec)};
}

auto enumerate_addrinfo(gsl::not_null<addrinfo*> list) noexcept
    -> enumerable<sockaddr> {
    // RAII clean up for the assigned addrinfo
    // This holder guarantees clean up
    //      when the generator is destroyed
    auto d1 = gsl::finally([list]() noexcept { ::freeaddrinfo(list); });

    for (addrinfo* it = list; it != nullptr; it = it->ai_next) {
        if (it->ai_addr)
            co_yield*(it->ai_addr);
    }
}

int32_t resolve(enumerable<sockaddr>& g, const addrinfo& hint, //
                czstring_host name, czstring_serv serv) noexcept {
    addrinfo* list = nullptr;
    if (const auto ec = ::getaddrinfo(name, serv, //
                                      addressof(hint), addressof(list)))
        return ec;

    g = enumerate_addrinfo(list);
    return 0;
}

} // namespace coro
