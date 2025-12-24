#include <vector>
#include <stdexcept>
#include <cstddef>

//Todo:Rework and implement with memory allocator
template <typename T>
class CircularVector {
public:
    CircularVector(size_t size) 
        : vec(size), internalIndex(0) {}

    void resize(size_t newSize) {
        vec.resize(newSize);
        if (internalIndex >= vec.size()){ internalIndex = 0};
    }

    size_t size() const { return vec.size(); }

    T& get(size_t index) {
        if (index >= vec.size()){ throw std::out_of_range("Index out of range")};
        return vec[index];
    }

    const T& get(size_t index) const {
        if (index >= vec.size()){ throw std::out_of_range("Index out of range")};
        return vec[index];
    }

    T& next() {
        if (vec.empty()) throw std::out_of_range("Vector is empty");
        T& elem = vec[internalIndex];
        internalIndex++;
        if (internalIndex >= vec.size()){ internalIndex = 0};
        return elem;
    }

    void reset() { internalIndex = 0; }

private:
    std::vector<T> vec;
    size_t internalIndex;
};
