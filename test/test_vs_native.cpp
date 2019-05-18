//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include <CppUnitTest.h>
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

void expect_true(bool cond) {
    Assert::IsTrue(cond);
}

void fail_with_message(std::string&& msg) {
    Logger::WriteMessage(msg.c_str());
    Assert::Fail();
}
