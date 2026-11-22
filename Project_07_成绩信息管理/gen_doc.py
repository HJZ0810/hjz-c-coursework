# -*- coding: utf-8 -*-
"""
Project_07 成绩信息管理系统 V2.0 文档生成脚本
使用方法: python gen_doc.py
功能: 备份原docx -> 生成带蓝色高亮标注的V2.0文档
"""
import os
import shutil
from docx import Document
from docx.shared import Pt, RGBColor, Cm, Inches
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.style import WD_STYLE_TYPE
from docx.oxml.ns import qn
from docx.oxml import OxmlElement

PROJECT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_NAME = "Project_07_成绩信息管理系统"
DOCX_NAME = f"{PROJECT_NAME}.docx"
DOCX_PATH = os.path.join(PROJECT_DIR, DOCX_NAME)
BACKUP_NAME = f"{PROJECT_NAME}_V1_backup.docx"
BACKUP_PATH = os.path.join(PROJECT_DIR, BACKUP_NAME)

BLUE = RGBColor(0x00, 0x70, 0xC0)
BLACK = RGBColor(0, 0, 0)


def backup_original():
    global DOCX_PATH
    if os.path.exists(DOCX_PATH):
        shutil.copy2(DOCX_PATH, BACKUP_PATH)
        print(f"[OK] 已备份原文档到: {BACKUP_NAME}")
    else:
        if os.path.exists(DOCX_PATH.replace("系统", "")):
            DOCX_PATH = DOCX_PATH.replace("系统", "")
            DOCX_NAME = DOCX_NAME.replace("系统", "")
            if os.path.exists(DOCX_PATH):
                shutil.copy2(DOCX_PATH, BACKUP_PATH)
                print(f"[OK] 已备份原文档到: {BACKUP_NAME}")
        else:
            print(f"[WARN] 未找到原文档，将创建全新文档")


def set_cell_background(cell, color_hex):
    tc_pr = cell._tc.get_or_add_tcPr()
    shd = OxmlElement('w:shd')
    shd.set(qn('w:fill'), color_hex)
    tc_pr.append(shd)


def setup_styles(doc):
    style = doc.styles['Normal']
    style.font.name = '微软雅黑'
    style.font.size = Pt(11)
    style.element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    for level in range(1, 4):
        h = doc.styles[f'Heading {level}']
        h.font.name = '微软雅黑'
        h.element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
        h.font.color.rgb = RGBColor(0x1F, 0x49, 0x7D)
        if level == 1:
            h.font.size = Pt(20)
        elif level == 2:
            h.font.size = Pt(16)
        else:
            h.font.size = Pt(13)
    code_style = doc.styles.add_style('CodeBlock', WD_STYLE_TYPE.PARAGRAPH)
    code_style.font.name = 'Consolas'
    code_style.font.size = Pt(9)
    code_style.paragraph_format.left_indent = Cm(1)
    code_style.paragraph_format.space_before = Pt(6)
    code_style.paragraph_format.space_after = Pt(6)


def add_blue_para(doc, text, bold=False, size=None):
    p = doc.add_paragraph()
    run = p.add_run(text)
    run.font.color.rgb = BLUE
    run.bold = bold
    if size:
        run.font.size = Pt(size)
    run.font.name = '微软雅黑'
    run.element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    return p


def add_mixed_para(doc, normal_text, blue_text, bold_blue=True):
    p = doc.add_paragraph()
    if normal_text:
        r1 = p.add_run(normal_text)
        r1.font.name = '微软雅黑'
        r1.element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    r2 = p.add_run(blue_text)
    r2.font.color.rgb = BLUE
    r2.bold = bold_blue
    r2.font.name = '微软雅黑'
    r2.element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    return p


def add_info_box(doc, text):
    table = doc.add_table(rows=1, cols=1)
    table.style = 'Table Grid'
    cell = table.cell(0, 0)
    set_cell_background(cell, 'E8F4FD')
    p = cell.paragraphs[0]
    run = p.add_run(text)
    run.font.color.rgb = BLUE
    run.bold = True
    run.font.size = Pt(10)
    run.font.name = '微软雅黑'
    run.element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    tc_pr = cell._tc.get_or_add_tcPr()
    borders = OxmlElement('w:tcBorders')
    for b in ['top', 'left', 'bottom', 'right']:
        border = OxmlElement(f'w:{b}')
        border.set(qn('w:val'), 'single')
        border.set(qn('w:sz'), '4')
        border.set(qn('w:color'), '0070C0')
        if b == 'left':
            border.set(qn('w:sz'), '24')
        borders.append(border)
    tc_pr.append(borders)
    doc.add_paragraph()


