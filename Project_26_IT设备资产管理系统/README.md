<div align="center">

# IT设备资产管理系统 V2.0

### IT Equipment Asset Management System V2.0

![Language](https://img.shields.io/badge/Language-C99-blue?style=for-the-badge&logo=c&logoColor=white)
![Data Structure](https://img.shields.io/badge/Data_Structure-动态数组-green?style=for-the-badge)
![Status](https://img.shields.io/badge/Status-V2.0_Release-brightgreen?style=for-the-badge)
![Project](https://img.shields.io/badge/Project-26-orange?style=for-the-badge)
![Tests](https://img.shields.io/badge/Tests-18_PASS-success?style=for-the-badge)

---

**作者：HuiJunzhuo**
**版本：V2.0**
**日期：2026**

---

*一个功能完善、代码规范、彩色交互、自带自测的IT设备资产管理系统*

</div>

---

## 目录

- [V2.0 升级亮点](#v20-升级亮点)
- [编译运行](#编译运行)
- [命令行参数](#命令行参数)
- [功能模块](#功能模块)
- [文件格式](#文件格式)
- [内置自测](#内置自测)
- [项目文件](#项目文件)

---

## V2.0 升级亮点

相比V1版本进行了**全面重写与架构升级**：

| 改进项 | V1 | V2.0 |
|:---|:---|:---|
| 编码 | GBK（中文乱码） | **UTF-8 BOM + SetConsoleOutputCP(65001)** |
| 终端输出 | 单色 | **ANSI彩色终端（8种颜色+粗体）** |
| CLI参数 | 无 | **-h/--help、-t/--test** |
| 数据结构 | 单链表 | **动态数组（2倍自动扩容，缓存友好）** |
| 错误处理 | 分散 | **统一 AssetStatus 错误码枚举（10种）** |
| 文件格式 | 无Magic裸结构体 | **Magic Header "ASSTV2" + 版本号** |
| 自测 | 无 | **18项全功能自测（CRUD/领用/统计/文件I/O/压力）** |
| 中文支持 | 乱码 | **完美UTF-8中文** |
| 编译警告 | 未严格验证 | **-Wall -Wextra -O2 零警告** |

---

## 编译运行

### 环境要求

- Windows 10/11（支持ANSI VT序列）或 Linux
- GCC 编译器（推荐 `D:\bin\gcc.exe`）
- C99 标准

### 编译命令

```bash
# 使用指定GCC
D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o asset.exe main.c

# 或使用Makefile
make          # 编译生成 asset.exe
make test     # 编译并运行自测
make run      # 编译并启动交互模式
make clean    # 清理生成文件
```

### 运行

```bash
asset.exe           # 启动交互菜单模式
asset.exe --help    # 查看帮助
asset.exe --test    # 运行内置自测
```

---

## 命令行参数

| 参数 | 说明 |
|:---|:---|
| `(无参数)` | 启动交互菜单模式 |
| `-h`, `--help` | 显示帮助信息 |
| `-t`, `--test` | 运行内置自测套件（18项） |

---

## 功能模块

### 1. 设备管理（CRUD）
- **添加**：名称/序列号必填，序列号唯一性校验，自动分配ID
- **查询**：ID精确查找 + 关键词模糊搜索（名称/品牌/序列号/领用人/型号）
- **修改**：保持ID不变，可更新所有字段，自动校验序列号冲突
- **删除**：二次确认防止误删
- **浏览**：彩色表格展示，状态颜色区分

### 2. 领用/归还
- **领用登记**：设备ID+领用人 → 状态自动变为"使用中"
- **归还登记**：设备ID → 状态自动恢复为"闲置"
- **状态约束**：已领用不可重复领、未领用不可归还、报废不可领用

### 3. 统计报表
- 设备总数、各状态（闲置/使用中/维修中/报废）数量统计
- 按状态统计资产价值
- 资产利用率自动计算 + 评级提示

### 4. 设备状态

| 状态 | 值 | 颜色 | 说明 |
|:---|:---:|:---:|:---|
| 闲置 | 0 | 绿色 | 在库可用 |
| 使用中 | 1 | 蓝色 | 已被领用 |
| 维修中 | 2 | 黄色 | 送修中 |
| 已报废 | 3 | 红色 | 不可再用 |

---

## 文件格式

数据文件 `assets.dat` 使用带Magic Header的二进制格式：

```
偏移  长度    内容
0     6B      Magic Header: "ASSTV2"
6     4B      版本号 (uint32 = 2)
10    4B      设备数量 count (int32)
14    4B      下一个ID next_id (int32)
18    ...     Asset 结构体连续存储
```

**安全机制**：
- 读取前验证Magic Header，防止误读非本系统文件
- 数量合法性校验（0~100000），防止损坏文件导致崩溃
- 读取失败自动回滚初始化

---

## 内置自测

V2.0内置18项自动化测试用例，运行 `asset.exe --test` 一键验证：

| 编号 | 测试项 | 模块 |
|:---:|:---|:---|
| T01 | 初始化空数组 | 动态数组 |
| T02 | 添加单个设备 | CRUD-C |
| T03 | 按ID查找设备 | CRUD-R |
| T04 | ID自增验证 | ID管理 |
| T05 | 重复序列号拒绝添加 | 约束校验 |
| T06 | 修改设备信息 | CRUD-U |
| T07 | 删除设备 | CRUD-D |
| T08 | 删除不存在设备返回错误 | 错误处理 |
| T09 | 设备领用流程 | 领用归还 |
| T10 | 重复领用拒绝 | 状态约束 |
| T11 | 设备归还流程 | 领用归还 |
| T12 | 未领用归还拒绝 | 状态约束 |
| T13 | 动态数组自动扩容 | 动态数组 |
| T14 | 文件保存加载（Magic Header） | 文件I/O |
| T15 | 魔数错误校验 | 格式校验 |
| T16 | 统计功能正确性 | 统计报表 |
| T17 | 批量添加1000台压力测试 | 性能 |
| T18 | 空参数校验 | 参数校验 |

**验证结果：18项全部PASS，0项FAIL**

---

## 项目文件

| 文件 | 说明 |
|:---|:---|
| `main.c` | 主程序源码（UTF-8 BOM, C99, ~1160行, 中文注释） |
| `Makefile` | 编译脚本 |
| `asset.exe` | 编译产物（可执行文件） |
| `assets.dat` | 数据持久化文件（首次运行自动生成） |
| `gen_doc.py` | 文档生成脚本（python-docx） |
| `Project_26_IT设备资产管理系统.docx` | 项目文档 V2.0 |
| `Project_26_IT设备资产管理系统_V1_backup.docx` | V1文档备份 |
| `README.md` | 本说明文件 |

---

> **作者：HuiJunzhuo** | **版本：V2.0** | **2026**
> 编译零警告 · 自测18项全通过 · 彩色交互 · Magic Header持久化
