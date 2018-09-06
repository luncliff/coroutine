
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

TEST_CLASS(MemoryTest)
{
    magic::index_pool<uint64_t, 10> pool{};

  public:
    TEST_METHOD(LocalityTest)
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

    TEST_METHOD(CapacityTest)
    {
        using namespace std;

        int i = 0;
        array<void *, 11> set{};

        for (; i < 11; ++i)
            set[i] = pool.allocate();

        for_each(cbegin(set), set.cbegin() + 10, [this](void *ptr) {
            Assert::IsTrue(ptr != nullptr);
            Assert::IsTrue(pool.get_id(ptr) >= 0 &&
                           pool.get_id(ptr) < pool.capacity());
        });
        Assert::IsTrue(set[10] == nullptr);

        for_each(cbegin(set), set.cbegin() + 10,
                 [this](void *ptr) { this->pool.deallocate(ptr); });
    }
};
