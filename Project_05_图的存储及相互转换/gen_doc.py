# -*- coding: utf-8 -*-
"""
图的存储及相互转换 V2.0 文档生成器
更新现有Word文档，添加V2.0升级说明页
"""

from docx import Document
from docx.shared import Pt, Cm, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT, WD_ALIGN_VERTICAL
from docx.oxml.ns import qn, nsdecls
from docx.oxml import parse_xml
import os

COLOR_ORANGE = RGBColor(0xFF, 0x8C, 0x00)
COLOR_YELLOW_BG = "FFFF00"
COLOR_BLUE = RGBColor(0x1A, 0x56, 0xDB)
COLOR_GREEN = RGBColor(0x21, 0x96, 0x53)
COLOR_DARK = RGBColor(0x1F, 0x29, 0x37)
COLOR_GRAY = RGBColor(0x6B, 0x72, 0x80)
COLOR_WHITE = RGBColor(0xFF, 0xFF, 0xFF)
FONT_NAME = '微软雅黑'

def set_cell_shading(cell, color_hex):
    shading = parse_xml(f'<w:shd {nsdecls("w")} w:fill="{color_hex}" w:val="clear"/>')
    cell._tc.get_or_add_tcPr().append(shading)

def add_styled_heading(doc, text, level=1, color=None):
    heading = doc.add_heading(text, level=level)
    for run in heading.runs:
        run.font.name = FONT_NAME
        run._element.rPr.rFonts.set(qn('w:eastAsia'), FONT_NAME)
        if color:
            run.font.color.rgb = color
        else:
            run.font.color.rgb = COLOR_BLUE if level <= 2 else COLOR_DARK
    return heading

def add_para(doc, text, bold=False, italic=False, size=11, color=None, align=None, indent_first=True):
    p = doc.add_paragraph()
    if align:
        p.alignment = align
    run = p.add_run(text)
    run.font.name = FONT_NAME
    run._element.rPr.rFonts.set(qn('w:eastAsia'), FONT_NAME)
    run.font.size = Pt(size)
    run.bold = bold
    run.italic = italic
    if color:
        run.font.color.rgb = color
    else:
        run.font.color.rgb = COLOR_DARK
    pf = p.paragraph_format
    pf.space_after = Pt(6)
    pf.line_spacing = 1.5
    if indent_first:
        pf.first_line_indent = Cm(0.74)
    return p

def add_bullet(doc, text, level=0, bold_prefix="", color=None):
    p = doc.add_paragraph(style='List Bullet')
    p.paragraph_format.left_indent = Cm(1.2 + level * 0.8)
    p.paragraph_format.space_after = Pt(3)
    p.paragraph_format.line_spacing = 1.4
    if bold_prefix:
        r1 = p.add_run(bold_prefix)
        r1.bold = True
        r1.font.name = FONT_NAME
        r1._element.rPr.rFonts.set(qn('w:eastAsia'), FONT_NAME)
        r1.font.size = Pt(11)
        r1.font.color.rgb = COLOR_BLUE
    r2 = p.add_run(text)
    r2.font.name = FONT_NAME
    r2._element.rPr.rFonts.set(qn('w:eastAsia'), FONT_NAME)
    r2.font.size = Pt(11)
    if color:
        r2.font.color.rgb = color
    else:
        r2.font.color.rgb = COLOR_DARK
    return p

def add_v2_banner(doc):
    table = doc.add_table(rows=1, cols=1)
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    cell = table.cell(0, 0)
    set_cell_shading(cell, COLOR_YELLOW_BG)
    p = cell.paragraphs[0]
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p.paragraph_format.space_before = Pt(20)
    p.paragraph_format.space_after = Pt(20)
    run = p.add_run("🚀 V2.0 版本升级说明 🚀")
    run.bold = True
    run.font.name = FONT_NAME
    run._element.rPr.rFonts.set(qn('w:eastAsia'), FONT_NAME)
    run.font.size = Pt(28)
    run.font.color.rgb = COLOR_ORANGE
    doc.add_paragraph()

