<div align="center">

# 🏥 看病预约信息管理系统 V2.0

### 📅 2025-2026学年第二学期数据结构课程实习

![Language](https://img.shields.io/badge/Language-C99-blue?style=for-the-badge&logo=c&logoColor=white)
![Data Structure](https://img.shields.io/badge/Data_Structure-动态数组-green?style=for-the-badge)
![Status](https://img.shields.io/badge/Status-V2.0_Complete-brightgreen?style=for-the-badge)
![Project](https://img.shields.io/badge/Project-14-orange?style=for-the-badge)
![Tests](https://img.shields.io/badge/Tests-18_Passed-success?style=for-the-badge)

---

**👨‍🏫 指导教师：数据结构课程组** | **学号：424123888888**
**🎓 班级：计算机科学与技术A班**
**✍️ 作者：HuiJunzhuo**

---

*基于动态数组的C语言看病预约信息管理系统 V2.0*

</div>

---

## 📋 目录

- [1. 项目概述](#1-项目概述)
- [2. V2.0 新特性](#2-v20-新特性)
- [3. 系统架构](#3-系统架构)
- [4. 编译与运行](#4-编译与运行)
- [5. CLI 参数](#5-cli-参数)
- [6. 数据结构](#6-数据结构)
- [7. 错误码](#7-错误码)
- [8. 文件持久化](#8-文件持久化)
- [9. 自测用例](#9-自测用例)
- [10. 项目文件](#10-项目文件)

---

## 1. 项目概述

本系统是一个使用 C99 标准编写的看病预约信息管理系统，V2.0 版本在 V1.0 链表实现基础上进行了全面重构升级。系统实现了患者信息管理、医生信息管理、预约记录管理三大核心功能模块，支持 CRUD 全操作、多条件查询、数据统计，并提供彩色终端交互界面和完善的自测套件。

### 1.1 功能模块

| 模块 | 实现方式 | 说明 |
|:---:|:---|:---|
| 患者信息管理 | 动态数组 | 增删改查 + 编号唯一性校验 + 关联预约查询 |
| 医生信息管理 | 动态数组 | 增删改查 + 专长/科室索引 + 关联预约查询 |
| 预约记录管理 | 动态数组 | 创建/取消/查询 + 患者医生存在性校验 + 状态标记 |
| 数据持久化 | 二进制文件 | "HOSPV2"魔数+版本号校验+二进制序列化 |
| 终端界面 | ANSI彩色 | 成功绿/失败红/警告黄/信息青/菜单紫 多色输出 |
| 内置自测 | TEST_ASSERT宏 | 18组测试、129+断言，全自动化验证 |
| CLI命令行 | 参数解析 | -h/--help/-t/--test 标准Unix风格参数 |

---

## 2. V2.0 新特性

| 特性 | V1.0 | V2.0 |
|:---|:---|:---|
| 数据结构 | 单向链表 | **动态数组（自动2倍扩容）** |
| 文件存储 | 文本文件(\|分隔) | **二进制文件(HOSPV2魔数+版本号)** |
| 错误处理 | printf输出 | **统一HospStatus枚举错误码** |
| 终端输出 | 单色文本 | **ANSI彩色终端** |
| 编码支持 | GBK/乱码 | **UTF-8 BOM + SetConsoleOutputCP(65001)** |
| 测试支持 | 无 | **18项内置自测(--test)** |
| CLI参数 | 无 | **-h/--help/-t/--test** |
| 预约状态 | 无 | **有效/已取消状态标记** |
| 数据统计 | 无 | **患者/医生/预约/有效预约统计** |
| 内存管理 | 手动链表 | **统一arrFree自动释放** |

---

## 3. 系统架构

```
┌─────────────────────────────────────────────┐
│         用户界面层（ANSI彩色菜单）            │
│  患者管理 │ 医生管理 │ 预约管理 │ 数据统计     │
├─────────────────────────────────────────────┤
│         CLI参数层（-h / -t / 交互模式）       │
├─────────────────────────────────────────────┤
│         业务逻辑层（CRUD + 校验）             │
│  patientAdd │ doctorAdd │ apptAdd/Cancel     │
├─────────────────────────────────────────────┤
│         容器层（DynArray动态数组）            │
│  arrInit/arrPush/arrGet/arrRemove/arrFree   │
├─────────────────────────────────────────────┤
│         持久化层（HOSPV2二进制格式）          │
│  Magic:6B │ Version:4B │ Counts │ Data...   │
└─────────────────────────────────────────────┘
```

---

## 4. 编译与运行

### 4.1 编译器要求

- **操作系统**: Windows 10/11 (支持ANSI转义序列)
- **编译器**: GCC (MinGW-w64)，路径 `D:/bin/gcc.exe`
- **标准**: C99
- **编译选项**: `-std=c99 -Wall -Wextra -O2`（零警告编译）

### 4.2 使用 Makefile（推荐）

```bash
make          # 编译
make test     # 编译并运行自测
make run      # 编译并启动交互模式
make clean    # 清理生成文件
```

### 4.3 手动编译

```bash
D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o hosp_v2.exe main.c
```

---

## 5. CLI 参数

| 参数 | 功能 |
|:---|:---|
| `-h`, `--help` | 显示帮助信息 |
| `-t`, `--test` | 运行内置自测套件（18项测试） |
| *(无参数)* | 启动彩色交互菜单模式 |

**示例：**
```bash
./hosp_v2.exe --help    # 显示帮助
./hosp_v2.exe --test    # 运行自测
./hosp_v2.exe           # 启动交互模式
```

---

## 6. 数据结构

### 6.1 DynArray 动态数组

V2.0核心容器，替代V1.0链表：

```c
typedef struct {
    void   **data;      // 元素指针数组
    size_t   size;      // 当前元素个数
    size_t   capacity;  // 当前容量（满时自动2倍扩容）
    size_t   elemSize;  // 单个元素字节大小
} DynArray;
```

**核心API：**
- `arrInit()` - 初始化动态数组
- `arrPush()` - 尾部添加元素（自动扩容）
- `arrGet()` - 按索引获取元素
- `arrRemove()` - 按索引删除元素
- `arrFind()` - 按比较函数查找
- `arrFree()` - 释放所有内存

### 6.2 业务结构体

**Patient（患者）:**
| 字段 | 类型 | 说明 |
|:---|:---|:---|
| id | char[20] | 患者编号（如P2025001） |
| name | char[50] | 姓名 |
| age | int | 年龄（0-150校验） |
| gender | char[10] | 性别（男/女） |
| job | char[30] | 职业 |
| addr | char[100] | 地址 |

**Doctor（医生）:**
| 字段 | 类型 | 说明 |
|:---|:---|:---|
| id | char[20] | 医生编号（如D001） |
| name | char[50] | 姓名 |
| specialty | char[50] | 专长（如心血管内科） |
| dept | char[30] | 科室（如内科） |
| years | int | 从业年限 |

**Appointment（预约）:**
| 字段 | 类型 | 说明 |
|:---|:---|:---|
| id | int | 预约号（系统自动递增） |
| patientId | char[20] | 关联患者编号 |
| doctorId | char[20] | 关联医生编号 |
| datetime | char[20] | 预约时间（如2025-06-15 09:00） |
| desc | char[200] | 病情描述 |
| note | char[200] | 备注 |
| status | int | 状态（1=有效, 0=已取消） |

---

## 7. 错误码

HospStatus 统一错误码枚举：

| 错误码 | 值 | 含义 |
|:---|:---:|:---|
| HOSP_OK | 0 | 操作成功 |
| HOSP_ERR_MEMORY | -1 | 内存分配失败 |
| HOSP_ERR_NOT_FOUND | -2 | 记录未找到 |
| HOSP_ERR_DUPLICATE | -3 | 编号重复 |
| HOSP_ERR_INVALID_PARAM | -4 | 无效参数 |
| HOSP_ERR_FILE_OPEN | -5 | 无法打开文件 |
| HOSP_ERR_FILE_READ | -6 | 文件读取错误 |
| HOSP_ERR_FILE_WRITE | -7 | 文件写入错误 |
| HOSP_ERR_FILE_MAGIC | -8 | 文件魔数不匹配 |
| HOSP_ERR_FILE_VERSION | -9 | 文件版本不兼容 |
| HOSP_ERR_EMPTY | -10 | 数据为空 |
| HOSP_ERR_PATIENT_EXISTS | -11 | 患者已存在 |
| HOSP_ERR_DOCTOR_EXISTS | -12 | 医生已存在 |
| HOSP_ERR_INVALID_ID | -13 | 无效编号 |

---

## 8. 文件持久化

V2.0使用带魔数校验的二进制文件格式（`hosp_v2.dat`）：

```
偏移  长度        内容
──────────────────────────────────────
0     6字节       Magic Header: "HOSPV2"
6     4字节       Version: 2 (int)
10    8字节       患者数量 (size_t)
18    8字节       医生数量 (size_t)
26    8字节       预约数量 (size_t)
34    4字节       下一个预约号 (int)
38    N*sizeof(Patient)  患者数据
...   N*sizeof(Doctor)   医生数据
...   N*sizeof(Appointment) 预约数据
```

**安全性：**
- 魔数校验：加载时验证前6字节是否为"HOSPV2"，防止加载非本系统文件
- 版本校验：version字段为2，防止不兼容版本数据
- 自动保存：数据修改后退出时自动保存，也可菜单手动保存

---

## 9. 自测用例

V2.0内置18项测试、129个断言，覆盖全功能模块：

| # | 测试名称 | 验证内容 |
|:---:|:---|:---|
| 1 | 动态数组基础操作 | init/push/get/size基本功能 |
| 2 | 动态数组自动扩容 | 20次push验证2倍自动扩容 |
| 3 | 系统初始化 | arrInit/arrFree完整性 |
| 4 | 患者添加 | 正常添加患者并验证字段 |
| 5 | 患者编号重复检测 | 重复编号返回DUPLICATE错误 |
| 6 | 患者查找 | 存在/不存在两种情况 |
| 7 | 患者删除 | 正常删除+删除不存在记录 |
| 8 | 医生添加 | 正常添加医生并验证字段 |
| 9 | 医生编号重复检测 | 重复编号返回DUPLICATE错误 |
| 10 | 医生查找 | 存在/不存在两种情况 |
| 11 | 医生删除 | 正常删除+删除不存在记录 |
| 12 | 有效预约创建 | 患者医生均存在时创建成功 |
| 13 | 无效患者预约拒绝 | 患者不存在时返回NOT_FOUND |
| 14 | 无效医生预约拒绝 | 医生不存在时返回NOT_FOUND |
| 15 | 预约取消 | 正常取消+取消不存在预约 |
| 16 | 预约查找 | 预约号查找+不存在查找 |
| 17 | 文件持久化 | Save→清空→Load数据完整性验证 |
| 18 | 错误码字符串 | hospStatusStr中文消息正确性 |

**运行自测：**
```bash
./hosp_v2.exe --test
```

---

## 10. 项目文件

| 文件 | 说明 |
|:---|:---|
| `main.c` | V2.0完整源代码（UTF-8 BOM，中文注释） |
| `Makefile` | GCC编译配置（CC=D:/bin/gcc.exe） |
| `gen_doc.py` | 文档生成脚本（python-docx蓝色高亮） |
| `Project_14_看病预约信息管理系统.docx` | V2.0设计文档（蓝色高亮标记新特性） |
| `_V1_backup.docx` | V1.0文档备份 |
| `README.md` | 本说明文件 |
| `hosp_v2.exe` | 编译生成的可执行文件 |
| `hosp_v2.dat` | 运行时生成的二进制数据文件 |

---

## ✅ V2.0 验收标准

- [x] UTF-8 BOM编码
- [x] SetConsoleOutputCP(65001) UTF-8输出
- [x] ANSI彩色终端（多色输出）
- [x] CLI参数：-h/--help/-t/--test
- [x] 动态数组存储（患者+医生+预约）
- [x] HospStatus统一错误码枚举
- [x] "HOSPV2" Magic Header二进制持久化
- [x] 18项内置自测（≥15项要求）
- [x] 交互菜单（预约/取消/查询/统计）
- [x] 中文注释
- [x] Makefile（指定D:/bin/gcc.exe）
- [x] -Wall -Wextra -O2 零警告编译
- [x] --test全部通过（129断言0失败）
- [x] gen_doc.py + _V1_backup.docx + docx蓝色高亮

---

> **👨‍🏫 指导教师：数据结构课程组** | **学号：424123888888** | **班级：计算机科学与技术A班**
> **✍️ 作者：HuiJunzhuo** | **学年学期：2025-2026学年第二学期**
