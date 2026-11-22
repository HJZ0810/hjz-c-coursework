<div align="center">

# 高考志愿填报信息管理系统 V2.0

### 2025-2026学年第二学期数据结构课设

![Language](https://img.shields.io/badge/Language-C99-blue?style=for-the-badge&logo=c&logoColor=white)
![Data Structure](https://img.shields.io/badge/Data_Structure-动态数组-green?style=for-the-badge)
![Status](https://img.shields.io/badge/Status-V2.0_Complete-brightgreen?style=for-the-badge)
![Project](https://img.shields.io/badge/Project-Project_15-orange?style=for-the-badge)

---

**作者：数据库侠客**  **学号：424123888888**
**班级：计算机A班**
**作者：HuiJunzhuo**

---

*功能完整的高考志愿填报信息管理系统 V2.0 升级版*

</div>

---

## V2.0 升级亮点

| 功能 | V1 (链表版) | V2.0 (动态数组版) |
|:---:|:---|:---|
| 编码支持 | GBK/ANSI | **UTF-8 BOM + SetConsoleOutputCP(65001)** |
| 终端颜色 | 无 | **ANSI 彩色输出 (6色)** |
| CLI参数 | 无 | **-h/--help, -t/--test** |
| 数据结构 | 链表 | **动态数组 (自动扩容)** |
| 错误处理 | 返回0/-1 | **统一错误码 enum GaokaoStatus (15种)** |
| 文件格式 | 管道符文本 | **Magic Header "GKAOV2" 二进制持久化** |
| 自测用例 | 11项 | **20项内置单元测试** |
| 核心功能 | CRUD | **填报/查询/调剂/统计 四大模块** |
| 录取算法 | 无 | **分数优先 + 志愿清 + 调剂** |

---

## 目录

- [1. 系统概述](#1-系统概述)
- [2. 编译与运行](#2-编译与运行)
- [3. CLI参数](#3-cli参数)
- [4. 数据结构](#4-数据结构)
- [5. 错误码说明](#5-错误码说明)
- [6. 文件格式](#6-文件格式)
- [7. 自测报告](#7-自测报告)
- [8. 功能模块](#8-功能模块)

---

## 1. 系统概述

### 1.1 技术栈
- **编译器**: GCC (D:/bin/gcc.exe)
- **标准**: C99
- **编译选项**: `-std=c99 -Wall -Wextra -O2` (零警告)
- **数据结构**: 动态数组 (Dynamic Array)
- **持久化**: 二进制文件 with Magic Header

### 1.2 V2.0特性详解

1. **UTF-8中文支持**: 文件带UTF-8 BOM，程序启动时调用 `SetConsoleOutputCP(65001)` 设置控制台代码页，完美支持中文输入输出。

2. **ANSI彩色终端**:
   - 绿色: 成功/通过
   - 红色: 错误/失败/未录取
   - 黄色: 警告/提示
   - 蓝色: 标题/统计
   - 青色: 信息/列表
   - 紫色: 调剂模块

3. **动态数组**: 使用 `realloc` 实现自动扩容，初始容量16，每次扩容2倍。替代V1的链表实现，随机访问O(1)，缓存友好。

4. **统一错误码**: `typedef enum { ... } GaokaoStatus;` 定义15种错误状态，替代V1的简单0/-1返回值。

5. **二进制持久化**: 文件开头6字节为Magic `"GKAOV2"`，后跟版本号、各表记录数、时间戳，最后是紧凑二进制数据。

6. **录取调剂算法**:
   - 分数优先：按高考分数降序排序
   - 志愿清：从第1志愿到第6志愿依次检索
   - 服从调剂：志愿未录取且服从调剂的，调剂到未满额专业

---

## 2. 编译与运行

### 2.1 使用Makefile编译
```makefile
CC      = D:/bin/gcc.exe
CFLAGS  = -std=c99 -Wall -Wextra -O2
TARGET  = main.exe
SOURCE  = main.c
```

**编译命令**:
```bash
make          # 编译
make test     # 编译并运行自测
make run      # 编译并运行
make clean    # 清理
```

### 2.2 手动编译
```bash
D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o main.exe main.c
```

### 2.3 运行
```bash
main.exe              # 启动交互菜单
main.exe -h           # 显示帮助
main.exe --help       # 显示帮助
main.exe -t           # 运行自测
main.exe --test       # 运行自测
```

---

## 3. CLI参数

| 参数 | 功能 |
|:---|:---|
| `-h`, `--help` | 显示帮助信息，列出所有选项和模块 |
| `-t`, `--test` | 运行内置20项单元测试，返回0表示全部通过 |
| (无参数) | 启动彩色交互菜单系统 |

---

## 4. 数据结构

### 4.1 Student 学生
| 字段 | 类型 | 说明 |
|:---|:---|:---|
| id | char[32] | 考生号 (唯一键) |
| name | char[64] | 姓名 |
| school | char[128] | 毕业学校 |
| score | int | 高考分数 (0-750) |
| address | char[128] | 户籍地址 |
| phone | char[128] | 联系电话 |

### 4.2 Major 专业/院校
| 字段 | 类型 | 说明 |
|:---|:---|:---|
| code | char[32] | 专业代码 (唯一键) |
| name | char[64] | 专业名称 |
| category | char[128] | 学科门类 |
| university | char[128] | 开设院校 |
| planCount | int | 招生计划数 |
| minScore | int | 录取分数线 |
| admitted | int | 已录取人数 (运行时) |

### 4.3 Volunteer 志愿
| 字段 | 类型 | 说明 |
|:---|:---|:---|
| studentId | char[32] | 考生号 (外键) |
| majorCode | char[32] | 专业代码 (外键) |
| priority | int | 志愿优先级 (1-6) |
| adjust | char[8] | 是否服从调剂 ("是"/"否") |
| remark | char[128] | 备注信息 |

### 4.4 动态数组结构
```c
#define DECLARE_ARR(T, N)     typedef struct { T *data; int cnt; int cap; } N##Array;
```
- `data`: 指向连续内存的指针
- `cnt`: 当前元素个数
- `cap`: 当前容量 (自动扩容)

---

## 5. 错误码说明

| 错误码 | 枚举值 | 含义 |
|:---:|:---|:---|
| 0 | GK_OK | 成功 |
| -1 | GK_ERR_NULL | 空指针错误 |
| -2 | GK_ERR_MEM | 内存分配失败 |
| -3 | GK_ERR_NOT_FOUND | 记录不存在 |
| -4 | GK_ERR_DUP | 重复记录 (唯一键冲突) |
| -5 | GK_ERR_FILE_OPEN | 文件打开失败 |
| -6 | GK_ERR_FILE_READ | 文件读取失败 |
| -7 | GK_ERR_FILE_WRITE | 文件写入失败 |
| -8 | GK_ERR_MAGIC | Magic Header校验失败 (旧版/损坏) |
| -9 | GK_ERR_VERSION | 版本号不兼容 |
| -10 | GK_ERR_PARAM | 参数无效 |
| -11 | GK_ERR_SCORE | 分数无效 (0-750) |
| -12 | GK_ERR_FULL | 名额已满 |
| -13 | GK_ERR_INPUT | 输入错误 |
| -14 | GK_ERR_CAP | 容量错误 |

---

## 6. 文件格式 (gaokao_v2.dat)

```
+-----------------+
|  Magic "GKAOV2" |  6 bytes
+-----------------+
|  Version (2)    |  4 bytes (int)
+-----------------+
|  Student Count  |  4 bytes
+-----------------+
|  Major Count    |  4 bytes
+-----------------+
|  Volunteer Count|  4 bytes
+-----------------+
|  Save Timestamp |  8 bytes (time_t)
+-----------------+
|  Student[]      |  Student * n
+-----------------+
|  Major[]        |  Major * m
+-----------------+
|  Volunteer[]    |  Volunteer * v
+-----------------+
```

加载时校验Magic Header，不是"GKAOV2"则判定为旧版/损坏文件，拒绝加载。

---

## 7. 自测报告 (20项全部通过)

```
=== 内置自测 - V2.0 ===
[PASS] TC01: 初始学生数为0
[PASS] TC02: 添加学生成功
[PASS] TC03: 重复学号被拒
[PASS] TC04: 无效分数(800)被拒
[PASS] TC05: 添加李四成功
[PASS] TC06: 查找张三成功
[PASS] TC07: 查找不存在学生返回-1
[PASS] TC08: 添加计算机专业成功
[PASS] TC09: 重复专业代码被拒
[PASS] TC10: 添加软工专业成功
[PASS] TC11: 计划数为0被拒
[PASS] TC12: 张三报计算机(第1志愿)
[PASS] TC13: 重复志愿被拒
[PASS] TC14: 不存在学生报志愿被拒
[PASS] TC15: 不存在专业报志愿被拒
[PASS] TC16: 无效优先级(9)被拒
[PASS] TC17: 李四报软工(第1志愿)
[PASS] TC18: 数据保存成功
[PASS] TC19: 数据加载成功
[PASS] TC20: Magic header验证
```

---

## 8. 功能模块

### 8.1 志愿填报模块
- 添加学生信息 (学号唯一校验、分数范围校验)
- 添加专业/院校信息 (代码唯一校验、计划数校验)
- 填报高考志愿 (学生/专业存在性校验、重复志愿校验、优先级1-6校验、服从调剂选项)

### 8.2 信息查询模块
- 学生列表：表格化显示所有学生信息
- 专业列表：显示专业代码、名称、门类、院校、计划数、分数线
- 志愿列表：显示志愿优先级、调剂选项、备注

### 8.3 录取调剂模块
**录取算法**:
1. 将所有学生按分数降序排序
2. 按分数从高到低依次处理
3. 对每个学生，从第1志愿到第6志愿依次尝试
4. 如果分数>=专业分数线且名额未满，则录取
5. 如果所有志愿均未录取且服从调剂，则调剂到任意未满额专业
6. 不服从调剂或无名额则标记为未录取

### 8.4 数据统计模块
- 基本统计：学生总数、专业总数、志愿记录数
- 分数段统计：平均分、600+比例、500-599比例、500以下比例
- 招生计划统计：各专业计划数、分数线、总计划

---

## 版本历史

| 版本 | 日期 | 数据结构 | 文件格式 | 自测 | 备注 |
|:---:|:---|:---|:---|:---:|:---|
| V1 | 2025 | 链表 | 管道符文本 | 11项 | 基础CRUD |
| **V2.0** | **2026** | **动态数组** | **GKAOV2二进制** | **20项** | **彩色CLI+录取调剂+统计** |

---

> **作者：数据库侠客** | **学号：424123888888** | **班级：计算机A班**
> **版本：V2.0** | **编译器：D:/bin/gcc.exe (-std=c99 -Wall -Wextra -O2 零警告)**
