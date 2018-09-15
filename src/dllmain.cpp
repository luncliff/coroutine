// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  License
//      CC BY 4.0
//
// ---------------------------------------------------------------------------
#ifdef _WIN32
#define PROCEDURE
#else
#define PROCEDURE __attribute__((constructor))
#endif

#include <memory>
#include <cassert>

#include <magic/coroutine.hpp>
#include <magic/linkable.h>

namespace magic
{

_INTERFACE_ uint16_t version() noexcept { return (1 << 8) | 0; }

auto check_coroutine_available() -> magic::unplug
{
  co_await stdex::suspend_never{};
}

auto check_generator_available() -> std::experimental::generator<uint16_t>
{
  auto value = version();
  co_yield value;
  co_yield value;
  co_return;
}

// this function is reserved for library initialization
// for now, it just checks some coroutine expressions
PROCEDURE void on_load(void *) noexcept
{
  auto version_code = version();

  check_coroutine_available();

  auto g = check_generator_available();
  // just check expression.
  for (const auto &v : g)
  {
    version_code += v;
  }
  assert(version_code == version() * 3);
}
} // namespace magic

#ifdef _WIN32
#include <sdkddkver.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// https://msdn.microsoft.com/en-us/library/windows/desktop/ms682583(v=vs.85).aspx

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID)
{
  try
  {
    if (fdwReason == DLL_PROCESS_ATTACH)
      magic::on_load(hinstDLL);

    return TRUE;
  }
  catch (const std::exception &e)
  {
    ::perror(e.what());
  }
  return FALSE;
}

#endif // _WIN32

// We are using VC++ headers, but the compiler is not msvc.
// Redirect resumable functions support intrinsics
#ifdef __clang__

size_t _coro_resume(void *a)
{
  __builtin_coro_resume(a);
  return true;
}

void _coro_destroy(void *a) { return __builtin_coro_destroy(a); }

size_t _coro_done(void *a)
{
  const bool is_done = __builtin_coro_done(a);
  return static_cast<size_t>(is_done);
}

// size_t _coro_frame_size();
// void *_coro_frame_ptr();
// void _coro_init_block();
// void *_coro_resume_addr();
// void _coro_init_frame(void *);
// void _coro_save(size_t);
// void _coro_suspend(size_t);
// void _coro_cancel();
// void _coro_resume_block();
#endif
