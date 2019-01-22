#include <catch2/catch.hpp>

#include "switch.h"

TEST_CASE("switch")
{
    auto example_code = [](switch_to_2 &sw, uint32_t &status) -> unplug {
        status = 2;
        co_await sw;
        status = 3;
    };

    switch_to_2 sw{};
    uint32_t status{};

    example_code(sw, status);
    REQUIRE(status == 2);

    for (auto task : sw.schedule())
    {
        REQUIRE(task);
        REQUIRE_NOTHROW(task.resume());
    }

    REQUIRE(status == 3);
}

#include <poll.h>
#include <vector>
#include <chrono>
#include <algorithm>

using namespace std::chrono;

class task_buffer
{
  public:
    auto event_files() -> std::vector<pollfd> &;
    auto event_tasks() -> std::vector<coroutine_task_t> &;

    void clear() noexcept;
};

uint16_t bbi = 0; // back buffer index
uint16_t fbi = 1; // front buffer index
task_buffer buffers[2]{};

void poll_files_for(milliseconds timeout) noexcept(false)
{
    auto &front = buffers[fbi];

    auto &poll_files = front.event_files();
    auto &poll_tasks = front.event_tasks();

    auto count =
        poll(poll_files.data(), poll_files.size(),
             static_cast<int>(timeout.count()));

    if (count < 0) // failure
        throw std::system_error{errno, std::system_category(), "poll"};
    if (count == 0) // timeout. try again
        return;

    // positive for number of file descriptors that have been selected
    // revent is non-zero

    // policy: std::execution::par
    for (auto i = 0u; count > 0;)
    {
        auto &pf = poll_files[i];

        if (pf.revents == 0) // no event for the file
            continue;
        if (pf.revents & POLLNVAL) // invalid fd
            continue;

        count -= 1; // keep processing until zero

        // POLLRDBAND; // ...
        // POLLWRBAND;
        // POLLIN;
        // POLLOUT;
        // POLLERR;
        // POLLHUP;
        if (auto task = poll_tasks[i])
            task.resume();
    }

    // next invoke will use another buffer
    front.clear();

    auto &back = buffers[bbi];
    std::swap(front, back);
}
