// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/sync.h>

#include <array>
#include <condition_variable>

class messaging_queue_darwin_t final : public messaging_queue_t
{
    static constexpr auto capacity = 500 + 1;

    using index_type = uint16_t;

  private:
    std::mutex cs{};
    std::condition_variable cv{};
    index_type begin = 0;
    index_type end = 0;
    std::array<message_t, capacity> storage{};

  private:
    static index_type next(index_type i) noexcept
    {
        return (i + 1) % capacity;
    }

  private:
    bool is_full() const noexcept
    {
        return next(end) == begin;
    }
    bool empty() const noexcept
    {
        return begin == end;
    }

    bool push(const message_t& msg) noexcept
    {
        if (is_full())
            return false;

        const auto index = end;
        end = next(end); // increase count;

        storage.at(index) = msg; // expect copy
        return true;
    }

    [[nodiscard]] bool try_pop(message_t& msg) noexcept
    {
        if (empty())
            return false;

        const auto index = begin;
        begin = next(begin); // decrease count;

        msg = std::move(storage.at(index)); // expect move
        return true;
    }

  public:
    bool post(message_t msg) noexcept override;
    bool peek(message_t& msg) noexcept override;
    bool wait(message_t& msg,
              std::chrono::nanoseconds timeout) noexcept override;
};

bool messaging_queue_darwin_t::post(message_t msg) noexcept
{
    std::unique_lock lck{cs};
    if (this->push(msg) == true)
    {
        cv.notify_one();
        return true;
    }
    return false;
}

bool messaging_queue_darwin_t::peek(message_t& msg) noexcept
{
    std::unique_lock lck{cs};
    return this->try_pop(msg);
}

bool messaging_queue_darwin_t::wait(message_t& msg,
                                    std::chrono::nanoseconds timeout) noexcept
{
    msg = message_t{}; // zero the memory

    std::unique_lock lck{cs};
    if (this->empty())
        cv.wait_for(lck, timeout);

    return this->try_pop(msg);
}

auto create_message_queue() noexcept(false)
    -> std::unique_ptr<messaging_queue_t>
{
    return std::make_unique<messaging_queue_darwin_t>();
}
