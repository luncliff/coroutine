/**
 * @author github.com/luncliff (luncliff@gmail.com)
 * @brief Get a string representation from the `sockaddr_in` object
 */
#include <coroutine/net.h>

using namespace std;
using namespace coro;

void socket_setup() noexcept(false);
void socket_teardown() noexcept;

int main(int, char* []) {
    socket_setup();
    auto on_return = gsl::finally([]() { socket_teardown(); });

    auto name_buf = make_unique<char[]>(NI_MAXHOST);
    auto serv_buf = make_unique<char[]>(NI_MAXSERV);
    {
        sockaddr_in in4{};
        in4.sin_family = AF_INET;
        in4.sin_addr.s_addr = INADDR_ANY;
        in4.sin_port = htons(7654);
        // non-zero for error.
        // the value is redirected from `getnameinfo`
        if (auto ec = get_name(in4, name_buf.get(), nullptr)) {
            return ec;
        }
        // retry with service name buffer
        if (auto ec = get_name(in4, name_buf.get(), serv_buf.get())) {
            return ec;
        }
    }
    {
        sockaddr_in6 in6{};
        in6.sin6_family = AF_INET6;
        in6.sin6_addr = in6addr_any;
        in6.sin6_port = htons(7654);
        // non-zero for error.
        // the value is redirected from `getnameinfo`
        if (auto ec = get_name(in6, name_buf.get(), nullptr)) {
            return ec;
        }
        // retry with service name buffer
        if (auto ec = get_name(in6, name_buf.get(), serv_buf.get())) {
            return ec;
        }
    }
    return EXIT_SUCCESS;
}
