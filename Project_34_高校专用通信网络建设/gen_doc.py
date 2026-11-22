#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
高校专用通信网络建设系统 V2.0 - 文档生成脚本
生成Markdown格式的项目文档，可转换为Word/PDF
"""

import os
import sys
from datetime import datetime

def generate_markdown():
    """生成V2.0版本文档"""
    md = f"""# 高校专用通信网络建设系统 V2.0

### 2025-2026学年第二学期 算法课程设计

![Language](https://img.shields.io/badge/Language-C99-blue?style=for-the-badge&logo=c&logoColor=white)
![Version](https://img.shields.io/badge/Version-V2.0-green?style=for-the-badge)
![Status](https://img.shields.io/badge/Status-Completed-brightgreen?style=for-the-badge)
![Tests](https://img.shields.io/badge/Tests-19%20Passed-success?style=for-the-badge)

---

## V2.0 升级亮点 (蓝色高亮标注)

<span style="color:blue; font-weight:bold">✓ UTF-8 BOM编码支持</span>
<span style="color:blue; font-weight:bold">✓ SetConsoleOutputCP(65001) Windows控制台UTF-8设置</span>
<span style="color:blue; font-weight:bold">✓ ANSI彩色终端输出(7种颜色+粗体)</span>
<span style="color:blue; font-weight:bold">✓ CLI命令行参数: -h/--help, -t/--test</span>
<span style="color:blue; font-weight:bold">✓ Prim + Kruskal 双MST算法实现</span>
<span style="color:blue; font-weight:bold">✓ 动态内存管理(顶点/边动态数组，自动扩容)</span>
<span style="color:blue; font-weight:bold">✓ 统一错误码枚举 NetStatus (14种状态)</span>
<span style="color:blue; font-weight:bold">✓ "NETWV2"魔数二进制文件持久化格式</span>
<span style="color:blue; font-weight:bold">✓ 19项内置自测(MST+文件I/O+内存管理)</span>
<span style="color:blue; font-weight:bold">✓ 交互式彩色菜单系统</span>
<span style="color:blue; font-weight:bold">✓ 零警告编译 (GCC -std=c99 -Wall -Wextra -O2)</span>
<span style="color:blue; font-weight:bold">✓ 完整中文注释</span>

---

## 目录

1. [项目背景](#1-项目背景)
2. [系统架构](#2-系统架构)
3. [核心数据结构](#3-核心数据结构)
4. [算法实现](#4-算法实现)
5. [文件格式](#5-文件格式)
6. [编译与运行](#6-编译与运行)
7. [测试报告](#7-测试报告)
8. [使用说明](#8-使用说明)

---

## 1. 项目背景

中国移动安北省分公司计划为省内12所高校建设专用通信网络。需要在所有高校之间铺设通信光缆，使总建设成本最小。

本项目使用图论中带权无向图模型，求解最小生成树(Minimum Spanning Tree, MST)问题。

**V2.0版本**在V1基础上进行了全面重构：
- 从固定大小数组升级为动态内存管理
- 新增Prim算法实现
- 新增ANSI彩色终端
- 新增CLI参数支持
- 新增二进制文件持久化
- 新增19项自动化测试
- 统一错误处理机制

---

## 2. 系统架构

```
┌─────────────────────────────────────────────────────┐
│              CLI参数解析层 (-h/-t)                  │
├─────────────────────────────────────────────────────┤
│              交互式菜单层 (彩色UI)                   │
├─────────────────────────────────────────────────────┤
│     MST算法层 (Prim O(V²) / Kruskal O(E log E))     │
├─────────────────────────────────────────────────────┤
│        数据结构层 (动态图/并查集/邻接表)             │
├─────────────────────────────────────────────────────┤
│           I/O层 ("NETWV2"魔数二进制文件)            │
└─────────────────────────────────────────────────────┘
```

---

## 3. 核心数据结构

### 3.1 NetStatus错误码枚举

```c
typedef enum {{
    NET_OK = 0,                 // 操作成功
    NET_ERR_MEMORY,             // 内存分配失败
    NET_ERR_FILE_OPEN,          // 无法打开文件
    NET_ERR_FILE_READ,          // 文件读取失败
    NET_ERR_FILE_WRITE,         // 文件写入失败
    NET_ERR_MAGIC,              // 文件魔数校验失败
    NET_ERR_VERTEX_EXISTS,      // 顶点已存在
    NET_ERR_VERTEX_NOT_FOUND,   // 顶点不存在
    NET_ERR_EDGE_EXISTS,        // 边已存在
    NET_ERR_EDGE_NOT_FOUND,     // 边不存在
    NET_ERR_INVALID_PARAM,      // 无效参数
    NET_ERR_NO_MST,             // 图不连通,无MST
    NET_ERR_INDEX_OUT_OF_RANGE, // 索引越界
    NET_ERR_INTERNAL            // 内部错误
}} NetStatus;
```

### 3.2 Graph动态图结构

```c
typedef struct {{
    int vCap, vNum, eNum;    // 顶点容量/数量, 边数量
    char **names;            // 顶点名称动态数组
    int **mat;               // 邻接矩阵(动态二维数组)
    AdjEdge **adj;           // 邻接表链表数组
}} Graph;
```

**V2.0动态内存特性:**
- 初始容量: 16顶点
- 扩容策略: 容量翻倍 (类似C++ vector)
- 邻接矩阵行随扩容自动realloc

---

## 4. 算法实现

### 4.1 Prim算法

**思想**: 从顶点0出发，每次选取连接已选集合与未选集合的最小权边
**复杂度**: O(V²)
**适用场景**: 稠密图

```c
static NetStatus mstPrim(const Graph *g, Edge *mst, int *total);
```

### 4.2 Kruskal算法

**思想**: 将所有边按权值排序，依次选取不构成环的边（并查集判环）
**复杂度**: O(E log E)（排序主导）
**适用场景**: 稀疏图

```c
static NetStatus mstKruskal(const Graph *g, Edge *mst, int *total);
```

### 4.3 并查集(Union-Find)

**优化**: 路径压缩 + 按秩合并
**复杂度**: 近似O(1) per operation

---

## 5. 文件格式

V2.0使用二进制格式持久化，带有魔数校验：

| 偏移 | 长度 | 内容 |
|:---:|:---:|:---|
| 0 | 6字节 | **"NETWV2"** 魔数标识 |
| 6 | 4字节 | 顶点数 V (uint32 LE) |
| 10 | 4字节 | 边数 E (uint32 LE) |
| 14+ | - | V个顶点: 1字节长度 + UTF-8名称 |
| - | - | E条边: u(int32) + v(int32) + w(int32) |

---

## 6. 编译与运行

### 6.1 Makefile

```makefile
CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = net.exe
```

### 6.2 编译命令

```bash
# 使用make
mingw32-make

# 或直接编译
D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o net.exe main.c
```

### 6.3 运行方式

```bash
# 显示帮助
./net.exe -h
./net.exe --help

# 运行内置测试
./net.exe -t
./net.exe --test

# 交互式菜单
./net.exe
```

---

## 7. 测试报告

**测试总数: 19项，全部通过**

### 数据结构与内存管理 (4项)
- T01: 图创建成功
- T02: 添加单个顶点成功
- T03: 重复顶点检测
- T04: 越界边参数检测

### MST算法 (9项)
- T05: Prim 3顶点图MST计算成功
- T06: Prim 3顶点总权值=3 (1+2)
- T07: Kruskal 3顶点图MST计算成功
- T08: Kruskal 3顶点总权值=3 (与Prim一致)
- T09: Prim 12顶点完整图MST成功
- T10: Kruskal 12顶点完整图MST成功
- T11: Prim与Kruskal总权值一致
- T12: 双顶点图MST成功
- T13: 双顶点总权值=42

### 文件I/O与持久化 (6项)
- T14: NETWV2格式文件保存成功
- T15: NETWV2格式文件加载成功
- T16: 加载后顶点数一致
- T17: 加载后边数一致
- T18: 持久化前后MST总权值一致
- T19: 不存在文件加载错误处理

---

## 8. 使用说明

### 8.1 交互式菜单选项

| 选项 | 功能 |
|:---:|:---|
| 1 | 显示所有高校与线路信息 |
| 2 | Prim算法计算MST |
| 3 | Kruskal算法计算MST |
| 4 | 修改/添加线路成本 |
| 5 | 保存数据到文件(NETWV2格式) |
| 6 | 从文件加载数据(NETWV2格式) |
| 7 | 重置为默认数据(12高校/25线路) |
| 8 | 对比Prim与Kruskal结果 |
| 0 | 退出系统 |

### 8.2 默认数据

默认加载12所高校、25条通信线路的初始数据：

**安北市区:** 安北工业大学、北方大学、空科大学
**东山:** 东山学院、南宛职业大学
**枣强县:** 东山大学
**桐济:** 安北大学、安北农业大学
**石板房:** 石板房职业大学、安北师范大学、安北技工大学、安北健康护理大学

---

*文档生成时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}*
*V2.0 - 高校专用通信网络建设系统*
"""
    return md

def main():
    print("=" * 60)
    print("  高校专用通信网络建设系统 V2.0 - 文档生成工具")
    print("=" * 60)

    out_md = "README_V2.md"
    
    content = generate_markdown()
    
    with open(out_md, "w", encoding="utf-8") as f:
        f.write(content)
    
    print(f"\n[OK] Markdown文档已生成: {out_md}")
    print("\n要生成Word文档，可以使用pandoc:")
    print(f"  pandoc {out_md} -o Project_34_V2.docx")
    print("\nV2.0蓝色高亮更新项已在文档中标注。")

if __name__ == "__main__":
    main()
