// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#pragma once
#include <coroutine/switch.h>
#include <messaging/concurrent.h>

#include <array>

class thread_data final
{
  public:
    std::condition_variable cv;
    concurrent_message_queue queue;

  private:
    thread_data(thread_data&) = delete;
    thread_data(thread_data&&) = delete;
    thread_data& operator=(thread_data&) = delete;
    thread_data& operator=(thread_data&&) = delete;

  public:
    thread_data() noexcept(false);
    ~thread_data() noexcept;

  public:
    thread_id_t get_id() const noexcept;
};

// - Note
//      Get the address of thread data for this library
auto get_local_data() noexcept -> thread_data*;

class thread_registry final
{
  private:
    // If the program is goiing to use more threads,
    // this library must be recompiled after changing this limit
    static constexpr auto max_thread_count = 300;
    static constexpr auto invalid_idx = uint16_t{UINT16_MAX};
    static constexpr auto invalid_key = thread_id_t{UINT64_MAX};

  public:
    using key_type = thread_id_t;
    using value_type = thread_data*;
    using pointer = value_type*;

  private:
    section mtx;
    std::array<key_type, max_thread_count> keys;
    std::array<value_type, max_thread_count> values;

  private:
    thread_registry(thread_registry&) = delete;
    thread_registry(thread_registry&&) = delete;
    thread_registry& operator=(thread_registry&) = delete;
    thread_registry& operator=(thread_registry&&) = delete;

  public:
    thread_registry() noexcept(false);
    ~thread_registry() noexcept;

  public:
    auto find_or_insert(key_type id) noexcept(false) -> pointer;
    void erase(key_type id) noexcept(false);
};

// - Note
//      Get manager type for threads
auto get_thread_registry() noexcept -> thread_registry*;
