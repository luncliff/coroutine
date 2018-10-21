// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  License
//      CC BY 4.0
//
// ---------------------------------------------------------------------------

// We are using VC++ headers, but the compiler is not msvc.
// Redirect some intrinsics from msvc to clang
#ifndef __clang__
#error "This file must be compiled with clang 6.0 or later";
#else

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
