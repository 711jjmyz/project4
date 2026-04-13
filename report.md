# CSC3060 Project 4 Report 

## 1. Team Information

- Name 1: Yuezhe Meng
- Student ID 1: 124090475
- Name 2: Mingze Zhou
- Student ID 2: 124090947
- Chosen Seed Student ID: 124090475


## 2. Implementation Summary

**Task 1:** Implemented core cache access logic including address decomposition
(`get_index`, `get_tag`, `reconstruct_addr`), hit/miss detection, dirty write-back,
victim selection, and LRU replacement policy.

**Task 2:** Connected L1 and L2 into a two-level hierarchy. L1 misses are now
served by L2 before going to main memory, reducing main memory traffic and AMAT.

**Task 3:** Implemented SRRIP and BIP replacement policies, and Stride and NextLine
prefetchers. Analyzed the personalized trace and tuned the final configuration
to achieve AMAT of 1.73 cycles, below the Best_AMAT of 1.75 cycles.

## 3. Address Mapping Explanation

A memory address is divided into three fields:
| tag bits | index bits | offset bits |
|---|---|----|
|  high    |   middle   |    low      |

**Block Offset** — lowest `offset_bits` bits:
```
offset_bits = log2(block_size)
```

**Set Index** — middle `index_bits` bits:
```
index_bits  = log2(num_sets)
index = (addr >> offset_bits) & ((1 << index_bits) - 1)
```

**Tag** — remaining high bits:
```
tag = addr >> (offset_bits + index_bits)
```

**Reconstruct address** (used in write-back):
```
addr = (tag << (offset_bits + index_bits)) | (index << offset_bits)
```

---

### Effect of Cache Geometry Changes

| Change | Effect |
|--------|--------|
| Larger block size | More offset bits → fewer index bits |
| More sets | More index bits → fewer tag bits |
| Higher associativity | No change to bit fields |
| Larger cache size | More sets → more index bits |

## 4. Task 1 Testing

Correctness was verified by running
```
./cache_sim trace_sanity.txt 16 4 64 1 100
./cache_sim trace_sanity.txt 32 4 32 1 100
./cache_sim trace_sanity.txt 64 8 128 1 100
```
  and checking hit rate, miss count, write-back count, and AMAT.

### Results

| Configuration | Hit Rate | Access | Miss | WB | AMAT |
|---------------|----------|--------|------|----|------|
| 32KB, 8-way, 64B (default) | 21.43% | 56 | 44 | 2 | 79.57 |
| 16KB, 4-way, 64B | 21.43% | 56 | 44 | 6 | 79.57 |
| 32KB, 4-way, 32B | 21.43% | 56 | 44 | 2 | 79.57 |
| 64KB, 8-way, 128B | 60.71% | 56 | 22 | 0 | 40.29 |

The default configuration matches the expected reference output exactly.
The 64KB configuration achieves a significantly higher hit rate (60.71%) and
lower AMAT (40.29 cycles) due to its larger capacity fitting more blocks.
The 16KB configuration produces more write-backs (6 vs 2) because its smaller
capacity causes more dirty victims to be evicted. All configurations correctly
compute tag, index, and offset, confirming that the implementation works across
different cache geometries.


## 5. Task 2 Hierarchy Explanation

### How L1, L2, and Memory Interact

- **L1 Hit:** data returned immediately at L1 latency, L2 and memory not accessed
- **L1 Miss + L2 Hit:** data fetched from L2, loaded into L1,
  latency = L1 latency + L2 latency
- **L1 Miss + L2 Miss:** data fetched from main memory, loaded into both L2 and L1,
  latency = L1 latency + L2 latency + memory latency
- **Dirty write-back:** evicted dirty lines from L1 are written to L2, and dirty
  lines evicted from L2 are written to main memory

### What Changed After Adding L2

| Metric | L1 Only | L1 + L2 |
|--------|---------|---------|
| L1 Hit Rate | 21.43% | 21.43% |
| L2 Hit Rate | — | 50.00% |
| Main Memory Accesses | 46 | 23 |
| Total Cycles | 4456 | 2532 |
| AMAT | 79.57 cycles | 45.21 cycles |

