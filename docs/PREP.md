# OKX C++ 交易系统工程师面试准备指南

> ⚠️ **信息来源说明**：本文档区分了「✅ 真实来源」和「📝 通用准备」两类信息。
> 真实来源均标注了可验证的链接，通用准备是基于行业经验的建议。

---

## 📋 面试信息

| 项目 | 详情 |
|------|------|
| **职位** | Software Engineer - C++ (Exchange Platform - Trading Services) |
| **公司** | OKX (欧易) |
| **地点** | Hong Kong |
| **面试时间** | 2026年1月14日 15:00-16:00 HKT |
| **面试官** | Chaoyang Xu |
| **面试形式** | 视频面试 + 在线编程 |
| **Coderpad** | https://app.coderpad.io/CXPHA2HA |
| **Zoom** | 会议ID: 929 3049 2275 / 密码: 470452 |
| **紧急联系** | +8615989422206 |

---

## ✅ 真实来源信息

### 1. OKX 面试流程（来源：hw.glich.co）

**可验证链接**：
- P4: https://hw.glich.co/resources/companies/okx/interview-process/p4-software-engineer
- P5: https://hw.glich.co/resources/companies/okx/interview-process/p5-software-engineer
- P6.1: https://hw.glich.co/resources/companies/okx/interview-process/p6.1-software-engineer
- P6.2: https://hw.glich.co/resources/companies/okx/interview-process/p6.2-software-engineer
- P7.1: https://hw.glich.co/resources/companies/okx/interview-process/p7.1-software-engineer
- P7.3: https://hw.glich.co/resources/companies/okx/interview-process/p7.3-software-engineer

**面试流程结构**：

| 轮次 | 内容 | 说明 |
|------|------|------|
| **Round 1** | Technical Coding | 数据结构和算法 (DSA) |
| **Round 2** | System Design | 可扩展性、可靠性、可维护性 |
| **Round 3** | Behavioral | STAR 方法评估文化契合度 |

**面试周期**：
- P7.1 级别：平均 **14 天**，**3 轮**面试
- P5 级别：编码挑战 + 技术行为评估

### 2. 各职级真实面试题汇总 (来源：hw.glich.co)

#### **一、P4 职级**
| 薪资范围 | 经验要求 | 面试周期 | 面试轮数 |
|----------|----------|----------|----------|
| $140K - $180K USD | 4-7 years | 14 days | 4 轮 |

- **面试流程**
  1. **Round 1**: Data Structures and Algorithms (45 min) - Coding
  2. **Round 2**: System Design and Architecture (60 min) - System Design
  3. **Round 3**: Behavioral and Cultural Fit (45 min) - Behavioral
  4. **Round 4**: Senior Leadership Discussion (30 min) - Leadership
- **Example Questions（个人真题）**
  1. 两数之和（LeetCode 1）
  2. 合并两个有序链表（LeetCode 21）
  3. `vector` 的 `emplace_back` 与 `push_back` 的区别
  4. 实现线程安全的单例模式
- **Commonly Asked Questions（高频考题）**
  1. Number of Islands（LeetCode 200）- Medium
  2. Kth Smallest Element in a Sorted Matrix（LeetCode 378）- Medium
  3. Count Subarrays With Fixed Bounds（LeetCode 2444）- Hard
- **Asia 地区考察重点**
  - 区块链、高频交易系统、分布式数据库深度考察
  - 算法在真实场景中的实际应用
  - 高吞吐、低延迟环境的系统设计

#### **二、P5 职级**
| 薪资范围 | 经验要求 | 面试周期 | 面试轮数 |
|----------|----------|----------|----------|
| $120K - $150K USD | 5-8 years | 14 days | 3 轮 |

- **面试流程**
  1. **Round 1**: Coding Challenge (45 min) - DSA
  2. **Round 2**: System Design Interview (60 min) - 分布式系统设计
  3. **Round 3**: Behavioral and Managerial (45 min) - 文化契合度
- **Example Questions（个人真题）**
  1. 两数之和（LeetCode 1）
  2. 反转链表（LeetCode 206）
  3. 验证二叉搜索树（LeetCode 98）