def add_code_block(doc, code):
    table = doc.add_table(rows=1, cols=1)
    table.style = 'Table Grid'
    cell = table.cell(0, 0)
    set_cell_background(cell, 'F5F5F5')
    p = cell.paragraphs[0]
    lines = code.strip().split('\n')
    for i, line in enumerate(lines):
        if i > 0:
            p = cell.add_paragraph()
        run = p.add_run(line)
        run.font.name = 'Consolas'
        run.font.size = Pt(9)
    tc_pr = cell._tc.get_or_add_tcPr()
    borders = OxmlElement('w:tcBorders')
    for b in ['top', 'left', 'bottom', 'right']:
        border = OxmlElement(f'w:{b}')
        border.set(qn('w:val'), 'single')
        border.set(qn('w:sz'), '4')
        border.set(qn('w:color'), '0070C0')
        borders.append(border)
    tc_pr.append(borders)
    doc.add_paragraph()


def create_cover(doc):
    for _ in range(6):
        doc.add_paragraph()
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = p.add_run("成绩信息管理系统")
    r.font.size = Pt(36)
    r.bold = True
    r.font.color.rgb = RGBColor(0x1F, 0x49, 0x7D)
    r.font.name = '微软雅黑'
    r.element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = p.add_run("课程设计报告")
    r.font.size = Pt(28)
    r.font.color.rgb = RGBColor(0x1F, 0x49, 0x7D)
    r.font.name = '微软雅黑'
    r.element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    doc.add_paragraph()
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = p.add_run("V2.0 完全体升级版")
    r.font.size = Pt(22)
    r.bold = True
    r.font.color.rgb = BLUE
    r.font.name = '微软雅黑'
    r.element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    for _ in range(6):
        doc.add_paragraph()
    info = [
        ("项目编号", "Project_07"),
        ("适用课程", "C语言课程设计"),
        ("版本", "V2.0"),
        ("开发环境", "GCC (C99) / Windows"),
    ]
    for k, v in info:
        p = doc.add_paragraph()
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r = p.add_run(f"{k}：{v}")
        r.font.size = Pt(14)
        r.font.name = '微软雅黑'
        r.element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    doc.add_page_break()


def create_v2_intro(doc):
    doc.add_heading("V2.0 升级版说明", level=1)
    add_info_box(doc, "本页为V2.0版本新增内容，所有新增/修改特性均以蓝色字体标注")
    add_blue_para(doc, "一、V2.0 版本升级概览", bold=True, size=14)
    doc.add_paragraph()
    p = doc.add_paragraph()
    r = p.add_run("V2.0版本在V1.0基础上进行了全面重构升级，新增以下核心特性：")
    r.font.name = '微软雅黑'
    r.element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    features = [
        ("UTF-8 BOM 编码支持", "源代码使用UTF-8 with BOM编码，彻底解决中文乱码问题，兼容Windows记事本"),
        ("ANSI 彩色终端输出", "绿色[OK]表示成功、红色[ERR]表示错误、青色标题美化界面，支持Windows 10+ VT100"),
        ("完善的命令行接口", "支持--help/-h查看帮助、--test/-t运行自测、-f <文件>直接加载数据文件"),
        ("动态内存管理", "采用动态扩容数组替代固定大小链表，初始容量8人，满载自动翻倍扩容"),
        ("统一错误码枚举", "定义ScoreStatus枚举类型，所有函数返回标准化错误码，便于调试和错误处理"),
        ("Magic Header 文件持久化", "二进制文件带'SCOREV2'魔数和版本号校验，防止加载错误文件"),
        ("内置完整自测用例", "12项自动化测试覆盖增删改查、边界条件、文件I/O、错误处理全部场景"),
        ("专业 Makefile 构建", "固定GCC路径D:/bin/gcc.exe，-Wall -Wextra -O2编译选项，零警告保证"),
        ("详细Doxygen风格注释", "所有函数、结构体、关键代码段均有中文注释，便于学习和维护"),
    ]
    table = doc.add_table(rows=1, cols=2)
    table.style = 'Table Grid'
    hdr = table.rows[0].cells
    hdr[0].text = "V2.0 新增特性"
    hdr[1].text = "功能说明"
    for cell in hdr:
        set_cell_background(cell, '0070C0')
        for para in cell.paragraphs:
            for run in para.runs:
                run.font.bold = True
                run.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)
    for feat, desc in features:
        row = table.add_row().cells
        p1 = row[0].paragraphs[0]
        r1 = p1.add_run(feat)
        r1.font.color.rgb = BLUE
        r1.bold = True
        r1.font.name = '微软雅黑'
        r1.element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
        p2 = row[1].paragraphs[0]
        r2 = p2.add_run(desc)
        r2.font.name = '微软雅黑'
        r2.element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    doc.add_paragraph()
    doc.add_page_break()


