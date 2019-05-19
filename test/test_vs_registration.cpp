//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test_network.h"
#include "test_shared.h"

#include "test_concrt_latch.cpp"
#include "test_concrt_windows.cpp"
#include "test_coro_channel.cpp"
#include "test_coro_return.cpp"
#include "test_coro_yield_enumerable.cpp"
#include "test_coro_yield_sequence.cpp"
#include "test_coroutine_handle.cpp"
#include "test_net_echo_tcp.cpp"
#include "test_net_echo_udp.cpp"
#include "test_net_resolver.cpp"

#include <CppUnitTest.h>
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#define METHOD_INVOKE_TEMPLATE(test_name)                                      \
    TEST_METHOD_INITIALIZE(setup) {                                            \
        this->on_setup();                                                      \
    }                                                                          \
    TEST_METHOD_CLEANUP(teardown) {                                            \
        this->on_teardown();                                                   \
    }                                                                          \
    TEST_METHOD(test_name) {                                                   \
        this->on_test();                                                       \
    }

class coroutine_handle_move : public TestClass<coroutine_handle_move>,
                              public coroutine_handle_move_test {
    METHOD_INVOKE_TEMPLATE(test_coroutine_handle_move)
};
class coroutine_handle_swap : public TestClass<coroutine_handle_swap>,
                              public coroutine_handle_swap_test {
    METHOD_INVOKE_TEMPLATE(test_coroutine_handle_swap)
};

class coro_no_return : public TestClass<coro_no_return>,
                       public coro_no_return_test {
    METHOD_INVOKE_TEMPLATE(test_coro_no_return)
};
class coro_frame_empty : public TestClass<coro_frame_empty>,
                         public coro_frame_empty_test {
    METHOD_INVOKE_TEMPLATE(test_coro_frame_empty)
};
class coro_frame_return : public TestClass<coro_frame_return>,
                          public coro_frame_return_test {
    METHOD_INVOKE_TEMPLATE(test_coro_frame_return)
};
class coro_frame_first_suspend : public TestClass<coro_frame_first_suspend>,
                                 public coro_frame_first_suspend_test {
    METHOD_INVOKE_TEMPLATE(test_coro_frame_first_suspend)
};
class coro_frame_awaitable : public TestClass<coro_frame_awaitable>,
                             public coro_frame_awaitable_test {
    METHOD_INVOKE_TEMPLATE(test_coro_frame_awaitable)
};

class concrt_latch_wait_after_ready
    : public TestClass<concrt_latch_wait_after_ready>,
      public concrt_latch_wait_after_ready_test {
    METHOD_INVOKE_TEMPLATE(test_concrt_latch_wait_after_ready)
};
class concrt_latch_count_down_and_wait
    : public TestClass<concrt_latch_count_down_and_wait>,
      public concrt_latch_count_down_and_wait_test {
    METHOD_INVOKE_TEMPLATE(test_concrt_latch_count_down_and_wait)
};
class concrt_latch_throws_when_negative_from_positive
    : public TestClass<concrt_latch_throws_when_negative_from_positive>,
      public concrt_latch_throws_when_negative_from_positive_test {
    METHOD_INVOKE_TEMPLATE(test_concrt_latch_throws_when_negative_from_positive)
};
class concrt_latch_throws_when_negative_from_zero
    : public TestClass<concrt_latch_throws_when_negative_from_zero>,
      public concrt_latch_throws_when_negative_from_zero_test {
    METHOD_INVOKE_TEMPLATE(test_concrt_latch_throws_when_negative_from_zero)
};

