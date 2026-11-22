#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
gen_doc.py - 图书馆管理信息系统 V2.0 文档生成脚本
功能:
  1. 读取现有 docx 文档
  2. 添加 V2.0 升级章节
  3. 新内容使用蓝色高亮标记
  4. 自动备份 V1 版本
"""

import os
import sys
import shutil
from datetime import datetime

try:
    from docx import Document
    from docx.shared import Pt, RGBColor
    from docx.enum.text import WD_COLOR_INDEX
    HAS_DOCX = True
except ImportError:
    HAS_DOCX = False
    print("警告: python-docx 未安装, 将仅生成 Markdown 版本")
    print("安装命令: pip install python-docx")

PROJECT_DIR = os.path.dirname(os.path.abspath(__file__))
V1_BACKUP = os.path.join(PROJECT_DIR, "Project_23_图书馆管理信息系统_V1_backup.docx")
ORIGINAL_DOCX = os.path.join(PROJECT_DIR, "Project_23_图书馆管理信息系统.docx")
V2_DOCX = os.path.join(PROJECT_DIR, "Project_23_图书馆管理信息系统_V2.docx")
README_MD = os.path.join(PROJECT_DIR, "README.md")
MAIN_C = os.path.join(PROJECT_DIR, "main.c")

BLUE_COLOR = RGBColor(0, 102, 204)
HIGHLIGHT_COLOR = RGBColor(0, 120, 215)


def add_v2_section(doc):
    """添加V2.0升级章节，使用蓝色字体高亮"""
    doc.add_page_break()

    heading = doc.add_heading('V2.0 版本升级说明', level=1)
    for run in heading.runs:
        run.font.color.rgb = BLUE_COLOR
        run.font.bold = True

    p = doc.add_paragraph()
    run = p.add_run(f'升级日期: {datetime.now().strftime("%Y-%m-%d")}')
    run.font.color.rgb = BLUE_COLOR
    run.font.italic = True

    doc.add_heading('升级要点', level=2)

    upgrades = [
        ("UTF-8 BOM 编码支持",
         "源代码文件添加UTF-8 BOM标记，配合SetConsoleOutputCP(65001)和SetConsoleCP(65001)，完美解决Windows控制台中文乱码问题。"),
        ("ANSI 彩色终端输出",
         "实现错误红/成功绿/提示黄/标题青/加粗等彩色输出，提升交互体验。Windows下自动启用ENABLE_VIRTUAL_TERMINAL_PROCESSING。"),
        ("CLI 命令行参数",
         "支持 -h/--help 显示帮助信息, -t/--test 运行内置自测, 无参数启动交互菜单模式。"),
        ("动态数组数据结构",
         "从V1链表重构为动态数组(BookArray/ReaderArray/BorrowArray)，支持自动扩容(GROW_FACTOR=2)，随机访问O(1)，缓存友好。"),
        ("统一错误码枚举",
         "定义LibStatus枚举，包含18种明确错误码(OK/MEMORY/FILE/EXIST/NOT_FOUND/LIMIT等)，替代散乱的printf错误提示。"),
        ("Magic Header 文件持久化",
         "统一二进制文件library_v2.dat，开头magic \"LIBV2\"标记，防止旧文件/损坏文件误读，数据顺序写入：books→readers→borrows。"),
        ("内置自测框架",
         "25项自动化测试覆盖: 初始化/CRUD/重复校验/查询/借还/上限/文件I/O/数据一致性/参数校验，确保代码质量。"),
        ("系统统计功能",
         "新增SystemStats统计结构体，统计图书种类/总册数/可借数/借出数/读者数/借阅记录数/当前在借数。"),
        ("C99标准零警告",
         "使用D:\\bin\\gcc.exe -std=c99 -Wall -Wextra -O2编译，严格零警告。"),
        ("完整中文注释",
         "所有函数、结构体、关键逻辑均有详细中文注释，便于教学和维护。")
    ]

    for title, desc in upgrades:
        p = doc.add_paragraph(style='List Bullet')
        run_t = p.add_run(f'{title}: ')
        run_t.font.color.rgb = HIGHLIGHT_COLOR
        run_t.font.bold = True
        run_d = p.add_run(desc)
        run_d.font.color.rgb = BLUE_COLOR

    doc.add_heading('V2.0 数据结构对比', level=2)

    table = doc.add_table(rows=1, cols=3)
    table.style = 'Table Grid'
    hdr_cells = table.rows[0].cells
    hdr_cells[0].text = '项目'
    hdr_cells[1].text = 'V1 (链表)'
    hdr_cells[2].text = 'V2 (动态数组)'
    for cell in hdr_cells:
        for paragraph in cell.paragraphs:
            for run in paragraph.runs:
                run.font.bold = True
                run.font.color.rgb = BLUE_COLOR

    comparisons = [
        ('内存结构', '离散节点+next指针', '连续数组+count/capacity'),
        ('随机访问', 'O(n)', 'O(1)'),
        ('尾部插入', 'O(1)(头插)', 'O(1)(摊销)'),
        ('删除操作', 'O(1)(已知位置)', 'O(n)(元素移动)'),
        ('缓存友好性', '差(指针跳转)', '好(连续内存)'),
        ('文件格式', '4个独立.dat', '1个带magic的.dat'),
        ('错误处理', 'printf直接输出', 'LibStatus统一返回'),
        ('测试覆盖', '10项手动测试', '25项自动自测'),
        ('彩色输出', '无', 'ANSI彩色'),
        ('CLI参数', '无', '-h/-t支持')
    ]

    for item, v1, v2 in comparisons:
        row_cells = table.add_row().cells
        row_cells[0].text = item
        row_cells[1].text = v1
        row_cells[2].text = v2
        for cell in [row_cells[2]]:
            for paragraph in cell.paragraphs:
                for run in paragraph.runs:
                    run.font.color.rgb = BLUE_COLOR
                    run.font.bold = True

    doc.add_heading('编译与运行', level=2)

    p = doc.add_paragraph()
    p.add_run('编译器: ').bold = True
    run = p.add_run('D:\\bin\\gcc.exe (MinGW GCC)')
    run.font.color.rgb = BLUE_COLOR

    p = doc.add_paragraph()
    p.add_run('编译命令: ').bold = True
    run = p.add_run('make  或  gcc -std=c99 -Wall -Wextra -O2 -o lib.exe main.c')
    run.font.color.rgb = BLUE_COLOR
    run.font.name = 'Consolas'

    p = doc.add_paragraph()
    p.add_run('运行测试: ').bold = True
    run = p.add_run('lib.exe --test    (25项全通过)')
    run.font.color.rgb = BLUE_COLOR
    run.font.name = 'Consolas'

    p = doc.add_paragraph()
    p.add_run('查看帮助: ').bold = True
    run = p.add_run('lib.exe --help')
    run.font.color.rgb = BLUE_COLOR
    run.font.name = 'Consolas'

    p = doc.add_paragraph()
    p.add_run('交互模式: ').bold = True
    run = p.add_run('lib.exe    (无参数进入菜单)')
    run.font.color.rgb = BLUE_COLOR
    run.font.name = 'Consolas'

    doc.add_heading('测试用例清单 (25项)', level=2)

    tests = [
        "T01 系统初始化(动态数组创建)",
        "T02 添加图书(正常)",
        "T03 添加重复ISBN(应失败)",
        "T04 查询图书(存在)",
        "T05 查询不存在图书(应-1)",
        "T06 修改图书信息",
        "T07 添加读者(正常)",
        "T08 添加重复读者ID(应失败)",
        "T09 查询读者(存在)",
        "T10 借书成功",
        "T11 还书成功",
        "T12 归还无记录(应失败)",
        "T13 借不存在图书(应失败)",
        "T14 不存在读者借书(应失败)",
        "T15 统计功能验证",
        "T16 文件保存(save_data)",
        "T17 文件加载(load_data)",
        "T18 持久化数据一致性",
        "T19 借阅上限(5本)限制",
        "T20 删除借出中图书(应失败)",
        "T21 空输入参数校验",
        "T22 修改不存在图书(应失败)",
        "T23 删除不存在读者(应失败)",
        "T24 修改读者信息",
        "T25 删除图书(正常)"
    ]

    for t in tests:
        p = doc.add_paragraph(style='List Bullet')
        run = p.add_run(f'✓ {t}')
        run.font.color.rgb = BLUE_COLOR


def generate_markdown():
    """生成README.md V2版本"""
    md_content = '''<div align="center">

#  图书馆管理信息系统 V2.0

###  2025-2026学年第二学期算法实习

![Language](https://img.shields.io/badge/Language-C99-blue?style=for-the-badge&logo=c&logoColor=white)
![Data Structure](https://img.shields.io/badge/Data_Structure-动态数组+LIBV2文件-green?style=for-the-badge)
![Status](https://img.shields.io/badge/Status-V2.0_Released-brightgreen?style=for-the-badge)
![Tests](https://img.shields.io/badge/Tests-25_PASS-success?style=for-the-badge)
![Project](https://img.shields.io/badge/Project-23-orange?style=for-the-badge)

---

**开发者: 数据库侠客** · **学号:424123888888**
**班级组别:计算机组A班**
**作者:HuiJunzhuo**

---

*动态数组+LIBV2持久化实现图书馆管理信息系统V2.0*

</div>

---

## V2.0 全新升级

| 特性 | V1 | V2.0 |
|:---|:---|:---|
| 数据结构 | 链表 | 动态数组(自动扩容) |
| 文件格式 | 4个独立.dat | 1个带LIBV2 magic的.dat |
| 错误处理 | 散乱printf | LibStatus统一错误码 |
| 中文支持 | GBK乱码 | UTF-8 BOM + CP65001 |
| 终端输出 | 黑白 | ANSI彩色 |
| CLI参数 | 无 | -h/--help, -t/--test |
| 测试覆盖 | 10项手动 | 25项自动自测 |
| 编译警告 | - | -Wall -Wextra 零警告 |
| 统计功能 | 无 | SystemStats完整统计 |

---

## 快速开始

```bash
# 编译
make
# 或
D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o lib.exe main.c

# 运行自测(25项)
lib.exe --test

# 查看帮助
lib.exe --help

# 交互菜单模式
lib.exe
```

---

## 功能模块

1. **图书管理**: 添加/修改/删除/列表/关键字查询
2. **读者管理**: 添加/修改/删除/列表
3. **借还管理**: 借书/还书/借阅记录(全部/在借)
4. **查询统计**: 图书种类/总册数/可借/借出/读者数/在借数统计
5. **数据持久化**: LIBV2二进制格式自动保存/加载

---

## 文件说明

| 文件 | 说明 |
|:---|:---|
| main.c | V2.0完整源代码(UTF-8 BOM) |
| Makefile | GCC编译配置 |
| lib.exe | 编译输出可执行文件 |
| library_v2.dat | LIBV2格式数据文件(运行后生成) |
| gen_doc.py | 文档生成脚本 |
| *_V1_backup.docx | V1版本备份 |

---

## 测试结果

```
=== 图书馆管理系统 V2.0 内置自测 ===
  [PASS] T01-T25 全部25项测试
  测试结果: 25 通过, 0 失败
```

---

> **开发者: 数据库侠客** · **学号:424123888888** · **班级组别:计算机组A班**
> **作者:HuiJunzhuo** · **2025-2026学年第二学期算法实习**
'''
    with open(README_MD, 'w', encoding='utf-8') as f:
        f.write(md_content)
    print(f"[OK] 已更新 {README_MD}")


def main():
    print("=" * 60)
    print("  图书馆管理信息系统 V2.0 文档生成工具")
    print("=" * 60)

    generate_markdown()

    if not HAS_DOCX:
        print("\n提示: 安装 python-docx 可生成Word文档:")
        print("  pip install python-docx")
        print("\nMarkdown版本已生成完毕。")
        return

    if os.path.exists(ORIGINAL_DOCX):
        if not os.path.exists(V1_BACKUP):
            shutil.copy2(ORIGINAL_DOCX, V1_BACKUP)
            print(f"[OK] 已创建V1备份: {os.path.basename(V1_BACKUP)}")
        try:
            doc = Document(ORIGINAL_DOCX)
        except Exception:
            doc = Document()
    else:
        doc = Document()

    add_v2_section(doc)
    doc.save(V2_DOCX)
    print(f"[OK] 已生成V2文档: {os.path.basename(V2_DOCX)}")
    print("[OK] V2.0新内容已使用蓝色高亮标记")

    print("\n" + "=" * 60)
    print("  文档生成完成!")
    print("=" * 60)


if __name__ == '__main__':
    main()
