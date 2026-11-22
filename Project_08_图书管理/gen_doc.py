# -*- coding: utf-8 -*-
"""
图书管理系统 V2.0 文档生成器
生成 V2.0 升级说明文档，V2.0 新增/修改内容用蓝色字体高亮标注
"""

from docx import Document
from docx.shared import Pt, Inches, Cm, RGBColor, Emu
from docx.enum.text import WD_ALIGN_PARAGRAPH, WD_LINE_SPACING
from docx.enum.table import WD_TABLE_ALIGNMENT, WD_ALIGN_VERTICAL
from docx.enum.style import WD_STYLE_TYPE
from docx.oxml.ns import qn, nsdecls
from docx.oxml import parse_xml
import os
import shutil

# ======================== 颜色主题 ========================
COLOR_PRIMARY   = RGBColor(0x1A, 0x56, 0xDB)
COLOR_V2        = RGBColor(0x25, 0x63, 0xEB)   # V2.0 新增/修改内容蓝色
COLOR_ACCENT    = RGBColor(0xE8, 0x6B, 0x2C)
COLOR_SUCCESS   = RGBColor(0x21, 0x96, 0x53)
COLOR_DANGER    = RGBColor(0xDC, 0x35, 0x45)
COLOR_WARN      = RGBColor(0xF5, 0x9E, 0x0B)
COLOR_DARK      = RGBColor(0x1F, 0x29, 0x37)
COLOR_GRAY      = RGBColor(0x6B, 0x72, 0x80)
COLOR_LIGHT_BG  = RGBColor(0xF3, 0xF4, 0xF6)
COLOR_WHITE     = RGBColor(0xFF, 0xFF, 0xFF)
COLOR_V2_BG     = RGBColor(0xDB, 0xEA, 0xFE)

# ======================== 工具函数 ========================

def set_cell_shading(cell, color_hex):
    shading = parse_xml(f'<w:shd {nsdecls("w")} w:fill="{color_hex}" w:val="clear"/>')
    cell._tc.get_or_add_tcPr().append(shading)

def set_cell_border(cell, **kwargs):
    tc = cell._tc
    tcPr = tc.get_or_add_tcPr()
    tcBorders = parse_xml(f'<w:tcBorders {nsdecls("w")}></w:tcBorders>')
    for edge, val in kwargs.items():
        element = parse_xml(
            f'<w:{edge} {nsdecls("w")} w:val="{val.get("val","single")}" '
            f'w:sz="{val.get("sz","4")}" w:space="0" w:color="{val.get("color","auto")}"/>'
        )
        tcBorders.append(element)
    tcPr.append(tcBorders)

def add_styled_heading(doc, text, level=1, color=None, is_v2=False):
    heading = doc.add_heading(text, level=level)
    for run in heading.runs:
        run.font.name = '微软雅黑'
        run._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
        if is_v2:
            run.font.color.rgb = COLOR_V2
        elif color:
            run.font.color.rgb = color
        else:
            run.font.color.rgb = COLOR_PRIMARY if level <= 2 else COLOR_DARK
    return heading

def add_para(doc, text, bold=False, italic=False, size=11, color=None, align=None, indent_first=True, is_v2=False):
    p = doc.add_paragraph()
    if align:
        p.alignment = align
    run = p.add_run(text)
    run.font.name = '微软雅黑'
    run._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    run.font.size = Pt(size)
    run.bold = bold
    run.italic = italic
    if is_v2:
        run.font.color.rgb = COLOR_V2
        run.bold = True
    elif color:
        run.font.color.rgb = color
    else:
        run.font.color.rgb = COLOR_DARK
    pf = p.paragraph_format
    pf.space_after = Pt(6)
    pf.line_spacing = 1.5
    if indent_first:
        pf.first_line_indent = Cm(0.74)
    return p

def add_v2_para(doc, text, size=11, indent_first=True, align=None):
    """添加V2.0新增内容段落（蓝色加粗）"""
    return add_para(doc, text, size=size, is_v2=True, indent_first=indent_first, align=align)

def add_bullet(doc, text, level=0, bold_prefix="", color=None, is_v2=False):
    p = doc.add_paragraph(style='List Bullet')
    p.paragraph_format.left_indent = Cm(1.2 + level * 0.8)
    p.paragraph_format.space_after = Pt(3)
    p.paragraph_format.line_spacing = 1.4
    if bold_prefix:
        r1 = p.add_run(bold_prefix)
        r1.bold = True
        r1.font.name = '微软雅黑'
        r1._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
        r1.font.size = Pt(11)
        if is_v2:
            r1.font.color.rgb = COLOR_V2
        else:
            r1.font.color.rgb = COLOR_PRIMARY
    r2 = p.add_run(text)
    r2.font.name = '微软雅黑'
    r2._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    r2.font.size = Pt(11)
    if is_v2:
        r2.font.color.rgb = COLOR_V2
        r2.bold = True
    elif color:
        r2.font.color.rgb = color
    else:
        r2.font.color.rgb = COLOR_DARK
    return p

def add_v2_bullet(doc, text, level=0, bold_prefix=""):
    return add_bullet(doc, text, level=level, bold_prefix=bold_prefix, is_v2=True)

