//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_FAST_COMPILE
#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)

#elif defined(_MSC_VER) // Windows
#define CATCH_CONFIG_WINDOWS_CRTDBG
#endif // Platform specific configuration
#include <catch2/catch.hpp>

void expect_true(bool cond) {
    REQUIRE(cond);
}
void print_message(std::string&& msg) {
    puts(msg.c_str());
}
void fail_with_message(std::string&& msg) {
    FAIL(msg);
}
