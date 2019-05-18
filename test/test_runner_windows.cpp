//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_shared.h"

#include <catch2/catch.hpp>

extern void run_test_with_catch2(test_adapter* test);

#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
static_assert(false, "this test file can't be used for unix")
#elif defined(_MSC_VER) // Windows

#endif // Platform specific running
