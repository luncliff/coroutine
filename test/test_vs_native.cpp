//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <CppUnitTest.h>
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#include "test_shared.h"

void expect_true(bool cond) {
    Assert::IsTrue(cond);
}
void fail_with_message(string&& msg) {
    print_message(move(msg));
    Assert::Fail();
}
void print_message(string&& msg) {
    Logger::WriteMessage(msg.c_str());
}