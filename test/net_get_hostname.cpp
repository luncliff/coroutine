//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_network.h"
#include "test_shared.h"

using namespace coro;

auto net_gethostname_test() {
    init_network_api();
    auto on_return = gsl::finally([]() { release_network_api(); });

    const auto name = host_name();
    REQUIRE(name != nullptr);
    return EXIT_SUCCESS;
}

#if defined(CMAKE_TEST)
int main(int, char* []) {
    return net_gethostname_test();
}

#elif __has_include(<CppUnitTest.h>)
class net_gethostname : public TestClass<net_gethostname> {
    TEST_METHOD(test_net_gethostname) {
        net_gethostname_test();
    }
};
#endif
