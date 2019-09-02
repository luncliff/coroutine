//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "test.h"

using namespace std;
using namespace std::literals;

#if defined(CMAKE_TEST) // for ctest

void _require_(bool expr) {
    if (expr)
        return;
    ::exit(EXIT_FAILURE);
}
void _require_(bool expr, gsl::czstring<> file, size_t line) {
    if (expr)
        return;
    ::printf("%s %zu\n", file, line);
    ::exit(static_cast<int>(line));
}
void _println_(gsl::czstring<> message) {
    ::printf("%s\n", message);
}
void _fail_now_(gsl::czstring<> message, gsl::czstring<> file, size_t line) {
    _println_(message);
    _require_(false, file, line);
}

#elif __has_include(<CppUnitTest.h>) // for visual studio test
#include <CppUnitTest.h>
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

void _require_(bool expr) {
    Assert::IsTrue(expr);
}

void _require_(bool expr, gsl::czstring<> file, size_t line) {
    if (expr == false) {
        char buffer[500]{};
        sprintf_s(buffer, "%s %zu", file, line);
        Logger::WriteMessage(buffer);
    }
    Assert::IsTrue(expr);
}

void _println_(gsl::czstring<> message) {
    Logger::WriteMessage(message);
}
void _fail_now_(gsl::czstring<> message, gsl::czstring<>, size_t) {
    _println_(message);
    Assert::Fail();
}

#endif
