// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#pragma once

#include <array>

template <typename ElemType>
class bounded_circular_queue_t
{
    static constexpr auto capacity = 500 + 1;

  public:
    using value_type = ElemType;
    using index_type = uint16_t;

  private:
    index_type begin = 0;
    index_type end = 0;
    std::array<value_type, capacity> storage{};

  public:
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

    bool push(const value_type& msg) noexcept
    {
        if (is_full())
            return false;

        const auto index = end;
        end = next(end); // increase count

        storage.at(index) = msg; // expect copy
        return true;
    }

    [[nodiscard]] bool try_pop(value_type& msg) noexcept
    {
        if (empty())
            return false;

        const auto index = begin;
        begin = next(begin); // decrease count

        msg = std::move(storage.at(index)); // expect move
        return true;
    }
};
