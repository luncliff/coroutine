// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/concrt.h>
#include <coroutine/net.h>

#include <cstdlib>
#include <cstring>
#include <system_error>

#include <sys/un.h>

#include "./kernel_queue.h"

namespace concrt {
using namespace std;

// see also: `linux/event.cpp`
kernel_queue_t selist{};

// mock eventfd using pimpl idiom.
// instead of `eventfd` in linux, we are going to use 'unix domain socket'
// possible alternative is `fifo`, but it also requires path management.
struct unix_event_t final : no_copy_move {
    int64_t sd;
    int64_t msg;
    sockaddr_un local;

  public:
    unix_event_t() noexcept(false) : sd{}, msg{}, local{} {
        constexpr const char pattern[] = "/tmp/coro_ev_XXXXXX"; // 19 char + 1
        static_assert(sizeof(pattern) == 20);
        static_assert(sizeof(pattern) < sizeof(sockaddr_un::sun_path));

        const auto len = strnlen(pattern, sizeof(pattern));
        assert(len == 19);

        strncpy(local.sun_path, pattern, len);
        assert(local.sun_path[len] == 0); // null terminated ?

        // ensure path exists using 'mkstemp'
        sd = mkstemp(local.sun_path);
        if (sd == -1)
            throw system_error{errno, system_category(), "mkstemp"};
        if (close(sd))
            throw system_error{errno, system_category(), "close"};
        if (remove(local.sun_path))
            throw system_error{errno, system_category(), "remove"};

        // prepare unix domain socket
        sd = socket(AF_UNIX, SOCK_DGRAM, 0);
        if (sd == -1)
            throw system_error{errno, system_category(), "socket"};

        // local.sun_path is already set.
        local.sun_family = AF_UNIX;

        if (::bind(sd, (sockaddr*)&local, SUN_LEN(&local))) {
            // this throw will make the object not-constructed
            auto se = system_error{errno, system_category(), "bind"};
            // so we have to make sure of the destruction
            this->~unix_event_t();
            throw se;
        }
    }
    ~unix_event_t() noexcept {
        close(sd);
        unlink(local.sun_path); // don't forget this!
    }

    void signal() noexcept(false) {
        socklen_t len = SUN_LEN(&local);
        // send 8 byte (size of state) to buffer
        auto sz = sendto(sd, &msg, sizeof(msg), 0, (sockaddr*)&local, len);
        if (sz == -1)
            throw system_error{errno, system_category(), "sendto"};

        msg = 1; // signaled
    }
    bool is_signaled() noexcept {
        return msg != 0;
    }
    void reset() noexcept(false) {
        // socket buffer is limited. we must consume properly
        auto sz = recvfrom(sd, &msg, sizeof(msg), 0, nullptr, nullptr);
        if (sz == -1)
            throw system_error{errno, system_category(), "recvfrom"};

        // by receiving message, it recovers non-signaled state
        // we are using assert since this is critical
        assert(msg == 0);
    }
};

event::event() noexcept(false) : state{} {
    auto* impl = new (std::nothrow) unix_event_t{};
    if (impl == nullptr)
        throw runtime_error{"failed to allocate data for event"};

    state = reinterpret_cast<uint64_t>(impl);
}
event::~event() noexcept {
    auto* impl = reinterpret_cast<unix_event_t*>(state);
    delete impl;
}

bool event::is_ready() const noexcept {
    auto* impl = reinterpret_cast<unix_event_t*>(state);
    return impl->is_signaled();
}

void event::set() noexcept(false) {
    auto* impl = reinterpret_cast<unix_event_t*>(state);
    if (impl->is_signaled())
        // !!! under the race condition, this check is not safe !!!
        return;

    impl->signal();
}

void event::on_suspend(task t) noexcept(false) {
    auto* impl = reinterpret_cast<unix_event_t*>(state);

    kevent64_s req{};
    req.ident = impl->sd;
    req.filter = EVFILT_READ;
    req.flags = EV_ADD | EV_ENABLE | EV_ONESHOT;
    req.fflags = 0;
    req.data = 0;
    req.udata = reinterpret_cast<uint64_t>(t.address());

    selist.change(req);
}
void event::on_resume() noexcept {
    auto* impl = reinterpret_cast<unix_event_t*>(state);
    try {
        // !!! throws `system_error` !!!
        // this is intended
        //  since the function must ensure it consumes message for the socket
        impl->reset();

    } catch (const system_error& e) {
        // be explicit instead of c++ exception handler behavior
        fputs(e.what(), stderr);

        // normally we don't have a way to handle uds error ...
        terminate();
    }
}

auto signaled_event_tasks() noexcept(false) -> coro::enumerable<event::task> {
    event::task t{};
    timespec ts{}; // zero wait

    // notice that the timeout is zero
    for (auto ev : selist.wait(ts)) {

        t = event::task::from_address(reinterpret_cast<void*>(ev.udata));
        // todo: check only for debug mode?
        if (t.done())
            throw invalid_argument{"event::task is already done state"};

        co_yield t;
    }
    co_return;
}

} // namespace concrt