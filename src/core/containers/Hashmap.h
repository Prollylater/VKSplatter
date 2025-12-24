#include <bits/stdc++.h>
//Todo: IMplement rehash
//Todo: Directly pass hash
//Todo: Statuate on the order of declaration

template <typename K, typename V /*, typename Hash = std::hash<K>*/>
class RobinHoodHashMap
{
private:
    static constexpr int MAX_LOAD_PERCENT = 90;
    static constexpr size_t EMPTY_HASH = 0;

public:
    RobinHoodHashMap(void *buffer, size_t cap)
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

 
    size_t size() const { return mNumElements; }
    bool empty() const { return mNumElements == 0; }

    void insert(const K &key, const V &value)
    {
        if (loadFactor() > MAX_LOAD_PERCENT)
        {
            //rehash
        };

        size_t hash = normalize_hash(std::hash<K>{}(key));
        size_t index = hash % capacity;
        size_t dist = 0;

        size_t index = hashToIndex(key);
        size_t dist = 0;
        Entry newEntry{key, value, hash};

        while (true)
        {
            Entry &e = table[index];

            if (e.hash == EMPTY_HASH)
            {
                new (&e) Entry(std::move(newEntry));
                ++mNumElements;
                return;
            }

            // Todo: have a specific insert for this ?
            //  we update existing
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

            index = (index + 1) % table.size();
            ++dist;
        }
    }

    V *find(const K &key)
    {
        size_t hash = normalize_hash(std::hash<K>{}(key));
        size_t index = hashToIndex(hash);
        size_t dist = 0;

        while (table[index].occupied)
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
                return nullptr; // early exit
            }

            index = (index + 1) % capacity;
            dist++;
        }
    }

    bool erase(const K &key)
    {
        size_t hash = normalize_hash(std::hash<K>{}(key));
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
                e.key.~K();
                e.value.~V();

                table[index].occupied = false;
                mNumElements--;

                // backward-shift deletion
                size_t next = (index + 1) % table.size();

                // If next is not already in it's ideal place
                while (table[next].hash != EMPTY_HASH &&
                       probeDistance(hashToIndex(table[next].hash), next) > 0)
                {
                    table[index] = std::move(table[next]);
                    index = next;
                    next = (next + 1) % table.size();
                }

                table[index].hash = EMPTY_HASH;
                --mNumElements;
                return true;
            }

            size_t ideal = hashToIndex(table[index].hash);
            if (probeDistance(ideal, index) < dist)
                return false; // won't be found

            index = (index + 1) % table.size();
            dist++;
        }
    }

private:
    struct Entry
    {
        K key;
        V value;
        size_t hash;
    };

    Entry *table = nullptr;
    size_t capacity = 0;
    size_t mNumElements = 0;

    // Normalize hash so it is never 0
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
        return double(mNumElements) / table.size();
    }

    size_t probeDistance(size_t ideal, size_t current) const
    {
        return (current + capacity - ideal) % capacity;
    }

    class Iterator
    {
        RobinHoodHashMap *map;
        size_t index;

        void skip_empty()
        {
            while (index < map->capacity && &&!map->table[index].hash != EMPTY_HASH)
                index++;
        }

    public:
        Iterator(RobinHoodHashMap *m, size_t i) : map(m), index(i) { skip_empty(); }

        pair<const K &, V &> operator*()
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
    Iterator end() { return Iterator(this, table.size()); }
};
