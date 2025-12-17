#include "Memory.h"

namespace cico
{

    namespace memory
    {

        StackAllocator::StackAllocator(size_t size)
            : mTotalSize(size), mOffset(0)
        {
            assert(size > 0);
            mMemStack = (uint8_t *)::std::malloc(size);
            if (!mMemStack)
            {
                throw ::std::bad_alloc();
            };
        }

        StackAllocator::~StackAllocator()
        {
            ::std::free(mMemStack);
            mMemStack = nullptr;
        }

        // Allocate aligned memory
        void *StackAllocator::allocate(size_t bytes, size_t alignment)
        {
            // More than one bit set to 1 == not ^2
            assert((alignment & (alignment - 1)) == 0);
            // Also remainder
            // 1010 - 0001 = 1001
            // 1010 & 1001 = 0101
            size_t currentOffset = (size_t)(mMemStack + mOffset);
            size_t misalignment = currentOffset & (alignment - 1);
            size_t padding = misalignment ? (alignment - misalignment) : 0;

            if (mOffset + padding + bytes > mTotalSize)
            {
                return nullptr;
            }

            // Can be interpreted as retroactively padding previous allocation
            mOffset += padding;

            void *ptr = mMemStack + mOffset;
            mOffset += bytes;
            return ptr;
        }

        size_t StackAllocator::getMarker() const
        {
            return mOffset;
        }

        void StackAllocator::freeToMarker(size_t marker)
        {
            assert(marker <= mOffset);
            mOffset = marker;
        }

        void StackAllocator::reset() { mOffset = 0; }

        ArenaAllocator::ArenaAllocator(size_t size)
            : mTotalSize(size)
        {
            assert(size > 0);
            mBasePtr = (uint8_t *)::std::malloc(size);
            if (!mBasePtr)
            {
                throw ::std::bad_alloc();
            };
            mAllocation = mBasePtr;
        }

        ArenaAllocator::~ArenaAllocator()
        {
            std::free(mBasePtr);
            mBasePtr = nullptr;
            mAllocation = nullptr;
        }

        void *ArenaAllocator::allocate(size_t bytes, size_t alignment)
        {
            // More or less the same as Stack Allocator above but aligned calculation streamline it

            assert((alignment & (alignment - 1)) == 0);
            size_t offset = getOffset();
            size_t alignedOffset = (offset + (alignment - 1)) & ~(alignment - 1);

            if (alignedOffset + bytes > mTotalSize)
            {
                return nullptr;
            }

            mAllocation = (uint8_t *)(mBasePtr + alignedOffset + bytes);
            return (void *)(mBasePtr + alignedOffset);
        }

        void ArenaAllocator::reset()
        {
            mAllocation = mBasePtr;
        }

        size_t ArenaAllocator::getOffset() const
        {
            return mAllocation - mBasePtr;
        }

        size_t ArenaAllocator::remaining() const
        {
            return mTotalSize - getOffset();
        }

        void ArenaAllocator::freeToOffset(size_t offset)
        {
            mAllocation = mBasePtr + offset;
        }

        ArenaTemp beginTempArena(ArenaAllocator *arena)
        {
            ArenaTemp temp;
            temp.mArena = arena;
            temp.mTempOffset = arena->getOffset();
            return temp;
        };

        void endTempArena(ArenaTemp &temp)
        {
            temp.mArena->freeToOffset(temp.mTempOffset);
            temp.mArena = nullptr;
        };

        PoolAllocator::PoolAllocator(size_t nChunks, size_t chunkSize, size_t alignment)
            : mChunkSize((chunkSize + alignment - 1) & ~(alignment - 1))
        {
            // Recalculate ChunkSize considering alignment
            // mChunkSize = (chunkSize + alignment - 1) & ~(alignment - 1);
            mTotalSize = nChunks * mChunkSize;
            assert(mTotalSize > 0);
            // mMemPool = (uint8_t *)::std::malloc(mTotalSize);
            mMemPool = (uint8_t *)::std::aligned_alloc(alignment, mTotalSize);

            if (!mMemPool)
            {
                throw ::std::bad_alloc();
            };

            // Free List Initialization
            reset();
        };

        void PoolAllocator::reset()
        {
            // Set all chunks to be free
            head = nullptr;

            size_t chunkN = mTotalSize / mChunkSize;
            // This "reverse the order of previous allocator"
            // Highlighting that this must not be used without the proper functino
            // firward would be like head = node , usr head->next fill it then etc..
            for (size_t i = 0; i < chunkN; i++)
            {
                PoolNode *node = (PoolNode *)(mMemPool + i * mChunkSize);
                node->next = head;
                head = node;
            }
        };

        PoolAllocator::~PoolAllocator()
        {
            ::std::free(mMemPool);
            mMemPool = nullptr;
        };

        // Alignment is already handled
        void *PoolAllocator::allocate()
        {
            if (head == nullptr)
            {
                return nullptr;
            }

            PoolNode *allocation = (PoolNode *)head;
            head = head->next;

            return (void *)allocation;
        }

        void PoolAllocator::deallocate(void *ptr)
        {
            if (ptr == nullptr)
            {
                return;
            }

            uint8_t *start = mMemPool;
            uint8_t *end = start + mTotalSize;

            // Ptr is within the memory bounds
            if (!(start <= ptr && ptr < end))
            {
                //"Memory is out of bounds of the buffer in this pool"
                return;
            }

            PoolNode *allocation = (PoolNode *)(ptr);
            allocation->next = head;
            head = allocation;

            // Notes:
            // ptr = nullptr;
        }

