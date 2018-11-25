//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#pragma once

#include <cstdint>

template<typename ResourceType>
class index_registry
{
  public:
    using resource_type = ResourceType;
    using pointer = resource_type*;

  public:
    virtual ~index_registry() noexcept = default;

  public:
    virtual pointer find(uint64_t id) const noexcept = 0;
    virtual pointer add(uint64_t id) noexcept(false) = 0;
    virtual void remove(uint64_t id) noexcept(false) = 0;
};

auto get_registry() noexcept(false) -> index_registry<uint64_t>&;
