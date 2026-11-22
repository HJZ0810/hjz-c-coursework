<div align="center">

# 🏪 学校超市选址问题 V2.0

### 📅 2025-2026学年第二学期算法课程设计

![Language](https://img.shields.io/badge/Language-C99-blue?style=for-the-badge&logo=c&logoColor=white)
![Data Structure](https://img.shields.io/badge/Data_Structure-Dynamic_Arrays-green?style=for-the-badge)
![Algorithm](https://img.shields.io/badge/Algorithm-Floyd_Warshall-orange?style=for-the-badge)
![Status](https://img.shields.io/badge/Status-V2.0_Completed-brightgreen?style=for-the-badge)
![Project](https://img.shields.io/badge/Project-37-red?style=for-the-badge)
![Tests](https://img.shields.io/badge/Tests-38_PASS-success?style=for-the-badge)

---

**👤 作者：HuiJunzhuo**
**🎓 班级：计算机科学与技术A班**
**📅 更新日期：2026-06-29**

---

*基于Floyd-Warshall算法与动态数组实现的学校超市选址系统V2.0*

</div>

---

## 📋 目录

- [1. 项目概述](#1-项目概述)
- [2. V2.0 新特性](#2-v20-新特性)
- [3. 功能模块](#3-功能模块)
- [4. 核心算法](#4-核心算法)
- [5. 数据结构](#5-数据结构)
- [6. 编译与运行](#6-编译与运行)
- [7. 测试报告](#7-测试报告)
- [8. 文件说明](#8-文件说明)

---

## 1. 项目概述

### 1.1 问题背景

校园规划中，超市选址是一个典型的优化问题。选址应使师生前往超市的总距离最小或最远点距离最小。各建筑物人流量不同，需用**权重**表示不同区域的人流密度，以体现人流密集区域（如宿舍区）的便利优先级。

### 1.2 项目简介

本项目使用C语言实现学校超市选址系统，将校园建模为带权无向图，顶点表示建筑物，边表示道路。采用 **Floyd-Warshall算法** 计算所有点对最短路径，提供两种选址策略：

- **中位点（Median）**：最小化加权距离总和
- **中心点（Center）**：最小化最大距离（偏心距）

### 1.3 V1.0 vs V2.0

| 特性 | V1.0 | V2.0 |
|:---|:---|:---|
| 内存管理 | 静态数组(MAX=50) | **动态数组自动扩容** |
| 编码支持 | GBK/ANSI | **UTF-8 BOM + SetConsoleOutputCP(65001)** |
| 终端显示 | 黑白文本 | **ANSI彩色终端** |
| CLI参数 | 无 | **-h/--help, -t/--test** |
| 选址算法 | 中位点 | **中位点+中心点双算法** |
| 错误处理 | 返回int | **统一LocStatus枚举** |
| 文件格式 | 二进制无标识 | **LOCV2魔数+版本号** |
| 自测用例 | 12项 | **38项全自动化测试** |
| 可执行文件 | main.exe | **loc.exe** |

---

## 2. V2.0 新特性 🔵

### 2.1 UTF-8 全链路支持
- 源文件以UTF-8 BOM编码保存
- Windows下调用 `SetConsoleOutputCP(65001)` 设置控制台输出为UTF-8
- 启用VT100虚拟终端模式以支持ANSI颜色

### 2.2 ANSI彩色终端
- 🔴 红色：错误信息
- 🟢 绿色：成功信息
- 🟡 黄色：提示信息
- 🔵 蓝色：标题加粗
- 🟣 洋红：菜单边框

### 2.3 双选址算法

**中位点算法（Median）**：
```
cost(i) = Σ dist[i][j] × weight[j]  (所有j≠i)
目标：minimize cost(i)
适用：人流量优先的场景
```

**中心点算法（Center）**：
```
ecc(i) = max{ dist[i][j] }  (所有j≠i)
目标：minimize ecc(i)
适用：公平性/紧急服务场景
```

### 2.4 动态内存管理
- 初始容量：8个地点、16条道路
- 容量不足时自动翻倍扩容（realloc）
- 邻接矩阵同步扩容
- 程序退出时完全释放内存，无泄漏

### 2.5 LOCV2 文件格式
```
[0-4]   "LOCV2" 魔数标识
[5-8]   版本号 (int = 2)
[9-12]  地点数量 (int)
[13-16] 道路数量 (int)
[...]   地点数据：id(4B) + name(48B) + weight(4B) + type(4B) + active(4B)
[...]   道路数据：from(4B) + to(4B) + distance(4B)
```

### 2.6 统一错误码
```c
typedef enum {
    LOC_OK = 0,
    LOC_ERR_MEMORY = -1,    // 内存分配失败
    LOC_ERR_FILE = -2,      // 文件操作失败
    LOC_ERR_MAGIC = -3,     // 魔数错误
    LOC_ERR_VERSION = -4,   // 版本不兼容
    LOC_ERR_FULL = -5,      // 容量满
    LOC_ERR_NOT_FOUND = -6, // 未找到
    LOC_ERR_INVALID_PARAM = -7, // 参数无效
    LOC_ERR_IO = -8,        // I/O错误
    LOC_ERR_DISCONNECTED = -9  // 图不连通
} LocStatus;
```

---

## 3. 功能模块

| 编号 | 功能 | 说明 |
|:---:|:---|:---|
| 1 | 查看建筑物 | 彩色表格显示名称、类型、人流量权重 |
| 2 | 查看道路 | 显示所有双向道路及距离 |
| 3 | 添加建筑物 | 交互式输入名称/类型/权重 |
| 4 | 添加道路 | 选择两点并输入距离 |
| 5 | 距离矩阵 | Floyd计算后显示n×n最短路径矩阵 |
| 6 | 中位点选址 | 加权距离总和最小，显示排名 |
| 7 | 中心点选址 | 最大距离最小，显示排名 |
| 8 | 重置数据 | 恢复默认12建筑28道路校园 |
| 9 | 保存数据 | 手动保存到campus.locv2 |
| 0 | 退出 | 自动保存数据 |

---

## 4. 核心算法

### 4.1 Floyd-Warshall 全源最短路径

**时间复杂度**：O(n³)  
**空间复杂度**：O(n²)

```c
for k = 0 to n-1:           // 中转点
    for i = 0 to n-1:       // 起点
        for j = 0 to n-1:   // 终点
            if dist[i][k] + dist[k][j] < dist[i][j]:
                dist[i][j] = dist[i][k] + dist[k][j]
                next_hop[i][j] = next_hop[i][k]
```

### 4.2 默认校园拓扑

12个建筑物，28条道路，包含：
- 教学楼（3栋）：权重70-80
- 宿舍楼（3栋）：权重180-200（人流最高）
- 食堂（2个）：权重50/150
- 图书馆、实验楼、行政楼、活动中心：权重40-120

---

## 5. 数据结构

```c
// 单个建筑物/地点
typedef struct {
    int id;                  // 内部ID（0-based）
    char name[MAX_NAME_LEN]; // 名称（最长47字符）
    int weight;              // 人流量权重
    LocType type;            // 类型：教学楼/宿舍/食堂/其他
    int active;              // 是否有效
} Location;

// 一条道路
typedef struct {
    int from;                // 起点ID
    int to;                  // 终点ID
    int distance;            // 距离（米）
} Road;

// 图结构（动态数组）
typedef struct {
    Location *locs;          // 地点数组
    int loc_capacity;        // 当前容量
    int loc_count;           // 当前数量
    Road *roads;             // 道路数组
    int road_capacity;
    int road_count;
    int **adj;               // 邻接矩阵（原始边权）
    int **dist;              // 最短路径矩阵
    int **next_hop;          // 路径追踪
    int floyd_computed;      // 是否已计算Floyd
} Graph;
```

---

## 6. 编译与运行

### 6.1 环境要求
- GCC 支持C99标准（MinGW-w64 GCC 14.2.0 测试通过）
- Windows 10/11（支持ANSI彩色终端）
- 无额外依赖库

### 6.2 编译命令

**使用Makefile（推荐）：**
```bash
make              # 编译
make test         # 编译并运行自测
make run          # 编译并运行
make clean        # 清理产物
make help         # 显示帮助
```

**手动编译：**
```bash
D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o loc.exe main.c
```

> ⚠️ 编译标准：`-std=c99 -Wall -Wextra -O2`，要求 **零警告** 编译通过。

### 6.3 运行方式

```bash
loc.exe              # 启动交互式菜单
loc.exe -h           # 显示帮助
loc.exe --help       # 显示帮助
loc.exe -t           # 运行38项自测
loc.exe --test       # 运行38项自测
```

---

## 7. 测试报告 🧪

V2.0包含 **38项自动化测试**，覆盖10个测试组：

| 测试组 | 测试项 | 数量 |
|:---|:---|:---:|
| 图初始化 | graph_init成功/初始计数/初始容量 | 3 |
| 添加地点 | 添加A/B/C栋/数量/ID分配 | 5 |
| 添加道路 | A-B/B-C/A-C/道路计数 | 4 |
| Floyd最短路 | 计算成功/自环/直达/绕路/连通性 | 6 |
| 中位点选址 | 计算成功/正确性/数值/正值 | 4 |
| 中心点选址 | 计算成功/偏心距/最优ID | 3 |
| 文件持久化 | 保存/加载/数量一致/距离正确 | 5 |
| 默认校园 | 12建筑/28道路/Floyd/连通性 | 4 |
| 动态扩容 | 添加50点/总数62/容量增长 | 3 |
| 边界条件 | 无边不连通检测 | 1 |
| **总计** | | **38** |

运行测试：
```bash
loc.exe --test
```

---

## 8. 文件说明

| 文件 | 说明 |
|:---|:---|
| `main.c` | V2.0完整源代码（约1125行） |
| `Makefile` | GCC编译配置（CC=D:/bin/gcc.exe） |
| `loc.exe` | 编译后的可执行文件 |
| `campus.locv2` | 数据持久化文件（LOCV2格式，自动生成） |
| `gen_doc.py` | 文档生成脚本 |
| `README.md` | 本说明文档 |

---

<div align="center">

**🔵 V2.0 更新内容蓝色高亮标记**

**👤 HuiJunzhuo** | **🎓 HuiJunzhuo** | **📅 2026-06-29**

*学校超市选址系统 V2.0 - 算法课程设计项目*

</div>
