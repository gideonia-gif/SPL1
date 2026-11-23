#include "GPU.h"
#include "MemoryBlock.h"
#include "Request.h"
#include <stdexcept>

using namespace std;

// ---------------- construction & Rule of 5 ----------------

GPU::GPU(size_t blocksNum, size_t blockSize)
    : blocksNum(blocksNum), blockSize(blockSize), memory(blocksNum, nullptr),
      slotCached(blocksNum, false), slotCacheKey(blocksNum), cacheIndex(),
      reservations(), totalReserved(0), cacheHits(0), evictions(0) {}

// Destructor: the GPU owns its blocks, so it deletes them.
GPU::~GPU() {
    clear();
}

void GPU::clear() {
    for (size_t i = 0; i < memory.size(); ++i) {
        delete memory[i];        // virtual destructor of MemoryBlock
        memory[i] = nullptr;
    }
}

// Copy constructor: deep copy. Every block is cloned (keeping its real type),
// and since a clone starts with refCount 0, the reference count is restored.
// All the other fields (cache keys, reservations, counters) are plain values.
GPU::GPU(const GPU& other)
    : blocksNum(other.blocksNum), blockSize(other.blockSize), memory(other.memory.size(), nullptr),
      slotCached(other.slotCached), slotCacheKey(other.slotCacheKey), cacheIndex(other.cacheIndex),
      reservations(other.reservations), totalReserved(other.totalReserved),
      cacheHits(other.cacheHits), evictions(other.evictions) {
    for (size_t i = 0; i < other.memory.size(); ++i) {
        if (other.memory[i] != nullptr) {
            memory[i] = other.memory[i]->clone();
            for (int r = 0; r < other.memory[i]->getRefCount(); ++r)
                memory[i]->acquire();
        }
    }
}

// Copy assignment: copy-and-swap. Build a full copy first (if that throws,
// *this is untouched), then swap it in; the temporary deletes our old blocks.
GPU& GPU::operator=(const GPU& other) {
    if (this != &other) {
        GPU tmp(other);
        *this = std::move(tmp);
    }
    return *this;
}

// Move constructor: take the vector of pointers (no block is copied).
// std::move on a vector leaves `other.memory` empty, so other's destructor
// deletes nothing. noexcept lets std::vector<GPU> move GPUs when it grows.
GPU::GPU(GPU&& other) noexcept
    : blocksNum(other.blocksNum), blockSize(other.blockSize), memory(std::move(other.memory)),
      slotCached(std::move(other.slotCached)), slotCacheKey(std::move(other.slotCacheKey)),
      cacheIndex(std::move(other.cacheIndex)), reservations(std::move(other.reservations)),
      totalReserved(other.totalReserved), cacheHits(other.cacheHits), evictions(other.evictions) {
    other.memory.clear();
    other.blocksNum = 0;
    other.totalReserved = 0;
    other.cacheHits = 0;
    other.evictions = 0;
}

// Move assignment: free our blocks, then take other's.
GPU& GPU::operator=(GPU&& other) noexcept {
    if (this != &other) {
        clear();
        blocksNum = other.blocksNum;
        blockSize = other.blockSize;
        memory = std::move(other.memory);
        slotCached = std::move(other.slotCached);
        slotCacheKey = std::move(other.slotCacheKey);
        cacheIndex = std::move(other.cacheIndex);
        reservations = std::move(other.reservations);
        totalReserved = other.totalReserved;
        cacheHits = other.cacheHits;
        evictions = other.evictions;
        other.memory.clear();
        other.blocksNum = 0;
        other.totalReserved = 0;
        other.cacheHits = 0;
        other.evictions = 0;
    }
    return *this;
}

// ---------------- capacity & reservations ----------------

// Free slots = all slots - blocks in use - blocks promised to admitted requests.
// Idle blocks (refCount 0) count as free: they can be evicted.
size_t GPU::availableSlots() const {
    size_t active = 0;
    for (size_t i = 0; i < memory.size(); ++i)
        if (memory[i] != nullptr && memory[i]->getRefCount() > 0)
            ++active;
    size_t used = active + totalReserved;
    return used >= blocksNum ? 0 : blocksNum - used;
}

// Number of blocks for `memory` integers, rounded up.
size_t GPU::getBlocksForMemory(size_t memory) const {
    return (memory + blockSize - 1) / blockSize;
}

void GPU::reserve(size_t n, const Request& request) {
    reservations[request.getId()] += n;
    totalReserved += n;
}

void GPU::unreserve(size_t n, const Request& request) {
    map<int, size_t>::iterator it = reservations.find(request.getId());
    if (it == reservations.end() || it->second < n)
        throw logic_error("unreserve: more blocks than reserved");
    it->second -= n;
    totalReserved -= n;
    if (it->second == 0)
        reservations.erase(it);
}

// ---------------- blocks ----------------

// Lowest empty slot; if none, evict the idle block with the lowest index.
// Slots marked in protectedSlots are never evicted.
size_t GPU::findFreeSlot(const vector<bool>& protectedSlots) {
    for (size_t i = 0; i < memory.size(); ++i)
        if (memory[i] == nullptr)
            return i;
    for (size_t i = 0; i < memory.size(); ++i) {
        if (isIdle(i) && !protectedSlots[i]) {
            deleteBlock(i);
            ++evictions;
            return i;
        }
    }
    throw logic_error("GPU has no empty or idle slot (admission bug)");
}

