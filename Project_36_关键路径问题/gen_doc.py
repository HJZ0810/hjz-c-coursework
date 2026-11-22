#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
gen_doc.py - 关键路径方法 V2.0 文档生成脚本
功能：在原docx文档开头插入V2.0升级说明，并以蓝色高亮标记新特性
使用：python gen_doc.py
"""

import os
import sys
import shutil
from datetime import datetime

try:
    from docx import Document
    from docx.shared import Pt, RGBColor, Inches
    from docx.enum.text import WD_ALIGN_PARAGRAPH, WD_COLOR_INDEX
    HAS_DOCX = True
except ImportError:
    HAS_DOCX = False
    print("[警告] 未安装 python-docx，将使用文本模式追加更新说明到 README.md")
    print("       可通过 pip install python-docx 安装以启用完整docx编辑功能")


DOCX_FILE = "Project_36_关键路径问题.docx"
BACKUP_FILE = "Project_36_关键路径问题_V1_backup.docx"
README_FILE = "README.md"


V20_FEATURES = [
    ("UTF-8 BOM 编码", "文件头加入UTF-8 BOM，配合SetConsoleOutputCP(65001)完美支持中文显示"),
    ("ANSI 彩色终端", "启用Windows虚拟终端处理，输出带颜色的日志/结果/错误信息"),
    ("CLI 命令行接口", "支持 -h/--help 查看帮助，-t/--test 运行自测套件"),
    ("动态内存管理", "顶点使用动态数组（自动扩容，初始16，2倍增长），边使用动态链表"),
    ("统一错误码 enum CpmStatus", "20种错误码枚举，所有函数返回CpmStatus统一错误处理"),
    ("CPMV2 二进制持久化", "带magic header \"CPMV2\" + 版本号的二进制文件格式，高效存储"),
    ("内置25项自测套件", "覆盖关键路径计算（8种图）、环检测（三角环/自环）、文件I/O、参数校验、动态扩容"),
    ("交互菜单重构", "8个菜单项：加载示例/手动构建/加载/保存/显示/计算/清空/查找"),
    ("DFS关键路径追踪", "递归DFS遍历输出完整关键路径，多关键路径自动分支显示"),
    ("C99标准零警告", "使用 D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 编译零警告"),
]


def update_readme():
    """更新README.md添加V2.0说明"""
    readme_path = README_FILE
    v2_header = """
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
"""
    with open(readme_path, "w", encoding="utf-8") as f:
        f.write(v2_header)
    print(f"[OK] README.md 已更新为V2.0版本")


def update_docx():
    """更新docx文档，添加蓝色高亮V2.0说明"""
    if not HAS_DOCX:
        print("[跳过] python-docx 未安装，docx文档未修改（已备份V1版本）")
        return

    if not os.path.exists(DOCX_FILE):
        print(f"[错误] 找不到文件 {DOCX_FILE}")
        return

    try:
        doc = Document(DOCX_FILE)

        # 在文档最前面插入V2.0升级说明
        # 先添加一些空行然后在开头插入（通过在第一个段落前插入元素）

        # 创建V2.0标题
        h = doc.add_heading("V2.0 升级说明", level=1)
        # 将标题移动到文档开头（将最后插入的元素移到body[0]之前）
        # python-docx没有直接的insert_at方法，我们用XML操作
        body = doc.element.body
        v20_section_elements = []

        def add_blue_para(text, bold=False, size=11):
            p = doc.add_paragraph()
            run = p.add_run(text)
            run.font.color.rgb = RGBColor(0, 0, 255)
            run.font.highlight_color = WD_COLOR_INDEX.TURQUOISE
            run.font.bold = bold
            run.font.size = Pt(size)
            v20_section_elements.append(p._element)
            return p

        # 在末尾添加内容后，我们会把整个V2区块移到前面
        title = doc.add_paragraph()
        title_run = title.add_run("【关键路径方法 V2.0 升级特性 - 蓝色高亮标记】")
        title_run.font.bold = True
        title_run.font.size = Pt(14)
        title_run.font.color.rgb = RGBColor(0, 0, 255)
        title_run.font.highlight_color = WD_COLOR_INDEX.YELLOW
        v20_section_elements.append(title._element)

        date_p = doc.add_paragraph()
        dr = date_p.add_run(f"升级日期：{datetime.now().strftime('%Y-%m-%d')}    编译：D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2    测试：25项全部PASS")
        dr.font.color.rgb = RGBColor(0, 0, 200)
        dr.font.size = Pt(10)
        v20_section_elements.append(date_p._element)

        doc.add_paragraph()  # 空行

        for feat, desc in V20_FEATURES:
            p = doc.add_paragraph()
            r1 = p.add_run(f"  ✅ {feat}：")
            r1.font.bold = True
            r1.font.color.rgb = RGBColor(0, 0, 255)
            r1.font.highlight_color = WD_COLOR_INDEX.BRIGHT_GREEN
            r2 = p.add_run(desc)
            r2.font.color.rgb = RGBColor(0, 0, 200)
            v20_section_elements.append(p._element)

        sep = doc.add_paragraph()
        sr = sep.add_run("─" * 60)
        sr.font.color.rgb = RGBColor(100, 100, 255)
        v20_section_elements.append(sep._element)
        doc.add_paragraph()

        # 将V2区块从末尾移到文档最前面
        first = body[0]
        for elem in reversed(v20_section_elements):
            body.remove(elem)
            first.addprevious(elem)

        doc.save(DOCX_FILE)
        print(f"[OK] {DOCX_FILE} 已更新，V2.0特性以蓝色高亮标记，V1版本已备份为 {BACKUP_FILE}")

    except Exception as e:
        print(f"[错误] 更新docx失败: {e}")
        import traceback
        traceback.print_exc()


def main():
    print("=" * 60)
    print("  关键路径方法 V2.0 文档生成脚本")
    print("=" * 60)

    # 备份检查
    if not os.path.exists(BACKUP_FILE) and os.path.exists(DOCX_FILE):
        shutil.copy2(DOCX_FILE, BACKUP_FILE)
        print(f"[备份] {DOCX_FILE} -> {BACKUP_FILE}")

    update_readme()
    update_docx()

    print()
    print("=" * 60)
    print("  文档处理完成")
    print("=" * 60)
    print(f"  ✅ 可执行文件: cpm.exe")
    print(f"  ✅ 源代码:     main.c (UTF-8 BOM)")
    print(f"  ✅ 编译脚本:   Makefile")
    print(f"  ✅ 文档脚本:   gen_doc.py")
    print(f"  ✅ V1备份:     {BACKUP_FILE}")
    print()
    print("  快速验证命令：")
    print("    ./cpm.exe -h        # 查看帮助")
    print("    ./cpm.exe --test    # 运行25项自测")
    print("    make                # 重新编译")


if __name__ == "__main__":
    main()
