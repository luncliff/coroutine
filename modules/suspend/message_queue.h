// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <chrono>
#include <memory>

struct message_t final
{
    union {
        uint32_t u32[2]{};
        uint64_t u64;
        void* ptr;
    };
};
static_assert(sizeof(message_t) == sizeof(uint64_t));

class messaging_queue_t
{
  public:
    using duration = std::chrono::microseconds;

  public:
    virtual ~messaging_queue_t() noexcept = default;

    virtual bool post(message_t msg) noexcept = 0;
    virtual bool peek(message_t& msg) noexcept = 0;
    virtual bool wait(message_t& msg, duration timeout) noexcept = 0;
};

auto create_message_queue() noexcept(false)
    -> std::unique_ptr<messaging_queue_t>;