- **Commonly Asked Questions（高频考题）**
  1. Number of Islands（LeetCode 200）- Medium
  2. Kth Smallest Element in a Sorted Matrix（LeetCode 378）- Medium
  3. Count Subarrays With Fixed Bounds（LeetCode 2444）- Hard

#### **三、P6.1 职级**
| 薪资范围 | 经验要求 | 面试周期 | 面试轮数 |
|----------|----------|----------|----------|
| $140K - $180K USD | 6-10 years | 14-21 days | 4 轮 |

- **面试流程**
  1. **Round 1**: Data Structures and Algorithms (45 min) - Coding
  2. **Round 2**: System Design (60 min) - 系统设计和架构能力
  3. **Round 3**: Behavioral and Cultural Fit (45 min) - 行为和文化契合度
  4. **Round 4**: Team Collaboration and Technical Alignment (30 min) - 技术对齐
- **Example Questions（个人真题）**
  1. 实现 Trie（前缀树）（LeetCode 208）
  2. 编辑距离（LeetCode 72）
  3. 合并区间（LeetCode 56）
  4. 寻找两个正序数组的中位数（LeetCode 4）
- **Commonly Asked Questions（高频考题）**
  1. Number of Islands（LeetCode 200）- Medium
  2. Kth Smallest Element in a Sorted Matrix（LeetCode 378）- Medium
  3. Count Subarrays With Fixed Bounds（LeetCode 2444）- Hard

#### **四、P6.2 职级**
| 薪资范围 | 经验要求 | 面试周期 | 面试轮数 |
|----------|----------|----------|----------|
| $140K - $180K USD | 7-12 years | 14-21 days | 4 轮 |

- **面试流程**
  1. **Round 1**: Data Structures and Algorithms (60 min) - 核心编码技能
  2. **Round 2**: System Design (60 min) - 复杂系统设计能力
  3. **Round 3**: Behavioral and Cultural Fit (45 min) - 行为和文化契合度
  4. **Round 4**: Managerial Fit (45 min) - 与招聘经理讨论职业目标
- **Commonly Asked Questions（高频考题）**
  1. Number of Islands（LeetCode 200）- Medium
  2. Kth Smallest Element in a Sorted Matrix（LeetCode 378）- Medium
  3. Count Subarrays With Fixed Bounds（LeetCode 2444）- Hard

#### **五、P7.1 职级**
| 薪资范围 | 经验要求 | 面试周期 | 面试轮数 |
|----------|----------|----------|----------|
| $150K - $200K USD | 8-15 years | 14 days | 3 轮 |

- **面试流程**
  1. **Round 1**: Technical Coding Round 1 (60 min) - DSA，算法思维
  2. **Round 2**: System Design Round (60 min) - 可扩展、健壮系统设计
  3. **Round 3**: Behavioral and Managerial (45 min) - 文化契合度、团队合作
- **Example Questions（个人真题）**
  1. 岛屿数量（LeetCode 200）
  2. 有序矩阵中第 K 小的元素（LeetCode 378）
  3. 实现 Trie（前缀树）（LeetCode 208）
  4. N 皇后（LeetCode 51）
- **Commonly Asked Questions（高频考题）**
  1. Number of Islands（LeetCode 200）- Medium
  2. Kth Smallest Element in a Sorted Matrix（LeetCode 378）- Medium
  3. Count Subarrays With Fixed Bounds（LeetCode 2444）- Hard

#### **六、P7.3 职级**
| 薪资范围 | 经验要求 | 面试周期 | 面试轮数 |
|----------|----------|----------|----------|
| $150K - $200K USD | 10+ years | 14-21 days | 4 轮 |

- **面试流程**
  1. **Round 1**: Data Structures and Algorithms (60 min) - 问题解决和算法能力
  2. **Round 2**: System Design and Architecture (60 min) - 可扩展系统设计
  3. **Round 3**: Behavioral and Cultural Fit (45 min) - 行为和文化契合度
  4. **Round 4**: Hiring Manager Discussion (45 min) - 职业目标、领导力、战略对齐
