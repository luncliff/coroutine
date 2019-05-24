//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_network.h"
#include "test_shared.h"

#include "test_concrt_latch.cpp"
#include "test_coro_channel.cpp"
#include "test_coro_return.cpp"
#include "test_coro_yield_enumerable.cpp"
#include "test_coro_yield_sequence.cpp"
#include "test_coroutine_handle.cpp"
#include "test_net_echo_tcp.cpp"
#include "test_net_echo_udp.cpp"
#include "test_net_resolver.cpp"

#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
#elif defined(_WINDOWS_) || defined(_MSC_VER) // Windows
#define CATCH_CONFIG_WINDOWS_CRTDBG
#endif // Platform specific configuration
#include <catch2/catch.hpp>

// simple template method pattern
void run_test_with_catch2(test_adapter* test) {
    auto on_return = gsl::finally([test]() { test->on_teardown(); });
    test->on_setup();
    REQUIRE_NOTHROW(test->on_test());
}

TEST_CASE_METHOD(coroutine_handle_move_test, //
                 "coroutine_handle move", "[primitive]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coroutine_handle_swap_test, //
                 "coroutine_handle swap", "[primitive]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_no_return_test, //
                 "no return from coroutine", "[return]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_frame_empty_test, //
                 "empty frame object", "[return]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_frame_return_test, //
                 "frame after return", "[return]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_frame_first_suspend_test, //
                 "frame after suspend", "[return]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_frame_awaitable_test, //
                 "frame supports co_await", "[return]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(concrt_latch_wait_after_ready_test, //
                 "latch wait after ready", "[concurrency]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(concrt_latch_count_down_and_wait_test, //
                 "latch count_down and wait", "[concurrency]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(concrt_latch_throws_when_negative_from_positive_test, //
                 "latch when underflow 1", "[concurrency]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(concrt_latch_throws_when_negative_from_zero_test, //
                 "latch when underflow 2", "[concurrency]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_enumerable_no_yield_test, //
                 "generator no yield", "[yield]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_enumerable_yield_once_test, //
                 "generator yield once", "[yield]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_enumerable_iterator_test, //
                 "generator iterator", "[yield]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_enumerable_after_move_test, //
                 "generator after move", "[yield]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_enumerable_accumulate_test, //
                 "generator accumulate", "[yield]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_enumerable_max_element_test, //
                 "generator max_element", "[yield]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_sequence_no_yield_test, //
                 "async generator no yield", "[yield][async]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_sequence_frame_status_test, //
                 "async generator status", "[yield][async]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_sequence_yield_once_test, //
                 "async generator yield once", "[yield][async]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_sequence_suspend_using_await_test, //
                 "async generator suspend using co_await", "[yield][async]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_sequence_suspend_using_yield_test, //
                 "async generator suspend using co_yield", "[yield][async]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_sequence_destroy_when_suspended_test, //
                 "async generator destroy when suspended", "[yield][async]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_channel_write_before_read_test, //
                 "channel write before read", "[channel]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_channel_read_before_write_test, //
                 "channel read before write", "[channel]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_channel_mutexed_write_before_read_test, //
                 "channel(with mutex) write before read",
                 "[channel][concurrency]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_channel_mutexed_read_before_write_test, //
                 "channel(with mutex) read before write",
                 "[channel][concurrency]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_channel_write_return_false_after_close_test, //
                 "channel close while write", "[channel]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_channel_read_return_false_after_close_test, //
                 "channel close while read", "[channel]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_channel_select_type_matching_test, //
                 "channel select type assertion", "[channel]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_channel_select_on_empty_test, //
                 "channel select bypass empty", "[channel]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_channel_select_peek_every_test, //
                 "channel select peek all non-empty", "[channel]") {
    run_test_with_catch2(this);
}
TEST_CASE_METHOD(coro_channel_no_leak_under_race_test, //
                 "channel no leak under race condition",
                 "[channel][concurrency]") {
    run_test_with_catch2(this);
}

// void run_network_test_with_catch2(test_adapter* test) {
//     auto on_return = gsl::finally([test]() {
//         test->on_teardown();
//         release_network_api();
//     });
//     init_network_api();
//     test->on_setup();
//     test->on_test();
// }

// TEST_CASE_METHOD(net_gethostname_test, //
//                  "current host name", "[network]") {
//     run_network_test_with_catch2(this);
// }
// TEST_CASE_METHOD(net_getnameinfo_v4_test, //
//                  "getnameinfo ipv4", "[network]") {
//     run_network_test_with_catch2(this);
// }
// TEST_CASE_METHOD(net_getnameinfo_v6_test, //
//                  "getnameinfo ipv6", "[network]") {
//     run_network_test_with_catch2(this);
// }
// TEST_CASE_METHOD(net_getaddrinfo_tcp6_connect_test, //
//                  "getaddrinfo tcp6 connect", "[network]") {
//     run_network_test_with_catch2(this);
// }
// TEST_CASE_METHOD(net_getaddrinfo_tcp6_listen_text_test, //
//                  "getaddrinfo tcp6 listen text", "[network]") {
//     run_network_test_with_catch2(this);
// }
// TEST_CASE_METHOD(net_getaddrinfo_tcp6_listen_numeric_test, //
//                  "getaddrinfo tcp6 listen numeric", "[network]") {
//     run_network_test_with_catch2(this);
// }
// TEST_CASE_METHOD(net_getaddrinfo_udp6_bind_unspecified_test, //
//                  "getaddrinfo udp6 bind unspecified", "[network]") {
//     run_network_test_with_catch2(this);
// }
// TEST_CASE_METHOD(net_getaddrinfo_udp6_bind_v4mapped_test, //
//                  "getaddrinfo udp6 bind v4mapped", "[network]") {
//     run_network_test_with_catch2(this);
// }
// TEST_CASE_METHOD(net_getaddrinfo_ip6_bind_test, //
//                  "getaddrinfo ip6 bind", "[network]") {
//     run_network_test_with_catch2(this);
// }
// TEST_CASE_METHOD(net_getaddrinfo_ip6_multicast_test, //
//                  "getaddrinfo ip6 multicast", "[network]") {
//     run_network_test_with_catch2(this);
// }
// TEST_CASE_METHOD(net_echo_tcp_test, //
//                  "socket async tcp echo", "[network]") {
//     run_network_test_with_catch2(this);
// }
// TEST_CASE_METHOD(net_echo_udp_test, //
//                  "socket async udp echo", "[network]") {
//     run_network_test_with_catch2(this);
// }
