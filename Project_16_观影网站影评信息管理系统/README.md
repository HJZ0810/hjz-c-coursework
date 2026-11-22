<div align="center">

# 🎬 观影网站影评信息管理系统 V2.0

### 📅 2025-2026学年第二学期数据结构实习

![Language](https://img.shields.io/badge/Language-C99-blue?style=for-the-badge&logo=c&logoColor=white)
![Data Structure](https://img.shields.io/badge/Data_Structure-Dynamic_Array-green?style=for-the-badge)
![Status](https://img.shields.io/badge/Status-V2.0_Release-brightgreen?style=for-the-badge)
![Tests](https://img.shields.io/badge/Tests-47_Passed-success?style=for-the-badge)
![Project](https://img.shields.io/badge/Project-16-orange?style=for-the-badge)

---

**学生：数据库课程** 🆔 **学号：424123888888**
**🏫 班级：计算机类A班**
**👤 姓名：HuiJunzhuo**

---

*基于动态数组(Dynamic Array)实现的观影网站影评信息管理系统 V2.0*

</div>

---

## 📋 目录

- [✨ V2.0 新特性](#-v20-新特性)
- [1. 项目概述](#1-项目概述)
- [2. 系统要求](#2-系统要求)
- [3. 编译运行](#3-编译运行)
- [4. CLI命令行](#4-cli命令行)
- [5. 功能特性](#5-功能特性)
- [6. 数据结构](#6-数据结构)
- [7. 文件格式](#7-文件格式)
- [8. 测试结果](#8-测试结果)
- [📂 文件清单](#-文件清单)

---

## ✨ V2.0 新特性

| 特性 | V1.0 | V2.0 |
|:---|:---:|:---:|
| 数据结构 | 单链表 | **🔹动态数组(Dynamic Array)** |
| 中文编码 | GBK/ANSI | **🔹UTF-8 BOM + SetConsoleOutputCP(65001)** |
| 终端输出 | 黑白文本 | **🔹ANSI彩色终端** |
| CLI参数 | ❌ | **🔹-h/--help/-t/--test** |
| 错误处理 | printf输出 | **🔹统一错误码枚举MovieStatus** |
| 文件格式 | 文本分隔符`\|` | **🔹二进制MOVVV2 Magic Header** |
| 内置测试 | ❌ | **🔹47项自测(CRUD+评分+统计+I/O)** |
| 统计功能 | ❌ | **🔹平均/最高/最低/评分分布** |
| 修改功能 | 部分支持 | **🔹完整电影修改(保留原值)** |
| 级联删除 | ❌ | **🔹删除电影自动删除关联影评** |
| 编译警告 | 未严格检查 | **🔹-Wall -Wextra 零警告** |
| exe名称 | main.exe | **🔹movie.exe** |

---

## 1. 项目概述

### 1.1 项目简介

观影网站影评信息管理系统V2.0是一个用C99标准编写的控制台应用程序，用于管理电影和影评信息。系统采用动态数组作为核心数据结构，支持电影和影评的增删改查(CRUD)操作，提供评分统计分析功能，并使用自定义二进制格式持久化存储数据。

### 1.2 V2.0核心改进

- **内存管理升级**：从单链表改为动态数组，支持随机访问、缓存友好、自动扩容
- **编码标准化**：UTF-8 BOM编码，Windows控制台设置65001代码页，完美支持中文
- **界面美化**：ANSI彩色输出，成功/错误/信息使用不同颜色区分
- **工程化**：统一错误码枚举，函数返回状态码而非直接printf
- **二进制持久化**：Magic Header "MOVVV2" + 版本号校验，防止读取错误文件
- **完善测试**：47项自动化测试用例，覆盖率广，一键执行
- **统计分析**：平均分、最高分、最低分、评分等级分布统计

---

## 2. 系统要求

| 项目 | 要求 |
|:---|:---|
| 操作系统 | Windows 10/11 (支持VT100) / Linux |
| 编译器 | GCC (MinGW) 支持C99标准 |
| 编译器路径 | D:/bin/gcc.exe |
| 编译标准 | `-std=c99 -Wall -Wextra -O2` |
| Python (可选) | 3.x (用于生成docx文档) |

---

## 3. 编译运行

### 3.1 使用Makefile（推荐）

```bash
# 编译项目
make

# 编译并运行测试
make test

# 编译并运行
make run

# 清理生成文件
make clean

# 查看帮助
make help
```

### 3.2 手动编译

```bash
D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o movie.exe main.c -lm
```

### 3.3 运行程序

```bash
# 交互模式
./movie.exe

# 查看帮助
./movie.exe --help

# 运行自测
./movie.exe --test
```

---

## 4. CLI命令行

| 参数 | 功能 |
|:---|:---|
| `(无参数)` | 启动交互模式 |
| `-h`, `--help` | 显示帮助信息 |
| `-t`, `--test` | 运行内置47项自测套件 |

---

## 5. 功能特性

### 5.1 电影管理
- ✅ 添加电影（标题、年份、导演、类型、分级）
- ✅ 修改电影信息（支持回车保留原值）
- ✅ 删除电影（**级联删除关联影评**）
- ✅ 浏览所有电影（表格显示）
- ✅ 自动计算平均分和影评数

### 5.2 影评管理
- ✅ 添加影评（电影ID、评论者、评分0-10、评论内容）
- ✅ 删除影评
- ✅ 浏览所有影评
- ✅ 评分范围校验（0.0-10.0）
- ✅ 外键校验（电影必须存在）
- ✅ 添加/删除后自动更新电影平均分

### 5.3 数据统计
- 📊 电影总数
- 📊 影评总数
- 📊 平均评分
- 📊 最高评分 / 最低评分
- 📊 评分分布（优秀/良好/一般/较差）+ 百分比

### 5.4 数据持久化
- 💾 启动自动加载数据文件
- 💾 退出自动保存
- 💾 手动保存选项
- 💾 MOVVV2二进制格式 + Magic校验
- 💾 首次运行自动生成示例数据（8电影+10影评）

---

## 6. 数据结构

### 6.1 动态数组 (Dynamic Array)

```c
#define INIT_CAPACITY   8    // 初始容量
#define GROWTH_FACTOR   2    // 2倍扩容策略

typedef struct {
    Movie  *items;     // 元素指针
    int     count;     // 当前元素数量
    int     capacity;  // 当前分配容量
} MovieArray;
```

**扩容策略**：当count >= capacity时，capacity *= 2，realloc重新分配内存。

### 6.2 核心结构体

**Movie（电影）**
| 字段 | 类型 | 说明 |
|:---|:---|:---|
| id | int | 电影ID(2001起自增) |
| title | char[64] | 电影名称 |
| year | int | 上映年份(1800-2100) |
| director | char[64] | 导演 |
| genre | char[64] | 类型 |
| rating | char[64] | 分级 |
| avgScore | float | 平均评分(自动计算) |
| reviewCount | int | 影评数(自动维护) |

**Review（影评）**
| 字段 | 类型 | 说明 |
|:---|:---|:---|
| id | int | 影评ID(3001起自增) |
| movieId | int | 所属电影ID |
| reviewer | char[64] | 评论者 |
| score | float | 评分(0.0-10.0) |
| comment | char[256] | 评论内容 |

### 6.3 错误码枚举 MovieStatus

| 错误码 | 值 | 说明 |
|:---|:---|:---|
| MOVIE_OK | 0 | 操作成功 |
| MOVIE_ERR_NULL_PTR | 1 | 空指针错误 |
| MOVIE_ERR_MEMORY | 2 | 内存分配失败 |
| MOVIE_ERR_NOT_FOUND | 3 | 未找到记录 |
| MOVIE_ERR_INVALID_PARAM | 5 | 无效参数 |
| MOVIE_ERR_FILE_OPEN | 6 | 文件打开失败 |
| MOVIE_ERR_FILE_READ | 7 | 文件读取失败 |
| MOVIE_ERR_FILE_WRITE | 8 | 文件写入失败 |
| MOVIE_ERR_MAGIC_MISMATCH | 9 | Magic头不匹配 |
| MOVIE_ERR_VERSION | 10 | 版本不兼容 |
| MOVIE_ERR_SCORE_RANGE | 14 | 评分范围错误 |

---

## 7. 文件格式

### 7.1 MOVVV2 二进制格式

数据文件 `movies_v2.dat` 采用自定义二进制格式：

```
偏移  长度  内容
0     6     Magic Header: "MOVVV2" (0x4D 0x4F 0x56 0x56 0x56 0x32)
6     4     Version: 2 (int32)
10    4     movieIdCounter (int32)
14    4     reviewIdCounter (int32)
18    4     movieCount (int32)
22    ?     movieCount × sizeof(Movie) 字节
?     4     reviewCount (int32)
?     ?     reviewCount × sizeof(Review) 字节
```

### 7.2 安全校验
1. 打开文件后首先读取6字节Magic Header，必须等于"MOVVV2"
2. 读取版本号，必须等于2
3. 校验失败返回相应错误码，不加载数据

---

## 8. 测试结果

### 8.1 编译测试
```
✅ D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2
✅ 零警告零错误
✅ 输出文件：movie.exe
```

### 8.2 自测套件（47项全通过）

| 测试分类 | 数量 | 结果 |
|:---|:---:|:---:|
| 初始化测试 | 3 | ✅ 全部通过 |
| 电影CRUD测试 | 6 | ✅ 全部通过 |
| 影评CRUD测试 | 8 | ✅ 全部通过 |
| 文件持久化测试 | 8 | ✅ 全部通过 |
| 更新电影测试 | 4 | ✅ 全部通过 |
| 删除测试（含级联） | 5 | ✅ 全部通过 |
| 统计功能测试 | 7 | ✅ 全部通过 |
| 动态扩容测试 | 6 | ✅ 全部通过 |
| **总计** | **47** | **🎉 全部通过** |

运行测试命令：
```bash
./movie.exe --test
```

---

## 📂 文件清单

| 文件名 | 说明 | V2新增 |
|:---|:---|:---:|
| `main.c` | V2.0主程序源代码(UTF-8 BOM) | 🔹 |
| `Makefile` | 编译配置(CC=D:/bin/gcc.exe) | 🔹 |
| `movie.exe` | 编译生成的可执行文件 | 🔹 |
| `movies_v2.dat` | V2数据文件(MOVVV2二进制，运行时生成) | 🔹 |
| `gen_doc.py` | Python文档生成脚本 | 🔹 |
| `Project_16_观影网站影评信息管理系统.docx` | V2.0课程设计文档(蓝色高亮新特性) | 🔹 |
| `Project_16_观影网站影评信息管理系统_V1_backup.docx` | V1.0文档备份 | 🔹 |
| `README.md` | 本说明文件 | |

---

## 🎯 主菜单

```
🎬  观影网站影评信息管理系统 V2.0  🎬

  【主菜单】
  1. 📋 浏览所有电影
  2. ➕ 添加新电影
  3. ✏️  修改电影信息    [V2新增]
  4. 🗑️  删除电影(级联)  [V2增强]
  5. 📝 浏览所有影评
  6. ✍️  添加影评
  7. ❌ 删除影评
  8. 📊 数据统计        [V2新增]
  9. 💾 手动保存
  0. 🚪 退出系统
```

---

<div align="center">

> **学生：数据库课程** 🆔 **学号：424123888888** 🏫 **班级：计算机类A班**
> **👤 姓名：HuiJunzhuo** 📅 **2025-2026学年第二学期数据结构实习**

</div>
