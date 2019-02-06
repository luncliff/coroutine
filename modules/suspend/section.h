// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#pragma once
#include <mutex>

// - Note
//      Basic lockable for criticial section
class section final
{
    // reserve enough size to provide platform compatibility
    const uint64_t storage[16]{};

  public:
    section(section&) = delete;
    section(section&&) = delete;
    section& operator=(section&) = delete;
    section& operator=(section&&) = delete;

    section() noexcept(false);
    ~section() noexcept;

    bool try_lock() noexcept;
    void lock() noexcept(false);
    void unlock() noexcept(false);
};
