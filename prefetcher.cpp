#include "prefetcher.h"

std::vector<uint64_t> NextLinePrefetcher::calculatePrefetch(uint64_t current_addr, bool miss) {
    (void)miss;
    std::vector<uint64_t> prefetches;

    // TODO: Task 3
    // 1. Align current_addr down to the current cache block.
    // 2. Prefetch the next sequential block.
    uint64_t block_addr = current_addr & ~(uint64_t)(block_size - 1);
    prefetches.push_back(block_addr + block_size);

    return prefetches;
}

std::vector<uint64_t> StridePrefetcher::calculatePrefetch(uint64_t current_addr, bool miss) {
    (void)miss;
    std::vector<uint64_t> prefetches;

    // TODO: Task 3
    // Suggested design:
    // 1. Track the previous accessed block.
    // 2. Compute the current stride in block units.
    // 3. If the same stride repeats enough times, prefetch the next block at that stride.
    // 4. Update last_block / last_stride / confidence.
    uint64_t current_block = current_addr & ~(uint64_t)(block_size - 1);
    if (!has_last_block) {
        has_last_block = true;
        last_block = current_block;
        return prefetches;
    }

    int64_t stride = static_cast<int64_t>(current_block) - static_cast<int64_t>(last_block);
    if (stride != 0 && stride == last_stride) {
        ++confidence;
    } else {
        confidence = 1;
    }

    if (confidence >= 2 && stride != 0) {
        prefetches.push_back(static_cast<uint64_t>(static_cast<int64_t>(current_block) + stride));
    }

    last_block = current_block;
    last_stride = stride;

    return prefetches;
}

Prefetcher* createPrefetcher(std::string name, uint32_t block_size) {
    if (name == "NextLine") return new NextLinePrefetcher(block_size);
    if (name == "Stride") return new StridePrefetcher(block_size);
    return new NoPrefetcher();
}
