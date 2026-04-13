# CSC3060 Project 4 Report Draft

## 1. Student / Team Information

- Name 1: [请填写]
- Student ID 1: [请填写]
- Name 2: [请填写]
- Student ID 2: [请填写]
- Chosen Seed Student ID: [请填写]

说明：
- 如果是双人组，报告、ZIP 文件名、PDF 文件名以及个性化 trace 生成都必须使用同一个 seed student ID。
- 如果是单人完成，删除第二位成员信息即可。

## 2. Implementation Summary

本项目目前已完成 Task 1 和 Task 2 的主要实现，整体目标是构建一个简化版 cache simulator，并将其从单级 `L1` 扩展为多级 `L1 + L2` cache hierarchy。

根据项目说明 PDF 的要求，本阶段重点检查了：

- Task 1 是否正确实现单级 `L1 -> Main Memory`
- Task 2 是否正确实现多级 `L1 -> L2 -> Main Memory`
- `trace_sanity.txt` 下默认配置与多种 cache geometry 下的行为是否合理
- `write-back` 是否既更新统计，也正确计入 `Total Cycles` 和 `AMAT`

### Task 1

Task 1 完成了单级 `L1 cache` 的核心访问逻辑，主要包括：

- 根据 cache 几何参数计算地址的 `block offset`、`set index` 和 `tag`
- 根据 `index` 选中目标 set，并在 set 中查找是否存在有效且匹配的 cache line
- 在 hit 时更新命中统计、replacement metadata，以及写请求对应的 dirty bit
- 在 miss 时优先寻找 invalid line，否则使用 replacement policy 选择 victim
- 在替换 dirty victim 时执行 write-back
- 从下一层 memory 取回 block，并安装到 `L1`
- 实现了 `LRU` replacement policy

本次检查中又额外修正了一个统计正确性问题：

- 先前 dirty victim 的 write-back 虽然已发送到下一层，但其 latency 没有计入当前访问的总周期
- 现已修改为：write-back helper 返回下层访问 latency，并累加到本次 access 的 `lat`
- 修正后 `WB`、`Total Cycles` 和 `AMAT` 三者保持一致

### Task 2

Task 2 在 Task 1 的基础上加入了 `L2 cache`，将单级层次扩展为：

`L1 -> L2 -> Main Memory`

Task 2 的主要实现包括：

- 在命令行传入 `--enable-l2` 时构造 `L2 cache`
- 默认情况下保持 Task 1 模式，即 `L1 -> Main Memory`
- 启用 `L2` 时，将 `L1` 的下一层从 main memory 改为 `L2`
- 保持 miss path 为递归式访问路径
- 为 `L2` 输出独立统计信息

另外，本次复核时也确认了：

- `L1` dirty eviction 写回 `L2` 时，其 latency 现在会正确反映在总周期中
- `L2` miss 时继续访问 main memory，整体 miss path 保持递归式层次结构

## 3. Address Mapping Explanation

在 cache 中，一个物理地址被分解为三部分：

- `block offset`
- `set index`
- `tag`

### Block Offset

`block offset` 由 cache line size 决定。  
如果 block size 是 `64 bytes`，那么需要 `log2(64) = 6` 位来表示一个 block 内部的字节偏移。

### Set Index

`set index` 由 set 的数量决定。  
如果 cache 总容量为 `32KB`，相联度为 `8-way`，block size 为 `64B`，则：

- total bytes = `32 * 1024 = 32768`
- number of lines = `32768 / 64 = 512`
- number of sets = `512 / 8 = 64`
- 所以 `index bits = log2(64) = 6`

### Tag

`tag` 是地址中去掉 `offset bits` 和 `index bits` 后剩余的高位。  
在当前实现中：

- `index = (addr >> offset_bits) & ((1ULL << index_bits) - 1)`
- `tag = addr >> (offset_bits + index_bits)`

当 cache 的大小、相联度、set 数或 block size 改变时，`offset bits` 和 `index bits` 都可能改变，因此同一个地址映射到的 set 和 tag 也会变化。

## 4. Task 1 Testing

Task 1 的测试重点是验证单级 `L1 cache` 是否在不同配置下都能正确工作，而不是只在一种固定参数下工作。