- **Commonly Asked Questions（高频考题）**
  1. Number of Islands（LeetCode 200）- Medium
  2. Kth Smallest Element in a Sorted Matrix（LeetCode 378）- Medium
  3. Count Subarrays With Fixed Bounds（LeetCode 2444）- Hard

---

### 3. 题型与考察频次汇总（来源：hw.glich.co）

| 题型 | 题目 | 难度 | 考察频次/职级 |
| :--- | :--- | :--- | :--- |
| **数组/滑动窗口** | Number of Islands (LC 200) | Medium | **全职级高频** (P4-P7.3) |
| | Kth Smallest in Sorted Matrix (LC 378) | Medium | **全职级高频** (P4-P7.3) |
| | Count Subarrays With Fixed Bounds (LC 2444) | Hard | **全职级高频** (P4-P7.3) |
| | 两数之和 (LC 1) | Easy | P4, P5 (个人真题) |
| | 合并区间 (LC 56) | Medium | P6.1 (个人真题) |
| | 寻找两个正序数组的中位数 (LC 4) | Hard | P6.1 (个人真题) |
| **链表** | 合并有序链表 (LC 21) | Easy | P4 (个人真题) |
| | 反转链表 (LC 206) | Easy | P5 (个人真题) |
| **树/图/搜索** | 验证 BST (LC 98) | Medium | P5 (个人真题) |
| | 实现 Trie (LC 208) | Medium | P6.1, P7.1 (个人真题) |
| | N 皇后 (LC 51) | Hard | P7.1 (个人真题) |
| **动态规划** | 编辑距离 (LC 72) | Medium | P6.1 (个人真题) |
| **C++ 基础** | emplace_back vs push_back | - | P4 (个人真题) |
| | 线程安全单例模式 | - | P4 (个人真题) |

---

### 4. 官方职位要求（来源：OKX Greenhouse JD）

**C++ Trading Services Senior Engineer**
- 链接：https://job-boards.greenhouse.io/okx/jobs/6615074003

**职位要求原文**：
- Bachelor's degree in Computer Science, Software Engineering, or a related technical field
- **10+ years** of software development experience
- Extensive experience in designing, building, and operating **high-performance, ultra-low-latency distributed systems** in financial services or trading
- Proficient in **C/C++ programming** and software design skills, including debugging, performance profiling, and test design
- Familiarity with **networking and I/O on Linux**; knowledge of performance optimization techniques and tools on Linux platforms

**C++ 一般职位**：
- 链接：https://job-boards.greenhouse.io/okx/jobs/6314612003

**Java/C++ Matching Engine**：
- 链接：https://startup.jobs/staff-software-engineer-matching-engine-java-c-okx-6624955
- 要求：低延迟、高吞吐量系统开发经验

### 5. 面试难度（来源：一亩三分地 Job多多）

- 链接：https://jobs.1point3acres.com/companies/okx/interview
- 难度评分：**1.3 / 5**（偏简单）
- 样本数：3 人

---

## 📊 薪资范围（来源：Levels.fyi / Glassdoor）

| 级别 | 年薪 (HKD) | 来源 |
|------|-----------|------|
| P4 (Entry) | ~463K | Levels.fyi |
| P5 (Mid) | ~720K | Levels.fyi |
| P6.1 (Senior) | ~1.08M | Levels.fyi |
| 平均 | 420K - 644K | Glassdoor |

---

## 🎯 本轮面试预期（一面）

根据你收到的邀请函 + hw.glich.co 的流程信息：

| 时间 | 内容 | 说明 |
|------|------|------|
| 0-5min | 自我介绍 | 简洁突出亮点 |
| 5-10min | 项目经验 | 可能会深入问 |
| **10-45min** | **Coderpad 编程** | Medium 难度算法题 |
| 45-55min | 技术问答 | C++ / 系统设计 |
| 55-60min | Q&A | 向面试官提问 |

---

## 💻 算法题准备（重点！）

### 真实考过的题目

根据 hw.glich.co 记录，OKX 面试出现过：

**1. Number of Islands (LC 200)** ⭐ 必刷

