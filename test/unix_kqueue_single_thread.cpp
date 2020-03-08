/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#undef NDEBUG
#include <cassert>
#include <iostream>
#include <string>
#include <string_view>

#include <coroutine/return.h>
#include <coroutine/unix.h>
#include <unistd.h>

using namespace coro;
using namespace std;

auto send_async(kqueue_owner& kq, uint64_t fd) -> frame_t {
    kevent64_s req{
        .ident = fd,
        .filter = EVFILT_WRITE,
        .flags = EV_ADD | EV_ENABLE | EV_ONESHOT,
    };
    co_await kq.submit(req);

    auto message = "hello"sv;
    auto sz = write(fd, message.data(), message.length());
    if (sz < 0)
        throw system_error{errno, system_category(), "write"};
}
auto recv_async(kqueue_owner& kq, uint64_t fd, std::string& message)
    -> frame_t {
    kevent64_s req{
        .ident = fd,
        .filter = EVFILT_READ,
        .flags = EV_ADD | EV_ENABLE | EV_ONESHOT,
    };
    co_await kq.submit(req);

    array<char, 64> buf{};
    auto sz = read(fd, buf.data(), buf.size());
    if (sz < 0)
        throw system_error{errno, system_category(), "read"};

    message = string_view{buf.data(), static_cast<size_t>(sz)};
}

auto open_pipe() -> std::tuple<uint64_t, uint64_t> {
    int fds[2]{};
    if (pipe(fds) < 0)
        throw system_error{errno, system_category(), "pipe"};
    return std::make_tuple(static_cast<uint64_t>(fds[0]),
                           static_cast<uint64_t>(fds[1]));
}

int main(int, char*[]) {
    uint64_t rfd, wfd;
    tie(rfd, wfd) = open_pipe();
    auto on_return_1 = gsl::finally([=]() {
        close(rfd);
        close(wfd);
    });

    kqueue_owner kq{};
    // spawn some coroutines ... notice that we are reading before write
    string message{};
    auto reader = recv_async(kq, rfd, message);
    auto writer = send_async(kq, wfd);
    auto on_return_2 = gsl::finally([&reader, &writer]() {
        reader.destroy();
        writer.destroy();
    });

    // poll the kqueue ...
    array<kevent64_s, 2> events{};
    auto num_work = events.size();
    while (num_work) {
        auto b = events.data();
        auto e = b + kq.events(timespec{.tv_sec = 1}, events);
        for_each(b, e, [&num_work](kevent64_s& event) {
            auto coro = coroutine_handle<void>::from_address(
                reinterpret_cast<void*>(event.udata));
            coro.resume();
            --num_work;
        });
    }
    // expected message ?
    return message != "hello" ? EXIT_FAILURE : EXIT_SUCCESS;
}