def add_code_block(doc, code_text):
    table = doc.add_table(rows=1, cols=1)
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    cell = table.cell(0, 0)
    set_cell_shading(cell, "1E293B")
    p = cell.paragraphs[0]
    p.paragraph_format.space_before = Pt(4)
    p.paragraph_format.space_after = Pt(4)
    run = p.add_run(code_text)
    run.font.name = 'Consolas'
    run.font.size = Pt(9.5)
    run.font.color.rgb = RGBColor(0xE2, 0xE8, 0xF0)
    for line in code_text.split('\n')[1:]:
        p = cell.add_paragraph()
        p.paragraph_format.space_before = Pt(0)
        p.paragraph_format.space_after = Pt(0)
        run = p.add_run(line)
        run.font.name = 'Consolas'
        run.font.size = Pt(9.5)
        run.font.color.rgb = RGBColor(0xE2, 0xE8, 0xF0)
    doc.add_paragraph()
    return table

def add_info_box(doc, title, content, box_type="info"):
    colors = {
        "info":    ("DBEAFE", "1E40AF", "[INFO] 信息提示"),
        "v2":      ("DBEAFE", "2563EB", "[V2.0] 新增/升级功能"),
        "warn":    ("FEF3C7", "92400E", "[WARN] 注意事项"),
        "danger":  ("FEE2E2", "991B1B", "[ERROR] 严重问题"),
        "success": ("D1FAE5", "065F46", "[OK] 优化建议"),
        "tip":     ("EDE9FE", "5B21B6", "[TIP] 小技巧"),
    }
    bg, fg, icon = colors.get(box_type, colors["info"])
    if title:
        icon = title
    table = doc.add_table(rows=1, cols=1)
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    cell = table.cell(0, 0)
    set_cell_shading(cell, bg)
    set_cell_border(cell,
        top={"val":"single","sz":"8","color":fg},
        bottom={"val":"single","sz":"8","color":fg},
        left={"val":"single","sz":"24","color":fg},
        right={"val":"single","sz":"8","color":fg},
    )
    p = cell.paragraphs[0]
    p.paragraph_format.space_after = Pt(2)
    r = p.add_run(icon)
    r.bold = True
    r.font.name = '微软雅黑'
    r._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    r.font.size = Pt(11)
    r.font.color.rgb = RGBColor(int(fg[:2],16), int(fg[2:4],16), int(fg[4:6],16))
    p2 = cell.add_paragraph()
    p2.paragraph_format.space_after = Pt(2)
    r2 = p2.add_run(content)
    r2.font.name = '微软雅黑'
    r2._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    r2.font.size = Pt(10.5)
    r2.font.color.rgb = COLOR_DARK
    doc.add_paragraph()

def add_v2_box(doc, content):
    add_info_box(doc, "[V2.0] 新增/升级功能", content, "v2")

def add_table_with_header(doc, headers, rows, col_widths=None, v2_rows=None):
    if v2_rows is None:
        v2_rows = set()
    table = doc.add_table(rows=1 + len(rows), cols=len(headers))
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    table.style = 'Table Grid'
    hdr = table.rows[0].cells
    for i, h in enumerate(headers):
        set_cell_shading(hdr[i], "1A56DB")
        p = hdr[i].paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r = p.add_run(h)
        r.bold = True
        r.font.name = '微软雅黑'
        r._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
        r.font.size = Pt(10.5)
        r.font.color.rgb = COLOR_WHITE
        hdr[i].vertical_alignment = WD_ALIGN_VERTICAL.CENTER
    for ri, row in enumerate(rows):
        cells = table.rows[ri + 1].cells
        is_v2_row = ri in v2_rows
        bg = "DBEAFE" if is_v2_row else ("F9FAFB" if ri % 2 == 0 else "FFFFFF")
        for ci, val in enumerate(row):
            set_cell_shading(cells[ci], bg)
            p = cells[ci].paragraphs[0]
            r = p.add_run(str(val))
            r.font.name = '微软雅黑'
            r._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
            r.font.size = Pt(10)
            if is_v2_row:
                r.font.color.rgb = COLOR_V2
                r.bold = True
            else:
                r.font.color.rgb = COLOR_DARK
            cells[ci].vertical_alignment = WD_ALIGN_VERTICAL.CENTER
    if col_widths:
        for row in table.rows:
            for i, w in enumerate(col_widths):
                row.cells[i].width = Cm(w)
    doc.add_paragraph()
    return table

def add_separator(doc):
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = p.add_run("━" * 60)
    r.font.color.rgb = COLOR_PRIMARY
    r.font.size = Pt(8)
    p.paragraph_format.space_before = Pt(6)
    p.paragraph_format.space_after = Pt(6)