```cpp
// 给定 m x n 的 '1'(陆地) 和 '0'(水) 组成的网格，计算岛屿数量
class Solution {
public:
    int numIslands(vector<vector<char>>& grid) {
        if (grid.empty()) return 0;
        
        int count = 0;
        int m = grid.size(), n = grid[0].size();
        
        for (int i = 0; i < m; i++) {
            for (int j = 0; j < n; j++) {
                if (grid[i][j] == '1') {
                    count++;
                    dfs(grid, i, j);
                }
            }
        }
        return count;
    }
    
private:
    void dfs(vector<vector<char>>& grid, int i, int j) {
        if (i < 0 || i >= grid.size() || 
            j < 0 || j >= grid[0].size() || 
            grid[i][j] != '1') {
            return;
        }
        
        grid[i][j] = '0';  // 标记已访问
        dfs(grid, i + 1, j);
        dfs(grid, i - 1, j);
        dfs(grid, i, j + 1);
        dfs(grid, i, j - 1);
    }
};
```

**时间复杂度**: O(m × n)
**空间复杂度**: O(m × n) 最坏情况递归栈

---

**2. Kth Smallest Element in a Sorted Matrix (LC 378)** ⭐ 必刷

```cpp
// 给定 n x n 矩阵，每行每列升序，返回第 k 小元素
class Solution {
public:
    int kthSmallest(vector<vector<int>>& matrix, int k) {
        int n = matrix.size();
        int left = matrix[0][0];
        int right = matrix[n-1][n-1];
        
        while (left < right) {
            int mid = left + (right - left) / 2;
            int count = countLessOrEqual(matrix, mid);
            
            if (count < k) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }
        return left;
    }
    
private:
    int countLessOrEqual(vector<vector<int>>& matrix, int target) {
        int n = matrix.size();
        int count = 0;
        int row = n - 1, col = 0;
        
        while (row >= 0 && col < n) {
            if (matrix[row][col] <= target) {
                count += row + 1;
                col++;
            } else {
                row--;
            }
        }
        return count;
    }
};
```

**时间复杂度**: O(n × log(max - min))
**空间复杂度**: O(1)

---

### 📝 其他建议刷的 Medium 题目（通用准备）

基于交易系统场景和常见面试题型：

| 题目 | LeetCode | 场景 |
|------|----------|------|
| **LRU Cache** | LC 146 | 缓存系统 |
| **Sliding Window Maximum** | LC 239 | 行情处理 |
| **Top K Frequent Elements** | LC 347 | 数据统计 |
| **Merge K Sorted Lists** | LC 23 | 多源数据合并 |
| **Design Hit Counter** | LC 362 | 流量统计 |
| **Binary Search** | LC 704 | 基础必会 |
| **Two Sum** | LC 1 | 热身题 |

---

## 🔧 C++ 技术问答准备（📝 通用准备）

### 必须掌握的知识点

#### 1. 智能指针

```cpp
// unique_ptr - 独占所有权
std::unique_ptr<Order> order = std::make_unique<Order>();

// shared_ptr - 共享所有权
std::shared_ptr<OrderBook> book = std::make_shared<OrderBook>();

// weak_ptr - 打破循环引用
std::weak_ptr<Node> parent;
```

**面试常问**：
- `shared_ptr` 是线程安全的吗？
  - 引用计数操作是原子的 ✅
  - 对象本身的访问不是线程安全的 ❌

#### 2. 移动语义

```cpp
// std::move vs std::forward
std::move(x);     // 无条件转为右值
std::forward<T>(x); // 保持原有值类别（完美转发）

// 移动构造函数
Order(Order&& other) noexcept 
    : data_(std::move(other.data_)) {}
```

**面试常问**：
- 移动后的对象处于什么状态？
  - "有效但未指定"状态，通常为空

#### 3. 内存序 (Memory Order)

```cpp
std::atomic<int> x{0};

// relaxed - 只保证原子性
x.store(1, std::memory_order_relaxed);

// acquire/release - 同步语义
x.store(1, std::memory_order_release);
int val = x.load(std::memory_order_acquire);

// seq_cst - 顺序一致性（最严格，默认）
x.store(1);  // 默认 seq_cst
```

#### 4. 虚函数机制

