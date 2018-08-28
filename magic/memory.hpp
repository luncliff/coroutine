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
#ifndef _MAGIC_MEMORY_HPP_
#define _MAGIC_MEMORY_HPP_

#include <algorithm>
#include <cassert>

namespace magic
{
    template <typename Unit, uint32_t Max>
    class index_pool final
    {
        using chunk_t = std::array<uint32_t, (sizeof(Unit) / sizeof(uint32_t)) + 1>;
    private:
        uint32_t available{};
        std::array<uint32_t, Max> indices{};
        std::array<chunk_t, Max> space{};

    public:
        explicit index_pool() : available{ Max }, space{}
        {
            uint32_t i = 0;
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
        auto allocate() noexcept->void *
        {
            if (available == 0)
                return nullptr;

            available -= 1;
            chunk_t* chunk = space.data() + indices[available];
            indices[available] = UINT32_MAX;
            return chunk;
        }

        void deallocate(void* a) noexcept
        {
            indices[available] = get_id(a);
            available += 1;
            return;
        }

        constexpr auto capacity() const noexcept
        {
            return Max;
        }

        uint32_t get_id(void* a) const noexcept
        {
            const chunk_t* chunk = reinterpret_cast<chunk_t*>(a);

            return static_cast<uint32_t>(std::distance(space.data(), chunk));
        }
    };
}

#endif
