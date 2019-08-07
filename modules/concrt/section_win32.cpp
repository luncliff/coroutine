//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <concurrency_helper.h>

using namespace std;

static_assert(is_move_assignable_v<section> == false);
static_assert(is_move_constructible_v<section> == false);
static_assert(is_copy_assignable_v<section> == false);
static_assert(is_copy_constructible_v<section> == false);

section::section() noexcept(false) {
    InitializeCriticalSectionAndSpinCount(this, 0600);
}
section::~section() noexcept {
    DeleteCriticalSection(this);
}
bool section::try_lock() noexcept {
    return TryEnterCriticalSection(this);
}
void section::lock() noexcept(false) {
    EnterCriticalSection(this);
}
void section::unlock() noexcept(false) {
    LeaveCriticalSection(this);
}
