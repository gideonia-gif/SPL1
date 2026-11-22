#include "MemoryBlock.h"

// ---------------- MemoryBlock ----------------

// Allocate `size` integers, all set to 0 (the "()" zero-initialises them).
// A new block is not held by anyone yet, so refCount starts at 0.
MemoryBlock::MemoryBlock(size_t size)
    : size(size), refCount(0), data(new int[size]()) {}

// 1. Destructor: free the array. It is virtual, so deleting a KVBlock or an
//    EmbeddingWorkspace through a MemoryBlock* runs the right destructor.
MemoryBlock::~MemoryBlock() {
    delete[] data;   // new[] must be matched with delete[]
}

// 2. Copy constructor: deep copy - allocate a NEW array and copy the values.
//    (Copying only the pointer would make two blocks delete[] the same array.)
//    A copy is a new block that nobody holds yet, so refCount is 0.
MemoryBlock::MemoryBlock(const MemoryBlock& other)
    : size(other.size), refCount(0), data(new int[other.size]) {
    for (size_t i = 0; i < size; ++i)
        data[i] = other.data[i];
}

// 3. Copy assignment: this block already owns an array, so we must free it.
//    Allocate and copy first, and only then delete the old array, so that if
//    `new` throws, this block is still valid. Also safe for `b = b`.
MemoryBlock& MemoryBlock::operator=(const MemoryBlock& other) {
    if (this != &other) {
        int* newData = new int[other.size];
        for (size_t i = 0; i < other.size; ++i)
            newData[i] = other.data[i];
        delete[] data;
        data = newData;
        size = other.size;
        refCount = 0;   // a copy has reference count 0
    }
    return *this;
}

// 4. Move constructor: steal the array instead of copying it, then leave
//    `other` empty (data == nullptr) so its destructor does nothing harmful.
MemoryBlock::MemoryBlock(MemoryBlock&& other)
    : size(other.size), refCount(other.refCount), data(other.data) {
    other.data = nullptr;
    other.size = 0;
    other.refCount = 0;
}

// 5. Move assignment: free our own array, steal other's, empty `other`.
MemoryBlock& MemoryBlock::operator=(MemoryBlock&& other) {
    if (this != &other) {
        delete[] data;
        data = other.data;
        size = other.size;
        refCount = other.refCount;
        other.data = nullptr;
        other.size = 0;
        other.refCount = 0;
    }
    return *this;
}

int MemoryBlock::getRefCount() const { return refCount; }
std::size_t MemoryBlock::getSize() const { return size; }

int MemoryBlock::readFromBlock(std::size_t offset) const {
    return data[offset];
}

void MemoryBlock::writeToBlock(std::size_t offset, int value) {
    data[offset] = value;
}

void MemoryBlock::acquire() {
    ++refCount;
}

// Decrease the count. When it reaches 0 the block itself decides what the GPU
// should do (onLastRelease). While others still hold it, it simply stays.
IdleAction MemoryBlock::release() {
    --refCount;
    if (refCount == 0)
        return onLastRelease();
    return IdleAction::KeepCached;
}

// ---------------- KVBlock ----------------

KVBlock::KVBlock(size_t size) : MemoryBlock(size) {}

// KV blocks may be reused by later requests with the same prefix: keep them.
IdleAction KVBlock::onLastRelease() const {
    return IdleAction::KeepCached;
}

// clone() = "copy me, keeping my real type". Uses the copy constructor.
MemoryBlock* KVBlock::clone() const {
    return new KVBlock(*this);
}

// ---------------- EmbeddingWorkspace ----------------

EmbeddingWorkspace::EmbeddingWorkspace(size_t size) : MemoryBlock(size) {}

// A workspace is never shared, so it is destroyed as soon as it is released.
IdleAction EmbeddingWorkspace::onLastRelease() const {
    return IdleAction::Destroy;
}

MemoryBlock* EmbeddingWorkspace::clone() const {
    return new EmbeddingWorkspace(*this);
}
