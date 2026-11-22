# -*- coding: utf-8 -*-
"""
gen_doc.py - Kindergarten Student Management V2.0 Documentation Generator
Updates Project_03_幼儿园培训班学生信息管理.docx with V2.0 change log and technical sections.
"""

from docx import Document
from docx.shared import Pt, RGBColor, Inches, Cm
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.style import WD_STYLE_TYPE
from docx.oxml.ns import qn
from docx.oxml import OxmlElement
import os

DOCX_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                          'Project_03_幼儿园培训班学生信息管理.docx')
FONT_NAME = 'Microsoft YaHei'
FONT_NAME_EN = 'Microsoft YaHei'

ORANGE = RGBColor(0xFF, 0x8C, 0x00)
YELLOW_BG = 'FFFF00'
BLUE = RGBColor(0x00, 0x70, 0xC0)
BLACK = RGBColor(0x00, 0x00, 0x00)
GRAY = RGBColor(0x60, 0x60, 0x60)
GREEN = RGBColor(0x00, 0x80, 0x00)
RED = RGBColor(0xC0, 0x00, 0x00)


def set_cell_shading(cell, color_hex):
    """Set cell background color."""
    shading_elm = OxmlElement('w:shd')
    shading_elm.set(qn('w:fill'), color_hex)
    shading_elm.set(qn('w:val'), 'clear')
    cell._tc.get_or_add_tcPr().append(shading_elm)


def set_paragraph_shading(paragraph, color_hex):
    """Set paragraph background color."""
    shading_elm = OxmlElement('w:shd')
    shading_elm.set(qn('w:fill'), color_hex)
    shading_elm.set(qn('w:val'), 'clear')
    paragraph._p.get_or_add_pPr().append(shading_elm)


def setup_styles(doc):
    """Configure document styles with Microsoft YaHei font."""
    style = doc.styles['Normal']
    font = style.font
    font.name = FONT_NAME
    font.size = Pt(11)
    font.color.rgb = BLACK
    style.element.rPr.rFonts.set(qn('w:eastAsia'), FONT_NAME)
    style.element.rPr.rFonts.set(qn('w:ascii'), FONT_NAME_EN)
    style.element.rPr.rFonts.set(qn('w:hAnsi'), FONT_NAME_EN)

    for level in range(1, 4):
        style_name = f'Heading {level}'
        if style_name in [s.name for s in doc.styles]:
            hs = doc.styles[style_name]
            hf = hs.font
            hf.name = FONT_NAME
            hf.color.rgb = BLUE
            hf.bold = True
            if level == 1:
                hf.size = Pt(22)
            elif level == 2:
                hf.size = Pt(16)
            else:
                hf.size = Pt(13)
            hs.element.rPr.rFonts.set(qn('w:eastAsia'), FONT_NAME)
            hs.element.rPr.rFonts.set(qn('w:ascii'), FONT_NAME_EN)
            hs.element.rPr.rFonts.set(qn('w:hAnsi'), FONT_NAME_EN)


def add_run(paragraph, text, bold=False, color=None, italic=False, size=None, font_name=None):
    """Add a formatted run to a paragraph."""
    run = paragraph.add_run(text)
    run.bold = bold
    run.italic = italic
    fn = font_name or FONT_NAME
    run.font.name = fn
    run.element.rPr.rFonts.set(qn('w:eastAsia'), fn)
    run.element.rPr.rFonts.set(qn('w:ascii'), fn)
    run.element.rPr.rFonts.set(qn('w:hAnsi'), fn)
    if color:
        run.font.color.rgb = color
    if size:
        run.font.size = Pt(size)
    return run


def add_v2_change_item(doc, text, new=False):
    """Add a V2.0 change log item with yellow background and orange text for new features."""
    p = doc.add_paragraph()
    p.paragraph_format.space_after = Pt(2)
    p.paragraph_format.space_before = Pt(2)
    if new:
        set_paragraph_shading(p, YELLOW_BG)
        add_run(p, '🆕 ', bold=True, color=ORANGE)
        add_run(p, '[新增] ', bold=True, color=ORANGE)
        add_run(p, text, color=ORANGE)
    else:
        set_paragraph_shading(p, YELLOW_BG)
        add_run(p, '✅ ', bold=True, color=GREEN)
        add_run(p, '[增强] ', bold=True, color=ORANGE)
        add_run(p, text, color=ORANGE)
    return p


