// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <thread/types.h>

namespace hidden
{
thread_registry tr{};
}

using namespace std;

auto get_thread_registry() noexcept -> thread_registry*
{
    return addressof(hidden::tr);
}

thread_registry::thread_registry() noexcept(false) : mtx{}, keys{}, values{}
{
    for (auto& k : keys)
        k = invalid_key;
    for (auto& v : values)
        v = nullptr;
}
