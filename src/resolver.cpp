/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#include <coroutine/net.h>
#include <coroutine/yield.hpp>

using namespace std;
namespace coro {

GSL_SUPPRESS(type .1)
uint32_t get_name(const sockaddr_in& addr, //
                  gsl::basic_zstring<char, NI_MAXHOST> name, gsl::basic_zstring<char, NI_MAXSERV> serv,
                  int32_t flags) noexcept {
    const auto* ptr = reinterpret_cast<const sockaddr*>(addressof(addr));
    return ::getnameinfo(ptr, sizeof(sockaddr_in),                 //
                         name, NI_MAXHOST,                         //
                         serv, (serv == nullptr) ? 0 : NI_MAXSERV, //
                         flags);
}

GSL_SUPPRESS(type .1)
uint32_t get_name(const sockaddr_in6& addr, //
                  gsl::basic_zstring<char, NI_MAXHOST> name, gsl::basic_zstring<char, NI_MAXSERV> serv,
                  int32_t flags) noexcept {
    const auto* ptr = reinterpret_cast<const sockaddr*>(addressof(addr));
    return ::getnameinfo(ptr, sizeof(sockaddr_in6),                //
                         name, NI_MAXHOST,                         //
                         serv, (serv == nullptr) ? 0 : NI_MAXSERV, //
                         flags);
}

auto get_address(addrinfo* list) noexcept -> enumerable<sockaddr*> {
    auto on_return = gsl::finally([list]() noexcept {
        // RAII clean up for the assigned addrinfo
        ::freeaddrinfo(list);
    });
    for (addrinfo* it = list; it != nullptr; it = it->ai_next) {
        co_yield it->ai_addr;
    }
}

auto get_address(addrinfo* list, sockaddr_in addr) noexcept
    -> enumerable<sockaddr_in> {
    for (sockaddr* item : get_address(list)) {
        const auto* ptr = reinterpret_cast<sockaddr_in*>(item);
        addr = *ptr;
        co_yield addr;
    }
}

auto get_address(addrinfo* list, sockaddr_in6 addr) noexcept
    -> enumerable<sockaddr_in6> {
    for (sockaddr* item : get_address(list)) {
        const auto* ptr = reinterpret_cast<sockaddr_in6*>(item);
        addr = *ptr;
        co_yield addr;
    }
}

uint32_t get_address(const addrinfo& hint, //
                     gsl::czstring host, gsl::czstring serv,
                     gsl::span<sockaddr_in> output) noexcept {
    addrinfo* list = nullptr;
    if (const auto ec = ::getaddrinfo(host, serv, //
                                      &hint, &list))
        return ec; // std::system_error{ec, system_category(), ::gai_strerror(ec)};
    auto i = 0u;
    for (auto addr : get_address(list, sockaddr_in{})) {
        output[i++] = addr;
        if (i == output.size())
            break;
    }
    return 0;
}

uint32_t get_address(const addrinfo& hint, //
                     gsl::czstring host, gsl::czstring serv,
                     gsl::span<sockaddr_in6> output) noexcept {
    addrinfo* list = nullptr;
    if (const auto ec = ::getaddrinfo(host, serv, //
                                      &hint, &list))
        return ec;
    auto i = 0u;
    for (auto addr : get_address(list, sockaddr_in6{})) {
        output[i++] = addr;
        if (i == output.size())
            break;
    }
    return 0;
}

} // namespace coro
