# 并行归档设计方案

## 1. 当前瓶颈分析

### 1.1 串行处理流程

当前 `pack()` 函数的串行流程：

```
遍历目录 → 收集文件列表 → 逐个读取文件 → 逐个写入归档
   (1)         (2)            (3) 串行瓶颈      (4) 串行瓶颈
```

当文件数量成百上千时：
- **IO 瓶颈**：磁盘读取是串行的，无法利用多核 CPU
- **内存利用不足**：每次只处理一个文件，大量内存闲置
- **CPU 空转**：等待 IO 时 CPU 无事可做

### 1.2 性能测试基准

以 1000 个文件、平均 1MB/文件为例：

| 阶段 | 串行时间 | 并行潜力 |
|------|---------|---------|
| 磁盘读取 | ~10s | 可并行 |
| 计算校验和 | ~2s | 可并行 |
| 压缩（未来）| ~20s | 可并行 |
| 写入归档 | ~5s | 需同步 |
| **总计** | **~37s** | **可优化至 ~15s** |

## 2. 设计方案：生产者-消费者模型

### 2.1 架构图

```
┌─────────────────────────────────────────────────────────────┐
│                         主线程 (Main)                        │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────────┐  │
│  │  目录扫描器  │ →  │  任务队列   │ ←  │  归档写入器      │  │
│  │  (单线程)   │    │  (线程安全) │    │  (单线程,同步)  │  │
│  └─────────────┘    └─────────────┘    └─────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                      工作线程池 (N个)                        │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐      ┌─────────┐     │
│  │ Worker 1│  │ Worker 2│  │ Worker 3│ ...  │ Worker N│     │
│  │ 读取文件 │  │ 读取文件 │  │ 读取文件 │      │ 读取文件 │     │
│  │ 计算CRC │  │ 计算CRC │  │ 计算CRC │      │ 计算CRC │     │
│  └────┬────┘  └────┬────┘  └────┬────┘      └────┬────┘     │
└───────┼────────────┼────────────┼────────────────┼──────────┘
        └────────────┴────────────┘                │
                    ↓                              │
            ┌───────────────┐                      │
            │ 结果队列      │ ←────────────────────┘
            │ (已完成任务)  │
            └───────┬───────┘
                    ↓
            ┌───────────────┐
            │ 排序合并器    │
            │ (保证顺序)    │
            └───────────────┘
```

### 2.2 核心组件

#### 2.2.1 任务结构

```cpp
struct PackTask {
    fs::path file_path;          // 源文件路径
    fs::path rel_path;           // 相对路径（归档内）
    uint32_t task_id;            // 任务序号（用于保序）
    uint16_t permissions;        // 文件权限
};

struct PackResult {
    uint32_t task_id;            // 对应任务ID
    std::vector<char> content;   // 文件内容
    uint64_t modified_time;      // 修改时间
    uint32_t checksum;           // CRC32校验和
    std::string rel_path;        // 相对路径
    uint16_t permissions;        // 权限
};
```

#### 2.2.2 线程安全的任务队列

```cpp
template<typename T>
class ThreadSafeQueue {
private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cond_;
    bool stop_ = false;

public:
    void push(T item);
    bool pop(T& item, std::chrono::milliseconds timeout);
    void stop();
    bool empty() const;
    size_t size() const;
};
```

#### 2.2.3 线程池实现

```cpp
class ThreadPool {
private:
    std::vector<std::thread> workers_;
    ThreadSafeQueue<PackTask> task_queue_;
    ThreadSafeQueue<PackResult> result_queue_;
    std::atomic<uint32_t> completed_count_{0};
    std::atomic<bool> stop_{false};
    size_t num_threads_;

public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
    ~ThreadPool();

    void submit(PackTask task);
    bool get_result(PackResult& result, std::chrono::milliseconds timeout);
    void wait_all();
    
private:
    void worker_loop();
};
```

## 3. 并行 Pack 流程

```cpp
void pack_parallel(const fs::path& source_dir, const fs::path& archive_path) {
    // 阶段 1: 扫描目录（单线程）
    auto files = scan_directory(source_dir);
    
    // 阶段 2: 初始化线程池
    ThreadPool pool(std::thread::hardware_concurrency());
    
    // 阶段 3: 提交任务（主线程）
    uint32_t task_id = 0;
    for (const auto& file : files) {
        pool.submit({
            .file_path = file.path,
            .rel_path = fs::relative(file.path, source_dir),
            .task_id = task_id++,
            .permissions = get_permissions(file.path)
        });
    }
    
    // 阶段 4: 写入归档（单线程，保证顺序）
    std::ofstream archive(archive_path, std::ios::binary);
    write_global_header(archive, files.size());
    
    // 使用最小堆保证输出顺序
    std::priority_queue<PackResult, std::vector<PackResult>, std::greater<>> pending_results;
    uint32_t next_expected_id = 0;
    
    PackResult result;
    while (next_expected_id < files.size()) {
        if (pool.get_result(result, std::chrono::milliseconds(100))) {
            pending_results.push(std::move(result));
        }
        
        // 写入所有已就绪且顺序正确的结果
        while (!pending_results.empty() && 
               pending_results.top().task_id == next_expected_id) {
            write_entry(archive, pending_results.top());
            pending_results.pop();
            next_expected_id++;
        }
    }
}
```

