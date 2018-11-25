//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#pragma once

#include <array>
#include <cstdint>

template<uint16_t max_count>
struct circular_queue_t final
{
    static constexpr auto capacity = max_count + 1;

    using value_type = uint64_t;
    using pointer = value_type*;
    using reference = value_type&;

    using index_type = uint16_t;

    index_type begin = 0, end = 0;
    size_t count = 0;
    std::array<value_type, capacity> storage{};

  private:
    static index_type advance(index_type i) noexcept
    {
        return (i + 1) % capacity;
    }

  public:
    circular_queue_t() noexcept = default;

  public:
    bool is_full() const noexcept { return advance(end) == begin; }
    bool is_empty() const noexcept { return begin == end; }

    bool push(const value_type msg) noexcept
    {
        if (is_full()) return false;

        auto index = end;
        end = advance(end);
        count += 1;

        storage[index] = msg;
        return true;
    }

    [[nodiscard]] bool pop(value_type& msg) noexcept
    {
        if (is_empty()) return false;

        auto index = begin;
        begin = advance(begin);
        count -= 1;

        msg = std::move(storage[index]);
        return true;
    }
};