class thread_api_utility : public TestClass<thread_api_utility>,
                           public thread_api_test {
    METHOD_INVOKE_TEMPLATE(test_thread_api_utility)
};
class ptp_event_set : public TestClass<ptp_event_set>,
                      public ptp_event_set_test {
    METHOD_INVOKE_TEMPLATE(test_ptp_event_set)
};
class ptp_event_cancel : public TestClass<ptp_event_cancel>,
                         public ptp_event_cancel_test {
    METHOD_INVOKE_TEMPLATE(test_ptp_event_cancel)
};
class ptp_event_wait_one : public TestClass<ptp_event_wait_one>,
                           public ptp_event_wait_one_test {
    METHOD_INVOKE_TEMPLATE(test_ptp_event_wait_one)
};
class ptp_event_wait_multiple : public TestClass<ptp_event_wait_multiple>,
                                public ptp_event_wait_multiple_test {
    METHOD_INVOKE_TEMPLATE(test_ptp_event_wait_multiple)
};

class coro_enumerable_no_yield : public TestClass<coro_enumerable_no_yield>,
                                 public coro_enumerable_no_yield_test {
    METHOD_INVOKE_TEMPLATE(test_coro_enumerable_no_yield)
};
class coro_enumerable_yield_once : public TestClass<coro_enumerable_yield_once>,
                                   public coro_enumerable_yield_once_test {
    METHOD_INVOKE_TEMPLATE(test_coro_enumerable_yield_once)
};
class coro_enumerable_iterator : public TestClass<coro_enumerable_iterator>,
                                 public coro_enumerable_iterator_test {
    METHOD_INVOKE_TEMPLATE(test_coro_enumerable_iterator)
};
class coro_enumerable_accumulate : public TestClass<coro_enumerable_accumulate>,
                                   public coro_enumerable_accumulate_test {
    METHOD_INVOKE_TEMPLATE(test_coro_enumerable_accumulate)
};
class coro_enumerable_max_element
    : public TestClass<coro_enumerable_max_element>,
      public coro_enumerable_max_element_test {
    METHOD_INVOKE_TEMPLATE(test_coro_enumerable_max_element)
};

class coro_sequence_no_yield : public TestClass<coro_sequence_no_yield>,
                               public coro_sequence_no_yield_test {
    METHOD_INVOKE_TEMPLATE(test_coro_sequence_no_yield)
};
class coro_sequence_frame_status : public TestClass<coro_sequence_frame_status>,
                                   public coro_sequence_frame_status_test {
    METHOD_INVOKE_TEMPLATE(test_coro_sequence_frame_status)
};
class coro_sequence_yield_once : public TestClass<coro_sequence_yield_once>,
                                 public coro_sequence_yield_once_test {
    METHOD_INVOKE_TEMPLATE(test_coro_sequence_yield_once)
};
class coro_sequence_suspend_using_await
    : public TestClass<coro_sequence_suspend_using_await>,
      public coro_sequence_suspend_using_await_test {
    METHOD_INVOKE_TEMPLATE(test_coro_sequence_suspend_using_await)
};
class coro_sequence_suspend_using_yield
    : public TestClass<coro_sequence_suspend_using_yield>,
      public coro_sequence_suspend_using_yield_test {
    METHOD_INVOKE_TEMPLATE(test_coro_sequence_suspend_using_yield)
};
class coro_sequence_destroy_when_suspended
    : public TestClass<coro_sequence_destroy_when_suspended>,
      public coro_sequence_destroy_when_suspended_test {
    METHOD_INVOKE_TEMPLATE(test_coro_sequence_destroy_when_suspended)
};

