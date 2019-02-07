//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#pragma once
#include <thread>

enum thread_id_t : uint64_t;

// - Note
//      Thin wrapper to prevent constant folding of std namespace
//      thread id query
auto get_current_thread_id() noexcept -> thread_id_t;
