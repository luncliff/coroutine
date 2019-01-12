// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <thread/types.h>

#include <iterator>
#include <pthread.h>

using namespace std;

thread_registry::~thread_registry() noexcept
{
    // since we are allocating some resources,
    // this condition must be checked to prevent resource leak
    for (const auto& k : keys)
        // this might lead to delete using variables
        // current version doesn't support graceful closing for the usecase
        if (k != invalid_key)
            this->erase(k);
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
    auto& info = values.at(distance(keys.begin(), it));
    if (info == nullptr)
        // allocate thread data for the given ID
        info = new thread_data{};

    return addressof(info);
}

void thread_registry::erase(key_type key) noexcept(false)
{
    unique_lock lck{mtx}; // writer lock

    const auto it = find(keys.begin(), keys.end(), key);
    if (it == keys.end())
        // unregistered one. nothing to do
        return;

    // forget

    const auto idx = distance(keys.begin(), it);
    // deallocate thread data for the given ID
    delete values.at(idx);
    values.at(idx) = nullptr;

    *it = invalid_key;
}

thread_data* get_local_data() noexcept
{
    const void* p = (void*)pthread_self();
    const auto tid = reinterpret_cast<uint64_t>(p);

    auto info = get_thread_registry() //
                    ->find_or_insert( //
                        static_cast<thread_id_t>(tid));
    return *info;
}

thread_id_t current_thread_id() noexcept
{
    // trigger thread local object instantiation
    return get_local_data()->get_id();
}

thread_data::thread_data() noexcept(false) : queue{}
{
}

thread_data::~thread_data() noexcept
{
}

thread_id_t thread_data::get_id() const noexcept
{
    const void* p = (void*)pthread_self();
    const auto tid = reinterpret_cast<uint64_t>(p);
    return static_cast<thread_id_t>(tid);
}

bool thread_data::post(message_t msg) noexcept(false)
{
    return queue.push(msg);
}

bool thread_data::try_pop(message_t& msg) noexcept
{
    return queue.try_pop(msg);
}
