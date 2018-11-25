//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//  Note
//      Registry implementation for single thread
//
#include "./registry.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <new>

class registry_v12 final : public index_registry<uint64_t>
{
    static constexpr auto invalid_idx = std::numeric_limits<uint16_t>::max();
    static constexpr auto capacity = 50;

  public:
    using base_type = index_registry<uint64_t>;
    using pointer = typename base_type::pointer;
    using resource_type = typename base_type::resource_type;

  private:
    std::array<uint64_t, capacity> id_list{};
    std::array<uint64_t, capacity> spaces{};

  public:
    registry_v12() noexcept(false);
    ~registry_v12() noexcept;

  public:
    pointer find(uint64_t id) const noexcept override;
    pointer add(uint64_t id) noexcept(false) override;
    void remove(uint64_t id) noexcept(false) override;

  private:
    uint16_t index_of(uint64_t id) const noexcept;
    uint16_t allocate(uint64_t id) noexcept(false);
    void deallocate(uint16_t idx) noexcept(false);
    pointer resource_of(uint16_t idx) const noexcept;
};

registry_v12 reg{};

auto get_registry() noexcept(false) -> index_registry<uint64_t>&
{
    // return the instance
    return reg;
}

registry_v12::registry_v12() noexcept(false)
{
    for (auto& id : id_list)
        id = std::numeric_limits<uint64_t>::max();
}

registry_v12::~registry_v12() noexcept
{
    // ToDo: any requirement for destruction of the registry?
}

uint16_t registry_v12::index_of(uint64_t id) const noexcept
{
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

auto registry_v12::resource_of(uint16_t idx) const noexcept -> pointer
{
    auto* cptr = std::addressof(spaces[idx]);
    return const_cast<pointer>(cptr);
}

void registry_v12::deallocate(uint16_t idx) noexcept(false)
{
    // remove id
    //  'max' == 'not allocated'
    id_list[idx] = std::numeric_limits<uint64_t>::max();

    // remove resource
    //   truncate using swap. temp will call destuctor
    resource_type temp{};
    std::swap(temp, *(resource_of(idx)));
}

uint16_t registry_v12::allocate(uint64_t id) noexcept(false)
{
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

auto registry_v12::find(uint64_t id) const noexcept -> pointer
{
    // resource index
    const auto idx = index_of(id);
    return (idx == invalid_idx) ? nullptr : resource_of(idx);
}

// considering gsl::not_null for this code...
auto registry_v12::add(uint64_t id) noexcept(false) -> pointer
{
    auto idx = index_of(id);
    if (idx == invalid_idx)
        // unregistered id. allocate now
        idx = allocate(id);

    // now idx have some valid one.
    // return related resource
    return resource_of(idx);
}

auto registry_v12::remove(uint64_t id) noexcept(false) -> void
{
    const auto idx = index_of(id);
    if (idx != invalid_idx)
        // ignore unregistered id
        deallocate(idx);
}
