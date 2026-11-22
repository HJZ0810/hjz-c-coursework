<div align="center">

# ? 场地使用申请信息管理系统

### ? 2025-2026学年第二学期算法复习

![Language](https://img.shields.io/badge/Language-C99-blue?style=for-the-badge&logo=c&logoColor=white)
![Data Structure](https://img.shields.io/badge/Data_Structure-Linked_List-green?style=for-the-badge)
![Status](https://img.shields.io/badge/Status-Completed-brightgreen?style=for-the-badge)
![Project](https://img.shields.io/badge/Project-17-orange?style=for-the-badge)

---

**??? 作者：惠俊卓** · **学号：424123888888**
**? 班级：华信竞赛队A部**
**? 审核：HuiJunzhuo**

---

*基于Linked_List实现的场地使用申请信息管理系统*

</div>

---

## ? 目录

- [1. 需求分析](#1-需求分析)
- [2. 概要设计](#2-概要设计)
- [3. 详细设计](#3-详细设计)
- [4. 测试分析](#4-测试分析)
- [5. 总结](#5-总结)
- [? 快速??始](#-快速开始)
- [? 联系作者](#-联系作者)

---

## 1. 需求分析

### 1.1 项目背景

在各类组织和机构中，场地资源（如会议室、体育场馆、活动中心等）的使用需要有序管理。传统的人工登记方式效率低下，无法直观查看场地使用情况，容易出现重复预约和时间冲突等问题。随着场地使用需求的增加，急需一套信息化的管理系统来规范场地申请流程。

本项目旨在运用C语言和数据结构知识，设计并实现一个功能完善的场地使用申请信息管理系统。该系统基于单链表数据结构，支持单位信息管理、场地信息管理、场地使用申请登记与查询以及文件持久化存储等功能。系统通过「代码」字段模拟外键机制关联单位表和场地表，实现申请记录的逻辑关联。

### 1.2 功能需求

| 序号 | 功能模块 | 说明 |
|:---:|:---|:---|
| 1 | 单位信息管理 | 单位的增删改查浏览 |
| 2 | 场地信息管理 | 场地的增删改查浏览 |
| 3 | 申请登记 | 场地使用申请登记 |
| 4 | 申请查询 | 按关键字查询申请 |
| 5 | 数据持久化 | TSV文件存储 |
| 6 | 测试数据 | 内置测试数据加载 |

### 1.3 数据需求

**Unit（单位信息）结构体：**

| 字段 | 类型 | 说明 |
|:---|:---|:---|
| `code` | char[128] | 单位代码 |
| `name` | char[128] | 单位名称 |
| `level` | char[128] | 行政级别 |
| `leader` | char[128] | 负责人 |
| `address` | char[128] | 单位地址 |
| `next` | Unit* | 后继指针 |

**Venue（场地信息）结构体：**

| 字段 | 类型 | 说明 |
|:---|:---|:---|
| `code` | char[128] | 场地代码 |
| `address` | char[128] | 场地地址 |
| `category` | char[128] | 场地分类 |
| `capacity` | int | 容纳人数 |
| `price` | double | 租用价格 |
| `next` | Venue* | 后继指针 |

---

## 2. 概要设计

### 2.1 数据结构设计

系统采用三张单链表作为核心数据结构，分别存储单位信息、场地信息和申请记录。选择单链表的原因：1）插入/删除操作频繁时，时间复杂度为O(1)（已知位置）；2）无需预分配固定大小，内存使用更灵活；3）符合课程设计对数据结构的考察要求。三张表通过「code」字段形成逻辑关联，模拟了关系型数据库的外键机制：申请表中的unit_code和venue_code分别关联到单位表和场地表的主键。

文件持久化采用制表符分隔（TSV）格式，相比CSV的优势：1）单位名称、地址等字段中极可能包含逗号，但不太可能包含制表符；2）TSV格式更简单，无需处理引号转义；3）fscanf的%[]扫描集天然支持制表符分隔。相比二进制格式的优势：1）人可读，便于调试和手动编辑；2）跨平台，不同编译器的结构体内存布局可能不同；3）课程设计中更侧重逻辑正确性而非性能优化。

### 2.2 系统架构

```
┌──────────────────────???──────────────┐
│       表示层（终端菜单界面）         │
├─────────────────────────────────────┤
│    业务逻辑层（CRUD/查询/统计）      │
├─────────────────────────────────────┤
│   数据层（单链表 + 文件I/O）         │
└─────────────────────────────────────┘
```

### 2.3 模块划分

| 模块 | 功能 |
|:---|:---|
| 单位信息管理 | Unit struct CRUD |
| 场地信息管理 | Venue struct CRUD |
| 申请记录管理 | Apply struct 登记查询 |
| 文件持久化 | TSV格式存储 |
| 内置测试数据 | 预置测试数据加载 |
| 菜单系统 | 两级菜单驱动 |

---

## 3. 详细设计

### 3.1 核心数据结构

```c
#
d
e
f
i
n
e
 
M
A
X
_
S
T
R
 
1
2
8




t
y
p
e
d
e
f
 
s
t
r
u
c
t
 
U
n
i
t
 
{


 
 
 
 
c
h
a
r
 
c
o
d
e
[
M
A
X
_
S
T
R
]
;


 
 
 
 
c
h
a
r
 
n
a
m
e
[
M
A
X
_
S
T
R
]
;


 
 
 
 
c
h
a
r
 
l
e
v
e
l
[
M
A
X
_
S
T
R
]
;


 
 
 
 
c
h
a
r
 
l
e
a
d
e
r
[
M
A
X
_
S
T
R
]
;


 
 
 
 
c
h
a
r
 
a
d
d
r
e
s
s
[
M
A
X
_
S
T
R
]
;


 
 
 
 
s
t
r
u
c
t
 
U
n
i
t
 
*
n
e
x
t
;


}
 
U
n
i
t
;




t
y
p
e
d
e
f
 
s
t
r
u
c
t
 
V
e
n
u
e
 
{


 
 
 
 
c
h
a
r
 
c
o
d
e
[
M
A
X
_
S
T
R
]
;


 
 
 
 
c
h
a
r
 
a
d
d
r
e
s
s
[
M
A
X
_
S
T
R
]
;


 
 
 
 
c
h
a
r
 
c
a
t
e
g
o
r
y
[
M
A
X
_
S
T
R
]
;


 
 
 
 
i
n
t
 
c
a
p
a
c
i
t
y
;


 
 
 
 
d
o
u
b
l
e
 
p
r
i
c
e
;


 
 
 
 
s
t
r
u
c
t
 
V
e
n
u
e
 
*
n
e
x
t
;


}
 
V
e
n
u
e
;




t
y
p
e
d
e
f
 
s
t
r
u
c
t
 
A
p
p
l
y
 
{


 
 
 
 
c
h
a
r
 
u
n
i
t
_
c
o
d
e
[
M
A
X
_
S
T
R
]
;


 
 
 
 
c
h
a
r
 
v
e
n
u
e
_
c
o
d
e
[
M
A
X
_
S
T
R
]
;


 
 
 
 
c
h
a
r
 
t
i
m
e
_
s
t
r
[
M
A
X
_
S
T
R
]
;


 
 
 
 
c
h
a
r
 
r
e
a
s
o
n
[
M
A
X
_
S
T
R
]
;


 
 
 
 
c
h
a
r
 
r
e
m
a
r
k
[
M
A
X
_
S
T
R
]
;


 
 
 
 
s
t
r
u
c
t
 
A
p
p
l
y
 
*
n
e
x
t
;


}
 
A
p
p
l
y
;
/
/
 
三
个
全
局
链
表
头
指
针


s
t
a
t
i
c
 
U
n
i
t
 
 
*
g
_
u
n
i
t
_
h
e
a
d
 
 
=
 
N
U
L
L
;


s
t
a
t
i
c
 
V
e
n
u
e
 
*
g
_
v
e
n
u
e
_
h
e
a
d
 
=
 
N
U
L
L
;


s
t
a
t
i
c
 
A
p
p
l
y
 
*
g
_
a
p
p
l
y
_
h
e
a
d
 
=
 
N
U
L
L
;




/
/
 
清
空
输
入
缓
冲
区


s
t
a
t
i
c
 
v
o
i
d
 
c
l
e
a
r
_
i
n
p
u
t
(
v
o
i
d
)


{


 
 
 
 
i
n
t
 
c
;


 
 
 
 
w
h
i
l
e
 
(
(
c
 
=
 
g
e
t
c
h
a
r
(
)
)
 
!
=
 
'
\
n
'
 
&
&
 
c
 
!
=
 
E
O
F
)


 
 
 
 
 
 
 
 
;


}




/
/
 
安
全
读
取
一
行
输
入


s
t
a
t
i
c
 
v
o
i
d
 
r
e
a
d
_
l
i
n
e
(
c
h
a
r
 
*
b
u
f
,
 
i
n
t
 
s
i
z
e
)


{


 
 
 
 
i
f
 
(
f
g
e
t
s
(
b
u
f
,
 
s
i
z
e
,
 
s
t
d
i
n
)
 
!
=
 
N
U
L
L
)
 
{


 
 
 
 
 
 
 
 
s
i
z
e
_
t
 
l
e
n
 
=
 
s
t
r
l
e
n
(
b
u
f
)
;


 
 
 
 
 
 
 
 
i
f
 
(
l
e
n
 
>
 
0
 
&
&
 
b
u
f
[
l
e
n
 
-
 
1
]
 
=
=
 
'
\
n
'
)


 
 
 
 
 
 
 
 
 
 
 
 
b
u
f
[
l
e
n
 
-
 
1
]
 
=
 
'
\
0
'
;


 
 
 
 
}
 
e
l
s
e
 
{


 
 
 
 
 
 
 
 
b
u
f
[
0
]
 
=
 
'
\
0
'
;


 
 
 
 
}


}
```

### 3.2 核心函数

核心函数包括 safe_input(), add_contact(), search_contact(), login(), save/load_contacts() 等。

---

## 4. 测试分析

### 4.1 测试环境

| 项目 | 描述 |
|:---|:---|
| 操作系统 | Windows 10/11 64位 / Linux |
| 编译器 | GCC C99标准 |
| 编译命令 | gcc -std=c99 main.c -o main |

### 4.2 测试用例

| 编号 | 测试项 | 输入 | 结果 |
|:---:|:---|:---|:---:|
| 1 | 单位添加 | U001/计算机学院/处级/张三/主楼A301 | ? |
| 2 | 单位代码唯一性 | 再次输入U001 | ? |
| 3 | 场地添加 | V001/主楼A101/室内/120/200 | ? |
| 4 | 申请登记-外键有效 | U001→V001 | ? |
| 5 | 申请登记-外键无效 | U999→V001 | ? |
| 6 | 申请登记-场地无效 | U001→V999 | ? |
| 7 | 数据保存 | 选择保存 | ? |
| 8 | 数据加载 | 重新启动 | ? |
| 9 | 测试数据加载 | 首次运行加载 | ? |
| 10 | 浏览所有数据 | 选择浏览所有 | ? |

### 4.3 测试结果

所有测试用例通过。系统功能完整，运行稳定。

---

## 5. 总结

本系统基于C语言实现了场地使用申请信息管理功能，采用三张单链表作为核心数据结构，通过「代码」字段模拟外键机制关联单位表、场地表和申请表。系统提供了完整的单位管理（增删改查浏览）、场地管理（增删改查浏览）和申请管理（登记查询）功能，支持TSV格式的文件持久化存储。

通过本课程设计，加深了对单链表操作、文件I/O、模块化设计、外键关联和菜单驱动编程等概念的理解。系统代码注释详尽，采用结构体分层设计，数据结构合理，模块划分清晰，代码可读性强。系统可进一步扩展时间段冲突检测、审批流程管理、场地使用统计等功能。

### 改进方向

- **冲突检测**: 申请时检测时间重叠
- **审批流程**: 支持审批状态管理
- **使用统计**: 场地使用率和费用统计
- **动态内存**: 字符串按需分配
- **排序功能**: 多字段排序显示

---

## ? 快速开始

1. 打开 **Code::Blocks**
2. 创建新项目，将 `main.c` 中的代码完整复制粘贴到源文件中
3. 点击编译运行即可


---

## ? 联系作者

| 平台 | 账号/链接 |
|:---:|:---|
| ? 抖音 | `65178267913` |
| ? CSDN | [惠俊卓的博客](https://blog.csdn.net/hjz0810?type=blog) |

> **作者：惠俊卓** · **学号：424123888888** · **班级：华信竞赛队A部**
> **审核：HuiJunzhuo** · **2025-2026学年第二学期算法复习**
