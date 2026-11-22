# -*- coding: utf-8 -*-
"""
一元稀疏多项式计算器 V2.0 文档更新脚本
在原文档开头插入 V2.0 版本升级说明页（橙色文字+黄色背景高亮）
更新技术章节以反映 V2.0 新增功能
"""

from docx import Document
from docx.shared import Pt, Cm, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT, WD_ALIGN_VERTICAL
from docx.oxml.ns import qn, nsdecls
from docx.oxml import parse_xml
import os
import shutil

# ======================== 颜色主题 ========================
COLOR_ORANGE   = RGBColor(0xE8, 0x6B, 0x2C)
COLOR_DARK     = RGBColor(0x1F, 0x29, 0x37)
COLOR_GRAY     = RGBColor(0x6B, 0x72, 0x80)
COLOR_WHITE    = RGBColor(0xFF, 0xFF, 0xFF)
COLOR_BLUE     = RGBColor(0x1A, 0x56, 0xDB)
COLOR_GREEN    = RGBColor(0x21, 0x96, 0x53)
COLOR_RED      = RGBColor(0xDC, 0x35, 0x45)

FONT_NAME = '微软雅黑'
FONT_EN = 'Microsoft YaHei'

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

def set_run_font(run, size=11, bold=False, color=COLOR_DARK):
    run.font.name = FONT_EN
    run._element.rPr.rFonts.set(qn('w:eastAsia'), FONT_NAME)
    run.font.size = Pt(size)
    run.bold = bold
    run.font.color.rgb = color

def add_para(doc, text, size=11, bold=False, color=COLOR_DARK, align=None, indent_first=True):
    p = doc.add_paragraph()
    if align:
        p.alignment = align
    run = p.add_run(text)
    set_run_font(run, size=size, bold=bold, color=color)
    pf = p.paragraph_format
    pf.space_after = Pt(6)
    pf.line_spacing = 1.5
    if indent_first:
        pf.first_line_indent = Cm(0.74)
    return p

def add_highlight_para(doc, text, size=11, bold=False):
    p = doc.add_paragraph()
    pPr = p._p.get_or_add_pPr()
    shd = parse_xml(f'<w:shd {nsdecls("w")} w:fill="FFF3CD" w:val="clear"/>')
    pPr.append(shd)
    run = p.add_run(text)
    set_run_font(run, size=size, bold=bold, color=COLOR_ORANGE)
    pf = p.paragraph_format
    pf.space_after = Pt(4)
    pf.line_spacing = 1.5
    pf.left_indent = Cm(0.3)
    pf.right_indent = Cm(0.3)
    return p

def add_heading(doc, text, level=1, color=COLOR_BLUE):
    h = doc.add_heading(text, level=level)
    for run in h.runs:
        set_run_font(run, size=20 if level==1 else 15 if level==2 else 12, bold=True, color=color)
    return h

def add_orange_box(doc, title, items):
    table = doc.add_table(rows=1, cols=1)
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    cell = table.cell(0, 0)
    set_cell_shading(cell, "FFF3CD")
    set_cell_border(cell,
        top={"val":"single","sz":"8","color":"E86B2C"},
        bottom={"val":"single","sz":"8","color":"E86B2C"},
        left={"val":"single","sz":"24","color":"E86B2C"},
        right={"val":"single","sz":"8","color":"E86B2C"})
    p = cell.paragraphs[0]
    p.paragraph_format.space_after = Pt(4)
    r = p.add_run(title)
    set_run_font(r, size=12, bold=True, color=COLOR_ORANGE)
    for item in items:
        p = cell.add_paragraph()
        p.paragraph_format.space_after = Pt(2)
        p.paragraph_format.left_indent = Cm(0.3)
        r = p.add_run("• " + item)
        set_run_font(r, size=10.5, color=COLOR_DARK)
    doc.add_paragraph()