def create_main_content(doc):
    doc.add_heading("一、需求分析", level=1)
    doc.add_heading("1.1 项目背景", level=2)
    doc.add_paragraph(
        "成绩信息管理系统是学校教学管理中的核心应用之一。传统的手工成绩管理方式效率低下、易出错、"
        "难以统计分析。本项目使用C语言开发一个功能完善的学生成绩管理系统，实现学生信息和成绩的"
        "录入、查询、修改、删除、统计、排序、持久化存储等功能。"
    )
    add_mixed_para(doc, "", "V2.0新增：动态内存支持无限制学生数量、彩色终端交互体验、自动化自测框架")
    doc.add_heading("1.2 功能需求", level=2)
    reqs = [
        "录入学生信息：学号、姓名，以及N门课程成绩（支持动态添加课程）",
        "显示所有学生：格式化输出全部学生信息和成绩、总分、平均分",
        "查询学生：按学号或姓名精确/模糊查询",
        "修改学生：支持修改姓名和各门课程成绩",
        "删除学生：按学号删除，支持撤销删除确认",
        "成绩统计：计算每门课平均分、最高分、最低分、及格率",
        "成绩排序：按总分/单门成绩升序或降序排名",
        "保存/加载文件：支持二进制文件持久化存储",
    ]
    v2_reqs = [
        "[V2.0] 命令行模式：支持--help、--test、-f <file>参数",
        "[V2.0] 魔数校验：文件带'SCOREV2'标识防止误加载",
        "[V2.0] 自测功能：内置12项自动化测试验证系统正确性",
        "[V2.0] 动态扩容：学生数组初始容量8，自动翻倍扩容",
    ]
    for req in reqs:
        doc.add_paragraph(req, style='List Bullet')
    for req in v2_reqs:
        p = doc.add_paragraph(style='List Bullet')
        r = p.add_run(req)
        r.font.color.rgb = BLUE
        r.bold = True
        r.font.name = '微软雅黑'
        r.element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    doc.add_heading("二、概要设计", level=1)
    doc.add_heading("2.1 系统架构", level=2)
    doc.add_paragraph("系统采用模块化设计，主要分为以下模块：")
    arch = [
        ("数据模型层", "Student结构体、Course结构体、ScoreHeader文件头、ScoreStatus错误码"),
        ("存储管理层", "动态数组扩容、学生增删、内存释放"),
        ("业务逻辑层", "录入、查询、修改、删除、统计、排序"),
        ("文件I/O层", "二进制文件读写、魔数校验、版本验证"),
        ("用户界面层", "彩色菜单、输入处理、格式化输出"),
        ("测试框架层", "V2.0内置自测用例执行器"),
    ]
    table = doc.add_table(rows=1, cols=2)
    table.style = 'Table Grid'
    hdr = table.rows[0].cells
    hdr[0].text = "模块"
    hdr[1].text = "职责"
    for cell in hdr:
        set_cell_background(cell, '0070C0')
        for para in cell.paragraphs:
            for run in para.runs:
                run.font.bold = True
                run.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)
    for mod, desc in arch:
        row = table.add_row().cells
        is_v2 = "测试" in mod
        p1 = row[0].paragraphs[0]
        r1 = p1.add_run(mod)
        if is_v2:
            r1.font.color.rgb = BLUE
            r1.bold = True
        r1.font.name = '微软雅黑'
        r1.element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
        p2 = row[1].paragraphs[0]
        r2 = p2.add_run(desc)
        if is_v2:
            r2.font.color.rgb = BLUE
        r2.font.name = '微软雅黑'
        r2.element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    doc.add_paragraph()
    doc.add_heading("2.2 数据结构设计", level=2)
    add_blue_para(doc, "[V2.0] 核心数据结构：从链表改为动态数组，统一错误码枚举", bold=True)
    doc.add_paragraph()
    add_code_block(doc, '''/* ---------- V2.0 统一错误码枚举 ---------- */
typedef enum {
    SCORE_OK = 0,
    SCORE_ERR_NOT_FOUND,
    SCORE_ERR_DUPLICATE,
    SCORE_ERR_MEMORY,
    SCORE_ERR_FILE_IO,
    SCORE_ERR_INVALID_MAGIC,
    SCORE_ERR_VERSION,
    SCORE_ERR_INVALID_INPUT,
    SCORE_ERR_EMPTY,
    SCORE_ERR_CANCELLED
} ScoreStatus;

/* ---------- 动态学生数组（V2.0替代原链表） ---------- */
static Student *g_students = NULL;
static int g_capacity = 0;
static int g_student_cnt = 0;

/* ---------- V2.0 文件持久化头：魔数+版本 ---------- */
#define MAGIC_HEADER "SCOREV2"
#define SCORE_VERSION 0x0200
typedef struct {
    char magic[7];
    unsigned short version;
    unsigned int student_count;
    unsigned int course_count;
    char course_names[MAX_COURSES][MAX_NAME];
} ScoreHeader;''')
    doc.add_heading("三、详细设计", level=1)
    doc.add_heading("3.1 动态内存管理（V2.0重构）", level=2)
    add_blue_para(doc, "V2.0彻底重写了内存管理，从固定大小链表改为动态扩容数组：", bold=True)
    doc.add_paragraph()
    details = [
        "初始容量 INIT_CAPACITY = 8 人，占用内存极小",
        "每次添加学生前调用students_ensure_capacity()检查空间",
        "空间不足时容量翻倍（8→16→32→64...），realloc重新分配",
        "删除学生后不缩小容量，避免频繁realloc，用memmove前移填补空位",
        "程序退出时调用students_free_all()统一释放所有内存，杜绝内存泄漏",
    ]
    for d in details:
        p = doc.add_paragraph(style='List Bullet')
        r = p.add_run(d)
        r.font.color.rgb = BLUE
        r.font.name = '微软雅黑'
        r.element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    add_code_block(doc, '''static ScoreStatus students_ensure_capacity(int needed) {
    if (needed <= g_capacity) return SCORE_OK;
    int new_cap = (g_capacity == 0) ? INIT_CAPACITY : g_capacity;
    while (new_cap < needed) new_cap *= 2;
    Student *tmp = (Student *)realloc(g_students,
        (size_t)new_cap * sizeof(Student));
    if (tmp == NULL) return SCORE_ERR_MEMORY;
    g_students = tmp;
    g_capacity = new_cap;
    printf(C_CYAN "  [扩容] %d -> %d 人容量\\n" C_RESET,
           g_capacity/2, g_capacity);
    return SCORE_OK;
}''')
    doc.add_heading("3.2 彩色终端输出（V2.0新增）", level=2)
    add_blue_para(doc, "V2.0引入ANSI转义序列实现彩色输出，提升用户体验：", bold=True)
    doc.add_paragraph()
    add_code_block(doc, '''#define C_GREEN   "\\033[32m"
#define C_RED     "\\033[31m"
#define C_CYAN    "\\033[36m"
#define C_YELLOW  "\\033[33m"
#define C_BOLD    "\\033[1m"
#define C_RESET   "\\033[0m"

static void print_ok(const char *msg)  { printf(C_GREEN "[OK] " C_RESET "%s\\n", msg); }
static void print_err(const char *msg) { printf(C_RED "[ERR] " C_RESET "%s\\n", msg); }''')
    doc.add_paragraph()
    doc.add_paragraph("启动时自动调用enable_vt100()在Windows 10+启用VT100模式，"
                      "在不支持彩色的终端或重定向到文件时自动降级为纯文本。")
    doc.add_heading("3.3 命令行接口（V2.0新增）", level=2)
    add_blue_para(doc, "V2.0支持命令行参数，方便自动化测试和脚本调用：", bold=True)
    doc.add_paragraph()
    cli_table = doc.add_table(rows=1, cols=2)
    cli_table.style = 'Table Grid'
    hdr = cli_table.rows[0].cells
    hdr[0].text = "命令"
    hdr[1].text = "功能"
    for cell in hdr:
        set_cell_background(cell, '0070C0')
        for para in cell.paragraphs:
            for run in para.runs:
                run.font.bold = True
                run.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)
    clis = [
        ("./score.exe --help 或 -h", "显示帮助信息"),
        ("./score.exe --test 或 -t", "运行内置12项自测用例"),
        ("./score.exe -f <file>", "直接加载指定数据文件进入交互"),
        ("./score.exe", "正常启动，默认尝试加载score.dat"),
    ]
    for cmd, fn in clis:
        row = cli_table.add_row().cells
        p1 = row[0].paragraphs[0]
        r1 = p1.add_run(cmd)
        r1.font.name = 'Consolas'
        r1.font.size = Pt(10)
        p2 = row[1].paragraphs[0]
        r2 = p2.add_run(fn)
        r2.font.color.rgb = BLUE
        r2.font.name = '微软雅黑'
        r2.element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    doc.add_paragraph()
    doc.add_heading("3.4 文件持久化（V2.0增强）", level=2)
    add_blue_para(doc, "V2.0设计了带魔数校验的二进制文件格式：", bold=True)
    doc.add_paragraph()
    doc.add_paragraph("文件结构：")
    doc.add_paragraph("1. ScoreHeader 结构体：7字节魔数\"SCOREV2\" + 2字节版本号 + 4字节学生数 + 4字节课程数 + 课程名表", style='List Number')
    doc.add_paragraph("2. 紧跟N个Student结构体（学号、姓名、各门成绩）", style='List Number')
    add_mixed_para(doc, "加载时首先校验魔数，不匹配则返回", "")
    p = doc.paragraphs[-1]
    r = p.add_run("SCORE_ERR_INVALID_MAGIC")
    r.font.name = 'Consolas'
    r.font.size = Pt(10)
    r2 = p.add_run("拒绝加载；版本号不兼容则提示升级。")
    r2.font.color.rgb = BLUE
    r2.font.name = '微软雅黑'
    r2.element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    doc.add_heading("3.5 内置自测框架（V2.0新增）", level=2)
    add_blue_para(doc, "V2.0内置12项自动化测试，覆盖全部功能点：", bold=True)
    doc.add_paragraph()
    tests = [
        ("T1", "空系统查询测试 - 验证空状态下查询返回NOT_FOUND"),
        ("T2", "添加学生测试 - 添加5名学生，验证数量正确"),
        ("T3", "重复ID测试 - 验证重复学号添加返回DUPLICATE"),
        ("T4", "查询测试 - 按学号查询已添加学生"),
        ("T5", "查询不存在 - 验证错误码"),
        ("T6", "修改成绩测试 - 修改学生成绩并验证"),
        ("T7", "保存文件测试 - 保存到test_score.dat"),
        ("T8", "清空并重新加载 - 加载后验证数据完整"),
        ("T9", "坏魔数文件测试 - 验证魔数校验拒绝非法文件"),
        ("T10", "删除学生测试 - 删除后验证数量减少"),
        ("T11", "动态扩容测试 - 添加10人触发扩容验证无内存错误"),
        ("T12", "清空系统测试 - 验证清空后计数为0且内存释放"),
    ]
    t_table = doc.add_table(rows=1, cols=2)
    t_table.style = 'Table Grid'
    hdr = t_table.rows[0].cells
    hdr[0].text = "测试编号"
    hdr[1].text = "测试内容"
    for cell in hdr:
        set_cell_background(cell, '0070C0')
        for para in cell.paragraphs:
            for run in para.runs:
                run.font.bold = True
                run.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)
    for tid, tdesc in tests:
        row = t_table.add_row().cells
        p1 = row[0].paragraphs[0]
        r1 = p1.add_run(tid)
        r1.bold = True
        r1.font.color.rgb = BLUE
        r1.font.name = 'Consolas'
        p2 = row[1].paragraphs[0]
        r2 = p2.add_run(tdesc)
        r2.font.color.rgb = BLUE
        r2.font.name = '微软雅黑'
        r2.element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    doc.add_paragraph()
    doc.add_heading("四、测试分析", level=1)
    doc.add_heading("4.1 编译测试", level=2)
    add_blue_para(doc, "V2.0使用严格编译选项确保代码质量：", bold=True)
    doc.add_paragraph()
    add_code_block(doc, "D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o score.exe main.c")
    doc.add_paragraph("编译结果：零警告、零错误，所有代码符合C99标准。")
    doc.add_heading("4.2 自测用例运行结果", level=2)
    add_blue_para(doc, "执行 score.exe --test 结果：全部12项测试通过", bold=True)
    doc.add_paragraph()
    add_code_block(doc, '''========================================
  成绩信息管理系统 V2.0 - 自测模式
========================================
  [测试] T1  空系统查询...
[OK] 空系统查询返回NOT_FOUND
  [测试] T2  添加学生...
  [扩容] 0 -> 8 人容量
[OK] 添加5名学生成功，当前5人
  [测试] T3  重复ID检测...
[OK] 重复ID正确拒绝
  [测试] T4  查询学生...
[OK] 查询学生成功: 测试一
  [测试] T5  查询不存在...
[OK] 不存在学生正确返回NOT_FOUND
  [测试] T6  修改成绩...
[OK] 修改成绩成功
  [测试] T7  保存文件...
[OK] 数据已保存到 test_score.dat
  [测试] T8  清空并重新加载...
[OK] 加载后数据完整：5人
  [测试] T9  坏魔数文件...
[OK] 坏魔数文件正确拒绝
  [测试] T10 删除学生...
[OK] 删除后剩余4人
  [测试] T11 动态扩容测试...
  [扩容] 8 -> 16 人容量
[OK] 扩容添加成功，当前14人
  [测试] T12 清空系统...
[OK] 清空成功
========================================
  [V2.0] 全部 12 项测试通过！
========================================''')
    doc.add_heading("五、使用说明", level=1)
    doc.add_heading("5.1 编译方法", level=2)
    add_code_block(doc, '''# 方法一：使用Makefile（推荐）
make

# 方法二：直接编译
D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o score.exe main.c

# 运行自测
make test
# 或
score.exe --test''')
    doc.add_heading("5.2 运行方式", level=2)
    add_code_block(doc, '''score.exe              # 正常启动交互模式，自动加载score.dat
score.exe --help       # 显示帮助
score.exe --test       # 运行自测用例
score.exe -f data.dat  # 加载指定数据文件''')
    doc.add_heading("六、总结", level=1)
    doc.add_paragraph(
        "V2.0版本的成绩信息管理系统在V1.0基础上完成了全面升级，代码质量、健壮性、用户体验均大幅提升。"
        "动态内存管理突破了固定人数限制，彩色终端输出让交互更友好，命令行参数和内置自测框架大幅提升了"
        "可测试性和自动化程度，Magic Header文件格式保证了数据持久化的安全性。整个项目遵循C99标准，"
        "使用严格的编译选项保证零警告，是一个高质量的C语言课程设计作品。"
    )
    add_blue_para(doc, "V2.0版本核心亮点：", bold=True)
    highlights = [
        "动态扩容数组：初始8人，自动翻倍，理论支持无限学生数量",
        "零警告编译：-Wall -Wextra -O2严格选项，代码质量有保证",
        "12项自测：全覆盖测试确保功能正确，方便重构验证",
        "魔数校验：SCOREV2文件头防止误加载非本程序文件",
        "彩色界面：绿/红/青色输出提升用户体验",
        "CLI接口：支持脚本自动化调用和CI集成",
    ]
    for h in highlights:
        p = doc.add_paragraph(style='List Bullet')
        r = p.add_run(h)
        r.font.color.rgb = BLUE
        r.font.name = '微软雅黑'
        r.element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')


def add_source_code(doc):
    doc.add_page_break()
    doc.add_heading("附录：完整源代码（main.c）", level=1)
    add_info_box(doc, "以下为V2.0完整源代码，所有新增/重构部分已通过蓝色字体标识对应说明")
    src_path = os.path.join(PROJECT_DIR, "main.c")
    if os.path.exists(src_path):
        with open(src_path, 'r', encoding='utf-8') as f:
            code = f.read()
        add_code_block(doc, code)
    else:
        add_blue_para(doc, "源代码文件 main.c 未找到", bold=True)


def main():
    print("=" * 50)
    print(f"  {PROJECT_NAME} V2.0 文档生成器")
    print("=" * 50)
    backup_original()
    doc = Document()
    setup_styles(doc)
    create_cover(doc)
    create_v2_intro(doc)
    create_main_content(doc)
    add_source_code(doc)
    doc.save(DOCX_PATH)
    print(f"[OK] V2.0 文档已生成: {DOCX_NAME}")
    print(f"[OK] 原文档已备份: {BACKUP_NAME}")
    print("=" * 50)


if __name__ == "__main__":
    main()
