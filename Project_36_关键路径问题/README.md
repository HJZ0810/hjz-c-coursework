
<div align="center">

# 🔗 关键路径方法 V2.0

### 📅 2025-2026学年第二学期算法课程设计

![Language](https://img.shields.io/badge/Language-C99-blue?style=for-the-badge&logo=c&logoColor=white)
![Version](https://img.shields.io/badge/Version-V2.0-brightgreen?style=for-the-badge)
![Data Structure](https://img.shields.io/badge/Data_Structure-Dynamic_Adjacency_List-green?style=for-the-badge)
![Status](https://img.shields.io/badge/Status-Completed-brightgreen?style=for-the-badge)
![Tests](https://img.shields.io/badge/Tests-25_PASS-orange?style=for-the-badge)
![Compiler](https://img.shields.io/badge/Compiler-GCC_MinGW-red?style=for-the-badge)

---

**👤 作者：数据结构**
**🏫 班级：计算机类A班**
**✍️ 姓名：HuiJunzhuo**

---

*基于动态邻接表实现的关键路径方法 (Critical Path Method) V2.0*

</div>

---

## 📢 V2.0 升级亮点

| 新特性 | 说明 |
|:---|:---|
| ✅ UTF-8 BOM + SetConsoleOutputCP(65001) | 完美中文支持 |
| ✅ ANSI 彩色终端输出 | Windows VT100虚拟终端，彩色日志 |
| ✅ CLI 命令行 | -h/--help, -t/--test |
| ✅ 动态内存 | 顶点动态数组+边动态链表，自动扩容 |
| ✅ CpmStatus 错误码 | 20种统一枚举错误码 |
| ✅ CPMV2 二进制持久化 | Magic Header "CPMV2" 二进制文件 |
| ✅ 25项内置自测 | 关键路径+环检测+文件I/O+参数校验 |
| ✅ 交互菜单 | 8个菜单，手动建图/文件保存加载 |
| ✅ 编译零警告 | gcc -std=c99 -Wall -Wextra -O2 |
| ✅ Makefile | CC=D:/bin/gcc.exe，一键编译/测试/运行 |

---

## 📁 文件清单

| 文件 | 说明 |
|:---|:---|
| `main.c` | V2.0 完整源代码（UTF-8 BOM，中文注释） |
| `Makefile` | GCC编译脚本（CC=D:/bin/gcc.exe） |
| `cpm.exe` | 编译生成的可执行文件 |
| `gen_doc.py` | 文档生成脚本 |
| `Project_36_关键路径问题.docx` | 课程设计报告文档 |
| `Project_36_关键路径问题_V1_backup.docx` | V1版本文档备份 |
| `cpm_data.bin` | 运行时生成的CPMV2二进制数据文件 |

---

## 🚀 快速开始

### 编译
```powershell
make
# 或直接编译
D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o cpm.exe main.c
```

### 运行自测
```powershell
./cpm.exe --test
# 或 make test
```

### 查看帮助
```powershell
./cpm.exe -h
```

### 交互模式
```powershell
./cpm.exe
# 进入菜单后按 1 加载经典示例，按 6 计算关键路径
```

---

## 🧪 测试套件（25项全通过）

| 编号 | 测试项 | 预期结果 |
|:---:|:---|:---|
| T01 | 空图处理 | 返回 CPM_ERR_EMPTY_GRAPH |
| T02 | 经典10顶点例拓扑排序 | 成功 |
| T03 | 经典例无环 | true |
| T04 | 经典例总工期 | 31 |
| T05 | 经典例有关键活动 | true |
| T06 | 线性链总工期 | 9 |
| T07 | 线性链全部为关键活动 | 3个 |
| T08 | 并行路径总工期 | 6 |
| T09 | 菱形结构总工期 | 4 |
| T10 | 三角环检测 | CPM_ERR_CYCLE_DETECTED |
| T11 | 自环检测 | CPM_ERR_CYCLE_DETECTED |
| T12 | 单顶点总工期 | 0 |
| T13 | 两顶点单边总工期 | 10 |
| T14 | 6顶点教材例总工期 | 9 |
| T15 | 三并行路径总工期 | 5 |
| T16 | 三路径全部为关键活动 | 6个 |
| T17 | 二进制保存 | 成功 |
| T18 | 二进制加载 | 成功 |
| T19 | 加载后顶点/边数一致 | true |
| T20 | 加载后计算总工期仍为31 | true |
| T21 | 错误魔数文件检测 | CPM_ERR_FILE_MAGIC |
| T22 | 首次添加顶点成功 | CPM_OK |
| T23 | 重复顶点返回错误 | CPM_ERR_DUPLICATE_VERTEX |
| T24 | 负权值拒绝 | CPM_ERR_INVALID_WEIGHT |
| T25 | 动态扩容至50顶点 | 成功 |

---

## 🏗️ 系统架构

```
┌──────────────────────────────────────────────────────────┐
│                    CLI / 交互菜单层                        │
│  (-h/--help, -t/--test, 菜单1-8)                         │
├──────────────────────────────────────────────────────────┤
│                    输出显示层 (ANSI彩色)                    │
│  graph_print() / result_print() / dfs_crit_path()        │
├──────────────────────────────────────────────────────────┤
│                    CPM核心算法层                           │
│  cpm_calc() - 拓扑排序(Kahn) + etv/ltv计算 + 关键路径判定  │
├──────────────────────────────────────────────────────────┤
│                    图操作层 (动态内存)                      │
│  graph_init/add_vertex/add_edge/find/ensure_cap/free     │
├──────────────────────────────────────────────────────────┤
│                    持久化层 (CPMV2二进制)                   │
│  graph_save_binary() / graph_load_binary()               │
│  Magic "CPMV2" + Version(2) + Vertices + Edges           │
├──────────────────────────────────────────────────────────┤
│                    错误处理层                              │
│  enum CpmStatus (20种错误码) + status_msg()               │
└──────────────────────────────────────────────────────────┘
```

---

## 📊 核心数据结构

```c
/* 边（活动）- 动态链表节点 */
typedef struct Edge {
    int          to;        // 终点索引
    int          weight;    // 活动持续时间（权值）
    char         name[64];  // 活动名（如a1, a2）
    struct Edge *next;      // 下一条边指针
} Edge;

/* 顶点（事件）- 动态数组元素 */
typedef struct {
    char  name[64];   // 事件名（如V0, V1）
    Edge *first_edge; // 出边链表头指针
} Vertex;

/* AOE 图（动态邻接表）*/
typedef struct {
    Vertex *vertices;   // 顶点动态数组
    int     vert_count; // 当前顶点数
    int     vert_cap;   // 数组容量（自动扩容）
    int     edge_count; // 当前边数
} Graph;

/* CPM计算结果 */
typedef struct {
    int *etv, *ltv;     // 顶点最早/最晚发生时间
    int *topo;          // 拓扑序列
    int  topo_cnt;      // 拓扑顶点数
    int  source, sink;  // 源点/汇点索引
    int  total_duration;// 总工期（关键路径长度）
    int  crit_edge_count;// 关键活动数
} CpmResult;

/* 统一错误码 */
typedef enum {
    CPM_OK = 0,
    CPM_ERR_NULL_PTR, CPM_ERR_MEMORY,
    CPM_ERR_FILE_OPEN, CPM_ERR_FILE_READ, CPM_ERR_FILE_WRITE,
    CPM_ERR_FILE_MAGIC, CPM_ERR_FILE_VERSION, CPM_ERR_FILE_CORRUPT,
    CPM_ERR_VERTEX_NOT_FOUND, CPM_ERR_DUPLICATE_VERTEX, CPM_ERR_DUPLICATE_EDGE,
    CPM_ERR_INVALID_WEIGHT, CPM_ERR_CYCLE_DETECTED, CPM_ERR_EMPTY_GRAPH,
    CPM_ERR_NO_SOURCE, CPM_ERR_NO_SINK, CPM_ERR_NOT_SAVED,
    CPM_ERR_INVALID_INPUT, CPM_ERR_INDEX_OUT_OF_RANGE
} CpmStatus;
```

---

## 🔬 算法原理

**关键路径方法 (CPM)** 四步流程：

1. **拓扑排序**：Kahn算法（入度表+栈），O(V+E)，同时计算 `etv[]`（顶点最早发生时间）
   - `etv[w] = max(etv[w], etv[v] + weight(v,w))`

2. **逆拓扑计算ltv**：从汇点反向遍历，初始化 `ltv[] = 总工期`
   - `ltv[v] = min(ltv[v], ltv[w] - weight(v,w))`

3. **活动时间参数**：
   - `ete = etv[起点]`（最早开始时间）
   - `lte = ltv[终点] - weight`（最晚开始时间）

4. **关键活动判定**：`ete == lte`（时差为0）的活动为关键活动，由关键活动构成的路径即为关键路径。

---

## 💾 CPMV2 二进制文件格式

| 偏移 | 长度 | 内容 | 说明 |
|:---|:---|:---|:---|
| 0 | 5字节 | "CPMV2" | Magic Header魔数标识 |
| 5 | 4字节 | int=2 | 版本号 |
| 9 | 4字节 | int | 顶点数 |
| 13 | 4字节 | int | 边数 |
| 17 | ... | 顶点表 | 每顶点: 1字节名字长度 + UTF-8名字 |
| ... | ... | 边表 | 每顶点: 4字节出边数 + 各边(to/weight/namelen/name) |

---

## 📞 联系方式

| 平台 | 账号/链接 |
|:---:|:---|
| 📱 QQ | `65178267913` |
| 🌐 CSDN | [数据结构的博客](https://blog.csdn.net/hjz0810?type=blog) |

---

> **👤 作者：数据结构**
> **🏫 班级：计算机类A班**
> **✍️ 姓名：HuiJunzhuo**
> **📅 版本：V2.0 | 2025-2026学年第二学期算法课程设计**