### 测试方法

- 使用提供的 `trace_sanity.txt`
- 使用默认 `Makefile` 目标运行
- 按 PDF 要求测试多种 cache geometry
- 检查 hit / miss / write-back / AMAT 是否合理

### 测试命令

```bash
make task1
./cache_sim trace_sanity.txt 16 4 64 1 100
./cache_sim trace_sanity.txt 32 4 32 1 100
./cache_sim trace_sanity.txt 64 8 128 1 100
```

### 默认配置结果

- `L1 Hit Rate = 21.43%`
- `Access = 56`
- `Miss = 44`
- `WB = 2`
- `Main Memory Accesses = 46`
- `Total Cycles = 4656`
- `AMAT = 83.14 cycles`

### 多组几何配置测试

项目说明 PDF 明确要求 Task 1 不能只测试一种固定几何配置，因此补充测试了以下几组：

| Configuration | L1 Hit Rate | Miss | WB | Main Memory Accesses | Total Cycles | AMAT |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `16KB, 4-way, 64B` | 21.43% | 44 | 6 | 50 | 5056 | 90.29 |
| `32KB, 4-way, 32B` | 21.43% | 44 | 2 | 46 | 4656 | 83.14 |
| `64KB, 8-way, 128B` | 60.71% | 22 | 0 | 22 | 2256 | 40.29 |

### 正确性说明

从结果可以看出：

- 单级 `L1` 已经能够处理 hit / miss
- miss 会继续访问下一层 main memory
- 替换时出现了 dirty write-back，且 write-back latency 已正确计入总周期
- 程序能够完成整条 trace 的回放并输出统计结果
- 更改 cache size / associativity / block size 后，tag/index 映射和统计结果会发生变化，说明实现不是写死在某一种配置上

## 5. Task 2 Hierarchy Explanation

Task 2 的目标是在原有 `L1` 基础上加入 `L2`，理解多级 cache hierarchy 的访问路径。

### 层级结构

未开启 `L2` 时：

`L1 -> Main Memory`

开启 `L2` 时：

`L1 -> L2 -> Main Memory`

### 访问行为

#### 情况 1：L1 hit

如果访问地址在 `L1` 中命中，则直接返回 `L1 latency`，不访问 `L2` 和 main memory。

#### 情况 2：L1 miss, L2 hit

如果 `L1` miss，但 `L2` hit，则：

- `L1` 记录一次 miss
- 请求继续发送到 `L2`
- `L2` 命中后返回数据
- 数据被安装回 `L1`
- 访问不会继续到 main memory

#### 情况 3：L1 miss, L2 miss

如果 `L1` miss 且 `L2` 也 miss，则：

- `L1` 将请求发送给 `L2`
- `L2` 再发送到 main memory
- main memory 返回数据
- 数据先安装到 `L2`
- 再回填到 `L1`

#### Dirty Write-Back

如果某层 cache 替换出的 victim 是 dirty line，则：

- 重建被驱逐 block 的地址
- 把一次写访问发送到下一层
- 如果下一层也是 cache，则会继续按照它自己的规则处理
- 该写回访问产生的 latency 也会计入当前访问的总周期

这说明 write-back 逻辑在多级层次中是递归传播的。

## 6. Task 3 Design Choices

Task 3 目前尚未完成，因此此部分先保留报告结构，后续补充：

- 实现了哪些 replacement policies
- 实现了哪些 prefetchers
- 是否实现自定义 prefetcher
- 为什么选择最终配置

## 7. Trace Analysis

Task 3 需要基于个性化 trace 进行分析。目前尚未生成和分析个性化 trace，因此此部分待补充。后续应至少说明：

- trace 是否表现为 sequential / stride / reuse-heavy / pollution-heavy
- hot blocks 和 hot sets 是否明显
- 是否存在 phase behavior
- 这些特征如何影响 replacement policy 和 prefetcher 的选择

## 8. Experimental Results

### Task 1 Baseline

| Configuration | L1 Hit Rate | Main Memory Accesses | Total Cycles | AMAT |
| --- | ---: | ---: | ---: | ---: |
| L1 only, 32KB, 8-way, 64B, LRU, None | 21.43% | 46 | 4656 | 83.14 |