def main():
    doc_path = "Project_05_图的存储及相互转换.docx"
    if not os.path.exists(doc_path):
        print(f"! 文档不存在: {doc_path}，将创建新文档")
        doc = Document()
    else:
        doc = Document(doc_path)

    doc.add_page_break()

    add_v2_banner(doc)

    add_styled_heading(doc, "版本信息", level=1)
    info_table = doc.add_table(rows=5, cols=2)
    info_table.alignment = WD_TABLE_ALIGNMENT.CENTER
    info_table.style = 'Table Grid'
    info_data = [
        ("版本号", "V2.0"),
        ("升级日期", "2026年"),
        ("开发环境", "GCC (C99标准)"),
        ("主要改进", "动态内存、彩色UI、BFS/DFS可视化、最短路径"),
        ("保留功能", "邻接矩阵/邻接表存储、双向转换、有向/无向、带权/不带权"),
    ]
    for i, (k, v) in enumerate(info_data):
        cells = info_table.rows[i].cells
        cells[0].width = Cm(4)
        cells[1].width = Cm(12)
        p1 = cells[0].paragraphs[0]
        p1.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r1 = p1.add_run(k)
        r1.bold = True
        r1.font.name = FONT_NAME
        r1._element.rPr.rFonts.set(qn('w:eastAsia'), FONT_NAME)
        r1.font.size = Pt(11)
        r1.font.color.rgb = COLOR_WHITE
        set_cell_shading(cells[0], "1A56DB")
        p2 = cells[1].paragraphs[0]
        r2 = p2.add_run(v)
        r2.font.name = FONT_NAME
        r2._element.rPr.rFonts.set(qn('w:eastAsia'), FONT_NAME)
        r2.font.size = Pt(11)
        r2.font.color.rgb = COLOR_DARK
        cells[0].vertical_alignment = WD_ALIGN_VERTICAL.CENTER
        cells[1].vertical_alignment = WD_ALIGN_VERTICAL.CENTER
    doc.add_paragraph()

    add_styled_heading(doc, "V2.0 新增功能详解", level=1)

    add_styled_heading(doc, "1. 动态内存分配", level=2)
    add_para(doc, "V1.0版本使用静态数组 MAX_VERTEX=100 限制顶点数量，V2.0全面采用动态内存分配：")
    add_bullet(doc, "邻接表：vertices数组按需扩容，初始容量为4，自动2倍增长", bold_prefix="邻接表：")
    add_bullet(doc, "邻接矩阵：matrix为二维动态数组，vexnames为顶点名称动态数组", bold_prefix="邻接矩阵：")
    add_bullet(doc, "边表：使用链表动态管理，无固定上限", bold_prefix="边表：")
    add_bullet(doc, "所有malloc配套free，程序退出前自动释放全部内存，无内存泄漏", bold_prefix="内存管理：")

    add_styled_heading(doc, "2. 彩色终端UI", level=2)
    add_para(doc, "V2.0采用ANSI转义序列实现彩色终端输出，Windows 10+/Linux/macOS终端均支持：")
    colors = [
        ("绿色", COLOR_GREEN, "操作成功、欢迎信息、确认提示"),
        ("红色", RGBColor(0xDC,0x35,0x45), "错误提示、失败信息"),
        ("黄色", RGBColor(0xF5,0x9E,0x0B), "警告信息、注意事项"),
        ("蓝色", COLOR_BLUE, "菜单标题、表头、重要数据"),
        ("青色", RGBColor(0x06,0xB6,0xD4), "提示信息、普通文本高亮"),
        ("紫色", RGBColor(0x8B,0x5C,0xF6), "BFS/DFS访问顺序、路径结果"),
    ]
    color_table = doc.add_table(rows=1+len(colors), cols=3)
    color_table.alignment = WD_TABLE_ALIGNMENT.CENTER
    color_table.style = 'Table Grid'
    hdr = color_table.rows[0].cells
    for i, h in enumerate(["颜色", "RGB值", "使用场景"]):
        set_cell_shading(hdr[i], "1A56DB")
        p = hdr[i].paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r = p.add_run(h)
        r.bold = True
        r.font.name = FONT_NAME
        r._element.rPr.rFonts.set(qn('w:eastAsia'), FONT_NAME)
        r.font.size = Pt(10.5)
        r.font.color.rgb = COLOR_WHITE
    for ri, (name, rgb, scene) in enumerate(colors):
        cells = color_table.rows[ri+1].cells
        bg = "F9FAFB" if ri%2==0 else "FFFFFF"
        for c in cells: set_cell_shading(c, bg)
        p0 = cells[0].paragraphs[0]
        p0.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r0 = p0.add_run(name)
        r0.font.name = FONT_NAME
        r0._element.rPr.rFonts.set(qn('w:eastAsia'), FONT_NAME)
        r0.font.size = Pt(10)
        r0.font.color.rgb = rgb
        p1 = cells[1].paragraphs[0]
        p1.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r1 = p1.add_run(f"({rgb[0]},{rgb[1]},{rgb[2]})")
        r1.font.name = 'Consolas'
        r1.font.size = Pt(10)
        r1.font.color.rgb = COLOR_GRAY
        p2 = cells[2].paragraphs[0]
        r2 = p2.add_run(scene)
        r2.font.name = FONT_NAME
        r2._element.rPr.rFonts.set(qn('w:eastAsia'), FONT_NAME)
        r2.font.size = Pt(10)
        r2.font.color.rgb = COLOR_DARK
    doc.add_paragraph()

    add_styled_heading(doc, "3. 命令行参数支持", level=2)
    add_para(doc, "V2.0支持多种命令行参数，方便脚本调用和快速测试：")
    add_bullet(doc, "graph.exe --help / -h", bold_prefix="帮助：", color=COLOR_GRAY)
    add_para(doc, "显示完整使用帮助和参数说明", indent_first=False, size=10, color=COLOR_GRAY)
    add_bullet(doc, "graph.exe -t / --test", bold_prefix="自动测试：", color=COLOR_GRAY)
    add_para(doc, "运行内置自动测试用例，无需手动输入", indent_first=False, size=10, color=COLOR_GRAY)
    add_bullet(doc, "graph.exe -f filename.grph", bold_prefix="加载文件：", color=COLOR_GRAY)
    add_para(doc, "启动时自动加载指定图文件", indent_first=False, size=10, color=COLOR_GRAY)
    add_bullet(doc, "graph.exe (无参数)", bold_prefix="交互模式：", color=COLOR_GRAY)
    add_para(doc, "默认进入菜单交互模式", indent_first=False, size=10, color=COLOR_GRAY)

    add_styled_heading(doc, "4. 图统计信息", level=2)
    add_para(doc, "新增图统计面板，一键展示图的完整属性：")
    add_bullet(doc, "顶点总数、边总数", bold_prefix="基础统计：")
    add_bullet(doc, "图类型：有向/无向、带权/不带权", bold_prefix="类型判断：")
    add_bullet(doc, "图密度：实际边数/最大可能边数", bold_prefix="密度计算：")
    add_bullet(doc, "无向图：每个顶点的度", bold_prefix="度统计：")
    add_bullet(doc, "有向图：每个顶点的入度、出度、总度", bold_prefix="度统计：")

    add_styled_heading(doc, "5. BFS/DFS遍历（带彩色访问顺序）", level=2)
    add_para(doc, "V2.0完整实现了广度优先搜索(BFS)和深度优先搜索(DFS)遍历：")
    add_bullet(doc, "BFS使用队列实现，层序遍历", bold_prefix="BFS：")
    add_bullet(doc, "DFS提供递归实现", bold_prefix="DFS：")
    add_bullet(doc, "遍历结果用紫色高亮显示访问顺序", bold_prefix="彩色输出：")
    add_bullet(doc, "支持指定起始顶点", bold_prefix="起始点选择：")
    add_bullet(doc, "自动处理非连通图，遍历所有顶点", bold_prefix="非连通图：")

    add_styled_heading(doc, "6. 最短路径算法", level=2)
    add_para(doc, "V2.0实现两种最短路径算法：")
    add_bullet(doc, "适用于无权图，时间复杂度O(V+E)，输出完整路径", bold_prefix="BFS最短路径：")
    add_bullet(doc, "适用于非负权带权图，时间复杂度O(V²)（邻接矩阵），输出路径和总权重", bold_prefix="Dijkstra算法：")
    add_bullet(doc, "自动判断图类型选择合适算法，路径用蓝色高亮显示", bold_prefix="自动选择：")

    add_styled_heading(doc, "7. 文件持久化（带魔数校验）", level=2)
    add_para(doc, "V2.0的文件保存/加载功能新增魔数校验和版本号：")
    add_bullet(doc, "文件开头魔数 'GRPH' 用于验证文件格式", bold_prefix="魔数校验：")
    add_bullet(doc, "版本号字段支持未来向后兼容", bold_prefix="版本号：")
    add_bullet(doc, "保存所有图属性：类型标记、顶点数、边数、顶点名称、邻接关系、权重", bold_prefix="完整保存：")
    add_bullet(doc, "加载时验证魔数不匹配会提示错误，避免打开非图文件崩溃", bold_prefix="错误校验：")

    add_styled_heading(doc, "8. 改进的图可视化", level=2)
    add_para(doc, "邻接矩阵和邻接表显示效果大幅提升：")
    add_bullet(doc, "邻接矩阵带表头，行列对齐，∞ 表示无边，对角线显示0", bold_prefix="邻接矩阵：")
    add_bullet(doc, "邻接表每个顶点单独一行，边用 -> 连接，带权图显示权重", bold_prefix="邻接表：")
    add_bullet(doc, "使用Unicode制表符 ┌─┬─┐│ │├─┼─┤└─┴─┘ 美化边框", bold_prefix="边框美化：")

    add_styled_heading(doc, "V2.0 统一错误码", level=1)
    add_para(doc, "所有函数使用统一的 GraphStatus 枚举返回错误码，替代原来的零散0/-1返回值：")
    error_codes = [
        ("GRAPH_OK", "0", "操作成功"),
        ("GRAPH_ERR_MEMORY", "-1", "内存分配失败"),
        ("GRAPH_ERR_OPEN_FILE", "-2", "无法打开文件"),
        ("GRAPH_ERR_READ_FILE", "-3", "读取文件失败"),
        ("GRAPH_ERR_WRITE_FILE", "-4", "写入文件失败"),
        ("GRAPH_ERR_MAGIC", "-5", "文件魔数不匹配（不是本程序保存的图文件）"),
        ("GRAPH_ERR_VERSION", "-6", "文件版本不支持"),
        ("GRAPH_ERR_EMPTY", "-7", "图为空"),
        ("GRAPH_ERR_NOT_FOUND", "-8", "顶点不存在"),
        ("GRAPH_ERR_INVALID", "-9", "输入参数无效"),
    ]
    err_table = doc.add_table(rows=1+len(error_codes), cols=3)
    err_table.alignment = WD_TABLE_ALIGNMENT.CENTER
    err_table.style = 'Table Grid'
    hdr = err_table.rows[0].cells
    for i, h in enumerate(["错误码", "值", "含义"]):
        set_cell_shading(hdr[i], "DC3545")
        p = hdr[i].paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r = p.add_run(h)
        r.bold = True
        r.font.name = FONT_NAME
        r._element.rPr.rFonts.set(qn('w:eastAsia'), FONT_NAME)
        r.font.size = Pt(10.5)
        r.font.color.rgb = COLOR_WHITE
    for ri, (name, val, desc) in enumerate(error_codes):
        cells = err_table.rows[ri+1].cells
        bg = "F9FAFB" if ri%2==0 else "FFFFFF"
        for c in cells: set_cell_shading(c, bg)
        p0 = cells[0].paragraphs[0]
        r0 = p0.add_run(name)
        r0.font.name = 'Consolas'
        r0.font.size = Pt(10)
        r0.font.color.rgb = COLOR_BLUE
        p1 = cells[1].paragraphs[0]
        p1.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r1 = p1.add_run(val)
        r1.font.name = 'Consolas'
        r1.font.size = Pt(10)
        r1.font.color.rgb = COLOR_GRAY
        p2 = cells[2].paragraphs[0]
        r2 = p2.add_run(desc)
        r2.font.name = FONT_NAME
        r2._element.rPr.rFonts.set(qn('w:eastAsia'), FONT_NAME)
        r2.font.size = Pt(10)
        r2.font.color.rgb = COLOR_DARK
    doc.add_paragraph()

    def add_code_block_simple(doc, code_text):
        table = doc.add_table(rows=1, cols=1)
        table.alignment = WD_TABLE_ALIGNMENT.CENTER
        cell = table.cell(0, 0)
        set_cell_shading(cell, "1E293B")
        p = cell.paragraphs[0]
        p.paragraph_format.space_before = Pt(4)
        p.paragraph_format.space_after = Pt(4)
        p.paragraph_format.left_indent = Cm(0.5)
        run = p.add_run(code_text)
        run.font.name = 'Consolas'
        run.font.size = Pt(10)
        run.font.color.rgb = RGBColor(0xE2, 0xE8, 0xF0)
        doc.add_paragraph()

    add_styled_heading(doc, "编译与运行", level=1)
    add_para(doc, "V2.0使用Makefile构建，编译器为 C:\\bin\\gcc.exe：")
    add_para(doc, "编译命令：", bold=True, indent_first=False)
    add_code_block_simple(doc, "C:\\bin\\gcc.exe -std=c99 -Wall -O2 -o graph.exe main.c")
    add_para(doc, "或者使用make命令（MinGW环境）：", bold=True, indent_first=False)
    add_code_block_simple(doc, "mingw32-make")

    add_styled_heading(doc, "保留的V1.0功能", level=1)
    add_bullet(doc, "邻接矩阵（MatrixGraph）存储结构")
    add_bullet(doc, "邻接表（AdjListGraph）存储结构")
    add_bullet(doc, "邻接矩阵 → 邻接表 双向转换")
    add_bullet(doc, "邻接表 → 邻接矩阵 双向转换")
    add_bullet(doc, "支持有向图 / 无向图")
    add_bullet(doc, "支持带权图 / 不带权图")
    add_bullet(doc, "从键盘输入创建图")
    add_bullet(doc, "显示邻接矩阵")
    add_bullet(doc, "显示邻接表")
    add_bullet(doc, "文件保存与加载")

    add_styled_heading(doc, "数据结构对比", level=1)
    compare_table = doc.add_table(rows=6, cols=3)
    compare_table.alignment = WD_TABLE_ALIGNMENT.CENTER
    compare_table.style = 'Table Grid'
    hdr = compare_table.rows[0].cells
    for i, h in enumerate(["项目", "V1.0", "V2.0"]):
        set_cell_shading(hdr[i], "219653")
        p = hdr[i].paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r = p.add_run(h)
        r.bold = True
        r.font.name = FONT_NAME
        r._element.rPr.rFonts.set(qn('w:eastAsia'), FONT_NAME)
        r.font.size = Pt(11)
        r.font.color.rgb = COLOR_WHITE
    compare_data = [
        ("顶点上限", "MAX_VERTEX=100（固定）", "动态分配，无固定上限（受内存限制）"),
        ("源码编码", "GBK", "UTF-8"),
        ("界面颜色", "无（黑白）", "ANSI彩色输出"),
        ("遍历算法", "无", "BFS + DFS（彩色访问顺序）"),
        ("最短路径", "无", "BFS无权 + Dijkstra带权"),
    ]
    for ri, (item, v1, v2) in enumerate(compare_data):
        cells = compare_table.rows[ri+1].cells
        bg = "F9FAFB" if ri%2==0 else "FFFFFF"
        for c in cells: set_cell_shading(c, bg)
        p0 = cells[0].paragraphs[0]
        p0.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r0 = p0.add_run(item)
        r0.bold = True
        r0.font.name = FONT_NAME
        r0._element.rPr.rFonts.set(qn('w:eastAsia'), FONT_NAME)
        r0.font.size = Pt(10.5)
        r0.font.color.rgb = COLOR_BLUE
        p1 = cells[1].paragraphs[0]
        r1 = p1.add_run(v1)
        r1.font.name = FONT_NAME
        r1._element.rPr.rFonts.set(qn('w:eastAsia'), FONT_NAME)
        r1.font.size = Pt(10)
        r1.font.color.rgb = COLOR_GRAY
        p2 = cells[2].paragraphs[0]
        r2 = p2.add_run(v2)
        r2.font.name = FONT_NAME
        r2._element.rPr.rFonts.set(qn('w:eastAsia'), FONT_NAME)
        r2.font.size = Pt(10)
        r2.font.color.rgb = COLOR_GREEN
    doc.add_paragraph()

    add_para(doc, "—— V2.0升级说明结束 ——", align=WD_ALIGN_PARAGRAPH.CENTER, color=COLOR_GRAY, italic=True, indent_first=False)

    doc.save(doc_path)
    print(f"✓ 文档已更新: {doc_path}")

if __name__ == "__main__":
    main()
