// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
//  Note
//    https://en.cppreference.com/w/cpp/experimental/concurrency
//    https://github.com/alasdairmackintosh/google-concurrency-library
//
// ---------------------------------------------------------------------------
#pragma once
// clang-format off
#ifdef USE_STATIC_LINK_MACRO // ignore macro declaration in static build
#   define _INTERFACE_
#   define _HIDDEN_
#else 
#   if defined(_MSC_VER) // MSVC
#       define _HIDDEN_
#       ifdef _WINDLL
#           define _INTERFACE_ __declspec(dllexport)
#       else
#           define _INTERFACE_ __declspec(dllimport)
#       endif
#   elif defined(__GNUC__) || defined(__clang__)
#       define _INTERFACE_ __attribute__((visibility("default")))
#       define _HIDDEN_ __attribute__((visibility("hidden")))
#   else
#       error "unexpected compiler"
#   endif // compiler check
#endif
// clang-format on

#ifndef EXPERIMENTAL_CONCURRENCY_TS_ADAPTER_H
#define EXPERIMENTAL_CONCURRENCY_TS_ADAPTER_H

#include <cstddef>
#include <cstdint>

namespace concrt
{

// - Note
//		An opaque type for fork-join scenario
//		Its interface might a bit different with latch in the TS
// - See Also
//		https://en.cppreference.com/w/cpp/experimental/latch
class latch
{
    // reserve enough size to provide platform compatibility
    std::byte storage[128]{};

  public:
    latch(latch&) = delete;
    latch(latch&&) = delete;
    latch& operator=(latch&) = delete;
    latch& operator=(latch&&) = delete;

    _INTERFACE_ explicit latch(uint32_t count) noexcept(false);
    _INTERFACE_ ~latch() noexcept;

    _INTERFACE_ void count_down_and_wait() noexcept(false);
    _INTERFACE_ void count_down(uint32_t n = 1) noexcept(false);
    _INTERFACE_ bool is_ready() const noexcept;
    _INTERFACE_ void wait() noexcept(false);
};

} // namespace concrt

#endif // EXPERIMENTAL_CONCURRENCY_TS_ADAPTER_H
