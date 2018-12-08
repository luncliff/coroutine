// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#pragma once
#include <coroutine/sync.h>

#include <array>
#include <cstdint>

template <typename ItemType>
struct circular_queue
{
    static constexpr auto capacity = 500 + 1;

    using value_type = ItemType;
    using pointer = value_type*;
    using reference = value_type&;

    using index_type = uint16_t;

  private:
    index_type begin = 0, end = 0;
    std::array<value_type, capacity> storage{};

  private:
    static index_type next(index_type i) noexcept
    {
        return (i + 1) % capacity;
    }

  public:
    bool is_full() const noexcept
    {
        return next(end) == begin;
    }
    bool empty() const noexcept
    {
        return begin == end;
    }

    bool push(const value_type& msg) noexcept;
    [[nodiscard]] bool try_pop(reference msg) noexcept;
};

template <typename ItemType>
bool circular_queue<ItemType>::push(const value_type& msg) noexcept
{
    if (is_full())
        return false;

    const auto index = end;
    end = next(end); // count += 1;

    storage.at(index) = msg; // expect copy
    return true;
}

template <typename ItemType>
bool circular_queue<ItemType>::try_pop(reference msg) noexcept
{
    if (empty())
        return false;

    const auto index = begin;
    begin = next(begin); // count -= 1;

    msg = std::move(storage.at(index)); // expect move
    return true;
}

// alternative of concurrency::concurrent_queue<message_t> qu{};
struct concurrent_message_queue final
{
    section cs{};
    circular_queue<message_t> qu{};

  public:
    using value_type = message_t;
    using pointer = value_type*;
    using reference = value_type&;

  public:
    bool is_full() const noexcept;
    bool empty() const noexcept;

    bool push(const value_type msg) noexcept;
    [[nodiscard]] bool try_pop(reference msg) noexcept;
};
/*
#else

#include <concurrent_queue.h>

// alternative of concurrency::concurrent_queue<message_t> qu{};
struct concurrent_message_queue final
{
    concurrency::concurrent_queue<message_t> qu{};

  public:
    using value_type = message_t;
    using pointer = value_type*;
    using reference = value_type&;

  public:
    bool is_full() const noexcept;
    bool empty() const noexcept;

    bool push(const value_type msg) noexcept;
    [[nodiscard]] bool try_pop(reference msg) noexcept;
};

#endif
*/