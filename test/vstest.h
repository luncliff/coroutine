// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#pragma once

#include "./test.h"

#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

template<typename TestCase>
using TestClass = Microsoft::VisualStudio::CppUnitTestFramework::TestClass<TestCase>;

char reserved[1024]{};

template<typename... Args>
auto println(const char* format, Args&&... args)
{
    sprintf_s(reserved, format, std::forward<Args>(args)...);
    Logger::WriteMessage(reserved);
}