class coro_channel_write_before_read
    : public TestClass<coro_channel_write_before_read>,
      public coro_channel_write_before_read_test {
    METHOD_INVOKE_TEMPLATE(test_coro_channel_write_before_read)
};
class coro_channel_read_before_write
    : public TestClass<coro_channel_read_before_write>,
      public coro_channel_read_before_write_test {
    METHOD_INVOKE_TEMPLATE(test_coro_channel_read_before_write)
};
class coro_channel_mutexed_write_before_read
    : public TestClass<coro_channel_mutexed_write_before_read>,
      public coro_channel_mutexed_write_before_read_test {
    METHOD_INVOKE_TEMPLATE(test_coro_channel_mutexed_write_before_read)
};
class coro_channel_mutexed_read_before_write
    : public TestClass<coro_channel_mutexed_read_before_write>,
      public coro_channel_mutexed_read_before_write_test {
    METHOD_INVOKE_TEMPLATE(test_coro_channel_mutexed_read_before_write)
};
class coro_channel_write_return_false_after_close
    : public TestClass<coro_channel_write_return_false_after_close>,
      public coro_channel_write_return_false_after_close_test {
    METHOD_INVOKE_TEMPLATE(test_coro_channel_write_return_false_after_close)
};
class coro_channel_select_type_matching
    : public TestClass<coro_channel_select_type_matching>,
      public coro_channel_select_type_matching_test {
    METHOD_INVOKE_TEMPLATE(test_coro_channel_select_type_matching)
};
class coro_channel_select_on_empty
    : public TestClass<coro_channel_select_on_empty>,
      public coro_channel_select_on_empty_test {
    METHOD_INVOKE_TEMPLATE(test_coro_channel_select_on_empty)
};
class coro_channel_select_peek_every
    : public TestClass<coro_channel_select_peek_every>,
      public coro_channel_select_peek_every_test {
    METHOD_INVOKE_TEMPLATE(test_coro_channel_select_peek_every)
};
class coro_channel_no_leak_under_race
    : public TestClass<coro_channel_no_leak_under_race>,
      public coro_channel_no_leak_under_race_test {
    METHOD_INVOKE_TEMPLATE(test_coro_channel_no_leak_under_race)
};
//
//class net_gethostname : public TestClass<net_gethostname>,
//                        public net_gethostname_test {
//    METHOD_INVOKE_TEMPLATE
//};
//class net_getnameinfo_v4 : public TestClass<net_getnameinfo_v4>,
//                           public net_getnameinfo_v4_test {
//    METHOD_INVOKE_TEMPLATE
//};
//class net_getnameinfo_v6 : public TestClass<net_getnameinfo_v6>,
//                           public net_getnameinfo_v6_test {
//    METHOD_INVOKE_TEMPLATE
//};
//class net_getaddrinfo_tcp6_connect
//    : public TestClass<net_getaddrinfo_tcp6_connect>,
//      public net_getaddrinfo_tcp6_connect_test {
//    METHOD_INVOKE_TEMPLATE
//};
//class net_getaddrinfo_tcp6_listen_text
//    : public TestClass<net_getaddrinfo_tcp6_listen_text>,
//      public net_getaddrinfo_tcp6_listen_text_test {
//    METHOD_INVOKE_TEMPLATE
//};
//class net_getaddrinfo_tcp6_listen_numeric
//    : public TestClass<net_getaddrinfo_tcp6_listen_numeric>,
//      public net_getaddrinfo_tcp6_listen_numeric_test {
//    METHOD_INVOKE_TEMPLATE
//};
//class net_getaddrinfo_udp6_bind_unspecified
//    : public TestClass<net_getaddrinfo_udp6_bind_unspecified>,
//      public net_getaddrinfo_udp6_bind_unspecified_test {
//    METHOD_INVOKE_TEMPLATE
//};
//class net_getaddrinfo_udp6_bind_v4mapped
//    : public TestClass<net_getaddrinfo_udp6_bind_v4mapped>,
//      public net_getaddrinfo_udp6_bind_v4mapped_test {
//    METHOD_INVOKE_TEMPLATE
//};
//class net_getaddrinfo_ip6_bind : public TestClass<net_getaddrinfo_ip6_bind>,
//                                 public net_getaddrinfo_ip6_bind_test {
//    METHOD_INVOKE_TEMPLATE
//};
//class net_getaddrinfo_ip6_multicast
//    : public TestClass<net_getaddrinfo_ip6_multicast>,
//      public net_getaddrinfo_ip6_multicast_test {
//    METHOD_INVOKE_TEMPLATE
//};
//class net_echo_udp : public TestClass<net_echo_udp>, public net_echo_udp_test {
//    METHOD_INVOKE_TEMPLATE
//};
//class net_echo_tcp : public TestClass<net_echo_tcp>, public net_echo_tcp_test {
//    METHOD_INVOKE_TEMPLATE
//};
