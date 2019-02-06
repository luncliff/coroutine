#include <catch2/catch.hpp>

#include <chrono>
#include <iostream>

#include <sys/epoll.h>
#include <unistd.h>

TEST_CASE("epoll", "[stage]")
{
    using namespace std::chrono;

    constexpr auto timeout = 10ms;
    constexpr auto max_event_size = 20;
    auto evs = std::make_unique<epoll_event[]>(max_event_size);

    auto epfd = epoll_create1(EPOLL_CLOEXEC);
    REQUIRE(epfd != -1);

    SECTION("timeout")
    {
        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.ptr = std::addressof(ev);
        REQUIRE(epoll_ctl(epfd, EPOLL_CTL_ADD,
                          STDERR_FILENO, // the input will *never* happen
                          &ev)
                == 0);
        auto nfds
            = epoll_wait(epfd, evs.get(), max_event_size, timeout.count());
        if (nfds == -1) // error
            FAIL(strerror(errno));
        REQUIRE(nfds == 0);
    }

    SECTION("immediate")
    {
        epoll_event ev{};
        ev.events = EPOLLIN | EPOLLET; // EPOLLIN | EPOLLERR;
        ev.data.ptr = nullptr;
        REQUIRE(epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) == 0);
        auto nfds = epoll_wait(epfd, evs.get(), max_event_size,
                               0 // infinite wait
        );
        if (nfds == -1) // error
            FAIL(strerror(errno));

        REQUIRE(nfds >= 0);
        REQUIRE(ev.data.ptr == nullptr);
    }

    SECTION("retry add")
    {
        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.ptr = nullptr;
        REQUIRE(epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) == 0);
        REQUIRE(epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) != 0);
        REQUIRE(errno == EEXIST);
        ev.events = EPOLLIN | EPOLLERR;
        REQUIRE(epoll_ctl(epfd, EPOLL_CTL_MOD, STDIN_FILENO, &ev) == 0);
    }

    REQUIRE(close(epfd) == 0);
}
