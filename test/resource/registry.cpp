//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
#include "./test.h"
#include <catch.hpp>

#include "../modules/shared/registry.h"

TEST_CASE("RegistryTest", "[resource]")
{
    auto get_some_id = []() -> uint64_t {
        return static_cast<uint32_t>(rand());
    };

    // using registry_t = index_registry<uint64_t>;
    auto registry = create_registry();

    SECTION("Add")
    {
        // once id is registered, returns resouce pointer
        SECTION("Once")
        {
            auto id = get_some_id();

            auto* res = registry->add(id);
            REQUIRE(res != nullptr);
        }

        // same id returns same resource
        SECTION("Same")
        {
            auto id = get_some_id();

            auto* res1 = registry->add(id);
            REQUIRE(res1 != nullptr);
            auto* res2 = registry->add(id);
            REQUIRE(res2 != nullptr);

            REQUIRE(res1 == res2);
        }

        // different id returns same resource
        SECTION("Different")
        {
            auto id1 = get_some_id();
            auto id2 = get_some_id();

            auto* res1 = registry->add(id1);
            REQUIRE(res1 != nullptr);
            auto* res2 = registry->add(id2);
            REQUIRE(res2 != nullptr);

            REQUIRE(res1 != res2);
        }
    }

    SECTION("Find")
    {
        // normal usecase. after registration, given id's resource is returned
        // and it is equal to return of `add`
        SECTION("Registered")
        {
            auto id = get_some_id();

            auto* res1 = registry->add(id);
            REQUIRE(res1 != nullptr);

            auto* res2 = registry->find(id);
            REQUIRE(res2 != nullptr);

            REQUIRE(res1 == res2);
        }

        // if given id is unregistered, returns nullptr
        // user must ensure if the id need resource
        SECTION("Unknown")
        {
            auto id = get_some_id();

            auto* res = registry->find(id);
            REQUIRE_FALSE(res);
        }
    }

    SECTION("Remove")
    {
        // removing registerd id will be always successful
        // if the operation was successful, it won't throw any exception
        SECTION("Registered")
        {
            auto id = get_some_id();

            auto* res = registry->add(id);
            REQUIRE(res);

            REQUIRE_NOTHROW(registry->remove(id));
        }

        // removing unknown id will be always successful
        SECTION("Unknown")
        {
            auto id = get_some_id();

            REQUIRE_NOTHROW(registry->remove(id));
        }
    }
}
