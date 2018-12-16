// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <thread/types.h>

#include <iterator>
using namespace std;

namespace hidden
{
thread_registry tr{};
}

auto get_thread_registry() noexcept -> thread_registry*
{
    return std::addressof(hidden::tr);
}

thread_registry::thread_registry() noexcept(false) : mtx{}, keys{}, values{}
{
    for (auto& k : keys)
        k = invalid_key;
    for (auto& v : values)
        v = nullptr;
}

thread_registry::~thread_registry() noexcept
{
    // for (const auto& k : keys)
    //    assert(k == invalid_key);
    // for (const auto& v : values)
    //    assert(v == reinterpret_cast<thread_data*>(0xFFE0));
}

extern bool check_thread_exists(thread_id_t id) noexcept;

auto thread_registry::find_or_insert(key_type tid) noexcept(false) -> pointer
{
    unique_lock lck{mtx}; // writer lock
    auto it = find(keys.begin(), keys.end(), tid);
    if (it != keys.end())
        // already exists
        goto Found;

    // not found. need to create a new one

    // check if the thread id is valid.
    if (check_thread_exists(tid) == false)
        throw std::invalid_argument{"invalid thread id"};

    // find available memory location
    it = find(keys.begin(), keys.end(), invalid_key);
    if (it == keys.end())
        throw std::runtime_error{"registry is full"};

    // remember
    *it = tid;
Found:
    return addressof(values.at(distance(keys.begin(), it)));
}

void thread_registry::erase(key_type key) noexcept(false)
{
    unique_lock lck{mtx}; // writer lock

    const auto it = find(keys.begin(), keys.end(), key);
    if (it == keys.end())
        // unregistered one. nothing to do ?
        // throw runtime_error{"unregistered key received"};
        return;

    // forget
    const auto idx = distance(keys.begin(), it);
    values.at(idx) = nullptr;

    *it = invalid_key;
}