def add_v2_page(doc):
    """Add the V2.0 update page."""
    doc.add_page_break()

    # Title
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    set_paragraph_shading(p, 'E8F0FE')
    add_run(p, '═══════════════════════════════════════════', bold=True, color=BLUE, size=14)

    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    set_paragraph_shading(p, 'E8F0FE')
    add_run(p, 'V2.0 版本更新说明', bold=True, color=BLUE, size=20)

    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    set_paragraph_shading(p, 'E8F0FE')
    add_run(p, 'Kindergarten Student Info Management System V2.0', bold=True, color=BLUE, size=12)

    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    set_paragraph_shading(p, 'E8F0FE')
    add_run(p, '═══════════════════════════════════════════', bold=True, color=BLUE, size=14)

    doc.add_paragraph()

    # Section: V2.0 Changes
    doc.add_heading('📋 V2.0 变更概览', level=2)

    p = doc.add_paragraph()
    add_run(p, '以下内容为 V2.0 版本相对于 V1.0 的更新变更。', color=GRAY, italic=True)
    doc.add_paragraph()

    doc.add_heading('🎨 用户界面改进', level=3)
    add_v2_change_item(doc, '全面升级为 ANSI 彩色终端界面，Windows 10+ 自动启用 VT100 模式，Linux/macOS 原生支持', new=True)
    add_v2_change_item(doc, '彩色标题横幅、分隔线、菜单高亮、成功/警告/错误信息分色显示', new=True)
    add_v2_change_item(doc, '班级分色显示：幼儿班橙色、小班绿色、中班黄色、大班品红色', new=True)
    add_v2_change_item(doc, '性别分色显示：男生蓝色、女生品红色', new=True)
    add_v2_change_item(doc, '项目/班级分布进度条（███ 可视化条）', new=True)

    doc.add_heading('⚡ 功能新增', level=3)
    add_v2_change_item(doc, '按年龄排序功能（从小到大）', new=True)
    add_v2_change_item(doc, '按缴费日期排序功能（从早到晚）', new=True)
    add_v2_change_item(doc, '统计仪表盘：总人数、性别分布、项目分布、班级分布、总收入统计', new=True)
    add_v2_change_item(doc, '命令行参数支持：-f 指定数据文件、-t 测试模式、-h/--help 帮助', new=True)
    add_v2_change_item(doc, '文件覆盖确认提示（防止误覆盖数据）', new=True)
    add_v2_change_item(doc, '启动模式选择菜单（正常/标准测试/边界测试/空白启动）', new=True)
    add_v2_change_item(doc, '菜单内帮助信息 (H/?)', new=True)

    doc.add_heading('🔧 技术改进', level=3)
    add_v2_change_item(doc, '编码从 GBK 升级为 UTF-8，支持跨平台中文显示', new=False)
    add_v2_change_item(doc, '统一错误码体系（KgtStatus 枚举），取代魔法数字', new=False)
    add_v2_change_item(doc, '学生编号查找 Move-to-Front 优化，频繁访问的学生自动提升到链表头部', new=True)
    add_v2_change_item(doc, '插入排序算法，插入式链表排序不破坏原有数据结构', new=True)
    add_v2_change_item(doc, 'Windows VT100 终端模式自动检测与启用', new=True)
    add_v2_change_item(doc, '模块化代码组织：工具函数/链表操作/业务逻辑/UI/CLI 分层清晰', new=False)
    add_v2_change_item(doc, 'CSV 加载增加跳过无效记录功能，增强数据容错性', new=False)

    doc.add_heading('📁 编译与构建', level=3)
    add_v2_change_item(doc, '提供 Makefile，支持 GCC (MinGW-w64) 和 MSVC cl.exe 双编译器自动检测', new=True)
    add_v2_change_item(doc, '提供 _msvc_build.bat 一键编译脚本', new=True)
    add_v2_change_item(doc, '编译选项：-std=c99/c11 -Wall/W3 -O2 优化，开启警告', new=False)

    doc.add_paragraph()
    print_divider_p = doc.add_paragraph()
    set_paragraph_shading(print_divider_p, 'FFE0B2')
    add_run(print_divider_p, '─' * 60, color=ORANGE)
    doc.add_paragraph()

    # Section: Feature comparison table
    doc.add_heading('📊 功能对照表', level=2)
    table = doc.add_table(rows=1, cols=3)
    table.style = 'Table Grid'
    hdr = table.rows[0].cells
    hdr[0].text = ''
    hdr[1].text = ''
    hdr[2].text = ''
    headers = ['功能模块', 'V1.0', 'V2.0']
    for i, h in enumerate(headers):
        p = hdr[i].paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        add_run(p, h, bold=True, color=RGBColor(0xFF, 0xFF, 0xFF))
        set_cell_shading(hdr[i], '4472C4')

    rows_data = [
        ('学生录入/删除/查询', '✅', '✅ 增强'),
        ('CSV文件持久化', '✅', '✅ 增强（容错加载）'),
        ('按年龄自动分班', '✅', '✅'),
        ('日期合法性校验', '✅', '✅'),
        ('缴费到期提醒(30天)', '✅', '✅ 彩色标记'),
        ('退费计算', '✅', '✅ 彩色标记'),
        ('按项目查询', '✅', '✅ 增强（分色）'),
        ('按班级查询', '✅', '✅ 增强（分色）'),
        ('彩色终端界面', '❌', '🆕 全新'),
        ('命令行参数(-f/-t/-h)', '❌', '🆕 全新'),
        ('排序功能(年龄/日期)', '❌', '🆕 全新'),
        ('统计仪表盘', '❌', '🆕 全新'),
        ('文件覆盖确认', '❌', '🆕 全新'),
        ('Move-to-Front查找优化', '❌', '🆕 全新'),
        ('统一错误码', '❌', '🆕 全新'),
        ('UTF-8编码', '❌ (GBK)', '🆕 全新'),
        ('Makefile构建系统', '❌', '🆕 全新'),
    ]
    for func_name, v1, v2 in rows_data:
        row = table.add_row().cells
        for i, txt in enumerate([func_name, v1, v2]):
            p = row[i].paragraphs[0]
            if i == 0:
                add_run(p, txt)
            elif '🆕' in txt or '增强' in txt:
                add_run(p, txt, color=ORANGE, bold=True)
            elif txt == '✅':
                add_run(p, txt, color=GREEN)
            else:
                add_run(p, txt, color=GRAY)
            if '🆕' in txt:
                set_cell_shading(row[i], YELLOW_BG)

    doc.add_paragraph()

    # Section: Technical Architecture
    doc.add_heading('🏗️ 技术架构 (V2.0)', level=2)

    doc.add_heading('数据结构', level=3)
    p = doc.add_paragraph()
    add_run(p, 'Student 结构体（单向链表节点）：', bold=True)
    doc.add_paragraph()
    code_text = """typedef struct Student {
    char        id[MAX_ID_LEN];       // 学生编号 (最长15字符)
    char        name[MAX_NAME_LEN];   // 姓名 (最长31字符)
    Gender      gender;               // 性别 (GENDER_MALE/FEMALE)
    int         age;                  // 年龄 (3-9岁)
    ProjectType project;              // 培训项目 (6种)
    ClassType   class_level;          // 班级级别 (4种，按年龄自动分配)
    char        pay_date[MAX_DATE_LEN]; // 缴费日期 (yyyy-mm-dd)
    int         pay_years;            // 缴费年限 (1-5年)
    struct Student *next;             // 链表指针
} Student;"""
    cp = doc.add_paragraph()
    set_paragraph_shading(cp, 'F5F5F5')
    add_run(cp, code_text, font_name='Consolas', size=9, color=RGBColor(0x33, 0x33, 0x33))

    doc.add_heading('关键算法', level=3)
    p = doc.add_paragraph(style='List Bullet')
    add_run(p, '自动分班：', bold=True)
    add_run(p, '3-4岁→幼儿班, 5岁→小班, 6-7岁→中班, 8-9岁→大班')

    p = doc.add_paragraph(style='List Bullet')
    add_run(p, '日期校验：', bold=True)
    add_run(p, '正则式格式检查 + 闰年判断 (能被4整除但不能被100整除，或能被400整除)')

    p = doc.add_paragraph(style='List Bullet')
    add_run(p, '退费计算：', bold=True)
    add_run(p, '剩余天数 = 总天数(年限×365) - 已过天数；剩余月数 = ceil(剩余天数/30)；退费 = 剩余月数 × (年费/12)')

    p = doc.add_paragraph(style='List Bullet')
    add_run(p, '到期预警：', bold=True)
    add_run(p, '到期日期 = 缴费日期+年限；距今≤30天黄色预警，已过期红色警告')

    p = doc.add_paragraph(style='List Bullet')
    add_run(p, 'Move-to-Front优化：', bold=True)
    add_run(p, '查找到的节点移动到链表头部，实现访问局部性优化')

    p = doc.add_paragraph(style='List Bullet')
    add_run(p, '插入排序：', bold=True)
    add_run(p, 'O(n²)插入排序，适合小数据量场景，不分配额外内存，稳定排序')

    doc.add_heading('错误码定义', level=3)
    err_table = doc.add_table(rows=1, cols=2)
    err_table.style = 'Table Grid'
    eh = err_table.rows[0].cells
    for i, h in enumerate(['错误码', '含义']):
        ep = eh[i].paragraphs[0]
        ep.alignment = WD_ALIGN_PARAGRAPH.CENTER
        add_run(ep, h, bold=True, color=RGBColor(0xFF,0xFF,0xFF))
        set_cell_shading(eh[i], '4472C4')

    errors = [
        ('KGT_OK (0)', '操作成功'),
        ('KGT_ERR_OPEN_FILE', '文件打开失败'),
        ('KGT_ERR_READ_FILE', '文件读取失败'),
        ('KGT_ERR_WRITE_FILE', '文件写入失败'),
        ('KGT_ERR_MEMORY', '内存分配失败'),
        ('KGT_ERR_EMPTY', '数据为空'),
        ('KGT_ERR_DUPLICATE_ID', '编号重复'),
        ('KGT_ERR_INVALID_DATE', '日期格式无效'),
        ('KGT_ERR_INVALID_INPUT', '输入无效'),
        ('KGT_ERR_NOT_FOUND', '未找到'),
        ('KGT_ERR_CANCEL', '用户取消操作'),
        ('KGT_ERR_FILE_EXISTS', '文件已存在'),
    ]
    for code, desc in errors:
        row = err_table.add_row().cells
        add_run(row[0].paragraphs[0], code, font_name='Consolas', size=10, color=BLUE, bold=True)
        add_run(row[1].paragraphs[0], desc)

    doc.add_paragraph()

    # Section: Build & Usage
    doc.add_heading('🔨 编译与使用', level=2)

    doc.add_heading('编译命令', level=3)
    p = doc.add_paragraph()
    set_paragraph_shading(p, 'F5F5F5')
    add_run(p, 'gcc -std=c99 -Wall -O2 -o kindergarten.exe main.c', font_name='Consolas', size=10, color=BLACK)
    p = doc.add_paragraph()
    add_run(p, '或使用 MSVC cl.exe：', italic=True, color=GRAY)
    p = doc.add_paragraph()
    set_paragraph_shading(p, 'F5F5F5')
    add_run(p, 'cl /nologo /utf-8 /std:c11 /W3 /O2 /D_CRT_SECURE_NO_WARNINGS /Fe:kindergarten.exe main.c /link kernel32.lib',
            font_name='Consolas', size=9, color=BLACK)
    p = doc.add_paragraph()
    add_run(p, '或运行 _msvc_build.bat 一键编译', italic=True, color=GRAY)

    doc.add_heading('命令行用法', level=3)
    usage_items = [
        ('kindergarten.exe', '交互模式（启动菜单选择模式）'),
        ('kindergarten.exe -f <数据文件>', '指定CSV数据文件启动并加载'),
        ('kindergarten.exe -t', '测试模式（加载12条标准测试数据后退出）'),
        ('kindergarten.exe -h', '显示帮助信息'),
    ]
    for cmd, desc in usage_items:
        p = doc.add_paragraph(style='List Bullet')
        add_run(p, cmd, font_name='Consolas', size=10, color=BLUE, bold=True)
        add_run(p, '  —  ' + desc)

    doc.add_heading('交互菜单', level=3)
    menu_items = [
        '1 - 录入学生信息（自动按年龄分班）',
        '2 - 删除学生信息（二次确认）',
        '3 - 按编号查询（Move-to-Front优化）',
        '4 - 查看所有学生（表格显示）',
        '5 - 按项目分组统计',
        '6 - 按班级分组统计',
        '7 - 缴费到期提醒（30天预警）',
        '8 - 退费计算（按月折算）',
        '9 - 保存数据到CSV文件（覆盖确认）',
        '10 - 从CSV文件加载数据',
        '11 - 统计仪表盘（人数/分布/收入）',
        '12 - 排序功能（年龄/缴费日期）',
        '13 - 加载测试数据（标准/边界）',
        'H/? - 帮助信息',
        '0 - 退出系统（可选保存）',
    ]
    for item in menu_items:
        p = doc.add_paragraph(style='List Bullet')
        add_run(p, item)

    doc.add_heading('班级划分规则', level=3)
    class_table = doc.add_table(rows=5, cols=3)
    class_table.style = 'Table Grid'
    class_data = [
        ('班级', '年龄', '显示颜色'),
        ('幼儿班', '3-4岁', '橙色'),
        ('小班', '5岁', '绿色'),
        ('中班', '6-7岁', '黄色'),
        ('大班', '8-9岁', '品红色'),
    ]
    class_colors = [None, ORANGE, GREEN, RGBColor(0xCC,0x99,0x00), RGBColor(0xC0,0x00,0xC0)]
    for i, (c1, c2, c3) in enumerate(class_data):
        cells = class_table.rows[i].cells
        for j, txt in enumerate([c1, c2, c3]):
            p = cells[j].paragraphs[0]
            if i == 0:
                p.alignment = WD_ALIGN_PARAGRAPH.CENTER
                add_run(p, txt, bold=True, color=RGBColor(0xFF,0xFF,0xFF))
                set_cell_shading(cells[j], '4472C4')
            else:
                if j == 2:
                    add_run(p, txt, color=class_colors[i], bold=True)
                else:
                    add_run(p, txt)

    doc.add_paragraph()

    doc.add_heading('费用标准', level=3)
    p = doc.add_paragraph(style='List Bullet')
    add_run(p, '年费标准：', bold=True)
    add_run(p, '2400 元/年')
    p = doc.add_paragraph(style='List Bullet')
    add_run(p, '缴费年限：', bold=True)
    add_run(p, '1-5 年')
    p = doc.add_paragraph(style='List Bullet')
    add_run(p, '月费：', bold=True)
    add_run(p, '年费 ÷ 12 = 200 元/月')
    p = doc.add_paragraph(style='List Bullet')
    add_run(p, '退费规则：', bold=True)
    add_run(p, '剩余完整月数 × 月费（不足整月按整月算）')

    doc.add_heading('数据文件格式', level=3)
    p = doc.add_paragraph()
    add_run(p, 'CSV 格式（逗号分隔），每行一条学生记录：', italic=True, color=GRAY)
    p = doc.add_paragraph()
    set_paragraph_shading(p, 'F5F5F5')
    add_run(p, 'id,name,gender,age,project,class_level,pay_date,pay_years',
            font_name='Consolas', size=10, color=BLACK)

    doc.add_paragraph()

    # Version footer
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    set_paragraph_shading(p, 'E8F0FE')
    add_run(p, '── V2.0 文档结束 ──', bold=True, color=BLUE, size=12)

    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    add_run(p, '编码: UTF-8 | 编译器: GCC (C99) / MSVC (C11) | 数据结构: 单向链表', color=GRAY, size=9, italic=True)


def main():
    if not os.path.exists(DOCX_PATH):
        print(f'[ERROR] Document not found: {DOCX_PATH}')
        return

    print(f'[INFO] Opening document: {DOCX_PATH}')
    doc = Document(DOCX_PATH)

    setup_styles(doc)
    add_v2_page(doc)

    doc.save(DOCX_PATH)
    print(f'[OK] Document updated successfully: {DOCX_PATH}')
    print(f'[INFO] V2.0 update page added with orange-on-yellow change highlights')


if __name__ == '__main__':
    main()
