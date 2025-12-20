#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <cstdint>
#include <cstddef>
#include "Memory.h"
using namespace cico::memory;

//Test over allocation
//Test value stored
//Test 
//https://catch2-temp.readthedocs.io/en/latest/other-macros.html
// ------------------------
// StackAllocator Tests
// ------------------------
TEST_CASE("StackAllocator Construction", "[stackAllocator]") {
    StackAllocator alloc(1024);
    REQUIRE(alloc.getMarker() == 0);
}

//SECTION( "resizing bigger changes size and capacity" )
//BENCHMARK
TEST_CASE("StackAllocator Alignment", "[stackAllocator]") {
    StackAllocator alloc(1024);
    for (size_t a : {1,2,4,8,16,32,64}) {
        void* p = alloc.allocate(1, a);
        REQUIRE(reinterpret_cast<uintptr_t>(p) % a == 0);
    }
}

TEST_CASE("StackAllocator Linear Growth", "[stackAllocator]") {
    StackAllocator alloc(1024);
    void* a = alloc.allocate(64);
    void* b = alloc.allocate(64);
    REQUIRE(b > a);
}

TEST_CASE("StackAllocator Marker Restore", "[stackAllocator]") {
    StackAllocator alloc(1024);
    size_t m = alloc.getMarker();
    void* a = alloc.allocate(128);
    alloc.allocate(256);
    alloc.freeToMarker(m);
    void* b = alloc.allocate(128);
    REQUIRE(a == b);
}

TEST_CASE("StackAllocator Overflow", "[stackAllocator]") {
    StackAllocator alloc(128);
    void* p = alloc.allocate(1024);
    REQUIRE(p == nullptr);
}

// ------------------------
// ArenaAllocator Tests
// ------------------------
TEST_CASE("ArenaAllocator Sequential Allocation", "[arenaAllocator]") {
    ArenaAllocator arena(1024);
    void* a = arena.allocate(128);
    void* b = arena.allocate(128);
    REQUIRE(b > a);
}

TEST_CASE("ArenaAllocator Reset", "[arenaAllocator]") {
    ArenaAllocator arena(1024);
    void* a = arena.allocate(256);
    arena.reset();
    void* b = arena.allocate(256);
    REQUIRE(a == b);
}

TEST_CASE("ArenaAllocator Temp Scope Rollback", "[arenaAllocator]") {
    ArenaAllocator arena(1024);
    void* a = arena.allocate(128);
    {
        ArenaTemp tempArena = beginTempArena(&arena);
        void* b = arena.allocate(256);
        REQUIRE(reinterpret_cast<uint8_t*>(b) ==
            reinterpret_cast<uint8_t*>(a) + 128);
        endTempArena(tempArena);
    }
    void* c = arena.allocate(512);
    REQUIRE(reinterpret_cast<uint8_t*>(c) ==
            reinterpret_cast<uint8_t*>(a) + 128);
}

TEST_CASE("ArenaAllocator Overflow", "[arenaAllocator]") {
    ArenaAllocator arena(128);
    void* p = arena.allocate(256);
    REQUIRE(p == nullptr);
}

// ------------------------
// PoolAllocator Tests
// ------------------------
TEST_CASE("PoolAllocator Capacity", "[poolAllocator]") {
    PoolAllocator pool(4, sizeof(int));
    void* p[4];
    for (int i = 0; i < 4; i++)
        p[i] = pool.allocate();
    REQUIRE(pool.allocate() == nullptr);
}

//The tested behavior is not really desirable 
//Todo: Do we null pointer at deallocate ?
TEST_CASE("PoolAllocator Reuse", "[poolAllocator]") {
    PoolAllocator pool(4, sizeof(int));
    void* a = pool.allocate();
    int* aInt = (int*)a;
    *aInt = 5;
    void* b = pool.allocate();
    pool.deallocate(a);
    void* c = pool.allocate();
    REQUIRE(c == a);
    REQUIRE(*(int*)(c) == *(int*)(a));
    SUCCEED("Value is retained");
}

TEST_CASE("PoolAllocator Invalid Free", "[poolAllocator]") {
    PoolAllocator pool(4, sizeof(int));
    int x;
    pool.deallocate(&x);
    SUCCEED("Invalid free does not crash");
}

TEST_CASE("PoolAllocator Alignment", "[poolAllocator]") {
    PoolAllocator pool(8, 24, 16);
    void* p = pool.allocate();
    REQUIRE(reinterpret_cast<uintptr_t>(p) % 16 == 0);
}


// ------------------------
// FreeAllocator Tests
// ------------------------

TEST_CASE("FreeListAllocator basic allocation and deallocation", "[allocator]") {
    constexpr size_t heapSize = 1024;
    FreeListAllocator allocator(heapSize);

    SECTION("Allocate a single block") {
        size_t allocSize = 128;
        void* ptr = allocator.allocate(allocSize);
        REQUIRE(ptr != nullptr);
        REQUIRE(allocator.computeTotalSize(allocSize) <= heapSize);
    }

    SECTION("Allocate multiple blocks and deallocate") {
        void* ptr1 = allocator.allocate(128);
        void* ptr2 = allocator.allocate(256);
        void* ptr3 = allocator.allocate(64);

        REQUIRE(ptr1 != nullptr);
        REQUIRE(ptr2 != nullptr);
        REQUIRE(ptr3 != nullptr);

        allocator.deallocate(ptr2);
        allocator.deallocate(ptr1);
        allocator.deallocate(ptr3);

        // After deallocation, coalesce should combine free blocks
        allocator.coalesce();

        // Allocate a large block that fits all freed space
        void* ptr4 = allocator.allocate(128 + 256 + 64);
        REQUIRE(ptr4 != nullptr);
    }
}