L2 intercepts 50% of L1 misses, halving main memory accesses from 46 to 23.
Total cycles drop from 4456 to 2532 and AMAT improves from 79.57 to 45.21 cycles,
demonstrating that L2 effectively reduces the cost of L1 misses by serving them
at a lower latency than main memory.

## 6. Task 3 Design Choices

Task 3 目前仍在进行中，但已经完成了 prefetch 相关部分的基础实现。

### 已完成的 Prefetcher

目前已经实现：

- `NextLine` prefetcher
- `Stride` prefetcher
- `install_prefetch(...)` 预取填充路径

### NextLine 的实现思路

- 将当前访问地址按 cache block 对齐
- 预取下一个连续 block，即 `block_addr + block_size`
- 每次访问最多发起一个 next-line 预取请求

这种策略适合顺序访问或接近顺序访问的 workload。

### Stride 的实现思路

- 跟踪上一次访问的 block 地址 `last_block`
- 计算本次 block 与上一次 block 的差值，得到 stride
- 如果连续观察到相同 stride，则增加 `confidence`
- 当 `confidence` 达到阈值后，预取 `current_block + stride`

这种策略适合具有固定步长访问模式的 trace。

### Prefetch Fill Path

在 `memory_hierarchy.cpp` 中，已经补全 `install_prefetch(...)`，其行为为：

- 先检查待预取 block 是否已经存在于当前 cache
- 如果已存在，则不重复安装
- 如果不存在，则优先寻找 invalid line
- 若 set 已满，则复用 replacement policy 选择 victim
- 若 victim 为 dirty，则复用已有 write-back helper 写回下层
- 从下一层读取目标 block
- 以 `clean` 状态安装，并标记 `is_prefetched = true`

### 当前实现状态

- prefetch 已经接入 demand access 之后的执行路径
- `Prefetches Issued` 统计会在成功安装预取块时增加
- 当前实现会让 prefetch 真实访问下一层，从而影响下层 traffic
- 当前尚未与 personalized trace 结合进行最终调优
- 最终最佳配置仍需等 `SRRIP` / `BIP` 完成后，与不同 prefetcher 组合一起测试

## 7. Trace Analysis
- **124090475 was used as the trace-generation seed**
### Access Patterns Observed

The personalized trace (8,428 accesses, 96.82% reads, 3.18% writes) shows three distinct phases:

- **Scan phases (windows 256–2304 and 4096–end):** nearly 256 unique blocks per
  window across all 64 L1 sets, with almost no within-window reuse. This is a
  classic streaming pattern.
- **Hot reuse phase (windows 2304–4096):** only 1–2 sets are used with 14–34
  unique blocks per window. The top 8 blocks are each accessed 60 times,
  forming a small but highly reused working set.

The stride distribution is dominated by **stride=7 (48.08%)** and **stride=1
(26.85%)**, indicating structured, regular memory access.

---
### Influence on Design Decisions

- The **scan phases (Phase 2 & 4)** show almost no reuse, meaning LRU will be 
  completely polluted by streaming data. **SRRIP** is chosen for L1 because it 
  inserts new lines with RRPV=2 and only promotes to RRPV=0 on hit, allowing 
  scan data to age out quickly without evicting the hot working set.

- The dominant **stride=7 pattern (48.08%)** makes a **Stride prefetcher** the 
  natural choice for L1, converting predictable future misses into hits before 
  the data is demanded.

- **Set 51 receives 15.50% of all accesses**, indicating severe set pressure and 
  frequent conflict misses. Increasing **associativity to 64-way** gives each set 
  more capacity to hold competing blocks simultaneously, reducing conflict misses.

- **L2 uses LRU + Stride**: LRU is sufficient at this level due to L2's larger 
  capacity, and Stride prefetching further reduces main memory traffic.

| Level | Size  | Associativity | Block | Policy | Prefetcher |
|-------|-------|--------------|-------|--------|------------|
| L1    | 32KB  | 64-way        | 64B   | SRRIP  | Stride     |
| L2    | 128KB | 8-way         | 64B   | LRU    | Stride     |