size_t GPU::allocateBlock(const Request& request) {
    map<int, size_t>::iterator it = reservations.find(request.getId());
    if (it == reservations.end() || it->second == 0)
        throw logic_error("allocateBlock: request has no reserved block");
    size_t slot = findFreeSlot(vector<bool>(memory.size(), false));
    request.allocateOneBlock(*this, slot);   // double dispatch -> addMemoryBlock
    memory[slot]->acquire();                 // the request holds it right away
    unreserve(1, request);                   // one reserved block is now used
    return slot;
}

// Called back by GenerativeRequest::allocateOneBlock
void GPU::addMemoryBlock(const GenerativeRequest&, size_t slot) {
    memory[slot] = new KVBlock(blockSize);
}

// Called back by EmbeddingRequest::allocateOneBlock
void GPU::addMemoryBlock(const EmbeddingRequest&, size_t slot) {
    memory[slot] = new EmbeddingWorkspace(blockSize);
}

void GPU::acquire(size_t slot) {
    memory[slot]->acquire();
}

// When the last holder releases a block, the block decides (polymorphism):
// KVBlock -> KeepCached (stay as an idle block), EmbeddingWorkspace -> Destroy.
// A block without a prefix key can never be reused, so it is deleted too.
void GPU::release(size_t slot) {
    IdleAction action = memory[slot]->release();
    if (memory[slot]->getRefCount() == 0) {
        if (action == IdleAction::Destroy || !slotCached[slot])
            deleteBlock(slot);               // not an eviction
    }
}

void GPU::deleteBlock(size_t slot) {
    if (slotCached[slot]) {
        cacheIndex.erase(slotCacheKey[slot]);
        slotCached[slot] = false;
        slotCacheKey[slot].clear();
    }
    delete memory[slot];
    memory[slot] = nullptr;
}

int GPU::readFromBlock(size_t blockId, size_t offset) const {
    return memory[blockId]->readFromBlock(offset);
}

void GPU::writeToBlock(size_t blockId, size_t offset, int value) {
    memory[blockId]->writeToBlock(offset, value);
}

// ---------------- prefix cache ----------------

// Key = model id + the first `count` tokens. '\n' and ' ' never appear inside
// a model id or a token, so different (model, tokens) give different keys.
string GPU::makeKey(const string& modelId, const vector<string>& tokens, size_t count) {
    string key = modelId + "\n";
    for (size_t i = 0; i < count; ++i) {
        if (i > 0) key += " ";
        key += tokens[i];
    }
    return key;
}

void GPU::assignPrefixToBlock(size_t block, const vector<string>& prefixTokens, const string& modelId) {
    string key = makeKey(modelId, prefixTokens, prefixTokens.size());
    if (cacheIndex.count(key) > 0)
        return;          // another block already has this key: this one is not cached
    slotCached[block] = true;
    slotCacheKey[block] = key;
    cacheIndex[key] = block;
}

// t = tokens per block. Look up the blocks keyed by the first t, 2t, 3t ...
// prompt tokens, and stop at the first one that is missing.
PrefixMatch GPU::findLongestCachedPrefix(const string& modelId, size_t embeddingSize,
                                         const vector<string>& promptTokens) const {
    PrefixMatch match{0, vector<size_t>()};
    size_t t = blockSize / embeddingSize;
    for (size_t len = t; len <= promptTokens.size(); len += t) {
        map<string, size_t>::const_iterator it = cacheIndex.find(makeKey(modelId, promptTokens, len));
        if (it == cacheIndex.end())
            break;
        match.blockIds.push_back(it->second);
        match.matchedTokens = len;
    }
    return match;
}

// Copy the cached blocks `blocks` of `other` into this GPU, with their keys.
// Blocks whose key we already have are skipped. Copies are idle (refCount 0),
// and making room never evicts a block of the chain being copied.
void GPU::copyMemoryFromOtherGPU(const GPU& other, const vector<size_t>& blocks) {
    vector<bool> chain(memory.size(), false);
    for (size_t i = 0; i < blocks.size(); ++i) {
        const string& key = other.slotCacheKey[blocks[i]];
        map<string, size_t>::iterator existing = cacheIndex.find(key);
        if (existing != cacheIndex.end()) {
            chain[existing->second] = true;   // already here: protect it
            continue;
        }
        size_t slot = findFreeSlot(chain);
        memory[slot] = other.memory[blocks[i]]->clone();
        slotCached[slot] = true;
        slotCacheKey[slot] = key;
        cacheIndex[key] = slot;
        chain[slot] = true;
    }
}

// ---------------- helpers ----------------

size_t GPU::getBlocksNum() const { return blocksNum; }
size_t GPU::getBlockSize() const { return blockSize; }

bool GPU::isIdle(size_t slot) const {
    return memory[slot] != nullptr && memory[slot]->getRefCount() == 0;
}

void GPU::addCacheHits(size_t n) { cacheHits += n; }
size_t GPU::getCacheHits() const { return cacheHits; }
size_t GPU::getEvictions() const { return evictions; }
