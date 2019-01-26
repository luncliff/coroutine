#include <catch2/catch.hpp>

#include <chrono>
#include <iostream>

#include <sys/event.h>
#include <sys/types.h>
#include <unistd.h>

TEST_CASE("kqueue", "[stage]")
{
    using namespace std::chrono;

    constexpr auto timeout = 10ms;
    constexpr auto max_event_size = 20;

    auto kqfd = kqueue();
    auto events = std::make_unique<kevent64_s[]>(20);
    int count = 0;

    SECTION("return non-negative fd")
    {
        REQUIRE(kqfd != -1);
    }

    SECTION("attach new fd")
    {
        kevent64_s req{};
        req.ident = STDIN_FILENO;
        req.filter = EVFILT_READ; //| EVFILT_WRITE;
        req.flags = EV_ADD | EV_ENABLE | EV_DISPATCH | EV_ONESHOT;
        req.fflags = 0;
        req.data = 0;
        req.udata = reinterpret_cast<uint64_t>(nullptr);

        timespec* timeout = nullptr;
        // attach the event config
        count = kevent64(kqfd,       //
                         &req, 1,    // change
                         nullptr, 0, //
                         0, timeout);
        if (count == -1)
            FAIL(strerror(errno));

        // wait (indefinitely) for some event
        count = kevent64(kqfd,                         //
                         nullptr, 0,                   //
                         events.get(), max_event_size, //
                         0, timeout);
        if (count == -1)
            FAIL(strerror(errno));
        REQUIRE(count == 1);

        CAPTURE(events[0].ident);
        CAPTURE(events[0].data);
    }

    REQUIRE(close(kqfd) == 0);
}
