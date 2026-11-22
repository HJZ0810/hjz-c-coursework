<div align="center">

# 🏦 简单银行账户管理系统 V2.0

### 📚 2025-2026学年第二学期C语言课程实习

![Language](https://img.shields.io/badge/Language-C99-blue?style=for-the-badge&logo=c&logoColor=white)
![Version](https://img.shields.io/badge/Version-V2.0-green?style=for-the-badge)
![Status](https://img.shields.io/badge/Status-Completed-brightgreen?style=for-the-badge)
![Project](https://img.shields.io/badge/Project-40-orange?style=for-the-badge)
![Tests](https://img.shields.io/badge/Tests-38%20Passed-success?style=for-the-badge)

---

</div>

---

## 📋 V2.0 升级亮点

| 特性 | V1.0 | V2.0 |
|:---|:---:|:---:|
| 编码 | GBK/ANSI | **UTF-8 BOM + SetConsoleOutputCP(65001)** |
| 终端输出 | 黑白 | **ANSI彩色终端(绿/红/黄/青)** |
| CLI参数 | 无 | **-h/--help, -t/--test** |
| 数据结构 | 链表 | **动态数组(自动扩容)** |
| 错误处理 | printf提示 | **统一BankStatus枚举(17种)** |
| 文件格式 | 无标识 | **BANKV2 magic header + 版本号** |
| 自测 | 无 | **38项内置自测** |
| 编译警告 | 有 | **零警告 (-Wall -Wextra)** |

---

## 🚀 快速开始

### 编译

```bash
# 使用Makefile
make

# 或直接编译
D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o bank.exe main.c
```

### 运行自测

```bash
bank.exe --test
```

### 查看帮助

```bash
bank.exe -h
```

### 交互模式

直接运行 `bank.exe` 进入交互菜单。

---

## ✅ 测试结果

```
══════════════════════════════════════════════════════════
  自测结果: 38 通过, 0 失败
══════════════════════════════════════════════════════════
  ✓ 所有测试通过！
══════════════════════════════════════════════════════════
```

测试覆盖：开户、存款、取款、转账、查询、销户、余额校验、文件I/O、动态扩容、错误处理。

---

## 📁 项目文件

| 文件 | 说明 |
|:---|:---|
| `main.c` | 主程序源码 (V2.0) |
| `Makefile` | 编译配置 |
| `bank.exe` | 编译生成的可执行文件 |
| `gen_doc.py` | 文档生成脚本 |
| `Project_40_简单银行账户管理系统.docx` | 项目文档（已更新V2.0蓝色高亮） |
| `Project_40_简单银行账户管理系统_V1_backup.docx` | V1.0文档备份 |

---

## 🎮 交互菜单

```
  1.开借记卡  2.开信用卡  3.存款      4.取款
  5.转账      6.查余额    7.明细      8.销户
  9.账户列表  10.保存     11.加载     0.退出
```

---

## 🏗️ 核心架构

### BankStatus 错误码枚举

V2.0 统一使用 `BankStatus` 枚举作为所有操作的返回值，共定义17种状态码：

```c
typedef enum {
    BANK_OK = 0,                    // 操作成功
    BANK_ERR_INVALID_PARAM,         // 参数无效
    BANK_ERR_MEMORY,                // 内存分配失败
    BANK_ERR_ACCOUNT_NOT_FOUND,     // 账户不存在
    BANK_ERR_INSUFFICIENT_BALANCE,  // 余额不足
    BANK_ERR_ZERO_AMOUNT,           // 金额不能为零
    BANK_ERR_FILE_MAGIC,            // 文件格式错误
    // ... 共17种
} BankStatus;
```

### BANKV2 文件格式

持久化文件使用自定义二进制格式，开头为 magic header：

```
+---------+---------+---------+------------------+-----+
| "BANKV2"| version |  count  | next_account_no  | ... |
| 6 bytes | 4 bytes | 4 bytes |     4 bytes      | ... |
+---------+---------+---------+------------------+-----+
```

---

## 📊 编译器信息

- **编译器路径**: `D:/bin/gcc.exe`
- **标准**: C99
- **警告选项**: `-Wall -Wextra`
- **优化级别**: `-O2`
- **编译结果**: 零警告，零错误

---

*HuiJunzhuo - 2025-2026学年第二学期C语言课程实习 - Project 40*
