// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  License
//      CC BY 4.0
//
// ---------------------------------------------------------------------------
#include "../modules/memory.hpp"

#include <array>

#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

TEST_CLASS(IndexPoolTest)
{
    index_pool<uint64_t, 10> pool{};

  public:
    TEST_METHOD(Locality)
    {
        auto p1 = pool.allocate();
        auto p2 = pool.allocate();
        auto p3 = pool.allocate();
        auto p4 = pool.allocate();
        pool.deallocate(p2);
        pool.deallocate(p3);

        auto p5 = pool.allocate();
        Assert::IsTrue(p5 == p3);
        auto p6 = pool.allocate();
        Assert::IsTrue(p6 == p2);

        pool.deallocate(p1);
        pool.deallocate(p4);
        pool.deallocate(p5);
        pool.deallocate(p6);
    }

    TEST_METHOD(Capacity)
    {
        using namespace std;

        int i = 0;
        array<void*, 11> set{};

        for (; i < 11; ++i)
            set[i] = pool.allocate();

        for_each(cbegin(set), set.cbegin() + 10, [this](void* ptr) {
            Assert::IsTrue(ptr != nullptr);
            Assert::IsTrue(pool.get_id(ptr) >= 0 &&
                           pool.get_id(ptr) < pool.capacity());
        });
        Assert::IsTrue(set[10] == nullptr);

        for_each(cbegin(set), set.cbegin() + 10, [this](void* ptr) {
            this->pool.deallocate(ptr);
        });
    }
};

class SimpleQueueTest : public TestClass<SimpleQueueTest>
{
    struct circular_queue final
    {
        static constexpr auto capacity = 3;
        static constexpr auto invalid_index =
            std::numeric_limits<uint16_t>::max();

        using value_type = uint64_t;
        using index_type = uint16_t;

        index_type begin = 0, end = 0;
        size_t count = 0;
        std::array<value_type, capacity> storage{};

      private:
        static index_type advance(index_type i) noexcept
        {
            return (i + 1) % capacity;
        }

      public:
        circular_queue() noexcept = default;

      public:
        bool is_full() noexcept { 
            //bool f = (advance(begin) == end);
            //return f;
            return count == storage.max_size();
        }
        bool is_empty() noexcept { 
            return count == 0;
            //bool e = (begin == end);
            //return e;
        }

        bool push(const uint64_t msg)
        {
            if (is_full()) return false;

            auto index = end;
            end = advance(end);
            count += 1;

            storage[index] = msg;
            return true;
        }

        bool pop(uint64_t& msg)
        {
            if (is_empty()) return false;

            auto index = begin;
            begin = advance(begin);
            count -= 1;

            msg = storage[index];
            return true;
        }
    };

    circular_queue q{};

  public:
    TEST_METHOD(EmptyPop)
    {
        Assert::IsTrue(q.is_empty());

        uint64_t m = 0;
        Assert::IsFalse(q.pop(m));
    }
    TEST_METHOD(EmptyPush)
    {
        Assert::IsTrue(q.is_empty());
        Assert::IsTrue(q.push(6));
    }

    TEST_METHOD(NormalPop)
    {
        Assert::IsTrue(q.is_empty());

        Assert::IsTrue(q.push(7));
        Assert::IsTrue(q.push(8));

        uint64_t m = 0;
        Assert::IsTrue(q.pop(m));
        Assert::IsTrue(m == 7);
        Assert::IsTrue(q.pop(m));
        Assert::IsTrue(m == 8);
        Assert::IsTrue(q.is_empty());
        Assert::IsFalse(q.pop(m));
    }

    TEST_METHOD(NormalPush)
    {
        Assert::IsTrue(q.push(9));
        Assert::IsTrue(q.push(10));
        uint64_t m = 0;
        Assert::IsTrue(q.pop(m));
        Assert::IsTrue(m == 9);
        Assert::IsTrue(q.push(11));
        Assert::IsTrue(q.push(12));
    }

    TEST_METHOD(LimitPush)
    {
        Assert::IsTrue(q.push(9));
        Assert::IsTrue(q.push(10));
        Assert::IsTrue(q.push(11));
        Assert::IsTrue(q.is_full());
        Assert::IsFalse(q.push(12));
    }
};