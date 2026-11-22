#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
教师工资信息管理系统 V2.0 文档生成脚本
生成项目文档说明，标注V2.0升级特性（蓝色高亮标记）
"""

import os
import sys
from datetime import datetime

PROJECT_NAME = "教师工资信息管理系统"
VERSION = "V2.0"
AUTHOR = "HuiJunzhuo"
STUDENT_ID = "424123888888"
CLASS_NAME = "计算机A班"

def generate_markdown_doc():
    """生成Markdown格式文档"""
    md_content = f"""# {PROJECT_NAME} {VERSION}

<div align="center">

###  2025-2026学年第二学期·算法实习

![Language](https://img.shields.io/badge/Language-C99-blue?style=for-the-badge&logo=c&logoColor=white)
![Version](https://img.shields.io/badge/Version-V2.0-brightgreen?style=for-the-badge)
![Status](https://img.shields.io/badge/Status-Completed-success?style=for-the-badge)
![Compiler](https://img.shields.io/badge/Compiler-GCC-orange?style=for-the-badge)

---

**学号：{STUDENT_ID}**  **班级：{CLASS_NAME}**  
**作者：{AUTHOR}**

---

*V2.0版本全面重构升级，采用动态数组存储、统一错误码、SALV2二进制持久化格式、内置17项自测*

</div>

---

##  V2.0 升级特性（蓝色高亮）

<span style="color:blue; font-weight:bold;">【新增】UTF-8 BOM + SetConsoleOutputCP(65001) 完美中文支持</span>  
<span style="color:blue; font-weight:bold;">【新增】ANSI 彩色终端输出，界面更美观</span>  
<span style="color:blue; font-weight:bold;">【新增】CLI 命令行参数解析 (-h/--help, -t/--test)</span>  
<span style="color:blue; font-weight:bold;">【重构】动态数组存储（替代V1的链表结构）</span>  
<span style="color:blue; font-weight:bold;">【新增】统一错误码枚举 SalStatus (13种错误类型)</span>  
<span style="color:blue; font-weight:bold;">【新增】SALV2 Magic Header 二进制文件持久化格式</span>  
<span style="color:blue; font-weight:bold;">【新增】内置17项自动化自测（覆盖CRUD+工资计算+统计+文件I/O）</span>  
<span style="color:blue; font-weight:bold;">【新增】完善的交互菜单系统</span>  
<span style="color:blue; font-weight:bold;">【新增】全中文注释，代码可读性强</span>  
<span style="color:blue; font-weight:bold;">【新增】Makefile构建系统，零警告编译</span>

---

##  目录

- [1. 系统概述](#1-系统概述)
- [2. 主要功能](#2-主要功能)
- [3. 编译运行](#3-编译运行)
- [4. 数据结构](#4-数据结构)
- [5. 文件格式](#5-文件格式)
- [6. 错误码说明](#6-错误码说明)
- [7. 自测说明](#7-自测说明)

---

## 1. 系统概述

{PROJECT_NAME}{VERSION}是一个基于C99标准开发的控制台应用程序，用于管理学校教师工资信息。系统提供教师信息的增删改查（CRUD）、工资计算、统计分析以及数据持久化功能。

###  V1.0 → V2.0 对比

| 特性 | V1.0 | V2.0 |
|:---|:---|:---|
| 数据结构 | 多表链表 | 单表动态数组 |
| 中文编码 | GBK/ANSI | UTF-8 BOM + CP65001 |
| 终端颜色 | 无 | ANSI彩色输出 |
| CLI参数 | 无 | -h/--help, -t/--test |
| 错误处理 | 分散处理 | 统一SalStatus枚举 |
| 文件格式 | 文本分隔符 | SALV2二进制魔数 |
| 自测功能 | 无 | 17项自动化测试 |
| 编译警告 | -Wall/-Wextra未达零警告 | 零警告通过 |

---

## 2. 主要功能

| 编号 | 功能模块 | 说明 |
|:---:|:---|:---|
| 1 | 添加教师 | 录入教师基本信息和工资项目，自动计算实发工资 |
| 2 | 删除教师 | 根据编号删除教师记录，带确认提示 |
| 3 | 修改教师 | 修改教师信息，支持回车保留原值 |
| 4 | 查询教师 | 支持按编号查询、按姓名模糊查询、列出全部 |
| 5 | 教师列表 | 表格化显示所有教师信息 |
| 6 | 工资统计 | 统计总人数、各项工资总额、平均/最高/最低工资 |
| 7 | 数据持久化 | 自动保存/加载SALV2格式二进制文件 |
| 8 | 内置自测 | 17项测试覆盖核心功能 |

### 工资计算公式

```
实发工资 = 基本工资 + 绩效工资 + 津贴补贴 - 扣款
```

---

## 3. 编译运行

### 环境要求

- 操作系统：Windows 10/11
- 编译器：GCC (MinGW)，支持C99标准
- 编译器路径：`D:/bin/gcc.exe`

### 使用Makefile编译

```bash
# 编译
make

# 编译并运行自测
make test

# 运行程序
make run

# 显示帮助
make help

# 清理生成文件
make clean
```

### 手动编译命令

```bash
D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o salary.exe main.c
```

### 运行方式

```bash
# 正常交互模式
./salary.exe

# 显示帮助信息
./salary.exe -h
./salary.exe --help

# 运行内置自测
./salary.exe -t
./salary.exe --test
```

---

## 4. 数据结构

###  Teacher 教师结构体

| 字段 | 类型 | 说明 |
|:---|:---|:---|
| id | int | 教师编号（自动分配，唯一） |
| name | char[32] | 姓名 |
| dept | char[32] | 所在部门 |
| title | char[32] | 职称 |
| base_salary | double | 基本工资 |
| merit_pay | double | 绩效工资 |
| allowance | double | 津贴补贴 |
| deduction | double | 扣款金额 |
| total | double | 实发工资（自动计算） |

###  TeacherArray 动态数组

| 字段 | 类型 | 说明 |
|:---|:---|:---|
| data | Teacher* | 数据指针 |
| size | int | 当前元素数量 |
| capacity | int | 当前分配容量 |
| next_id | int | 下一个可用ID |

动态数组初始容量为8，元素数量达到容量时自动翻倍扩容。

---

## 5. SALV2 文件格式

<span style="color:blue; font-weight:bold;">【V2.0新增】采用二进制格式 + 魔数校验，防止误读非本系统文件</span>

| 偏移 | 长度 | 内容 | 说明 |
|:---|:---|:---|:---|
| 0 | 5字节 | "SALV2" | 魔数标识 |
| 5 | 4字节 | next_id | 下一个可用ID（int） |
| 9 | 4字节 | size | 记录数量（int） |
| 13 | size*sizeof(Teacher) | Teacher[] | 教师记录数组 |

数据文件：`salary_v2.dat`（自动生成于当前目录）

---

## 6. 错误码说明

| 错误码 | 枚举值 | 中文说明 |
|:---:|:---|:---|
| 0 | SAL_OK | 操作成功 |
| 1 | SAL_ERR_MEMORY | 内存分配失败 |
| 2 | SAL_ERR_NOT_FOUND | 未找到记录 |
| 3 | SAL_ERR_DUPLICATE_ID | ID重复 |
| 4 | SAL_ERR_FILE_OPEN | 文件打开失败 |
| 5 | SAL_ERR_FILE_READ | 文件读取失败 |
| 6 | SAL_ERR_FILE_WRITE | 文件写入失败 |
| 7 | SAL_ERR_FILE_MAGIC | 文件格式错误(魔数不匹配) |
| 8 | SAL_ERR_FILE_VERSION | 文件版本不兼容 |
| 9 | SAL_ERR_INVALID_PARAM | 无效参数 |
| 10 | SAL_ERR_INVALID_INPUT | 无效输入 |
| 11 | SAL_ERR_EMPTY | 无数据记录 |
| 12 | SAL_ERR_IO | IO错误 |

---

## 7. 自测说明

<span style="color:blue; font-weight:bold;">【V2.0新增】内置17项自测，共39个测试断言，覆盖所有核心功能</span>

运行自测：`./salary.exe --test`

| 测试编号 | 测试内容 | 覆盖功能 |
|:---:|:---|:---|
| TC01 | 动态数组初始化 | 内存分配 |
| TC02 | 初始状态验证 | size/capacity/next_id |
| TC03 | 添加第一条记录 | CRUD-Create |
| TC04 | 工资计算验证 | 工资计算逻辑 |
| TC05 | 添加多条记录 | CRUD-Create |
| TC06 | 按ID查找 | CRUD-Read |
| TC07 | 查找不存在ID | 边界处理 |
| TC08 | 动态扩容测试 | 数组扩容 |
| TC09 | 工资统计 | 统计功能 |
| TC10 | 删除记录 | CRUD-Delete |
| TC11 | 删除越界处理 | 错误处理 |
| TC12 | 文件保存 | 文件I/O |
| TC13 | 文件加载 | 文件I/O |
| TC14 | 魔数错误检测 | 文件校验 |
| TC15 | 空文件处理 | 边界处理 |
| TC16 | 错误码字符串映射 | 错误处理 |
| TC17 | 内存释放 | 内存管理 |

---

##  版本历史

- **V1.0** - 初始版本，多表链表结构，文本文件存储
- **V2.0** - 全面重构升级（当前版本）

---

> **学号：{STUDENT_ID}** **班级：{CLASS_NAME}**  
> **作者：{AUTHOR}** **{VERSION}**  
> 文档生成时间：{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
"""
    return md_content

def save_backup_note():
    """生成V1备份说明"""
    return """
【V1备份说明】
V1版本文件已备份，请手动将原docx复制为 _V1_backup.docx
原docx文档更新时，请将V2.0新增特性用蓝色字体高亮标注：
  - UTF-8 BOM + SetConsoleOutputCP(65001)
  - ANSI彩色终端
  - CLI参数(-h/-t)
  - 动态数组
  - SalStatus错误码枚举
  - SALV2魔数文件格式
  - 17项内置自测
  - Makefile零警告编译
"""

def main():
    print("=" * 60)
    print(f"  {PROJECT_NAME} {VERSION} 文档生成工具")
    print("=" * 60)
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(script_dir)
    
    md_path = "README_V2.md"
    md_content = generate_markdown_doc()
    with open(md_path, "w", encoding="utf-8") as f:
        f.write(md_content)
    print(f"  [OK] Markdown文档已生成: {md_path}")
    
    readme_path = "README.md"
    with open(readme_path, "w", encoding="utf-8") as f:
        f.write(md_content)
    print(f"  [OK] README.md已更新")
    
    print(save_backup_note())
    
    print("=" * 60)
    print("  文档生成完成！")
    print("=" * 60)
    return 0

if __name__ == "__main__":
    sys.exit(main())
