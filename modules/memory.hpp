// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  License
//      CC BY 4.0
//
//  Note
//      Simple memory pool class
//
// ---------------------------------------------------------------------------
#ifndef MEMORY_POOLS_HPP
#define MEMORY_POOLS_HPP

#include <algorithm>
#include <cassert>

template<typename Unit, uint16_t Max>
class index_pool final
{
    using value_type = Unit;

  private:
    uint16_t available{};
    std::array<uint16_t, Max> indices{};

  public:
    std::array<value_type, Max> space{};

  public:
    explicit index_pool() : available{Max}, space{}
    {
        uint16_t i = 0;
        for (auto& index : indices)
            index = i++;
    }

#ifdef _DEBUG
    // Check leakage in debug build
    ~index_pool() noexcept
    {
        assert(available == capacity());
        for (auto& index : indices)
            assert(index < capacity() && index >= 0);
    }
#endif

  public:
    auto allocate() noexcept -> void*
    {
        constexpr auto invalid_index = std::numeric_limits<uint16_t>::max();

        if (available == 0) return nullptr;

        available -= 1;
        value_type* chunk = space.data() + indices[available];
        indices[available] = invalid_index;
        return chunk;
    }

    void deallocate(void* a) noexcept
    {
        indices[available] = get_id(a);
        available += 1;
        return;
    }

    constexpr auto capacity() const noexcept { return Max; }

    uint16_t get_id(void* a) const noexcept
    {
        const value_type* chunk = reinterpret_cast<value_type*>(a);

        return static_cast<uint16_t>(std::distance(space.data(), chunk));
    }
};

#endif