TEST_CASE("FreeListAllocator Overflow", "[allocator]") {
    constexpr size_t heapSize = 512;
    FreeListAllocator allocator(heapSize);

    constexpr size_t headerSize = sizeof(FreeListAllocator::FreeListNode);
    constexpr size_t userBlockSize = 128;
    constexpr size_t totalBlockSize = userBlockSize + headerSize;

    SECTION("Allocate until heap is exactly full") {
        const size_t maxBlocks = heapSize / totalBlockSize;

        std::vector<void*> allocations;
        for (size_t i = 0; i < maxBlocks; ++i) {
            void* ptr = allocator.allocate(userBlockSize);
            REQUIRE(ptr != nullptr);
            allocations.push_back(ptr);
        }

        // Heap should now be full
        void* overflow = allocator.allocate(userBlockSize);
        REQUIRE(overflow == nullptr);
    }
}

TEST_CASE("FreeListAllocator Overflow (Header Ignored Demonstration)", "[allocator]") {
    constexpr size_t heapSize = 512;
    FreeListAllocator allocator(heapSize);

    constexpr size_t userBlockSize = 128;

    SECTION("Allocate until heap is exactly full while ignoring header") {
        // User assumes 512 / 128 == 4 allocations fit
        void* ptr1 = allocator.allocate(userBlockSize);
        void* ptr2 = allocator.allocate(userBlockSize);
        void* ptr3 = allocator.allocate(userBlockSize);
        void* ptr4 = allocator.allocate(userBlockSize);

        REQUIRE(ptr1 != nullptr);
        REQUIRE(ptr2 != nullptr);
        REQUIRE(ptr3 != nullptr);

        // This may fail because headers consume space
        REQUIRE(ptr4 == nullptr);
    }
}

//Todo:
//This should Benchmark a small optimization that skipped block while coalescing
//This might not really be useful
TEST_CASE("FreeListAllocator Coalescing Benchmark", "[allocator][benchmark]") {
    constexpr size_t headerSize = sizeof(FreeListAllocator::FreeListNode);
    constexpr size_t heapSize = 4096;
    constexpr size_t blockSize = 128;
    constexpr size_t numBlocks = heapSize / (blockSize + headerSize);

    auto runBenchmark = [&](const std::vector<size_t>& freeOrder) {
        FreeListAllocator allocator(heapSize);
        std::vector<void*> blocks(numBlocks);
        
        // Allocate all blocks
        for (size_t i = 0; i < numBlocks; ++i) {
            blocks[i] = allocator.allocate(blockSize);

            REQUIRE(blocks[i] != nullptr);
        }

        auto start = std::chrono::high_resolution_clock::now();

        // Free in specified order
        for (size_t index : freeOrder) {
            allocator.deallocate(blocks[index]);
        }

        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    };

    /*

    //N* denote block "coalesced" together
    Initial:
    [U][U][U][U][U][U][U][U]

    Free 1:
    [U][F][U][U][U][U][U][U]

    Free 3:
    [U][F][U][F][U][U][U][U]

    Now free evens:

    Free 0:
    [F*][F*][U][F][U][F][U][F]   

    Free 2:
    [F*][F*][F*][F*][U][F][U][F]   

    */

    BENCHMARK("Interleaved frees (near objects)") {
        std::vector<size_t> freeOrder;
        for (size_t i = 1; i < numBlocks; i += 2){
            freeOrder.push_back(i);}

        for (size_t i = 0; i < numBlocks; i += 2){
            freeOrder.push_back(i);}

        auto time = runBenchmark(freeOrder);
        INFO("Interleaved free time: " << time);
        REQUIRE(time > 0);
    };

    /*
    Initial:
    [U][U][U][U][U][U][U][U]

    Free 4:
    [U][U][U][U][F][U][U][U]

    Free 5:
    [U][U][U][U][F*][F*][U][U]

    Free 6:
    [U][U][U][U][F*][F*][F*][U]


    Now free first half:

    Free 0:
    [F][U][U][U][F*][F*][F*][U]

    Free 1:
    [F**][F**][U][U][F][F][F][F]

    */
    BENCHMARK("Clustered frees (far objects)") {
        std::vector<size_t> freeOrder;

        // Free last blocks first → large contiguous  region
        for (size_t i = numBlocks / 2; i < numBlocks; ++i){
            freeOrder.push_back(i);}

        for (size_t i = 0; i < numBlocks / 2; ++i){
            freeOrder.push_back(i);}

        auto time = runBenchmark(freeOrder);
        INFO("Clustered free time: " << time);
        REQUIRE(time > 0);
    };
};