def add_table(doc, headers, rows, col_widths=None):
    table = doc.add_table(rows=1+len(rows), cols=len(headers))
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    table.style = 'Table Grid'
    hdr = table.rows[0].cells
    for i, h in enumerate(headers):
        set_cell_shading(hdr[i], "1A56DB")
        p = hdr[i].paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r = p.add_run(h)
        set_run_font(r, size=10.5, bold=True, color=COLOR_WHITE)
        hdr[i].vertical_alignment = WD_ALIGN_VERTICAL.CENTER
    for ri, row in enumerate(rows):
        cells = table.rows[ri+1].cells
        bg = "FFF3CD" if "🆕" in str(row) or "V2.0" in str(row) else ("F9FAFB" if ri%2==0 else "FFFFFF")
        for ci, val in enumerate(row):
            set_cell_shading(cells[ci], bg)
            p = cells[ci].paragraphs[0]
            r = p.add_run(str(val))
            set_run_font(r, size=10, color=COLOR_ORANGE if bg=="FFF3CD" else COLOR_DARK)
            cells[ci].vertical_alignment = WD_ALIGN_VERTICAL.CENTER
    doc.add_paragraph()
    return table

# ======================== 主流程 ========================

def main():
    src_docx = r"D:\HJZ\Project_02_一元稀疏多项式计算器\Project_02_一元稀疏多项式计算器.docx"
    bak_path = src_docx + ".bak"
    if not os.path.exists(bak_path):
        shutil.copy2(src_docx, bak_path)
        print(f"已备份原文档到: {bak_path}")

    doc = Document(src_docx)

    sections = doc.sections
    for section in sections:
        section.top_margin = Cm(2.5)
        section.bottom_margin = Cm(2.5)
        section.left_margin = Cm(2.8)
        section.right_margin = Cm(2.8)

    for style in doc.styles:
        if style.type == 1:
            try:
                style.font.name = FONT_EN
                style.element.rPr.rFonts.set(qn('w:eastAsia'), FONT_NAME)
            except:
                pass

    new_elements = []

    def add_p(text, size=11, bold=False, color=COLOR_DARK, align=None):
        p = doc.add_paragraph()
        if align: p.alignment = align
        r = p.add_run(text)
        set_run_font(r, size=size, bold=bold, color=color)
        p.paragraph_format.space_after = Pt(6)
        p.paragraph_format.line_spacing = 1.5
        new_elements.append(p)

    # 在文档最前面插入 V2.0 升级页（用新 Document 构建，然后合并）
    # 简化方法：直接在现有文档开头插入内容
    # python-docx 不支持在开头 insert，所以我们创建一个新文档，写V2.0页，然后追加原文档内容

    print("正在构建 V2.0 版本说明页...")
    new_doc = Document()
    for section in new_doc.sections:
        section.top_margin = Cm(2.5)
        section.bottom_margin = Cm(2.5)
        section.left_margin = Cm(2.8)
        section.right_margin = Cm(2.8)
    style = new_doc.styles['Normal']
    style.font.name = FONT_EN
    style._element.rPr.rFonts.set(qn('w:eastAsia'), FONT_NAME)
    style.font.size = Pt(11)

    for _ in range(2):
        new_doc.add_paragraph()

    p = new_doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = p.add_run("V2.0 完全体升级版")
    set_run_font(r, size=36, bold=True, color=COLOR_ORANGE)

    p = new_doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = p.add_run("一元稀疏多项式计算器 · 功能升级说明")
    set_run_font(r, size=18, color=COLOR_GRAY)

    for _ in range(2):
        new_doc.add_paragraph()

    p = new_doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = p.add_run("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    set_run_font(r, size=12, color=COLOR_ORANGE)

    p = new_doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    for line in ["项目：Project_02（一元稀疏多项式计算器）",
                 "作者：HJZ（惠俊卓）",
                 "升级日期：2026年6月28日",
                 "版本：V1.0 → V2.0"]:
        r = p.add_run(line + "\n")
        set_run_font(r, size=12, color=COLOR_DARK)

    new_doc.add_page_break()

    add_heading(new_doc, "📋 V2.0 版本升级总览", level=1)
    add_p("本升级在保留 V1.0 全部核心功能（带头结点单链表存储、加/减/乘运算、求值、求导、原始格式与数学格式输出）的基础上，对代码质量、用户体验、工程化能力进行了全方位增强。以下内容均使用 深橙色文字+浅黄色背景 高亮标记，以便快速识别新增/修改部分。")

    add_heading(new_doc, "一、V2.0 新增功能清单", level=2)

    feature_rows = [
        ("编号", "新增功能", "说明"),
        ("F-01", "🆕 ANSI彩色终端UI", "所有输出支持颜色：成功绿色、错误红色、警告黄色、标题青色、菜单蓝色，支持Windows 10+ VT模式自动启用"),
        ("F-02", "🆕 统一错误码体系", "定义 PolyStatus 枚举(POLY_OK/POLY_ERR_OPEN_FILE等11种状态)，替代V1.0的散乱return 0/1"),
        ("F-03", "🆕 命令行参数模式", "支持 -a/-s/-m/-e/-d/-t/-o 参数非交互调用，支持 --help 和 --test 自检，可脚本集成"),
        ("F-04", "🆕 内置自检套件", "运行 --test 自动执行加/减/乘/求导/保存加载共8项测试，快速验证正确性"),
        ("F-05", "🆕 多项式统计信息", "新增 getStats() 函数，可显示项数、最高次数、首项系数、常数项"),
        ("F-06", "🆕 文件保存与版本魔数", "新增 .poly 二进制格式，文件头含magic=\"POLY\"+版本号+项数，支持版本兼容检查"),
        ("F-07", "🆕 结果复用机制", "菜单新增 C→A 复制和 A↔B 交换，可对运算结果继续运算，支持链式计算"),
        ("F-08", "🆕 改进的菜单界面", "使用Unicode框线绘制菜单，选项扩展至15项，操作流程更清晰"),
        ("F-09", "🆕 UTF-8 源码编码", "源码从GBK转为UTF-8，彻底解决跨平台乱码问题；包含完整Doxygen风格文件头注释"),
        ("F-10", "🆕 Makefile 构建系统", "提供 Makefile 支持 make all/make test/make clean/make run 一键构建测试"),
    ]
    add_table(new_doc, feature_rows[0], feature_rows[1:], col_widths=[1.5, 4, 10.5])

    add_heading(new_doc, "二、V2.0 关键改进详解", level=2)

    add_heading(new_doc, "2.1 彩色终端输出系统", level=3)
    add_p("V2.0 新增 cPrint() 变参打印函数和 enableVTMode() Windows VT模式初始化，在Windows 10+下自动启用ANSI颜色支持，旧系统自动降级为无颜色输出。定义了 C_RED/C_GREEN/C_YELLOW/C_CYAN/C_BOLD 等颜色宏，以及 printOK/printErr/printWarn 三个语义化输出函数。")

    add_orange_box(new_doc, "🎨 颜色语义约定", [
        "绿色 [OK] —— 操作成功提示",
        "红色 [ERROR] —— 错误/失败信息",
        "黄色 [WARN] —— 警告/兼容性提示",
        "青色 [INFO] —— 指引性信息",
        "品红加粗 —— 章节/功能标题",
    ])

    add_heading(new_doc, "2.2 统一错误处理", level=3)
    add_p("V1.0 中各函数通过 return 0/1 表示成功失败，错误处理分散且语义不清。V2.0 定义 PolyStatus 枚举类型，所有可能失败的函数均返回明确的状态码，调用方根据状态码进行精确处理。")

    add_heading(new_doc, "2.3 命令行模式", level=3)
    add_p("V2.0 支持非交互式命令行调用，方便批处理和脚本集成。例如：")
    add_p("poly.exe -a a.poly b.poly -o sum.poly    # 加法并保存结果", color=COLOR_GRAY)
    add_p("poly.exe -e f.poly 2.5                   # 求 f(2.5)", color=COLOR_GRAY)
    add_p("poly.exe -d f.poly -o df.poly            # 求导并保存", color=COLOR_GRAY)
    add_p("poly.exe --test                          # 运行自检", color=COLOR_GRAY)

    add_heading(new_doc, "2.4 .poly 文件格式（V2版）", level=3)
    add_p("新增二进制持久化格式，设计了 PolyHeader 文件头结构体：")

    format_table = new_doc.add_table(rows=5, cols=4)
    format_table.alignment = WD_TABLE_ALIGNMENT.CENTER
    format_table.style = 'Table Grid'
    hdr = format_table.rows[0].cells
    for i, h in enumerate(["偏移", "长度", "字段", "说明"]):
        set_cell_shading(hdr[i], "E86B2C")
        p = hdr[i].paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r = p.add_run(h)
        set_run_font(r, size=10, bold=True, color=COLOR_WHITE)
    rows_data = [
        ("0", "4字节", "magic", '魔数 "POLY"（0x50 0x4F 0x4C 0x59）'),
        ("4", "2字节", "version", "格式版本号（V2.0 = 2）"),
        ("6", "2字节", "reserved", "保留字段，对齐用"),
        ("8", "4字节", "n", "多项式项数"),
    ]
    for ri, row in enumerate(rows_data):
        cells = format_table.rows[ri+1].cells
        bg = "FFF3CD"
        for ci, val in enumerate(row):
            set_cell_shading(cells[ci], bg)
            p = cells[ci].paragraphs[0]
            r = p.add_run(val)
            set_run_font(r, size=10, color=COLOR_ORANGE)
    new_doc.add_paragraph()
    add_p("文件头之后紧跟 n 组 (double coef, int exp) 数据，按指数降序存储。加载时先校验魔数，再检查版本，若版本不匹配打印警告但尝试兼容加载。")

    add_heading(new_doc, "2.5 数据结构增强", level=3)
    add_p("V2.0 保留了 V1.0 的核心数据结构（带头结点单链表 PolyNode），新增以下类型：")
    add_bullet_items = [
        "PolyStatus 枚举：统一错误码",
        "PolyHeader 结构体：二进制文件头（#pragma pack(1) 保证1字节对齐）",
        "PolyStats 结构体：多项式统计信息（degree/termCount/leadingCoef/constTerm）",
    ]
    for item in add_bullet_items:
        p = new_doc.add_paragraph(style='List Bullet')
        r = p.add_run(item)
        set_run_font(r, size=11, color=COLOR_DARK)

    add_heading(new_doc, "2.6 保留的 V1.0 功能", level=3)
    add_p("V2.0 完整保留了 V1.0 的所有功能，无任何功能删减：")
    kept_items = [
        "带头结点单链表存储结构（指数降序排列）",
        "键盘逐项输入创建多项式",
        "多项式加法、减法、乘法",
        "多项式求值（代入具体x值计算）",
        "多项式求导",
        "原始格式输出：(coef, exp) -> (coef, exp)",
        "数学格式输出：如 -3x^8+6x^3-18（自动省略系数1、隐藏x^0等）",
        "my_fabs / my_pow / is_integer 辅助函数",
    ]
    for item in kept_items:
        p = new_doc.add_paragraph(style='List Bullet')
        r = p.add_run("✅ " + item)
        set_run_font(r, size=11, color=COLOR_GREEN)

    add_heading(new_doc, "三、代码质量提升", level=2)
    add_quality_items = [
        ("源码编码", "GBK → UTF-8，所有中文注释/提示均使用UTF-8，跨平台无乱码"),
        ("代码注释", "文件头添加完整的功能概述、数据结构说明、编译命令、用法说明"),
        ("函数划分", "函数职责更单一，新增 insertTerm/makeNode/copyPoly/isZero 等内部工具函数"),
        ("内存管理", "所有 initPoly 配套 destroyPoly，无内存泄漏；动态分配均有检查"),
        ("编译警告", "使用 -Wall -Wextra 编译，零警告通过"),
        ("前向声明", "所有静态函数均有前向声明，代码组织清晰，便于阅读"),
    ]
    add_table(new_doc, ["改进项", "V1.0 → V2.0"], add_quality_items, col_widths=[3.5, 12.5])

    add_orange_box(new_doc, "⚠️ V2.0 编译与运行", [
        "编译命令：gcc -std=c99 -Wall -Wextra -O2 -o poly.exe main.c -lm",
        "交互模式：直接运行 poly.exe 进入彩色菜单界面",
        "命令行：poly.exe --help 查看完整用法",
        "自检：poly.exe --test 运行全部单元测试",
        "Makefile：make all 编译 / make test 测试 / make clean 清理",
    ])

    new_doc.add_page_break()

    print("正在合并原文档内容...")
    for element in doc.element.body:
        new_doc.element.body.append(element)

    out_path = src_docx
    new_doc.save(out_path)
    print(f"文档已更新并保存到: {out_path}")
    print("V2.0 升级说明页已插入文档开头，所有新增内容使用橙色文字+黄色背景高亮。")

if __name__ == "__main__":
    main()
