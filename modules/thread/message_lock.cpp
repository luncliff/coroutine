// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#define NOMINMAX

#include <coroutine/sync.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <iterator>

#include "../shared/queue.hpp"
#include "../shared/registry.h"

// If the program is goiing to use more threads,
// this library must be recompiled after changing this limit
constexpr auto max_thread_count = 100;
#pragma message("Maximum number of thread: 100")

// thread message queue
struct tm_queue_t final : public circular_queue_t<message_t, 450>
{
};

// thread message registry
struct tm_registry_t final : public index_registry<tm_queue_t>
{
    static constexpr auto invalid_idx = std::numeric_limits<uint16_t>::max();

  public:
    using base_type = index_registry<tm_queue_t>;
    using pointer = typename base_type::pointer;
    using resource_type = typename base_type::resource_type;

  private:
    mutable section mtx{};

    std::array<uint64_t, max_thread_count> id_list{};
    std::array<tm_queue_t, max_thread_count> spaces{};

  public:
    tm_registry_t() noexcept(false);
    ~tm_registry_t() noexcept = default;

  public:
    pointer find(uint64_t id) const noexcept override;
    pointer add(uint64_t id) noexcept(false) override;
    void remove(uint64_t id) noexcept(false) override;

  private:
    uint16_t index_of(uint64_t id) const noexcept(false);
    uint16_t allocate(uint64_t id) noexcept(false);
    void deallocate(uint16_t idx) noexcept(false);
    pointer resource_of(uint16_t idx) const noexcept;
};

tm_registry_t& get_registry() noexcept;

void setup_messaging() noexcept(false)
{
    // create a registry (with some config)
    auto& registry = get_registry();
    assert(std::addressof(registry) != nullptr);

    // trigger required thread local variables' innitialization
    if (current_thread_id() == thread_id_t{})
        throw std::runtime_error{"current_thread_id is not performing"};
}

void teardown_messaging() noexcept(false)
{
    // // truncate using move operation
    // auto trunc = std::move(registry);
    // assert(registry.get() == nullptr);
    auto& registry = get_registry();
    assert(std::addressof(registry) != nullptr);
}

void add_messaging_thread(thread_id_t tid) noexcept(false)
{
    auto& registry = get_registry();
    registry.add(static_cast<uint64_t>(tid));
}

void remove_messaging_thread(thread_id_t tid) noexcept(false)
{
    auto& registry = get_registry();
    registry.remove(static_cast<uint64_t>(tid));
}

void post_message(thread_id_t tid, message_t msg) noexcept(false)
{
    tm_queue_t* queue{};
    auto& registry = get_registry();

    queue = registry.add(static_cast<uint64_t>(tid));
    // find returns nullptr for unregisterd id value.
    if (queue == nullptr)
        // in the case, invalid argument
        throw std::invalid_argument{"post_message: unregisterd thread id"};

    queue->push(msg);
}
bool peek_message(message_t& msg) noexcept(false)
{
    tm_queue_t* queue{};
    auto& registry = get_registry();

    queue = registry.add(static_cast<uint64_t>(current_thread_id()));

    // find returns nullptr for unregisterd id value.
    if (queue == nullptr)
        // in the case, invalid argument
        throw std::invalid_argument{"peek_message: unregisterd thread id"};

    return queue->pop(msg);
}

// ---- Thread Message Queue Registry ----

tm_registry_t::tm_registry_t() noexcept(false) : mtx{}, id_list{}, spaces{}
{
    for (auto& id : id_list)
        id = std::numeric_limits<uint64_t>::max();
}

uint16_t tm_registry_t::index_of(uint64_t id) const noexcept(false)
{
    // reader: prevent index modification
    std::unique_lock lck{mtx};

    using std::cbegin;
    using std::cend;
    using std::find;

    // simple linear search
    // !!! expect this implementation will be used in hundred-level scale !!!
    //     need to be optimized for larger scale
    const auto it = find(cbegin(id_list), cend(id_list), id);
    if (it == cend(id_list))
        return invalid_idx;
    else
        return static_cast<uint16_t>(std::distance(cbegin(id_list), it));
}

auto tm_registry_t::resource_of(uint16_t idx) const noexcept -> pointer
{
    auto* cptr = spaces.data() + idx;
    return const_cast<pointer>(cptr);
}

void tm_registry_t::deallocate(uint16_t idx) noexcept(false)
{
    // writer: allow `index_of` to escape before modification
    std::unique_lock lck{mtx};

    // remove id
    //  'max' == 'not allocated'
    id_list.at(idx) = std::numeric_limits<uint64_t>::max();

    // remove resource
    //   truncate using swap. temp will call destuctor
    resource_type temp{};
    std::swap(temp, *(resource_of(idx)));
}

uint16_t tm_registry_t::allocate(uint64_t id) noexcept(false)
{
    // writer: allow `index_of` to escape before modification
    std::unique_lock lck{mtx};

    using std::begin;
    using std::end;
    using std::find;

    // find available space to save the given id
    // simple linear search
    const auto it = find(
        begin(id_list), end(id_list), std::numeric_limits<uint64_t>::max());

    // storage is full
    if (it == end(id_list))
        throw std::runtime_error{
            "registry is full. can't allocate resource for given id"};

    // assign id and return the index
    *it = id;
    return static_cast<uint16_t>(std::distance(begin(id_list), it));
}

auto tm_registry_t::find(uint64_t id) const noexcept -> pointer
{
    // resource index
    const auto idx = index_of(id);
    return (idx == invalid_idx) ? nullptr : resource_of(idx);
}

// considering gsl::not_null for this code...
auto tm_registry_t::add(uint64_t id) noexcept(false) -> pointer
{
    auto idx = index_of(id);
    if (idx == invalid_idx)
        // unregistered id. allocate now
        idx = allocate(id);

    // now idx have some valid one.
    // return related resource
    return resource_of(idx);
}

auto tm_registry_t::remove(uint64_t id) noexcept(false) -> void
{
    const auto idx = index_of(id);
    if (idx != invalid_idx)
        // ignore unregistered id
        deallocate(idx);
}

// ---- ---- ---- ---- ----

std::unique_ptr<tm_registry_t> reg_ptr = nullptr;
tm_registry_t& get_registry() noexcept
{
    if (reg_ptr == nullptr)
    {
        // create a registry (with some config)
        reg_ptr = std::make_unique<tm_registry_t>();
    }
    assert(reg_ptr.get() != nullptr);
    return *reg_ptr;
}