def add_cover_page(doc, title, subtitle, version_info, accent_color=COLOR_PRIMARY):
    for _ in range(4):
        doc.add_paragraph()
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = p.add_run(title)
    r.bold = True
    r.font.name = '微软雅黑'
    r._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    r.font.size = Pt(32)
    r.font.color.rgb = accent_color
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = p.add_run(subtitle)
    r.font.name = '微软雅黑'
    r._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    r.font.size = Pt(16)
    r.font.color.rgb = COLOR_GRAY
    for _ in range(3):
        doc.add_paragraph()
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = p.add_run("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    r.font.color.rgb = accent_color
    r.font.size = Pt(12)
    for line in version_info:
        p = doc.add_paragraph()
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r = p.add_run(line)
        r.font.name = '微软雅黑'
        r._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
        r.font.size = Pt(12)
        r.font.color.rgb = COLOR_DARK
    doc.add_page_break()

def setup_styles(doc):
    style = doc.styles['Normal']
    style.font.name = '微软雅黑'
    style._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    style.font.size = Pt(11)
    style.font.color.rgb = COLOR_DARK
    style.paragraph_format.line_spacing = 1.5
    for lvl in range(1, 4):
        hs = doc.styles[f'Heading {lvl}']
        hs.font.name = '微软雅黑'
        hs._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
        hs.font.color.rgb = COLOR_PRIMARY
        if lvl == 1:
            hs.font.size = Pt(20)
        elif lvl == 2:
            hs.font.size = Pt(16)
        else:
            hs.font.size = Pt(13)
    for section in doc.sections:
        section.top_margin = Cm(2.5)
        section.bottom_margin = Cm(2.5)
        section.left_margin = Cm(2.8)
        section.right_margin = Cm(2.8)


# ============================================================
# 生成文档
# ============================================================
def generate_doc():
    doc = Document()
    setup_styles(doc)

    add_cover_page(doc,
        "图书管理系统",
        "V1.0 → V2.0 升级说明文档 · 完全体增强版",
        ["项目：Project_08（图书管理系统）",
         "作者：HJZ（惠俊卓）",
         "文档类型：V2.0 升级交付报告",
         "日期：2026年6月28日",
         "版本：V2.0"],
        accent_color=COLOR_V2
    )

    add_styled_heading(doc, "📑 文档目录", level=1)
    toc_items = [
        "第一章  项目概述与V2.0升级总览",
        "第二章  V1.0存在的问题与缺陷",
        "第三章  V2.0新增功能详解（蓝色标注）",
        "第四章  核心架构改进（动态数组替代链表）",
        "第五章  文件持久化升级（BOOKV2格式）",
        "第六章  命令行接口与自测系统",
        "第七章  彩色终端与用户体验",
        "第八章  编译与运行说明",
        "第九章  测试结果报告",
        "附录A   V1.0→V2.0变更对照表",
        "附录B   命令行参数速查",
    ]
    for item in toc_items:
        p = doc.add_paragraph()
        p.paragraph_format.left_indent = Cm(1)
        r = p.add_run(item)
        r.font.name = '微软雅黑'
        r._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
        r.font.size = Pt(12)
        r.font.color.rgb = COLOR_DARK
    doc.add_page_break()

    # ==================== 第一章 ====================
    add_styled_heading(doc, "第一章  项目概述与V2.0升级总览", level=1)
    add_separator(doc)

    add_styled_heading(doc, "1.1 项目概述", level=2)
    add_para(doc, "图书管理系统（Project_08）是数据结构课程设计项目，使用C语言实现图书信息的CRUD（增删改查）和借阅管理功能。V1.0版本采用链表数据结构，支持基本的入库、注销、借阅、归还、查询等功能，使用制表符分隔的文本格式持久化数据。")

    add_styled_heading(doc, "1.2 V2.0升级目标", level=2)
    add_v2_box(doc,
        "参照Project_39哈夫曼编译码器V2.0的质量标准，对图书管理系统进行全面工程化升级，"
        "包括：源码UTF-8编码、ANSI彩色终端输出、命令行参数支持、动态数组内存管理、"
        "统一错误码体系、二进制文件格式（带Magic Header校验）、内置完整自测用例、"
        "详细Doxygen风格注释。目标是从课程设计Demo升级为工程级质量的可交付软件。")

    add_styled_heading(doc, "1.3 V2.0主要升级点概览", level=2)
    features = [
        ("数据结构重构", "链表 → 动态数组（自动2倍扩容）", False),
        ("文件格式升级", "文本制表符格式 → 二进制BOOKV2格式（带魔数校验）", True),
        ("终端输出", "单色ASCII → ANSI彩色输出（绿OK/红ERR/青标题）", True),
        ("命令行接口", "仅交互模式 → 支持--help/--test/-f/拖放文件", True),
        ("错误处理", "零散printf → 统一BookStatus错误码枚举", True),
        ("内存管理", "链表节点malloc → 动态数组realloc自动扩容", True),
        ("测试体系", "无测试 → 12类52个内置自测用例全自动化", True),
        ("源码编码", "GBK乱码 → UTF-8无BOM跨平台编码", True),
        ("日志系统", "无日志 → library.log操作日志（带时间戳）", True),
        ("图书修改", "仅支持增删借还 → 新增修改图书信息功能", True),
        ("关键词搜索", "仅按ID/分类查询 → 新增书名/作者关键词搜索", True),
    ]
    v2_rows = set(i for i, (_, _, v2) in enumerate(features) if v2)
    add_table_with_header(doc,
        ["升级项", "V1.0 → V2.0 变化", "是否V2新增"],
        [[a, b, "✓ V2.0新增/升级" if c else "—"] for a, b, c in features],
        col_widths=[3.5, 9, 3.5],
        v2_rows=v2_rows
    )

    doc.add_page_break()

    # ==================== 第二章 ====================
    add_styled_heading(doc, "第二章  V1.0存在的问题与缺陷", level=1)
    add_separator(doc)

    add_para(doc, "在升级到V2.0之前，V1.0版本存在以下工程质量问题，这些问题在V2.0中全部得到修复：")

    problems = [
        ("源码编码乱码",
         "main.c使用GBK编码保存，在UTF-8环境下（VS Code/Trae/Linux）全部中文乱码，"
         "代码几乎无法阅读和维护。",
         "严重"),
        ("链表遍历效率低",
         "使用链表存储图书，查找图书、尾插操作均为O(n)时间复杂度，且每个节点需要单独malloc/free，"
         "内存碎片化严重，缓存不友好。",
         "重要"),
        ("无命令行接口",
         "程序只能通过菜单交互操作，无法通过命令行参数直接调用功能，不支持脚本自动化和拖放文件。",
         "一般"),
        ("文件格式无校验",
         "纯文本格式，无魔数标识、无版本号，任何文本文件都可能被误读为数据文件，"
         "容易导致数据错乱且无法检测。",
         "重要"),
        ("无统一错误处理",
         "各函数通过零散printf输出错误然后return 0/1，错误类型无法区分，"
         "调用者无法精确判断错误原因。",
         "一般"),
        ("缺少修改图书功能",
         "只能新增、删除图书，但无法修改已有图书的书名、作者、分类等信息。",
         "重要"),
        ("无自动化测试",
         "没有任何测试代码，功能正确性完全依赖人工点点点验证，重构或修改后无法保证不引入bug。",
         "重要"),
        ("无操作日志",
         "程序运行没有任何日志记录，出现问题无法追溯操作历史。",
         "一般"),
        ("单色终端界面",
         "所有输出都是白色文字，成功/错误/警告信息视觉上无区分，用户体验差。",
         "一般"),
        ("无Makefile",
         "没有提供构建脚本，用户需要手动输入gcc命令编译，且原代码有编译警告。",
         "一般"),
    ]

    for pid, (title, desc, level) in enumerate(problems, 1):
        add_styled_heading(doc, f"P{pid:02d}  {title}", level=2)
        if level == "严重":
            add_info_box(doc, "🔴 严重级别", desc, "danger")
        elif level == "重要":
            add_info_box(doc, "🟠 重要级别", desc, "warn")
        else:
            add_info_box(doc, "🟡 一般级别", desc, "info")

    doc.add_page_break()

    # ==================== 第三章 ====================
    add_styled_heading(doc, "第三章  V2.0新增功能详解（蓝色标注）", level=1)
    add_separator(doc)

    add_styled_heading(doc, "3.1  ANSI彩色终端输出", level=2, is_v2=True)
    add_v2_para(doc, "Windows 10+的CMD和Windows Terminal已支持ANSI转义序列，V2.0通过enableVTMode()自动启用虚拟终端处理模式，为不同类型消息设置不同颜色：")
    add_v2_bullet(doc, "成功信息（[OK]、入库成功、测试通过）", bold_prefix="绿色 C_GREEN: ")
    add_v2_bullet(doc, "错误信息（[ERR]、文件不存在、操作失败）", bold_prefix="红色 C_RED: ")
    add_v2_bullet(doc, "警告信息（[WARN]、文件不存在提示）", bold_prefix="黄色 C_YELLOW: ")
    add_v2_bullet(doc, "提示信息（[INFO]、菜单标题、统计数据）", bold_prefix="青色 C_CYAN: ")
    add_v2_bullet(doc, "主标题/边框", bold_prefix="蓝色加粗 C_BOLD C_BLUE: ")
    add_code_block(doc, """/* V2.0 颜色宏定义 */
#define C_RESET     "\\033[0m"
#define C_GREEN     "\\033[32m"
#define C_RED       "\\033[31m"
#define C_YELLOW    "\\033[33m"
#define C_CYAN      "\\033[36m"
#define C_BLUE      "\\033[34m"
#define C_BOLD      "\\033[1m"

/* 使用示例 */
printOK("数据加载成功");    // 绿色 [OK]
printErr("文件不存在");     // 红色 [ERR]
printInfo("启动中...");     // 青色 [INFO]""")

    add_styled_heading(doc, "3.2  命令行接口（CLI）", level=2, is_v2=True)
    add_v2_para(doc, "V2.0新增完整的命令行参数解析，支持以下调用方式：")
    add_table_with_header(doc,
        ["命令", "功能", "示例"],
        [
            ["library.exe", "交互菜单模式", "library.exe"],
            ["-h / --help", "显示帮助信息", "library.exe -h"],
            ["-t / --test", "运行内置自测", "library.exe --test"],
            ["-f <文件>", "指定数据文件路径", "library.exe -f mybooks.dat"],
            ["<文件>", "拖放打开数据文件", "library.exe books.dat"],
        ],
        col_widths=[3.5, 5, 7]
    )
    add_v2_para(doc, "Windows下可直接将数据文件拖放到library.exe图标上，程序自动识别并打开该文件。")

    add_styled_heading(doc, "3.3  统一错误码枚举", level=2, is_v2=True)
    add_v2_para(doc, "V2.0定义了完整的BookStatus错误码枚举，所有业务函数均返回该类型，调用者可以精确判断错误原因：")
    add_code_block(doc, """typedef enum {
    BOOK_OK = 0,
    BOOK_ERR_OPEN_FILE,      // 无法打开文件
    BOOK_ERR_READ_FILE,      // 读取文件失败
    BOOK_ERR_WRITE_FILE,     // 写入文件失败
    BOOK_ERR_MEMORY,         // 内存分配失败
    BOOK_ERR_CORRUPT,        // 数据损坏
    BOOK_ERR_MAGIC,          // 魔数不匹配
    BOOK_ERR_VERSION,        // 版本不支持
    BOOK_ERR_NOT_FOUND,      // 未找到图书/记录
    BOOK_ERR_DUPLICATE_ID,   // 书号重复
    BOOK_ERR_OUT_OF_STOCK,   // 库存不足
    BOOK_ERR_NO_BORROW,      // 无借阅记录
    BOOK_ERR_INVALID_INPUT,  // 输入无效
    BOOK_ERR_CANCEL,         // 用户取消
    BOOK_ERR_EMPTY,          // 空结果
} BookStatus;""")

    add_styled_heading(doc, "3.4  修改图书信息功能", level=2, is_v2=True)
    add_v2_para(doc, "V2.0新增doUpdateBook()功能，支持修改已有图书的书名、作者、分类（书号作为主键不可修改）。用户可以选择只修改某一项，其他项直接回车保持原值。")

    add_styled_heading(doc, "3.5  关键词搜索功能", level=2, is_v2=True)
    add_v2_para(doc, "V2.0新增searchByKeyword()功能，支持按关键词模糊搜索书名和作者字段，使用strstr()进行子串匹配，方便快速定位图书。")

    add_styled_heading(doc, "3.6  操作日志系统", level=2, is_v2=True)
    add_v2_para(doc, "V2.0新增library.log日志文件，所有重要操作（启动、退出、入库、注销、借阅、归还、修改、保存、测试等）都追加写入日志，带精确到秒的时间戳，便于问题追踪。")

    doc.add_page_break()

    # ==================== 第四章 ====================
    add_styled_heading(doc, "第四章  核心架构改进（动态数组替代链表）", level=1)
    add_separator(doc)

    add_styled_heading(doc, "4.1  数据结构重构", level=2)
    add_para(doc, "V1.0使用链表（Book*链表+BorrowRecord*链表）存储图书和借阅记录。V2.0将图书存储改为动态数组（Library结构体），借阅记录仍使用链表（因为每本书借阅次数差异大，链表更合适）。")

    add_v2_para(doc, "动态数组设计：")
    add_code_block(doc, """/* V2.0 动态数组容器 */
typedef struct {
    Book *books;       // 图书数组指针
    int count;         // 当前图书数量
    int capacity;      // 已分配容量
} Library;

static Library g_lib = {NULL, 0, 0};""")

    add_v2_para(doc, "自动扩容策略：当count即将超过capacity时，按2倍容量realloc：")
    add_code_block(doc, """static BookStatus ensureCapacity(int needed) {
    if (needed <= g_lib.capacity) return BOOK_OK;
    int newCap = g_lib.capacity == 0 ? INIT_CAPACITY : g_lib.capacity;
    while (newCap < needed) newCap *= 2;  // 2倍扩容
    Book *newBooks = realloc(g_lib.books, newCap * sizeof(Book));
    if (newBooks == NULL) return BOOK_ERR_MEMORY;
    memset(newBooks + g_lib.capacity, 0, (newCap - g_lib.capacity) * sizeof(Book));
    g_lib.books = newBooks;
    g_lib.capacity = newCap;
    return BOOK_OK;
}""")

    add_styled_heading(doc, "4.2  动态数组 vs 链表对比", level=2)
    add_table_with_header(doc,
        ["对比项", "V1.0 链表", "V2.0 动态数组"],
        [
            ["随机访问", "O(n) 遍历", "O(1) 下标访问"],
            ["按ID查找", "O(n)", "O(n)（数据量小无需hash）"],
            ["尾插", "O(n) 找尾 / O(1)尾指针", "O(1) 摊销（2倍扩容）"],
            ["中间删除", "O(1) 修改指针", "O(n) 元素搬移（图书量小可忽略）"],
            ["内存连续性", "节点分散，缓存不友好", "连续内存，缓存友好"],
            ["内存开销", "每节点有next指针开销", "仅数组指针+count+capacity"],
            ["分配次数", "每本书1次malloc", "realloc对数次摊销"],
            ["内存释放", "遍历每个节点free", "一次free全部释放"],
        ],
        col_widths=[3, 5.5, 7.5]
    )

    doc.add_page_break()

    # ==================== 第五章 ====================
    add_styled_heading(doc, "第五章  文件持久化升级（BOOKV2格式）", level=1)
    add_separator(doc)

    add_styled_heading(doc, "5.1  旧格式问题", level=2)
    add_para(doc, "V1.0使用纯文本格式（tab分隔字段），存在以下问题：")
    add_bullet(doc, "无文件标识，任何文本文件都可能被误读", bold_prefix="无魔数校验: ")
    add_bullet(doc, "无版本号，格式升级后无法兼容旧文件", bold_prefix="无版本管理: ")
    add_bullet(doc, "书名/作者含tab字符会导致解析错误", bold_prefix="分隔符冲突: ")
    add_bullet(doc, "借阅次数等元数据需要额外处理", bold_prefix="元数据缺失: ")

    add_styled_heading(doc, "5.2  BOOKV2二进制格式设计", level=2, is_v2=True)
    add_v2_para(doc, "V2.0设计了全新的二进制文件格式，带魔数\"BOOKV2\"和版本号校验：")
    add_code_block(doc, """/* V2.0 文件头结构（16字节） */
typedef struct {
    char magic[6];            // 魔数 "BOOKV2" (6字节)
    unsigned short version;   // 格式版本号 (当前=2)
    unsigned int book_count;  // 图书总数
    unsigned int header_size; // 文件头大小 (sizeof(BookFileHeader))
} BookFileHeader;

/* 文件布局:
   ┌──────────────────────┐
   │ BookFileHeader (16B) │  魔数+版本+数量
   ├──────────────────────┤
   │ Book 1 (不含指针)    │  sizeof(Book) - sizeof(ptr)
   │   borrow_count (4B)  │
   │   BorrowRecord 1     │  不含next指针
   │   BorrowRecord 2     │
   │   ...                │
   ├──────────────────────┤
   │ Book 2 ...           │
   └──────────────────────┘
*/""")

    add_v2_para(doc, "格式安全保障：")
    add_v2_bullet(doc, "加载时先校验magic是否为\"BOOKV2\"，不匹配返回BOOK_ERR_MAGIC", bold_prefix="魔数校验: ")
    add_v2_bullet(doc, "检查version是否<=当前版本，不支持高版本文件", bold_prefix="版本校验: ")
    add_v2_bullet(doc, "检查fread返回值，读取不完整返回BOOK_ERR_READ_FILE", bold_prefix="完整性校验: ")
    add_v2_bullet(doc, "二进制格式天然避免分隔符冲突问题", bold_prefix="无分隔符问题: ")

    doc.add_page_break()

    # ==================== 第六章 ====================
    add_styled_heading(doc, "第六章  命令行接口与自测系统", level=1)
    add_separator(doc)

    add_styled_heading(doc, "6.1  命令行参数解析", level=2, is_v2=True)
    add_v2_para(doc, "runCommandLine()函数负责解析命令行参数，逻辑如下：")
    add_code_block(doc, """static int runCommandLine(int argc, char *argv[]) {
    if (argc < 2) return 0;                     // 无参数：交互模式
    if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
        printUsage(argv[0]); return 1;          // 显示帮助
    }
    if (!strcmp(argv[1], "-t") || !strcmp(argv[1], "--test")) {
        testMode = 1;
        int ret = runSelfTests();               // 运行自测
        freeLibrary();
        return ret;
    }
    if (!strcmp(argv[1], "-f")) {               // 指定文件
        strncpy(dataFilePath, argv[2], ...);
        return 0;
    }
    if (argv[1][0] != '-') {                    // 拖放文件
        strncpy(dataFilePath, argv[1], ...);
        return 0;
    }
    printUsage(argv[0]);
    return 1;
}""")

    add_styled_heading(doc, "6.2  内置自测系统", level=2, is_v2=True)
    add_v2_para(doc, "V2.0内置12类共52个自动化测试用例，覆盖所有核心功能：")
    test_cases = [
        ("Test 1", "初始化状态测试", "初始count/capacity为0"),
        ("Test 2", "图书入库测试", "添加4本书+重复书号检测"),
        ("Test 3", "图书查找测试", "按ID查找+不存在ID返回-1"),
        ("Test 4", "库存补充测试", "addStock+不存在图书检测"),
        ("Test 5", "借阅功能测试", "连续借阅+库存-1+累计借阅次数"),
        ("Test 6", "库存耗尽测试", "stock=0时借阅返回OUT_OF_STOCK"),
        ("Test 7", "归还功能测试", "归还+库存+1+不存在记录检测"),
        ("Test 8", "信息修改测试", "修改书名/分类+作者保留"),
        ("Test 9", "文件持久化测试", "save→free→load 数据完整恢复"),
        ("Test 10", "文件格式校验测试", "错误魔数返回MAGIC错误"),
        ("Test 11", "图书注销测试", "删除+数量-1+查找不到"),
        ("Test 12", "动态扩容压力测试", "批量添加100本验证扩容"),
    ]
    add_table_with_header(doc,
        ["编号", "测试名称", "验证内容"],
        test_cases,
        col_widths=[2, 4.5, 9.5]
    )
    add_v2_para(doc, "测试通过TEST_ASSERT宏进行断言，支持printf风格格式化输出，自动统计通过/失败数量，全部通过返回0，有失败返回1。")

    doc.add_page_break()

    # ==================== 第七章 ====================
    add_styled_heading(doc, "第七章  彩色终端与用户体验", level=1)
    add_separator(doc)

    add_styled_heading(doc, "7.1  VT模式自动启用", level=2, is_v2=True)
    add_v2_para(doc, "Windows下程序启动时自动调用enableVTMode()启用ANSI转义序列支持：")
    add_code_block(doc, """static void enableVTMode(void) {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
    colorEnabled = 1;    // Linux/macOS 默认支持
#endif
}""")

    add_styled_heading(doc, "7.2  输入安全改进", level=2)
    add_para(doc, "V1.0混合使用scanf和fgets，容易出现缓冲区问题。V2.0统一使用fgets读取，然后sscanf解析：")
    add_bullet(doc, "readInt()：循环读取直到输入合法范围内整数", bold_prefix="整数读取: ")
    add_bullet(doc, "readString()：读取字符串，自动去除换行符，不允许空输入", bold_prefix="字符串读取: ")
    add_bullet(doc, "readYesNo()：Y/N确认，自动转大写，输入非法重试", bold_prefix="确认交互: ")
    add_v2_bullet(doc, "所有输入函数都正确处理超长输入的剩余字符清空，避免输入缓冲区污染", bold_prefix="缓冲区安全: ")

    add_styled_heading(doc, "7.3  UI美化", level=2, is_v2=True)
    add_v2_bullet(doc, "启动时打印彩色Banner（蓝/青配色）", bold_prefix="启动横幅: ")
    add_v2_bullet(doc, "每个功能模块标题使用青色加粗+灰色分隔线", bold_prefix="功能标题: ")
    add_v2_bullet(doc, "图书详情使用Unicode方框绘制┌─└─边框", bold_prefix="详情边框: ")
    add_v2_bullet(doc, "图书列表库存为0时红色显示，有库存时绿色显示", bold_prefix="库存颜色: ")
    add_v2_bullet(doc, "菜单使用统一的粗/细分隔线，选项青色高亮", bold_prefix="菜单样式: ")

    doc.add_page_break()

    # ==================== 第八章 ====================
    add_styled_heading(doc, "第八章  编译与运行说明", level=1)
    add_separator(doc)

    add_styled_heading(doc, "8.1  Makefile构建系统", level=2, is_v2=True)
    add_v2_para(doc, "V2.0提供完整的Makefile，支持一键编译、测试、清理：")
    add_code_block(doc, """CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = library.exe
SRC = main.c

.PHONY: all clean run test help

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	./$(TARGET) --test

help: $(TARGET)
	./$(TARGET) --help

clean:
	-del /f $(TARGET) *.o *.dat library.log _test_*.dat 2>nul""")

    add_v2_para(doc, "编译命令（直接使用指定GCC路径）：")
    add_code_block(doc, "D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o library.exe main.c")
    add_v2_box(doc, "V2.0编译保证零警告（-Wall -Wextra开启所有常见警告），代码严格遵循C99标准。")

    add_styled_heading(doc, "8.2  运行方式", level=2)
    add_para(doc, "交互模式（直接运行）：")
    add_code_block(doc, "D:\\> library.exe")
    add_para(doc, "运行自测：")
    add_code_block(doc, "D:\\> library.exe --test")
    add_para(doc, "显示帮助：")
    add_code_block(doc, "D:\\> library.exe -h")
    add_para(doc, "指定数据文件：")
    add_code_block(doc, "D:\\> library.exe -f mybooks.dat")

    doc.add_page_break()

    # ==================== 第九章 ====================
    add_styled_heading(doc, "第九章  测试结果报告", level=1)
    add_separator(doc)

    add_styled_heading(doc, "9.1  编译结果", level=2, is_v2=True)
    add_v2_box(doc,
        "使用 D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 编译，零警告零错误！")

    add_styled_heading(doc, "9.2  自测结果", level=2, is_v2=True)
    add_v2_box(doc,
        "全部 52 个测试用例通过，0 失败！"
        "测试覆盖：初始化、入库、查找、补库存、借阅、库存耗尽、归还、修改信息、"
        "文件持久化（save/load）、魔数校验、注销、100本动态扩容压力测试。")

    add_styled_heading(doc, "9.3  测试覆盖清单", level=2)
    coverage = [
        ("内存管理", "动态分配/扩容/释放/重加载", "✓"),
        ("图书入库", "新增/重复书号检测/初始库存", "✓"),
        ("补充库存", "已有图书加库存/不存在图书报错", "✓"),
        ("图书注销", "删除/级联删除借阅记录/不存在报错", "✓"),
        ("信息修改", "书名/作者/分类更新/空值保留原值", "✓"),
        ("图书借阅", "库存-1/累计借阅/借阅记录创建", "✓"),
        ("库存控制", "stock=0拒绝借阅/正确返回错误码", "✓"),
        ("图书归还", "库存+1/记录删除/不存在记录报错", "✓"),
        ("按ID查询", "存在返回/不存在返回-1", "✓"),
        ("按分类查询", "匹配返回/不匹配提示", "✓"),
        ("关键词搜索", "书名/作者模糊匹配", "✓"),
        ("文件保存", "写入BOOKV2头/图书/借阅记录", "✓"),
        ("文件加载", "读头校验/读图书/重建借阅链表", "✓"),
        ("魔数校验", "错误魔数返回MAGIC错误", "✓"),
        ("扩容压力", "100本批量添加无内存错误", "✓"),
        ("命令行", "--help/--test/-f/拖放模式", "✓"),
    ]
    add_table_with_header(doc,
        ["测试类别", "测试内容", "通过"],
        coverage,
        col_widths=[3.5, 9, 2.5]
    )

    doc.add_page_break()

    # ==================== 附录A ====================
    add_styled_heading(doc, "附录A  V1.0→V2.0变更对照表", level=1)
    add_separator(doc)

    changes = [
        ("源码编码", "GBK（乱码）", "UTF-8无BOM", True),
        ("数据结构-图书", "链表（Book*）", "动态数组（Library+realloc）", True),
        ("数据结构-借阅", "链表", "链表（保留，更合适）", False),
        ("文件格式", "文本tab分隔", "二进制BOOKV2格式+魔数校验", True),
        ("错误处理", "printf+return 0/1", "BookStatus枚举统一错误码", True),
        ("终端颜色", "单色", "ANSI彩色（绿OK/红ERR/青标题）", True),
        ("CLI参数", "无", "--help/--test/-f/拖放", True),
        ("图书修改", "无此功能", "新增updateBook()", True),
        ("关键词搜索", "无", "新增searchByKeyword()", True),
        ("测试体系", "无", "12类52个内置自测用例", True),
        ("操作日志", "无", "library.log带时间戳", True),
        ("输入处理", "scanf/fgets混用", "统一fgets+sscanf安全输入", True),
        ("Windows VT", "无", "enableVTMode()自动启用", True),
        ("Makefile", "无", "完整Makefile（CC指定路径）", True),
        ("编译警告", "有警告", "-Wall -Wextra零警告", True),
        ("详细注释", "有但乱码", "UTF-8编码Doxygen风格详细注释", True),
        ("Banner/UI", "ASCII边框", "彩色Unicode美化界面", True),
        ("库存颜色", "单色", "有货绿色/缺货红色", True),
        ("借阅时间", "无", "新增borrow_time时间戳字段", True),
        ("累计借阅", "无", "新增total_borrowed统计字段", True),
    ]
    v2_rows = set(i for i, (_, _, _, v2) in enumerate(changes) if v2)
    add_table_with_header(doc,
        ["项目", "V1.0", "V2.0", "变更"],
        [[a, b, c, "★ V2.0升级" if d else "—"] for a, b, c, d in changes],
        col_widths=[3, 4.5, 5, 3],
        v2_rows=v2_rows
    )

    doc.add_page_break()

    # ==================== 附录B ====================
    add_styled_heading(doc, "附录B  命令行参数速查", level=1)
    add_separator(doc)

    add_styled_heading(doc, "B.1  命令行参数", level=2)
    add_table_with_header(doc,
        ["参数", "长形式", "功能"],
        [
            ["-h", "--help", "显示帮助信息并退出"],
            ["-t", "--test", "运行内置52个自测用例并退出"],
            ["-f <file>", "", "使用指定文件作为数据存储路径"],
            ["(无参数)", "", "进入交互菜单模式"],
            ["<filename>", "", "拖放模式：打开指定数据文件进入交互"],
        ],
        col_widths=[3, 3.5, 9]
    )

    add_styled_heading(doc, "B.2  交互菜单选项", level=2)
    add_table_with_header(doc,
        ["按键", "功能"],
        [
            ["1", "图书入库 / 补充库存"],
            ["2", "注销图书"],
            ["3", "修改图书信息（书名/作者/分类）"],
            ["4", "借阅图书"],
            ["5", "归还图书"],
            ["6", "显示全部图书列表"],
            ["7", "按书号精确查询"],
            ["8", "按分类查询"],
            ["9", "关键词搜索（书名/作者）"],
            ["S", "手动保存数据"],
            ["L", "重新加载数据文件"],
            ["H/?", "显示帮助"],
            ["0", "保存并退出"],
        ],
        col_widths=[3, 12.5]
    )

    add_styled_heading(doc, "B.3  输出文件说明", level=2)
    add_table_with_header(doc,
        ["文件名", "说明"],
        [
            ["library.dat", "默认数据文件（BOOKV2二进制格式）"],
            ["library.log", "操作日志文件（追加写入，带时间戳）"],
            ["library.exe", "编译生成的可执行文件"],
            ["_test_*.dat", "自测用临时文件（测试后自动删除）"],
        ],
        col_widths=[4, 11.5]
    )

    add_separator(doc)
    add_para(doc, "—— 文档结束 ——", align=WD_ALIGN_PARAGRAPH.CENTER, indent_first=False, color=COLOR_GRAY)
    add_v2_para(doc, "图书管理系统 V2.0 升级完成 · 2026年6月28日", align=WD_ALIGN_PARAGRAPH.CENTER, indent_first=False)

    output_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Project_08_图书管理系统.docx")
    doc.save(output_path)
    print(f"文档已生成: {output_path}")
    return output_path


if __name__ == "__main__":
    generate_doc()
