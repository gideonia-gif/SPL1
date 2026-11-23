#ifndef GPU_H_
#define GPU_H_

#include <string>
#include <cstddef>
#include <vector>
#include <map>

struct PrefixMatch {
    std::size_t matchedTokens;
    std::vector<std::size_t> blockIds;
};

class MemoryBlock;
class Request;
class GenerativeRequest;
class EmbeddingRequest;

class GPU{
public:
    GPU(std::size_t blocksNum, std::size_t blockSize);

    // Rule of 5: the GPU owns the blocks in `memory`
    ~GPU();
    GPU(const GPU& other);
    GPU& operator=(const GPU& other);
    GPU(GPU&& other) noexcept;
    GPU& operator=(GPU&& other) noexcept;

    std::size_t availableSlots() const;
    std::size_t getBlocksForMemory(std::size_t memory) const;
    void reserve(std::size_t n, const Request& request);
    void unreserve(std::size_t n, const Request& request);
    std::size_t allocateBlock(const Request& request);
    void addMemoryBlock(const GenerativeRequest& request, std::size_t slot);
    void addMemoryBlock(const EmbeddingRequest& request, std::size_t slot);
    void acquire(std::size_t slot);
    void release(std::size_t slot);
    void copyMemoryFromOtherGPU(const GPU& other,const std::vector<std::size_t>& blocks);
    void assignPrefixToBlock(std::size_t block, const std::vector<std::string>& prefixTokens, const std::string& modelId);
    int readFromBlock(std::size_t blockId, std::size_t offset) const;
    void writeToBlock(std::size_t blockId, std::size_t offset, int value);
    PrefixMatch findLongestCachedPrefix(const std::string& modelId, std::size_t embeddingSize,
                                        const std::vector<std::string>& promptTokens) const;

    // Added helpers
    std::size_t getBlocksNum() const;
    std::size_t getBlockSize() const;
    bool isIdle(std::size_t slot) const;          // holds a block that nobody uses
    void addCacheHits(std::size_t n);
    std::size_t getCacheHits() const;
    std::size_t getEvictions() const;

private:
    std::size_t blocksNum;
    std::size_t blockSize;
    std::vector<MemoryBlock*> memory;             // one entry per slot, nullptr = empty

    // Prefix cache: which (model, prefix key) is stored in which slot
    std::vector<bool> slotCached;                 // does the block in this slot have a key?
    std::vector<std::string> slotCacheKey;        // its key (see makeKey)
    std::map<std::string, std::size_t> cacheIndex;// key -> slot

    // Reservations: blocks promised to admitted requests but not created yet
    std::map<int, std::size_t> reservations;      // request id -> reserved blocks
    std::size_t totalReserved;

    std::size_t cacheHits;
    std::size_t evictions;

    static std::string makeKey(const std::string& modelId, const std::vector<std::string>& tokens,
                               std::size_t count);
    std::size_t findFreeSlot(const std::vector<bool>& protectedSlots); // empty slot, or evict an idle one
    void deleteBlock(std::size_t slot);           // delete the block and clear its cache entry
    void clear();                                 // delete all blocks
};

#endif
