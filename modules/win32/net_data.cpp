// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/net.h>
#include <system_error>

#include <sdkddkver.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace net
{
struct api_info_t : public WSADATA
{
    api_info_t() noexcept(false) : WSADATA{}
    {
        // init version 2.2
        if (::WSAStartup(MAKEWORD(2, 2), this))
            // WSASYSNOTREADY
            // WSAVERNOTSUPPORTED
            // WSAEINPROGRESS
            // WSAEPROCLIM
            // WSAEFAULT
            throw std::error_code{WSAGetLastError(), std::system_category()};
    }
    ~api_info_t() noexcept
    {
        // release
        ::WSACleanup();
    }
};

api_info_t info{};
} // namespace net