        // FreeListAllocator

        FreeListAllocator::FreeListAllocator(size_t size) : mTotalSize(size)
        {
            assert(mTotalSize > 0);
            // mMemory = (uint8_t *)::std::malloc(mTotalSize);
            mMemory = (uint8_t *)::std::aligned_alloc(alignment, mTotalSize);

            if (!mMemory)
            {
                throw ::std::bad_alloc();
            };

            reset();
        };

        FreeListAllocator::~FreeListAllocator()
        {
            ::std::free(mMemory);
            mMemory = nullptr;
        };

        size_t FreeListAllocator::computeTotalSize(size_t userSize)
        {
            size_t newOffset = sizeof(FreeListNode) + userSize;
            size_t misalignment = newOffset & (alignment - 1);
            size_t padding = misalignment ? (alignment - misalignment) : 0;
            size_t total = newOffset + padding;

            // ensure LSB free but probably useless
            total = (total + 1) & ~size_t(1);
            return total;
        }

        void *FreeListAllocator::allocate(size_t userSize)
        {
            size_t requiredSize = computeTotalSize(userSize);

            uint8_t *cursor = nullptr;

            FreeListNode *h = (this->*mFindBlock)(mMemory, mTotalSize, requiredSize, &cursor);
            // uint8_t *cursor = (uint8_t*) h;

            if (!h)
                return nullptr;

            size_t chunkSize = h->getSize();

            // split if possible & add next Header
            if (chunkSize >= requiredSize + sizeof(FreeListNode))
            {
                FreeListNode *next =
                    (FreeListNode *)(cursor + requiredSize);
                next->mChunkSize = chunkSize - requiredSize;
                next->markFree();
            }

            h->mChunkSize = requiredSize;
            h->markAllocated();

            return cursor + sizeof(FreeListNode);
        }

        void FreeListAllocator::reset()
        {
            mAllocatedSpace = 0;
            FreeListNode *node = (FreeListNode *)(mMemory);
            node->mChunkSize = mTotalSize;
            node->markFree();
            // node->next = nullptr;
        }

        void FreeListAllocator::deallocate(void *ptr)
        {
            if (!ptr)
                return;

            FreeListNode *h = (FreeListNode *)((uint8_t *)ptr - sizeof(FreeListNode));

            h->markFree();
            //Todo: Should coalesce start right from h ?
            //We might miss one element behind however without the explicit design
            coalesce();

            // ptr = nullptr;
        }

        void FreeListAllocator::useFindFirst()
        {

            mFindBlock = &cico::memory::FreeListAllocator::findFirstFit;
        };

        void FreeListAllocator::useBestFit()
        {
            mFindBlock = &cico::memory::FreeListAllocator::findBestFit;
        };

        void FreeListAllocator::coalesce()
        {
            uint8_t *cursor = mMemory;
            uint8_t *end = mMemory + mTotalSize;

            // Notes: This could be wrotten in a prettier way
            while (cursor < end)
            {
                FreeListNode *h = (FreeListNode *)cursor;
                size_t size = h->getSize();
                if (!h->isAllocated())
                {
                    uint8_t *nextAddr = cursor + size;
                    if (nextAddr >= end)
                        break;

                    FreeListNode *next = (FreeListNode *)nextAddr;

                    if (!next->isAllocated())
                    {
                        h->mChunkSize = size + next->getSize();
                        continue;
                    }
                }
                cursor += size;

                // Skip ahead any allocated value
                while (cursor < end && ((FreeListNode *)cursor)->isAllocated())
                {
                    cursor += ((FreeListNode *)cursor)->getSize();
                }
            };
        }

        FreeListAllocator::FreeListNode *FreeListAllocator::findFirstFit(
            uint8_t *memory,
            size_t totalSize,
            size_t requiredSize,
            uint8_t **outCursor)
        {
            uint8_t *cursor = memory;
            uint8_t *end = memory + totalSize;

            while (cursor < end)
            {
                FreeListNode *h = (FreeListNode *)cursor;
                size_t chunkSize = h->getSize();

                if (!h->isAllocated() && chunkSize >= requiredSize)
                {
                    *outCursor = cursor;
                    return h;
                }

                cursor += chunkSize;
            }

            return nullptr;
        };

        // Todo:
        FreeListAllocator::FreeListNode *FreeListAllocator::findBestFit(
            uint8_t *memory,
            size_t totalSize,
            size_t requiredSize,
            uint8_t **outCursor) // I should drop this and return only Header
        {
            uint8_t *cursor = memory;
            uint8_t *end = memory + totalSize;

            uint8_t *bestFitCursor = nullptr;

            // Max Size_T
            size_t bestFitSize = ~size_t(0);

            while (cursor < end)
            {
                FreeListNode *h = (FreeListNode *)cursor;
                size_t chunkSize = h->getSize();

                if (!h->isAllocated() && chunkSize >= requiredSize)
                {
                    if (chunkSize < bestFitSize)
                    {
                        bestFitCursor = cursor;
                        bestFitSize = chunkSize;
                    }
                }

                cursor += chunkSize;
            }
            *outCursor = bestFitCursor;
            return nullptr;
        };
    }
}