### Task 1 Multi-Geometry Results

| Configuration | L1 Hit Rate | WB | Main Memory Accesses | Total Cycles | AMAT |
| --- | ---: | ---: | ---: | ---: | ---: |
| L1 only, 16KB, 4-way, 64B, LRU, None | 21.43% | 6 | 50 | 5056 | 90.29 |
| L1 only, 32KB, 4-way, 32B, LRU, None | 21.43% | 2 | 46 | 4656 | 83.14 |
| L1 only, 64KB, 8-way, 128B, LRU, None | 60.71% | 0 | 22 | 2256 | 40.29 |

### Task 2 Representative Result

```bash
make task2
```

结果如下：

- `L1 Hit Rate = 21.43%`
- `L1 Access = 56`
- `L1 Miss = 44`
- `L1 WB = 2`
- `L2 Hit Rate = 50.00%`
- `L2 Access = 46`
- `L2 Miss = 23`
- `L2 WB = 0`
- `Main Memory Accesses = 23`
- `Total Cycles = 2540`
- `AMAT = 45.36 cycles`

### Task 2 Additional Geometry Result

| Configuration | L1 Hit Rate | L2 Hit Rate | Main Memory Accesses | Total Cycles | AMAT |
| --- | ---: | ---: | ---: | ---: | ---: |
| L1+L2, 16KB/64KB, 4-way, 64B, LRU, None | 21.43% | 54.00% | 23 | 2556 | 45.64 |

### 结果对比

| Mode | L1 Hit Rate | L2 Hit Rate | Main Memory Accesses | Total Cycles | AMAT |
| --- | ---: | ---: | ---: | ---: | ---: |
| Task 1: L1 only | 21.43% | N/A | 46 | 4656 | 83.14 |
| Task 2: L1 + L2 | 21.43% | 50.00% | 23 | 2540 | 45.36 |

可以观察到：

- 加入 `L2` 后，`L1` 命中率不变，因为 `L1` 本身的容量和策略没有变化
- 一部分 `L1 miss` 被 `L2` 命中，`L2 hit rate = 50%`
- main memory access 从 `46` 降到 `23`
- `AMAT` 从 `83.14` 降到 `45.36`

这说明 `L2` 成功拦截了一部分原本会进入主存的请求，显著降低了总访问代价。

## 9. Best Configuration and Discussion

就当前 Task 2 阶段而言，最佳配置尚未进入正式调优阶段，但已经可以确认：

- 引入 `L2` 比单级 `L1` 有更低的 `AMAT`
- `L2` 有效减少了主存流量
- 多级 hierarchy 在相同 trace 下明显优于单级 `L1`

当前设计的优点：

- 结构清晰，支持单级和多级两种模式
- `L1` 和 `L2` 共享相同的访问接口
- write-back 路径统一由 helper 函数处理，便于后续扩展 Task 3
- 经过本次修正后，dirty write-back 不仅更新 `WB` 统计，也会进入 `Total Cycles` 和 `AMAT` 计算

当前设计的局限：

- `L2` 参数目前采用默认推导方式，而不是经过个性化 trace 调优
- 还没有实现更高级的 replacement policy 和 prefetcher
- 还没有根据个性化 trace 分析选择最终配置

## 10. External Resources and AI Usage

本报告及代码实现过程中使用了以下资源：

- 课程提供的 starter code 和项目说明 PDF
- 本地终端编译与测试结果
- `pypdf`，用于提取并阅读项目 PDF 正文
- LLM assistance: OpenAI Codex / ChatGPT，用于解释项目要求、实现 Task 2、整理报告草稿

### 本次检查与修正记录

- 安装 `pypdf` 并完整阅读 `csc3060_spring2026_project4.pdf`
- 按 PDF 明确要求重新核对 Task 1 / Task 2 的必做项与测试项
- 修复 dirty write-back latency 未计入 `Total Cycles` / `AMAT` 的 bug
- 重新运行 `make task1`、`make task2` 以及多组额外几何配置测试
- 根据修正后的真实结果更新本报告中的表格与说明

如果最终提交版本要求附上对话链接，请在此处补充：

- LLM conversation link: [请填写]
