//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//

#include <catch2/catch.hpp>

#include <coroutine/enumerable.hpp>
#include <coroutine/return.h>

#include <csignal>
#include <gsl/gsl>

#if __UNIX__ || __APPLE__ || __LINUX__
#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

using packet_chunk_t = std::array<gsl::byte, 4096>;
using coroutine_task_t = std::experimental::coroutine_handle<void>;
using endpoint_t = struct sockaddr_in;
using buffer_view_t = gsl::span<gsl::byte>;

struct io_work_t
{
    void* tag = nullptr;
    int socket = -1;
    buffer_view_t buffer{};
    union {
        endpoint_t* from{};
        const endpoint_t* to;
    };
    socklen_t length = 0;

  public:
    bool ready() const noexcept;
    uint32_t resume() noexcept;
};
static_assert(sizeof(io_work_t) <= 64);

class io_send_to final : public io_work_t
{
  public:
    void suspend(coroutine_task_t rh) noexcept(false);

  public:
    bool await_ready() const noexcept
    {
        return this->ready();
    }
    void await_suspend(coroutine_task_t rh) noexcept(false)
    {
        return this->suspend(rh);
    }
    uint32_t await_resume() noexcept
    {
        return this->resume();
    }
};

class io_recv_from final : public io_work_t
{
  public:
    void suspend(coroutine_task_t rh) noexcept(false);

  public:
    constexpr bool await_ready() const noexcept
    {
        return this->ready();
    }
    void await_suspend(coroutine_task_t rh) noexcept(false)
    {
        return this->suspend(rh);
    }
    uint32_t await_resume() noexcept
    {
        return this->resume();
    }
};

static_assert(sizeof(io_send_to) == sizeof(io_work_t));
static_assert(sizeof(io_recv_from) == sizeof(io_work_t));

auto send_to(int sd, const endpoint_t& remote, buffer_view_t buffer,
             io_work_t& work) noexcept(false) -> io_send_to&;
auto recv_from(int sd, endpoint_t& remote, buffer_view_t buffer,
               io_work_t& work) noexcept(false) -> io_recv_from&;

auto fetch_io_tasks() noexcept(false) -> enumerable<coroutine_task_t>;