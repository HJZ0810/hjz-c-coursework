#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
快餐店点餐系统 V2.0 - 文档生成与更新工具
功能：
1. 备份原V1文档为 _V1_backup.docx
2. 在原文档中添加V2.0升级章节，蓝色高亮新特性
"""

import os
import shutil
import sys
from datetime import datetime

try:
    from docx import Document
    from docx.shared import Pt, RGBColor, Inches
    from docx.enum.text import WD_ALIGN_PARAGRAPH
    from docx.oxml.ns import qn
    from docx.oxml import OxmlElement
except ImportError:
    print("Error: python-docx not installed.")
    print("Install with: pip install python-docx")
    sys.exit(1)


def set_blue_highlight(run):
    """为文本添加蓝色背景高亮"""
    rPr = run._r.get_or_add_rPr()
    highlight = OxmlElement('w:highlight')
    highlight.set(qn('w:val'), 'blue')
    rPr.append(highlight)
    run.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)


def set_blue_font(run):
    """设置字体为蓝色"""
    run.font.color.rgb = RGBColor(0x00, 0x00, 0xFF)
    run.bold = True


def add_heading_blue(doc, text, level=1):
    """添加蓝色标题"""
    heading = doc.add_heading(level=level)
    run = heading.add_run(text)
    run.font.color.rgb = RGBColor(0x00, 0x00, 0x8B)
    return heading


def add_feature_item(doc, text, highlight=False):
    """添加特性项，可蓝色高亮"""
    p = doc.add_paragraph(style='List Bullet')
    run = p.add_run(text)
    if highlight:
        set_blue_highlight(run)
    else:
        run.font.size = Pt(11)
    return p


def main():
    project_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(project_dir)

    original_docx = "Project_30_快餐店点餐系统.docx"
    backup_docx = "Project_30_快餐店点餐系统_V1_backup.docx"
    temp_docx = "temp_v2_update.docx"

    print("=" * 60)
    print("  快餐店点餐系统 V2.0 - 文档更新工具")
    print("=" * 60)

    # 检查原文档是否存在
    if not os.path.exists(original_docx):
        print(f"Error: 原文档不存在: {original_docx}")
        return 1

    # 步骤1: 备份V1文档
    print(f"\n[1/3] 备份原V1文档 -> {backup_docx}")
    if os.path.exists(backup_docx):
        os.remove(backup_docx)
    shutil.copy2(original_docx, backup_docx)
    print("  ✓ 备份完成")

    # 步骤2: 打开原文档
    print(f"\n[2/3] 打开原文档并添加V2.0内容")
    doc = Document(original_docx)

    # 添加分页符
    doc.add_page_break()

    # 添加V2.0升级标题
    title = doc.add_heading(level=0)
    run = title.add_run("快餐店点餐系统 V2.0 升级说明")
    run.font.color.rgb = RGBColor(0x00, 0x00, 0x8B)
    title.alignment = WD_ALIGN_PARAGRAPH.CENTER

    # 添加日期
    date_p = doc.add_paragraph()
    date_p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    date_run = date_p.add_run(f"升级日期: {datetime.now().strftime('%Y-%m-%d')}")
    date_run.font.italic = True
    date_run.font.color.rgb = RGBColor(0x80, 0x80, 0x80)

    doc.add_paragraph()

    # 版本概述
    add_heading_blue(doc, "一、V2.0 版本概述", level=1)
    p = doc.add_paragraph()
    p.add_run("V2.0版本在V1.0基础上进行了全面架构升级，采用动态数组替代链表，新增统一错误处理、UTF-8编码支持、ANSI彩色终端、文件持久化和内置自测框架，代码质量与可维护性大幅提升。")

    # 核心新特性 - 蓝色高亮
    add_heading_blue(doc, "二、V2.0 核心新特性", level=1)

    features = [
        ("UTF-8 BOM + Windows控制台编码支持", True),
        ("SetConsoleOutputCP(65001) 中文显示优化", True),
        ("ANSI彩色终端输出（标题/成功/错误/提示）", True),
        ("CLI命令行参数 (-h/--help, -t/--test)", True),
        ("动态内存数组替代链表 (菜品/订单/VIP/收银员)", True),
        ("统一错误码枚举 FoodStatus", True),
        ('Magic Header "FOODV2" 文件持久化（防篡改/版本校验）', True),
        ("内置27项自测用例 (CRUD+点餐+结账+统计+文件I/O)", True),
        ("完善的交互菜单系统", True),
        ("全代码中文注释", True),
        ("VIP会员系统（普通/银卡/金卡 三级折扣）", False),
        ("收银员业绩统计", False),
        ("营业数据统计与查询", False),
        ("安全字符串处理 safe_strncpy", True),
        ("零警告编译 (gcc -std=c99 -Wall -Wextra -O2)", True),
    ]

    for text, highlighted in features:
        add_feature_item(doc, text, highlight=highlighted)

    # 编译与运行
    add_heading_blue(doc, "三、编译与运行", level=1)

    doc.add_paragraph("编译命令：", style='List Bullet')
    code_p = doc.add_paragraph()
    code_run = code_p.add_run("    D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o food.exe main.c")
    code_run.font.name = 'Consolas'
    code_run.font.size = Pt(10)
    set_blue_font(code_run)

    doc.add_paragraph("或使用Makefile：", style='List Bullet')
    code_p2 = doc.add_paragraph()
    code_run2 = code_p2.add_run("    make all")
    code_run2.font.name = 'Consolas'
    code_run2.font.size = Pt(10)

    doc.add_paragraph("运行自测：", style='List Bullet')
    code_p3 = doc.add_paragraph()
    code_run3 = code_p3.add_run("    food.exe --test")
    code_run3.font.name = 'Consolas'
    code_run3.font.size = Pt(10)
    set_blue_font(code_run3)

    doc.add_paragraph("交互模式：", style='List Bullet')
    code_p4 = doc.add_paragraph()
    code_run4 = code_p4.add_run("    food.exe")
    code_run4.font.name = 'Consolas'
    code_run4.font.size = Pt(10)

    # FoodStatus 错误码
    add_heading_blue(doc, "四、统一错误码 (FoodStatus)", level=1)

    error_codes = [
        "FOOD_OK (0) - 操作成功",
        "FOOD_ERR_MEMORY - 内存分配失败",
        "FOOD_ERR_NOT_FOUND - 项目未找到",
        "FOOD_ERR_DUPLICATE - 重复添加",
        "FOOD_ERR_INVALID_PARAM - 参数无效",
        "FOOD_ERR_STOCK_INSUFFICIENT - 库存不足",
        "FOOD_ERR_ALREADY_SETTLED - 订单已结算",
        "FOOD_ERR_FILE_OPEN - 文件打开失败",
        "FOOD_ERR_FILE_WRITE - 文件写入失败",
        "FOOD_ERR_FILE_READ - 文件读取失败",
        "FOOD_ERR_MAGIC_MISMATCH - 魔数不匹配（文件格式错误）",
        "FOOD_ERR_VERSION_MISMATCH - 版本不兼容",
        "FOOD_ERR_IO - IO操作错误",
    ]

    for code in error_codes:
        p = doc.add_paragraph(style='List Bullet')
        run = p.add_run(code)
        run.font.size = Pt(10)

    # 自测覆盖范围
    add_heading_blue(doc, "五、自测用例覆盖范围 (27项)", level=1)

    test_categories = [
        ("系统初始化", 1),
        ("菜品CRUD操作", 8),
        ("订单创建与VIP折扣", 4),
        ("结算与收银员业绩", 3),
        ("边界条件测试", 3),
        ("文件持久化 (FOODV2)", 3),
        ("VIP等级升级与折扣", 2),
    ]

    table = doc.add_table(rows=1, cols=2)
    table.style = 'Light Grid Accent 1'
    hdr_cells = table.rows[0].cells
    hdr_cells[0].text = '测试类别'
    hdr_cells[1].text = '用例数量'

    for category, count in test_categories:
        row_cells = table.add_row().cells
        row_cells[0].text = category
        row_cells[1].text = str(count)

    doc.add_paragraph()

    # 数据文件格式
    add_heading_blue(doc, "六、数据文件格式 (foodv2.dat)", level=1)

    p = doc.add_paragraph()
    p.add_run("文件头部包含6字节魔数 ").font.size = Pt(11)
    run_magic = p.add_run('"FOODV2"')
    run_magic.bold = True
    set_blue_font(run_magic)
    p.add_run(" 和版本号，用于校验文件完整性和版本兼容性。").font.size = Pt(11)

    doc.add_paragraph()

    # V1与V2对比
    add_heading_blue(doc, "七、V1 vs V2 架构对比", level=1)

    table2 = doc.add_table(rows=1, cols=3)
    table2.style = 'Light Grid Accent 1'
    hdr2 = table2.rows[0].cells
    hdr2[0].text = '对比项'
    hdr2[1].text = 'V1.0'
    hdr2[2].text = 'V2.0 (蓝色高亮为改进项)'

    comparisons = [
        ("数据结构", "单向链表", "动态数组 ✓"),
        ("错误处理", "零散返回值", "统一FoodStatus枚举 ✓"),
        ("中文支持", "无特殊处理", "UTF-8 BOM + SetConsoleOutputCP ✓"),
        ("终端输出", "黑白文本", "ANSI彩色输出 ✓"),
        ("CLI参数", "无", "-h/--help, -t/--test ✓"),
        ("持久化", "无/TXT格式", "FOODV2二进制+魔数校验 ✓"),
        ("自测框架", "无", "27项内置自测 ✓"),
        ("内存安全", "手动管理", "统一分配/释放+安全字符串 ✓"),
        ("编译警告", "可能存在", "gcc零警告 ✓"),
    ]

    for item, v1, v2 in comparisons:
        row = table2.add_row().cells
        row[0].text = item
        row[1].text = v1
        p2 = row[2].paragraphs[0]
        if "✓" in v2:
            run_v2 = p2.add_run(v2)
            set_blue_highlight(run_v2)
        else:
            p2.add_run(v2)

    # 保存文档
    print(f"\n[3/3] 保存更新后的文档")
    doc.save(temp_docx)

    # 替换原文件
    if os.path.exists(original_docx):
        os.remove(original_docx)
    shutil.move(temp_docx, original_docx)
    print("  ✓ 文档更新完成")

    print("\n" + "=" * 60)
    print("  文档处理完成!")
    print(f"  - 原V1备份: {backup_docx}")
    print(f"  - 已更新: {original_docx} (V2.0新特性蓝色高亮)")
    print("=" * 60)

    return 0


if __name__ == "__main__":
    sys.exit(main())