## 4. 并行 Unpack 流程

解压的并行化与打包不同：

```cpp
void unpack_parallel(const fs::path& archive_path, const fs::path& target_dir) {
    // 阶段 1: 读取归档索引（单线程，顺序读取）
    auto entries = read_archive_index(archive_path);
    
    // 阶段 2: 创建目录结构（单线程）
    create_directory_structure(target_dir, entries);
    
    // 阶段 3: 并行写入文件
    ThreadPool pool(std::thread::hardware_concurrency());
    
    for (const auto& entry : entries) {
        pool.submit([entry, &archive_path, &target_dir]() {
            // 每个线程打开独立文件描述符
            // 随机读取归档中的文件内容
            // 写入目标文件
        });
    }
    
    pool.wait_all();
}
```

**注意**：解压时不需要保序写入，可以真正并行。

## 5. 关键技术点

### 5.1 顺序保证机制

由于 `.kar` 格式要求条目按顺序存储（便于顺序解压），并行打包需要保证**输出顺序与输入顺序一致**。

**解决方案**：
- 每个任务分配递增的 `task_id`
- 使用最小堆缓存乱序到达的结果
- 按 `task_id` 顺序写入归档

```cpp
// 最小堆比较器
struct TaskIdCompare {
    bool operator()(const PackResult& a, const PackResult& b) const {
        return a.task_id > b.task_id; // 小顶堆
    }
};
```

### 5.2 内存控制

防止同时加载过多文件导致内存溢出：

```cpp
class MemoryLimiter {
private:
    std::atomic<size_t> current_usage_{0};
    size_t max_usage_;
    std::mutex mutex_;
    std::condition_variable cond_;

public:
    explicit MemoryLimiter(size_t max_mb) : max_usage_(max_mb * 1024 * 1024) {}
    
    void acquire(size_t bytes);
    void release(size_t bytes);
};

// 在 worker 中使用
void worker_loop() {
    PackTask task;
    if (task_queue_.pop(task, timeout)) {
        // 预估内存需求
        size_t file_size = fs::file_size(task.file_path);
        memory_limiter_.acquire(file_size);
        
        auto result = process_task(task);
        memory_limiter_.release(file_size);
        
        result_queue_.push(std::move(result));
    }
}
```

### 5.3 IO 优化

- **预读 (readahead)**：对顺序读取的文件使用 `posix_fadvise`
- **异步 IO**：考虑使用 `aio_read` 或 io_uring（Linux）
- **内存映射**：对大文件使用 `mmap` 代替 `read`

```cpp
// 使用 mmap 读取大文件
std::vector<char> read_file_mmap(const fs::path& path) {
    int fd = open(path.c_str(), O_RDONLY);
    size_t size = fs::file_size(path);
    
    void* mapped = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    madvise(mapped, size, MADV_SEQUENTIAL);
    
    std::vector<char> buffer(static_cast<char*>(mapped), 
                             static_cast<char*>(mapped) + size);
    munmap(mapped, size);
    close(fd);
    return buffer;
}
```

## 6. 性能对比预估

| 文件数量 | 总大小 | 串行时间 | 并行时间(4核) | 加速比 |
|---------|-------|---------|--------------|-------|
| 100 | 100 MB | 2s | 0.8s | 2.5x |
| 1,000 | 1 GB | 20s | 6s | 3.3x |
| 10,000 | 10 GB | 200s | 55s | 3.6x |
| 100,000 | 100 GB | 2000s | 550s | 3.6x |

*注：实际加速比取决于磁盘 IO 类型（HDD/SSD/NVMe）*

## 7. 实现计划

### Phase 1: 基础框架
- [ ] 实现 `ThreadSafeQueue`
- [ ] 实现 `ThreadPool`
- [ ] 添加 `--threads` 命令行参数

### Phase 2: 并行 Pack
- [ ] 实现任务分发
- [ ] 实现结果收集与排序
- [ ] 添加内存限制器

### Phase 3: 并行 Unpack
- [ ] 实现索引预读取
- [ ] 实现并行文件写入
- [ ] 添加写入冲突检测

### Phase 4: 优化
- [ ] 添加 CRC32 并行计算
- [ ] 添加进度条
- [ ] 实现自适应线程数

## 8. 兼容性考虑

- **文件格式不变**：并行打包生成的 `.kar` 文件与串行版本完全兼容
- **回退机制**：单核环境下自动使用串行模式
- **错误处理**：任一任务失败，整体操作回滚

## 9. 风险评估

| 风险 | 可能性 | 影响 | 缓解措施 |
|-----|-------|-----|---------|
| 内存溢出 | 中 | 高 | 添加内存限制器 |
| 文件句柄耗尽 | 中 | 中 | 限制并发数，使用连接池 |
| 磁盘 IO 争用 | 高 | 低 | 自适应线程数 |
| 顺序错乱 | 低 | 高 | 严格的最小堆排序 |
