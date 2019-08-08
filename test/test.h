//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
//  Note
//      Test framework adapter
//
#pragma once

#include <gsl/gsl>

void _require_(bool expr);
void _require_(bool expr, gsl::czstring<> file, size_t line);
void _println_(gsl::czstring<> message);
void _fail_now_(gsl::czstring<> message, gsl::czstring<> file, size_t line);
