// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/frame.h>
#include <coroutine/net.h>

#define LIB_PROLOGUE __attribute__((constructor))
#define LIB_EPILOGUE __attribute__((destructor))

using namespace coro;

std::array<char, NI_MAXHOST> hnbuf{};

LIB_PROLOGUE void init_host_name() noexcept
{
    ::gethostname(hnbuf.data(), hnbuf.size());
}