## 8. Experimental Results

All experiments use `trace_sanity.txt` for Task 1/2 and `my_trace.txt` for Task 3.
Cache size is fixed at 32KB for L1 and 128KB for L2 unless otherwise noted.

### Task 1: Single-Level Cache

| Configuration | Hit Rate | Miss | WB | AMAT |
|---------------|----------|------|----|------|
| 32KB, 8-way, 64B (default) | 21.43% | 44 | 2 | 79.57 |
| 16KB, 4-way, 64B | 21.43% | 44 | 6 | 79.57 |
| 32KB, 4-way, 32B | 21.43% | 44 | 2 | 79.57 |
| 64KB, 8-way, 128B | 60.71% | 22 | 0 | 40.29 |

### Task 2: Two-Level Cache

| Configuration | L1 Hit Rate | L2 Hit Rate | Mem Accesses | AMAT |
|---------------|------------|------------|--------------|------|
| L1 only, LRU+None | 21.43% | — | 46 | 79.57 |
| L1+L2, LRU+None | 21.43% | 50.00% | 23 | 45.21 |

### Task 3: Policy and Prefetcher Optimization

| L1 Policy | L1 Prefetch | L2 Policy | L2 Prefetch | Assoc | AMAT |
|-----------|------------|-----------|------------|-------|------|
| LRU | None | LRU | None | 8 | 2.50 (Baseline) |
| BIP | Stride | LRU | None | 8 | 2.50 |
| BIP | Stride | LRU | Stride | 8 | 2.30 |
| SRRIP | Stride | LRU | Stride | 8 | 2.02 |
| SRRIP | Stride | LRU | Stride | 32 | 1.80 |
| SRRIP | Stride | LRU | Stride | 64 | **1.73** ✅ |

The final configuration (SRRIP + Stride, assoc=64, L2 LRU+Stride) achieves
AMAT of **1.73 cycles**, below the Best_AMAT of 1.75 cycles.

## 9. Best Configuration and Discussion

### Best-Performing Design

| Level | Size  | Associativity | Block | Policy | Prefetcher |
|-------|-------|--------------|-------|--------|------------|
| L1    | 32KB  | 64-way        | 64B   | SRRIP  | Stride     |
| L2    | 128KB | 8-way         | 64B   | LRU    | Stride     |

**AMAT: 1.73 cycles** (below Best_AMAT of 1.75 cycles)

---

### Why It Performs Well

- **SRRIP** effectively resists scan pollution in Phase 2 and 4 by inserting
  new lines with RRPV=2 and only promoting to RRPV=0 on hit. Scan data ages
  out quickly without evicting the hot working set from Phase 3.

- **Stride prefetcher** exploits the dominant stride=7 pattern (48.08%),
  prefetching future blocks before they are demanded and converting misses
  into hits.

- **64-way associativity** eliminates most conflict misses on set 51, which
  receives 15.50% of all accesses. Higher associativity gives the set enough
  capacity to hold all competing blocks simultaneously.

- **L2 with Stride prefetcher** further reduces main memory traffic by
  prefetching stride-pattern blocks into L2 before L1 requests them.

---

### Where It May Still Fail

- **Phase 2 and 4 are pure scans** with almost no reuse. Even with SRRIP,
  some useful lines may still be evicted during heavy scan traffic.

- **64-way associativity** increases hardware complexity and lookup time.
  In a real cache, this would add latency that is not modeled in this simulator.

- The **Stride prefetcher** relies on a stable stride. If the access pattern
  changes suddenly between phases, the prefetcher may issue incorrect prefetches
  and cause unnecessary pollution.

## 10. External Resources and AI Usage

### External Resources
- Claude AI (Anthropic) — used for implementation guidance, debugging,
  and report writing assistance

### AI Usage
This project used Claude (claude.ai) extensively for:
- Understanding cache concepts (LRU, SRRIP, BIP, prefetchers)
- Debugging implementation errors
- Analyzing trace results and choosing optimal configurations
- Writing the project report

**Conversation link:** https://claude.ai/chat/4fdfc257-9bda-4ded-a6c3-1a47f2a33e97
