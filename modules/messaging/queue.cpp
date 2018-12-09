// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <messaging/concurrent.h>

bool concurrent_message_queue::is_full() const noexcept
{
    return qu.is_full();
}
bool concurrent_message_queue::empty() const noexcept
{
    return qu.empty();
}
bool concurrent_message_queue::push(const value_type msg) noexcept
{
    std::unique_lock lck{cs};
    return qu.push(msg);
}
bool concurrent_message_queue::try_pop(reference msg) noexcept
{
    std::unique_lock lck{cs};
    return qu.try_pop(msg);
}

/*
#else
bool concurrent_message_queue::is_full() const noexcept
{
    return false;
}

// GSL_SUPPRESS(f .6)
bool concurrent_message_queue::empty() const noexcept
{
    return qu.empty();
}

// GSL_SUPPRESS(f .6)
bool concurrent_message_queue::push(const value_type msg) noexcept
{
    qu.push(msg);
    return true;
}

// GSL_SUPPRESS(f .6)
bool concurrent_message_queue::try_pop(reference msg) noexcept
{
    return qu.try_pop(msg);
}
#endif
*/
