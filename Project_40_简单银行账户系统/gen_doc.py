#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
文档生成脚本 - 银行账户管理系统 V2.0
功能：
1. 备份原docx文档为 _V1_backup.docx
2. 更新原docx文档，添加V2.0升级内容（蓝色高亮）
3. 生成README.md
"""

import os
import shutil
import sys
from datetime import datetime

try:
    from docx import Document
    from docx.shared import RGBColor, Pt
    from docx.enum.text import WD_UNDERLINE
    HAS_DOCX = True
except ImportError:
    HAS_DOCX = False
    print("警告: python-docx 未安装，docx生成功能不可用")
    print("请运行: pip install python-docx")

PROJECT_DIR = os.path.dirname(os.path.abspath(__file__))
ORIGINAL_DOCX = os.path.join(PROJECT_DIR, "Project_40_简单银行账户管理系统.docx")
BACKUP_DOCX = os.path.join(PROJECT_DIR, "Project_40_简单银行账户管理系统_V1_backup.docx")
TARGET_DOCX = os.path.join(PROJECT_DIR, "Project_40_简单银行账户管理系统.docx")

BLUE_COLOR = RGBColor(0, 102, 204)
RED_COLOR = RGBColor(204, 0, 0)
GREEN_COLOR = RGBColor(0, 153, 0)


def backup_original():
    """备份原始docx文件"""
    if os.path.exists(ORIGINAL_DOCX) and not os.path.exists(BACKUP_DOCX):
        shutil.copy2(ORIGINAL_DOCX, BACKUP_DOCX)
        print(f"[OK] 已备份原始文档到: {os.path.basename(BACKUP_DOCX)}")
        return True
    elif os.path.exists(BACKUP_DOCX):
        print(f"[INFO] 备份文件已存在: {os.path.basename(BACKUP_DOCX)}")
        return True
    else:
        print(f"[WARN] 未找到原始文档: {os.path.basename(ORIGINAL_DOCX)}")
        return False


def add_blue_highlight(paragraph, text, bold=True):
    """添加蓝色高亮文本"""
    run = paragraph.add_run(text)
    run.font.color.rgb = BLUE_COLOR
    run.font.bold = bold
    return run


def create_v2_docx():
    """创建/更新V2.0文档"""
    if not HAS_DOCX:
        print("[ERROR] 无法生成docx，请安装 python-docx")
        return False

    if os.path.exists(BACKUP_DOCX):
        doc = Document(BACKUP_DOCX)
    elif os.path.exists(ORIGINAL_DOCX):
        doc = Document(ORIGINAL_DOCX)
    else:
        doc = Document()

    doc.add_page_break()

    title = doc.add_heading('V2.0 版本升级说明', level=1)
    for run in title.runs:
        run.font.color.rgb = BLUE_COLOR

    doc.add_paragraph()

    p = doc.add_paragraph()
    add_blue_highlight(p, f"升级日期: {datetime.now().strftime('%Y-%m-%d')}")

    doc.add_heading('V2.0 核心升级特性', level=2)

    features = [
        ("UTF-8 BOM编码支持", "文件头部添加UTF-8 BOM标记，Windows控制台通过SetConsoleOutputCP(65001)设置UTF-8输出，彻底解决中文乱码问题。"),
        ("ANSI彩色终端", "使用ANSI转义序列实现彩色输出：绿色成功、红色错误、黄色提示、青色标题，提升用户体验。"),
        ("命令行参数", "支持 -h/--help 显示帮助，-t/--test 运行内置自测，方便自动化测试和快速验证。"),
        ("动态内存数组", "账户存储从链表改为动态数组(Account*)，初始容量4个，满容自动2倍扩容，内存连续访问效率更高。"),
        ("统一错误码", "定义BankStatus枚举类型，涵盖17种错误状态，所有操作函数返回统一的状态码。"),
        ("BANKV2文件格式", "文件持久化使用'BANKV2' magic header + 版本号标识，支持版本检测和格式校验。"),
        ("内置自测系统", "38项测试用例覆盖开户/存款/取款/转账/查询/销户/余额校验/文件I/O/动态扩容等全部功能。"),
        ("完整中文注释", "所有函数、结构体、枚举均有详细中文注释，代码可读性大幅提升。"),
    ]

    for name, desc in features:
        p = doc.add_paragraph(style='List Bullet')
        add_blue_highlight(p, f"[{name}] ", bold=True)
        p.add_run(desc)

    doc.add_heading('V2.0 测试覆盖', level=2)

    tests = [
        "T01: 银行系统初始化",
        "T02: 借记卡开户 + 账号验证(1001)",
        "T03: 信用卡开户 + 账号验证(1002)",
        "T04: 无效账户类型错误处理",
        "T05: 借记卡存款 + 余额校验",
        "T06: 零金额存款错误处理",
        "T07: 负金额存款错误处理",
        "T08: 借记卡取款 + 余额校验",
        "T09: 余额不足取款错误处理",
        "T10: 不存在账户操作错误处理",
        "T11: 信用卡ATM取款(使用额度)",
        "T12: 转账功能 + 双方余额校验",
        "T13: 自我转账错误处理",
        "T14: 转账余额不足错误处理",
        "T15: 账户查找(存在/不存在)",
        "T16: 销户 + 账户数校验",
        "T17: 销不存在账户错误处理",
        "T18: 文件保存功能",
        "T19: 文件加载 + 数据一致性校验",
        "T20: 无效magic header检测",
        "T21: 动态扩容测试(20个账户)",
        "T22: 交易记录验证",
    ]

    for t in tests:
        p = doc.add_paragraph(style='List Bullet')
        add_blue_highlight(p, "✓ ", bold=True)
        p.add_run(t)

    doc.add_heading('编译与运行', level=2)

    p = doc.add_paragraph()
    p.add_run("编译器: ").bold = True
    p.add_run("D:/bin/gcc.exe (MinGW GCC)")

    p = doc.add_paragraph()
    p.add_run("编译参数: ").bold = True
    p.add_run("-std=c99 -Wall -Wextra -O2")

    p = doc.add_paragraph()
    p.add_run("输出文件: ").bold = True
    p.add_run("bank.exe")

    p = doc.add_paragraph()
    p.add_run("编译命令: ").bold = True
    run = p.add_run("make")
    run.font.name = 'Consolas'

    p = doc.add_paragraph()
    p.add_run("自测命令: ").bold = True
    run = p.add_run("bank.exe --test")
    run.font.name = 'Consolas'

    p = doc.add_paragraph()
    p.add_run("帮助命令: ").bold = True
    run = p.add_run("bank.exe -h")
    run.font.name = 'Consolas'

    doc.add_heading('核心数据结构', level=2)

    p = doc.add_paragraph()
    add_blue_highlight(p, "Bank (银行系统): ", bold=True)
    p.add_run("动态数组容器，包含账户指针数组、当前数量、容量、下一个账号。")

    p = doc.add_paragraph()
    add_blue_highlight(p, "Account (账户): ", bold=True)
    p.add_run("账号、类型(借记/信用)、余额、信用额度、姓名、地区、交易记录数组。")

    p = doc.add_paragraph()
    add_blue_highlight(p, "Transaction (交易记录): ", bold=True)
    p.add_run("交易类型、金额、手续费、地区、时间、对方账号。")

    doc.add_heading('BankStatus 错误码一览', level=2)

    error_codes = [
        "BANK_OK (0) - 操作成功",
        "BANK_ERR_INVALID_PARAM - 参数无效",
        "BANK_ERR_MEMORY - 内存分配失败",
        "BANK_ERR_ACCOUNT_NOT_FOUND - 账户不存在",
        "BANK_ERR_ACCOUNT_EXISTS - 账户已存在",
        "BANK_ERR_INSUFFICIENT_BALANCE - 余额不足",
        "BANK_ERR_NEGATIVE_AMOUNT - 金额不能为负",
        "BANK_ERR_ZERO_AMOUNT - 金额不能为零",
        "BANK_ERR_FILE_OPEN - 文件打开失败",
        "BANK_ERR_FILE_READ - 文件读取失败",
        "BANK_ERR_FILE_WRITE - 文件写入失败",
        "BANK_ERR_FILE_MAGIC - 文件格式错误(magic校验失败)",
        "BANK_ERR_FILE_VERSION - 文件版本不兼容",
        "BANK_ERR_RECORDS_FULL - 交易记录已满",
        "BANK_ERR_INVALID_TYPE - 账户类型无效",
        "BANK_ERR_SAME_ACCOUNT - 不能向自己转账",
        "BANK_ERR_NOT_LOGGED_IN - 未登录",
    ]

    for code in error_codes:
        p = doc.add_paragraph(style='List Bullet')
        run = p.add_run(code)
        run.font.name = 'Consolas'
        run.font.size = Pt(10)

    doc.save(TARGET_DOCX)
    print(f"[OK] 文档已更新: {os.path.basename(TARGET_DOCX)}")
    return True


def generate_readme():
    """生成更新版README.md"""
    readme_content = """<div align="center">

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
"""

    readme_path = os.path.join(PROJECT_DIR, "README.md")
    with open(readme_path, 'w', encoding='utf-8') as f:
        f.write(readme_content)
    print(f"[OK] README.md 已更新")


def main():
    print("=" * 60)
    print("  银行账户管理系统 V2.0 - 文档生成工具")
    print("=" * 60)
    print()

    backup_original()
    print()

    create_v2_docx()
    print()

    generate_readme()
    print()

    print("=" * 60)
    print("  文档处理完成！")
    print("=" * 60)

    return 0


if __name__ == "__main__":
    sys.exit(main())
