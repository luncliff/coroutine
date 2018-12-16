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

struct thread_data final
{
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

thread_data* get_local_data() noexcept;

struct thread_registry final
{
  private:
    // If the program is goiing to use more threads,
    // this library must be recompiled after changing this limit
    static constexpr auto max_thread_count = 300;
    static constexpr uint16_t invalid_idx = UINT16_MAX;
    static constexpr uint64_t invalid_key = UINT64_MAX;

  public:
    using key_type = uint64_t;
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
    auto search(key_type id) noexcept -> pointer;
    auto reserve(key_type id) noexcept(false) -> pointer;
    void remove(key_type id) noexcept(false);

  private:
    uint16_t index_of(key_type id) const noexcept;
};