```cpp
class Base {
public:
    virtual void func() { }  // vtable 条目
    virtual ~Base() = default;
};

class Derived : public Base {
public:
    void func() override { }  // 覆盖 vtable 条目
};
```

**面试常问**：
- 虚函数调用的开销？
  - vtable 查找（一次内存访问）+ 间接调用
  - 比直接调用慢约 10-20%

---

## 🏗️ 系统设计准备（📝 通用准备）

### 交易系统核心概念

**关键词**（官方 JD 中提到的）：
- Low latency（低延迟）
- High availability（高可用）
- Scalability（可扩展性）
- High-performance distributed systems

### 可能的系统设计问题

1. **设计一个订单簿 (Order Book)**
2. **设计一个撮合引擎 (Matching Engine)**
3. **设计一个低延迟消息队列**
4. **如何优化交易系统的延迟？**

### 订单簿基本结构

```cpp
class OrderBook {
private:
    // 买单：价格从高到低
    std::map<double, std::list<Order>, std::greater<double>> bids_;
    // 卖单：价格从低到高
    std::map<double, std::list<Order>> asks_;
    
public:
    double getBestBid() const {
        return bids_.empty() ? 0 : bids_.begin()->first;
    }
    
    double getBestAsk() const {
        return asks_.empty() ? 0 : asks_.begin()->first;
    }
};
```

### 低延迟优化技术

| 技术 | 说明 |
|------|------|
| **Lock-free** | 无锁数据结构，避免锁竞争 |
| **CPU Affinity** | 绑定核心，减少上下文切换 |
| **Cache-line Alignment** | 64 字节对齐，避免 false sharing |
| **Kernel Bypass** | DPDK/RDMA，绕过内核网络栈 |
| **TCP_NODELAY** | 禁用 Nagle 算法 |

---

## ❓ 向面试官提问

准备 3-5 个问题：

### 关于团队
- 团队目前有多少人？C++ 和 Java 的比例？
- 正在从 Java 迁移到 C++ 吗？进度如何？

### 关于技术
- 目前系统的 P99 延迟是多少？
- 使用什么网络框架？（自研？Boost.Asio？）
- 撮合引擎是单线程还是多线程？

### 关于业务
- 团队目前最大的技术挑战是什么？
- 新人入职后的培训流程？

---

## 📝 面试当天 Checklist

### 技术准备
- [ ] 测试 Zoom：https://okx.zoom.us/j/92930492275
- [ ] 测试 Coderpad：https://app.coderpad.io/CXPHA2HA
- [ ] 备用设备准备好（手机 + 移动网络）
- [ ] 耳机麦克风测试

### 环境准备
- [ ] 安静房间
- [ ] 光线充足，避免背光
- [ ] 网络稳定

### 时间
- [ ] **14:50** 进入 Zoom 等候
- [ ] 面试时间：**15:00 - 16:00 HKT**

### 紧急联系
- 面试官电话：+8615989422206

---

## 🎯 最后 30 分钟复习清单

### 必刷算法题 ⭐⭐⭐
1. **Number of Islands** (LC 200) - 真实考过
2. **Kth Smallest in Sorted Matrix** (LC 378) - 真实考过
3. **LRU Cache** (LC 146) - 高频题

### C++ 必记要点
1. `shared_ptr` 引用计数线程安全，对象访问不安全
2. `std::move` 是类型转换，不实际移动
3. `memory_order_acquire/release` 同步语义
4. 虚函数通过 vtable 实现，有间接调用开销

### 面试心态
- 边写边说，展示思路
- 不会的题说思考过程
- 时间复杂度主动分析

---

## 📚 信息来源汇总

| 来源 | 链接 | 信息类型 |
|------|------|---------|
| Hello World | hw.glich.co/resources/companies/okx | 面试流程、算法题 |
| OKX Greenhouse | job-boards.greenhouse.io/okx | 官方职位要求 |
| 一亩三分地 | jobs.1point3acres.com/companies/okx | 面试难度评分 |
| Levels.fyi | levels.fyi | 薪资数据 |
| Glassdoor | glassdoor.com | 薪资数据 |

---

**面试时间**: 2026年1月14日 15:00 HKT（明天下午3点）

祝你好运！🍀
