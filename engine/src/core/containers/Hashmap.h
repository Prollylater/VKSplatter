#include <cstddef>
#include <functional>
#include <new>
#include <utility>

// Notes: absl/swiss implementation later on ?
namespace cico
{
    template <typename K, typename V /*, typename Hash = std::hash<K>*/>
    class RBHHashMap
    {

    public:
        RBHHashMap(void *buffer, size_t cap)
            : table(static_cast<Entry *>(buffer)), capacity(cap)
        {
            for (size_t i = 0; i < capacity; ++i)
            {
                table[i].hash = EMPTY_HASH;
            }
        }

        void clear()
        {
            for (size_t i = 0; i < capacity; ++i)
            {
                if (table[i].hash != EMPTY_HASH)
                {
                    table[i].key.~K();
                    table[i].value.~V();
                    table[i].hash = EMPTY_HASH;
                }
            }
            mNumElements = 0;
        }

        // This don't actually free the old table
        void rehash(void *newBuffer, size_t newCap)
        {
            Entry *oldTable = table;
            size_t oldCapacity = capacity;

            table = static_cast<Entry *>(newBuffer);
            capacity = newCap;
            mNumElements = 0;

            for (size_t i = 0; i < capacity; ++i)
            {
                table[i].hash = EMPTY_HASH;
            }

            // Reinsert from old table
            for (size_t i = 0; i < oldCapacity; ++i)
            {
                if (oldTable[i].hash != EMPTY_HASH)
                {
                    insert(
                        oldTable[i].hash,
                        oldTable[i].key,
                        oldTable[i].value);

                    oldTable[i].key.~K();
                    oldTable[i].value.~V();
                }
            }
        }

        size_t size() const { return mNumElements; }
        bool empty() const { return mNumElements == 0; }

        //Todo: Use default value
        bool try_insert(const K &key, const V &value)
        {
            if (loadFactor() > MAX_LOAD_PERCENT)
            {
                return false;
            }
            insert(key, value);
            return true;
        }

        bool try_insert(size_t _hash, const K &key, const V &value)
        {
            if (loadFactor() > MAX_LOAD_PERCENT)
            {
                return false;
            }
            insert(_hash, key, value);
            return true;
        }

        void insert(const K &key, const V &value)
        {
            size_t hash = std::hash<K>{}(key);
            return insert(hash, key, value);
        }

        void insert(size_t _hash, const K &key, const V &value)
        {
            // if (loadFactor() > MAX_LOAD_PERCENT)
            //{
            //     rehash() External rehash strategy
            // };

            size_t hash = normalize_hash(_hash);
            size_t index = hashToIndex(hash);
            size_t dist = 0;

            Entry newEntry{key, value, hash};

            while (true)
            {
                Entry &e = table[index];

                if (e.hash == EMPTY_HASH)
                {
                    new (&e) Entry(newEntry);
                    ++mNumElements;
                    return;
                }

                // Todo: have a specific insert to handle this case like []
                if (e.hash == hash && e.key == key)
                {
                    e.value = value;
                    return;
                }

                size_t existingIdeal = hashToIndex(e.hash);
                size_t existingDist = probeDistance(existingIdeal, index);

                if (existingDist < dist)
                {
                    std::swap(newEntry, e);
                    dist = existingDist;
                }

                index = (index + 1) % capacity;
                ++dist;
            }
        }

        V *find(const K &key)
        {
            size_t hash = std::hash<K>{}(key);
            return find(hash, key);
        }

        V *find(size_t _hash, const K &key)
        {
            size_t hash = normalize_hash(_hash);
            size_t index = hashToIndex(hash);
            size_t dist = 0;

            while (true)
            {
                Entry &e = table[index];
                if (e.hash == EMPTY_HASH)
                {
                    return nullptr;
                }

                if (e.hash == hash && e.key == key)
                {
                    return &e.value;
                }

                size_t ideal = hashToIndex(e.hash);
                if (probeDistance(ideal, index) < dist)
                {
                    return nullptr;  
                }

                index = (index + 1) % capacity;
                dist++;
            }
        }

        bool erase(const K &key)
        {
            size_t hash = std::hash<K>{}(key);
            return erase(hash, key);
        }

        bool erase(size_t _hash, const K &key)
        {
            size_t hash = normalize_hash(_hash);
            size_t index = hashToIndex(hash);
            size_t dist = 0;

            while (true)
            {
                Entry &e = table[index];
                if (e.hash == EMPTY_HASH)
                {
                    return false;
                }

                if (e.hash == hash && e.key == key)
                {
                    // In case ressources need to be released
                    e.key.~K();
                    e.value.~V();

                    // backward-shift deletion
                    size_t next = (index + 1) % capacity;

                    // If next is not already in it's ideal place we move it backward
                    while (table[next].hash != EMPTY_HASH &&
                           probeDistance(hashToIndex(table[next].hash), next) > 0)
                    {
                        table[index] = std::move(table[next]);
                        index = next;
                        next = (next + 1) % capacity;
                    }

                    table[index].hash = EMPTY_HASH;
                    --mNumElements;
                    return true;
                }

                size_t ideal = hashToIndex(table[index].hash);
                if (probeDistance(ideal, index) < dist)
                {
                    return false; // won't be found
                }

                index = (index + 1) % capacity;
                dist++;
            }
        }

        struct Entry
        {
            K key;
            V value;
            size_t hash;
        };
    private:
        

        static constexpr int MAX_LOAD_PERCENT = 90;
        static constexpr size_t EMPTY_HASH = 0;
        size_t capacity = 0;
        size_t mNumElements = 0;
        Entry *table = nullptr;

        // Normalize hash so it is never 0 aka EMPTY_HASH, (test overflow ?)
        static size_t normalize_hash(size_t hash)
        {
            return hash == EMPTY_HASH ? hash + 1 : hash;
        }

        size_t hashToIndex(size_t hash) const
        {
            return hash % capacity;
        }

        double loadFactor() const
        {
            return double(mNumElements) / capacity;
        }

        size_t probeDistance(size_t ideal, size_t current) const
        {
            return (current + capacity - ideal) % capacity;
        }

        class Iterator
        {
            RBHHashMap *map;
            size_t index;

            void skip_empty()
            {
                while (index < map->capacity  &&!map->table[index].hash != EMPTY_HASH)
                    index++;
            }

        public:
            Iterator(RBHHashMap *m, size_t i) : map(m), index(i) { skip_empty(); }

            std::pair<const K &, V &> operator*()
            {
                return {map->table[index].key, map->table[index].value};
            }

            Iterator &operator++()
            {
                index++;
                skip_empty();
                return *this;
            }

            bool operator==(const Iterator &other) const
            {
                return index == other.index;
            }

            bool operator!=(const Iterator &other) const { return index != other.index; }
        };

    public:
        Iterator begin() { return Iterator(this, 0); }
        Iterator end() { return Iterator(this, capacity); }
    };
} // namespace cico
