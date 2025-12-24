#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <string>
#include "Hashmap.h"
#include "Memory.h"

using namespace cico;
using namespace cico::memory;
//Todo: More verbosity 
// ------------------------
// RBHHashMap Tests
// ------------------------
TEST_CASE("RBHHashMap basic insert and find", "[rbhmap]")
{
    ArenaAllocator arena(1024 * 16);
    constexpr size_t capacity = 32;
    void *buffer = arena.allocate(sizeof(RBHHashMap<int, int>::Entry) * capacity,
                                  alignof(RBHHashMap<int, int>::Entry));
    RBHHashMap<int, int> map(buffer, capacity);

    REQUIRE(map.empty());

    INFO("Inserting some key-value pairs");
    REQUIRE(map.try_insert(1, 10));
    REQUIRE(map.try_insert(2, 20));
    REQUIRE(map.try_insert(3, 30));

    CAPTURE(map.size());
    REQUIRE(map.size() == 3);

    INFO("Finding value for key 2 : 20");
    int *v = map.find(2);
    CAPTURE(*v);
    REQUIRE(v != nullptr);
    REQUIRE(*v == 20);

    REQUIRE(map.find(42) == nullptr);
}

TEST_CASE("RBHHashMap handles collisions", "[rbhmap]")
{
    ArenaAllocator arena(1024 * 16);

    constexpr size_t capacity = 8;
    void *buffer = arena.allocate(sizeof(RBHHashMap<int, int>::Entry) * capacity,
                                  alignof(RBHHashMap<int, int>::Entry));

    RBHHashMap<int, int> map(buffer, capacity);

    // Force collisions by using same hash
    REQUIRE(map.try_insert(1, 100));
    REQUIRE(map.try_insert(1 + capacity, 200));
    REQUIRE(map.try_insert(1 + 2 * capacity, 300));

    REQUIRE(map.size() == 3);

    INFO("Checking values of colliding keys");
    CAPTURE(*map.find(1));
    REQUIRE(*map.find(1) == 100);
    CAPTURE(*map.find(1 + capacity));
    REQUIRE(*map.find(1 + capacity) == 200);
    CAPTURE(*map.find(1 + 2 * capacity));
    REQUIRE(*map.find(1 + 2 * capacity) == 300);
}

TEST_CASE("RBHHashMap external rehash", "[rbhmap]")
{
    ArenaAllocator arena(1024 * 32);

    constexpr size_t cap1 = 8;
    constexpr size_t cap2 = 32;

    void *buf1 = arena.allocate(sizeof(RBHHashMap<int, int>::Entry) * cap1,
                                alignof(RBHHashMap<int, int>::Entry));

    RBHHashMap<int, int> map(buf1, cap1);

    INFO("Inserting values before rehash");
    for (int i = 0; i < 6; ++i)
    {
        REQUIRE(map.try_insert(i, i * 10));
    }

    REQUIRE(map.size() == 6);

    void *buf2 = arena.allocate(sizeof(RBHHashMap<int, int>::Entry) * cap2,
                                alignof(RBHHashMap<int, int>::Entry));

    map.rehash(buf2, cap2);

    REQUIRE(map.size() == 6);

    INFO("Checking all values after rehash");
    for (int i = 0; i < 6; ++i)
    {
        int *v = map.find(i);
        REQUIRE(v != nullptr);
        REQUIRE(*v == i * 10);
    }
}

TEST_CASE("RBHHashMap erase", "[rbhmap]")
{
    ArenaAllocator arena(1024 * 16);

    constexpr size_t capacity = 16;
    void *buffer = arena.allocate(sizeof(RBHHashMap<int, int>::Entry) * capacity,
                                  alignof(RBHHashMap<int, int>::Entry));

    RBHHashMap<int, int> map(buffer, capacity);

    for (int i = 0; i < 10; ++i)
    {
        REQUIRE(map.try_insert(i, i));
    }
    REQUIRE(map.size() == 10);

    REQUIRE(map.erase(5));
    REQUIRE(map.find(5) == nullptr);

    REQUIRE(map.size() == 9);

    for (int i = 0; i < 10; ++i)
    {
        if (i == 5)
            continue;
        REQUIRE(map.find(i) != nullptr);
    }
}
