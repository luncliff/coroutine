// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <thread/types.h>

#include <Windows.h> // System API
#include <iterator>

using namespace std;

thread_registry::~thread_registry() noexcept
{
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

void thread_registry::erase(key_type tid) noexcept(false)
{
    unique_lock lck{mtx}; // writer lock

    const auto it = find(keys.begin(), keys.end(), tid);
    if (it == keys.end())
        // unregistered one. nothing to do
        return;

    // forget
    const auto idx = distance(keys.begin(), it);
    values.at(idx) = nullptr;
    *it = invalid_key;
}

thread_id_t current_thread_id() noexcept
{
    // trigger thread local object instantiation
    return get_local_data()->get_id();
}

thread_data::thread_data() noexcept(false) : queue{}
{
    const auto tid = static_cast<thread_id_t>(GetCurrentThreadId());
    auto ptr = get_thread_registry()->find_or_insert(tid);
    *ptr = this;
}

thread_data::~thread_data() noexcept
{
    // this line might throw exception.
    // which WILL kill the program.
    get_thread_registry()->erase(get_id());
}

thread_id_t thread_data::get_id() const noexcept
{
    return static_cast<thread_id_t>(GetCurrentThreadId());
}

bool thread_data::post(message_t msg) noexcept(false)
{
    return queue.push(msg);
}

bool thread_data::try_pop(message_t& msg) noexcept
{
    return queue.try_pop(msg);
}
