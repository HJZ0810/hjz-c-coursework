<div align="center">

# 研究生管理信息系统 V2.0

### 2025-2026学年第二学期算法课实习

![Language](https://img.shields.io/badge/Language-C99-blue?style=for-the-badge&logo=c&logoColor=white)
![Data Structure](https://img.shields.io/badge/Data_Structure-Dynamic_Array-green?style=for-the-badge)
![Persistence](https://img.shields.io/badge/Persistence-Binary_GRADV2-orange?style=for-the-badge)
![Status](https://img.shields.io/badge/Status-V2.0_Released-brightgreen?style=for-the-badge)
![Project](https://img.shields.io/badge/Project-22-orange?style=for-the-badge)

---

**教师：数据库老师** | **学号：424123888888**
**班级：计算机A班**
**姓名：HuiJunzhuo**

---

*基于动态数组(Dynamic Array)实现的研究生管理信息系统V2.0*

</div>

---

## 目录

- [V2.0 升级亮点](#v20-升级亮点)
- [1. 项目概述](#1-项目概述)
- [2. 主要功能](#2-主要功能)
- [3. 技术架构](#3-技术架构)
- [4. 编译运行](#4-编译运行)
- [5. 测试报告](#5-测试报告)
- [6. 文件清单](#6-文件清单)

---

## V2.0 升级亮点

| 升级项 | V1.0 | V2.0 |
|:---:|:---|:---|
| 数据结构 | 单链表 | **动态数组** |
| 持久化格式 | 管道分隔文本 | **二进制 + Magic "GRADV2"** |
| 错误处理 | 分散printf | **统一GradStatus枚举(15种)** |
| 终端输出 | 单色文本 | **ANSI彩色VT100** |
| 编码支持 | 无 | **UTF-8 BOM + CP65001** |
| CLI参数 | 无 | **-h/--help, -t/--test** |
| 数据模型 | 专业/课程/学生/成绩/用户 | **导师/研究生/成绩** |
| 测试覆盖 | 约12项 | **44项** |

---

## 1. 项目概述

### 1.1 项目背景

研究生教育管理中，导师信息、研究生信息及其成绩的管理是研究生培养工作的重要环节。V2.0版本在V1.0基础上进行全面架构升级，采用动态数组替代单链表以获得更好的缓存局部性和随机访问性能，使用二进制持久化格式替代文本文件以提高I/O效率和数据完整性校验能力，并新增统一错误码、ANSI彩色终端、UTF-8编码支持和CLI命令行参数。

### 1.2 功能清单

| 编号 | 功能模块 | 说明 |
|:---:|:---|:---|
| 1 | 导师管理 | 导师CRUD + 指导人数统计 |
| 2 | 研究生管理 | 研究生CRUD + 导师ID外键 |
| 3 | 导师分配 | 分配/解除/查看未分配学生 |
| 4 | 成绩管理 | 成绩CRUD + 平均分 + 彩色分级 |
| 5 | 统计信息 | 全局平均分/及格率/最高最低分 |
| 6 | 文件持久化 | 二进制GRADV2格式 + 魔数校验 |
| 7 | 内置自测 | 44项测试(CRUD+分配+成绩+I/O+边界) |
| 8 | CLI参数 | -h帮助 / -t自测 / 无参数交互 |

### 1.3 数据结构

**Advisor（导师）**

| 字段 | 类型 | 说明 |
|:---|:---|:---|
| `id` | char[20] | 导师工号（主键） |
| `name` | char[50] | 导师姓名 |
| `title` | char[30] | 职称（教授/副教授/讲师） |
| `department` | char[50] | 所属院系 |

**Graduate（研究生）**

| 字段 | 类型 | 说明 |
|:---|:---|:---|
| `id` | char[20] | 学号（主键） |
| `name` | char[50] | 姓名 |
| `gender` | char[10] | 性别 |
| `enrollment_year` | int | 入学年份 |
| `advisor_id` | char[20] | 导师工号（外键，空串=未分配） |
| `research_topic` | char[100] | 研究方向 |

**Grade（成绩）**

| 字段 | 类型 | 说明 |
|:---|:---|:---|
| `student_id` | char[20] | 学号（外键） |
| `course_name` | char[50] | 课程名称 |
| `score` | float | 成绩（0.0~100.0） |

---

## 2. 主要功能

### 2.1 动态数组容器

通用泛型动态数组实现，支持：
- `darray_init()` - 初始化，指定元素大小
- `darray_push()` - 尾部追加，自动2倍扩容
- `darray_get()` - 随机访问O(1)
- `darray_remove_at()` - 删除元素，memmove移动
- `darray_free()` - 释放内存
- `darray_reserve()` - 预分配容量

### 2.2 统一错误码

所有业务函数返回`GradStatus`枚举值，包含15种状态码：
- `GRAD_OK(0)` 成功
- `GRAD_ERR_NULL_PTR(-1)` 空指针
- `GRAD_ERR_MEMORY(-2)` 内存不足
- `GRAD_ERR_NOT_FOUND(-3)` 未找到
- `GRAD_ERR_DUPLICATE(-4)` ID重复
- `GRAD_ERR_INVALID_INPUT(-5)` 无效输入
- `GRAD_ERR_FILE_IO(-6)` 文件I/O错误
- `GRAD_ERR_INVALID_SCORE(-7)` 成绩越界
- `GRAD_ERR_ADVISOR_NOT_FOUND(-8)` 导师不存在
- `GRAD_ERR_STUDENT_NOT_FOUND(-9)` 研究生不存在
- `GRAD_ERR_ALREADY_ASSIGNED(-10)` 已分配导师
- `GRAD_ERR_NOT_ASSIGNED(-11)` 未分配导师
- `GRAD_ERR_FILE_CORRUPT(-12)` 文件损坏
- `GRAD_ERR_FILE_VERSION(-13)` 版本不兼容
- `GRAD_ERR_FILE_MAGIC(-14)` 魔数不匹配

### 2.3 GRADV2二进制文件格式

```
Offset 0:  Magic[6]   = "GRADV2"
Offset 6:  Version    = 2 (uint16)
Offset 8:  Reserved   = 0 (uint16)
Offset 10: AdvisorCount  (int32)
Offset 14: GraduateCount (int32)
Offset 18: GradeCount    (int32)
Offset 22: Advisor[]  (N × sizeof(Advisor) 字节)
Offset 22+N*sizeof(Advisor): Graduate[] (M × sizeof(Graduate))
...
```

---

## 3. 技术架构

```
┌─────────────────────────────────────────────┐
│   表现层 (交互菜单 + ANSI彩色终端)           │
├─────────────────────────────────────────────┤
│   业务层 (CRUD / 导师分配 / 成绩统计)        │
├─────────────────────────────────────────────┤
│   数据层 (动态数组容器 + GRADV2文件I/O)      │
└─────────────────────────────────────────────┘
```

---

## 4. 编译运行

### 4.1 环境要求

| 项目 | 要求 |
|:---|:---|
| 操作系统 | Windows 10/11 (支持VT100) |
| 编译器 | GCC C99 (`D:/bin/gcc.exe`) |
| 编译命令 | `gcc -std=c99 -Wall -Wextra -O2 -o grad.exe main.c` |

### 4.2 使用Makefile

```bash
make          # 编译 grad.exe
make test     # 编译并运行自测
make run      # 编译并启动交互模式
make clean    # 清理生成文件
```

### 4.3 CLI参数

```bash
grad.exe           # 启动交互式菜单
grad.exe -h        # 显示帮助
grad.exe --help    # 显示帮助
grad.exe -t        # 运行内置自测
grad.exe --test    # 运行内置自测
```

---

## 5. 测试报告

V2.0内置44项测试，覆盖CRUD、导师分配、成绩管理、文件I/O、动态扩容和边界条件：

| 测试分类 | 测试项 | 数量 |
|:---:|:---|:---:|
| 导师CRUD | 添加/重复/查找/修改/删除/空ID/不存在 | 11 |
| 研究生CRUD | 添加/重复/查找/修改/删除/空ID/不存在/级联 | 8 |
| 导师分配 | 分配/重复/不存在导师/解除/重复解除 | 7 |
| 成绩管理 | 录入/无效成绩/重复/修改/平均分计算 | 10 |
| 文件I/O | 保存/加载完整性/魔数校验 | 6 |
| 动态扩容 | 批量20条(超初始容量8触发realloc) | 1 |
| 边界安全 | 空输入/不存在实体/级联删除 | 融入上述 |

运行`grad.exe --test`验证全部通过：
```
测试结果: 44 通过, 0 失败, 共 44 项
所有测试通过! ✓
```

---

## 6. 文件清单

| 文件名 | 说明 |
|:---|:---|
| `main.c` | V2.0主程序(UTF-8 BOM, ~1660行) |
| `Makefile` | GNU Make构建脚本 |
| `grad.exe` | 编译生成的可执行文件 |
| `gradv2.dat` | V2.0数据文件(运行时自动生成) |
| `gen_doc.py` | 文档更新脚本(蓝色高亮V2更新) |
| `README.md` | 项目说明 |
| `Project_22_研究生管理信息系统.docx` | V2.0完整文档(含蓝色高亮更新章节) |
| `Project_22_研究生管理信息系统_V1_backup.docx` | V1.0文档备份 |

---

<div align="center">

**教师：数据库老师** | **学号：424123888888** | **班级：计算机A班**
**姓名：HuiJunzhuo** | **2025-2026学年第二学期算法课实习**

</div>
