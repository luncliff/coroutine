// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  License
//      CC BY 4.0
//
// ---------------------------------------------------------------------------
// #include <cassert>
#include <magic/linkable.h>
#include <magic/coroutine.hpp>

#ifdef _WIN32
#define PROCEDURE
#else
#define PROCEDURE __attribute__((constructor))
#endif

namespace magic
{

_INTERFACE_ uint16_t version() noexcept
{
  return (1 << 8) | 0;
}

auto check_coroutine_available() -> magic::unplug
{
  co_await stdex::suspend_never{};
}

auto check_generator_available() -> stdex::generator<uint16_t>
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

  // just check expression.
  for (const auto &v : check_generator_available())
  {
    version_code += v;
  }
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
