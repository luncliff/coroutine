// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <thread/types.h>

#include <iterator>
using namespace std;

thread_registry registry{};

thread_registry::thread_registry() noexcept(false) : mtx{}, keys{}, values{}
{
    for (auto& k : keys)
        k = invalid_key;
    for (auto& v : values)
        v = reinterpret_cast<thread_data*>(0xFFE0);
}

thread_registry::~thread_registry() noexcept
{
    // for (const auto& k : keys)
    //    assert(k == invalid_key);
    // for (const auto& v : values)
    //    assert(v == reinterpret_cast<thread_data*>(0xFFE0));
}

auto thread_registry::search(key_type key) noexcept -> pointer
{
    //  unique_lock lck{ reader(mtx) };
    const auto idx = index_of(key);
    if (idx == invalid_idx)
        return nullptr;

    value_type& ref = values.at(idx);
    return addressof(ref);
}

auto thread_registry::reserve(key_type key) noexcept(false) -> pointer
{
    //  unique_lock lck{ writer(mtx) };
    unique_lock lck{mtx};

    auto idx = index_of(key);
    if (idx != invalid_idx)
    {
        value_type& ref = values.at(idx);
        return addressof(ref);
    }

    // not found. create a new one
    const auto it = find(keys.begin(), keys.end(), invalid_key);
    if (it == keys.end())
        throw runtime_error{"registry is full"};

    // remember
    idx = static_cast<uint16_t>(distance(keys.begin(), it));
    *it = key;
    value_type& ref = values.at(idx);
    return addressof(ref);
}

void thread_registry::remove(key_type key) noexcept(false)
{
    //  unique_lock lck{ writer(mtx) };
    unique_lock lck{mtx};

    const auto it = find(keys.begin(), keys.end(), key);
    if (it == keys.end())
        // unregistered one. nothing to do ?
        // throw runtime_error{"unregistered key received"};
        return;

    // forget
    const auto idx = distance(keys.begin(), it);
    values.at(idx) = reinterpret_cast<thread_data*>(0xFFE0);
    *it = invalid_key;
}

uint16_t thread_registry::index_of(key_type key) const noexcept
{
    //  unique_lock lck{ reader(mtx) };
    const auto it = find(keys.cbegin(), keys.cend(), key);
    return (it != cend(keys))
               ? static_cast<uint16_t>(distance(cbegin(keys), it))
               : invalid_idx;
}
