// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
// ---------------------------------------------------------------------------
#include <coroutine/enumerable.hpp>
#include <coroutine/return.h>
#include <coroutine/sync.h>

#include <Windows.h>
#include <sdkddkver.h>

#include <CppUnitTest.h>
#include <TlHelp32.h>
#include <threadpoolapiset.h>

using namespace std::literals;
using namespace std::experimental;

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using Microsoft::VisualStudio::CppUnitTestFramework::TestClass;

auto current_threads() noexcept(false) -> enumerable<thread_id_t>;

class messaging_test : public TestClass<messaging_test>
{
  public:
    TEST_METHOD(send_message_to_itself)
    {
        message_t msg{};
        msg.u64 = 0xE81F;
        Assert::IsTrue(post_message(current_thread_id(), msg));
        msg.u64 = 0xE820;
        Assert::IsTrue(post_message(current_thread_id(), msg));

        Assert::IsTrue(peek_message(msg)); // receive in order
        Assert::IsTrue(msg.u64 == 0xE81F);
        Assert::IsTrue(peek_message(msg));
        Assert::IsTrue(msg.u64 == 0xE820);
    }

    TEST_METHOD(send_message_to_unknown_throws)
    {
        // since there is no thread with id 0,
        // this will be the id of unknown
        thread_id_t tid{};

        try
        {
            message_t msg{};
            Assert::IsFalse(post_message(tid, msg));
        }
        catch (const std::exception&)
        {
            // ok. send to unknown throwed exception
            return;
        }
        Assert::Fail(L"Expect an exception but nothing catched");
    }

    TEST_METHOD(send_message_to_peer_threads)
    {
        try
        {
            message_t msg{};
            for (auto tid : current_threads())
                if (tid != current_thread_id())
                {
                    msg.u64 = 59372;
                    Assert::IsTrue(post_message(tid, msg));
                }

            Assert::IsTrue(msg != message_t{});
        }
        catch (const std::exception& ex)
        {
            const char* zs = ex.what();
            std::wstring msg{zs, zs + strlen(zs)};

            Assert::Fail(msg.c_str());
            return;
        }
    }
};

auto current_threads() noexcept(false) -> enumerable<thread_id_t>
{
    DWORD pid{};
    thread_id_t tid{};
    HANDLE snapshot{};
    THREADENTRY32 entry{};

    pid = GetCurrentProcessId();

    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        throw std::system_error{static_cast<int>(GetLastError()),
                                std::system_category(),
                                "CreateToolhelp32Snapshot"};

    entry.dwSize = sizeof(entry);
    for (Thread32First(snapshot, &entry); Thread32Next(snapshot, &entry);
         entry.dwSize = sizeof(entry))
        // ignore other process's threads
        if (entry.th32OwnerProcessID == pid)
            co_yield tid = static_cast<thread_id_t>(entry.th32ThreadID);

    CloseHandle(snapshot);
}