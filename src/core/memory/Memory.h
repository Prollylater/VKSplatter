#pragma once

#include <memory>
#include <iostream>
#include <cstddef>
#include <cassert>

// Bucket List:
// Thread safetey for when proper multi threading is introduced (locking strategies )
// Memory Fragmentation handling
// Make sure no double free may happen, deletion and so on
// Only allocator should touch the pointer directly
// Add logging here ?

namespace cico
{
    // *Most Allocator here wont initialize anything at allocate no memset
    // *Most have been made using slightly different pattern
    // *The class are absolutely not fool proof yet

    // Notes: uintptr_t and where would ibetter of use considering Pointer Arithmetic vs Raw Address Arithmetic

    // Notes: Do i want toreally limit it to power of 2  alignment ?
    // Notes: Does it actually make sense for alignement to be different at each function ?
    // An allocator should only have one alignment
    namespace memory
    {
        enum class MemoryTag
        {

        };
        // This is more a Linear/Arena + marker than a good Stack Allocator since we don't have real header
        class StackAllocator
        {
        public:
            explicit StackAllocator(size_t size);

            StackAllocator(const StackAllocator &) = delete;
            StackAllocator &operator=(const StackAllocator &) = delete;

            ~StackAllocator();
            // Allocate aligned memory
            void *allocate(size_t bytes, size_t alignment = alignof(max_align_t));

            template <typename T>
            T *allocate(size_t count = 1)
            {
                return (T *)allocate(sizeof(T) * count, alignof(T));
            }

            size_t getMarker() const;
            void freeToMarker(size_t marker);

            void reset();

        private:
            uint8_t *mMemStack = nullptr;
            size_t mTotalSize;
            size_t mOffset;
        };

        struct ArenaTemp;

        class ArenaAllocator
        {
        public:
            friend struct ArenaTemp;
            ArenaAllocator(size_t size);

            ArenaAllocator(const ArenaAllocator &) = delete;
            ArenaAllocator &operator=(const ArenaAllocator &) = delete;

            ~ArenaAllocator();

            void *allocate(size_t bytes, size_t alignment = alignof(max_align_t));

            template <typename T>
            T *allocate(size_t count = 1)
            {
                return (T *)allocate(sizeof(T) * count, alignof(T));
            }

            void reset();
            size_t getOffset() const;
            size_t remaining() const;
            void freeToOffset(size_t offset);

        private:
            // This pattern force pointer arithmetic but remove storage need of size_t offset
            uint8_t *mBasePtr = nullptr;
            uint8_t *mAllocation = nullptr;
            size_t mTotalSize = 0;
        };

        struct ArenaTemp
        {
            ArenaAllocator *mArena = nullptr;
            size_t mTempOffset;

            /*
            ArenaTemp() = default;

            ArenaTemp(const ArenaTemp&) = delete;
            ArenaTemp& operator=(const ArenaTemp&) = delete;

            ~ArenaTemp()
            {
                if (mArena != nullptr)
                {
                    mArena->freeToOffset(mTempOffset);
                };
            }*/
        };

        ArenaTemp beginTempArena(ArenaAllocator *arena);

        // This might lead to problematic situation
        // Hence the additionnal destructor
        // Might need to also make it RAII
        // Or delete constructor
        void endTempArena(ArenaTemp &temp);

        class PoolAllocator
        {
        public:
            struct PoolNode
            {
                PoolNode *next;
            };

            PoolAllocator(size_t nChunks, size_t chunkSize, size_t alignment = alignof(std::max_align_t));
            void reset();

            ~PoolAllocator();

            // Alignment is already handled
            void *allocate();
            void deallocate(void *ptr);

        private:
            uint8_t *mMemPool = nullptr;
            size_t mTotalSize;
            size_t mChunkSize;
            PoolNode *head;
        };

        // Todo: Order of function should change
        // Todo: Explicit free list design
        // Todo: Implement Realloc
        // Discuss: Coalesce through moving memory ? Coalesce without spatial locality ?

        //Set size as  a function ?
        class FreeListAllocator
        {
        public:
            static constexpr size_t kFreeAllocBit = 1;

            struct FreeListNode
            {
                size_t mChunkSize = 0; // total size, LSB = allocated flag

                size_t getSize()
                {
                    return mChunkSize & ~kFreeAllocBit;
                }

                bool isAllocated()
                {
                    return mChunkSize & kFreeAllocBit;
                }

                void markAllocated()
                {
                    mChunkSize |= kFreeAllocBit;
                }

                void markFree()
                {
                    mChunkSize &= ~kFreeAllocBit;
                }
            };

            //static constexpr size_t alignment = alignof(std::max_align_t);
            //With padding > to below, we overpad
            static constexpr size_t alignment = alignof(FreeListNode);

            FreeListAllocator(size_t size);
            ~FreeListAllocator();

            void reset();

            // Alignment is already handled
            size_t computeTotalSize(size_t userSize);
            void *allocate(size_t userSize);
            void deallocate(void *ptr);
            void coalesce();

            using FindBlockFn = FreeListNode *(FreeListAllocator::*)(uint8_t *memory,
                                                                     size_t totalSize,
                                                                     size_t requiredSize,
                                                                     uint8_t **outCursor);

            FreeListNode *findFirstFit(
                uint8_t *memory,
                size_t totalSize,
                size_t requiredSize,
                uint8_t **outCursor);

            FreeListNode *findBestFit(
                uint8_t *memory,
                size_t totalSize,
                size_t requiredSize,
                uint8_t **outCursor);

            void useFindFirst();
            void useBestFit();

        private:
            uint8_t *mMemory = nullptr;
            size_t mAllocatedSpace = 0;
            size_t mTotalSize;
            FindBlockFn mFindBlock = &cico::memory::FreeListAllocator::findFirstFit; ;
        };
    }
}
