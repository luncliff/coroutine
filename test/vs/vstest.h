// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#pragma once

#include "../test.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <sdkddkver.h>
#include <threadpoolapiset.h>

#include <CppUnitTest.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

template <typename... Args>
auto println(const char* format, Args&&... args)
{
    std::string reserve{};
    reserve.resize(1024u);

    sprintf_s(reserve.data(), reserve.size(), format,
              std::forward<Args>(args)...);
    Logger::WriteMessage(reserve.c_str());
}
