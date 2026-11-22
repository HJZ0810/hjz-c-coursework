# -*- coding: utf-8 -*-
"""
gen_doc.py — Sports Meet Scoring V2.0 文档更新脚本
在原 docx 开头插入 V2.0 版本说明页（橙色文字黄底高亮），
并更新技术章节以反映动态内存/ANSI颜色/错误码等新特性。
使用 python-docx 操作，字体：微软雅黑；新增内容橙色高亮。
"""
import os, sys, shutil
from docx import Document
from docx.shared import Pt, RGBColor, Cm
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.oxml.ns import qn
from docx.oxml import OxmlElement

DOCX_NAME = "Project_01_运动会分数统计系统.docx"
BACKUP_NAME = "Project_01_运动会分数统计系统_V1_backup.docx"
FONT_NAME = "微软雅黑"
ORANGE = RGBColor(0xFF, 0x8C, 0x00)
YELLOW_BG = "FFFF00"


def set_font(run, name=FONT_NAME, size=11, bold=False, color=None, highlight=None):
    run.font.name = name
    run.font.size = Pt(size)
    run.bold = bold
    if color is not None:
        run.font.color.rgb = color
    rPr = run._element.get_or_add_rPr()
    rFonts = rPr.find(qn('w:rFonts'))
    if rFonts is None:
        rFonts = OxmlElement('w:rFonts')
        rPr.append(rFonts)
    rFonts.set(qn('w:eastAsia'), name)
    rFonts.set(qn('w:ascii'), name)
    rFonts.set(qn('w:hAnsi'), name)
    if highlight:
        hl = OxmlElement('w:highlight')
        hl.set(qn('w:val'), highlight)
        rPr.append(hl)
    shd = rPr.find(qn('w:shd'))
    if shd is not None:
        rPr.remove(shd)


def add_para(doc, text, size=11, bold=False, color=None, align=None, highlight=None, space_after=6):
    p = doc.add_paragraph()
    if align is not None:
        p.alignment = align
    pf = p.paragraph_format
    pf.space_after = Pt(space_after)
    pf.line_spacing = 1.5
    run = p.add_run(text)
    set_font(run, size=size, bold=bold, color=color, highlight=highlight)
    return p


def add_heading(doc, text, level=1):
    sizes = {0: 22, 1: 18, 2: 14, 3: 12}
    p = doc.add_heading(level=level)
    p.alignment = WD_ALIGN_PARAGRAPH.LEFT
    run = p.add_run(text)
    set_font(run, size=sizes.get(level, 12), bold=True, color=ORANGE)
    return p


def add_bullet(doc, text, new=False):
    p = doc.add_paragraph(style='List Bullet')
    pf = p.paragraph_format
    pf.space_after = Pt(3)
    pf.line_spacing = 1.4
    run = p.add_run(text)
    color = ORANGE if new else None
    hl = "yellow" if new else None
    set_font(run, size=11, color=color, highlight=hl)
    return p


def insert_paragraph_after(paragraph, text="", style=None):
    new_p = OxmlElement("w:p")
    paragraph._p.addnext(new_p)
    from docx.text.paragraph import Paragraph
    new_para = Paragraph(new_p, paragraph._parent)
    if style:
        new_para.style = style
    if text:
        run = new_para.add_run(text)
        set_font(run)
    return new_para


def build_v2_page(doc):
    """在文档最开头插入 V2.0 版本更新页（橙色字+黄色高亮）。"""
    body = doc.element.body
    first_child = body[0] if len(body) > 0 else None

    # 创建临时段落列表，稍后把它们的 XML 移动到文档开头
    elems = []

    # 大标题
    title = doc.add_paragraph()
    title.alignment = WD_ALIGN_PARAGRAPH.CENTER
    title.paragraph_format.space_after = Pt(6)
    r = title.add_run("V2.0 版本更新说明")
    set_font(r, size=24, bold=True, color=ORANGE, highlight="yellow")
    elems.append(title._p)

    sub = doc.add_paragraph()
    sub.alignment = WD_ALIGN_PARAGRAPH.CENTER
    sub.paragraph_format.space_after = Pt(12)
    r = sub.add_run("Sports Meet Scoring System — Enhanced Edition  |  2025-2026 第二学期")
    set_font(r, size=11, color=ORANGE, highlight="yellow")
    elems.append(sub._p)

    sep = doc.add_paragraph()
    elems.append(sep._p)

    add_heading(doc, "一、V2.0 版本重要变更总览", level=1)
    elems_2 = []
    changes = [
        ("编码与平台", "UTF-8 编码（原 GBK）；C99 标准；支持 Windows ANSI 颜色终端输出。"),
        ("动态内存管理", "彻底移除固定 MAX_SCHOOLS=50 限制，所有学校/项目/成绩均使用 malloc + realloc 动态数组，按倍增策略扩容。"),
        ("统一错误码", "引入 SptStatus 枚举（SPT_OK / SPT_ERR_MEMORY / SPT_ERR_MAGIC / SPT_ERR_VERSION 等），替代原 int 返回与 goto 错误处理。"),
        ("ANSI 彩色终端 UI", "新增彩色 banner、OK/ERROR/WARN 状态标记、进度条、加粗/橙色/青色高亮，支持 Windows 10+ VT 模式自动启用。"),
        ("命令行参数支持", "新增 --help / -h、-f <file>、-t（内置测试）、--stats <file> 命令行选项，非交互运行更方便。"),
        ("文件持久化 Magic Header", "数据文件以魔数 \"SPRT\" + 版本号开头，加载时校验魔数/版本/完整性，防止误读损坏文件。"),
        ("统计与排行榜", "新增统计功能：总学校数/项目数/成绩数、团体总分排名、按学校/按项目查询、男女项目分别计分。"),
        ("进度反馈", "批量写入/读取大量数据时显示进度条，提升用户体验。"),
        ("代码质量提升", "全量重构：函数分块清晰、静态函数化、注释规范、-Wall -Wextra 零警告通过。"),
        ("Makefile 构建", "新增 Makefile，使用 C:/bin/gcc.exe（MinGW-w64 GCC 14.2）一键编译，make test 自动运行内置测试。"),
    ]
    for title_text, desc in changes:
        p = doc.add_paragraph()
        p.paragraph_format.space_after = Pt(4)
        p.paragraph_format.line_spacing = 1.4
        p.paragraph_format.left_indent = Cm(0.5)
        r1 = p.add_run(f"● [{title_text}] ")
        set_font(r1, size=11, bold=True, color=ORANGE, highlight="yellow")
        r2 = p.add_run(desc)
        set_font(r2, size=11, color=ORANGE, highlight="yellow")
        elems_2.append(p._p)

    add_heading(doc, "二、保留的原有功能", level=1)
    elems_3 = []
    keeps = [
        "学校信息管理（编号、名称、男女团体分、总分）",
        "项目信息管理（编号、名称、取前3名/前5名类型）",
        "成绩录入：按名次自动计分（TOP3: 5,3,2 / TOP5: 7,5,3,2,1）",
        "学校团体总分排序输出",
        "按学校编号查询得分详情",
        "按项目编号查询结果",
        "数据文件持久化保存 / 读取",
    ]
    for k in keeps:
        add_bullet(doc, k, new=False)
    # 追加到 elems_3 (add_bullet 已加到 doc末尾, 收集末尾的p)
    elems_3 = doc.paragraphs[-len(keeps):]
    elems_3 = [p._p for p in elems_3]

    add_heading(doc, "三、命令行用法", level=1)
    elems_4 = []
    usage_lines = [
        "sports.exe                      进入交互菜单模式",
        "sports.exe --help / -h          显示帮助",
        "sports.exe -f <file.dat>        指定数据文件启动",
        "sports.exe -t                   运行内置测试（17 项全通过）",
        "sports.exe --stats <file.dat>   显示数据文件统计信息与总分排名",
    ]
    for line in usage_lines:
        p = doc.add_paragraph()
        p.paragraph_format.space_after = Pt(2)
        p.paragraph_format.left_indent = Cm(0.7)
        r = p.add_run("  " + line)
        set_font(r, size=10, color=ORANGE, highlight="yellow")
        elems_4.append(p._p)

    add_heading(doc, "四、数据结构升级说明", level=1)
    elems_5 = []
    ds_intro = doc.add_paragraph()
    ds_intro.paragraph_format.line_spacing = 1.5
    r = ds_intro.add_run(
        "V2.0 中 School / Project / Result 结构体保持与 V1.x 相同的字段语义，"
        "但存储方式由固定长度数组改为动态扩容数组（Dynamic Array）："
    )
    set_font(r, size=11, color=ORANGE, highlight="yellow")
    elems_5.append(ds_intro._p)
    ds_items = [
        "初始容量 INIT_CAPACITY = 8，扩容因子 GROWTH_FACTOR = 2",
        "ensure_school_cap/ensure_project_cap/ensure_result_cap 统一负责 realloc",
        "free_all_data() 在退出/重载时释放全部堆内存，无内存泄漏",
        "DataFileHeader 新增 magic[4]=\"SPRT\" + version 字段，向前兼容版本检查",
    ]
    for d in ds_items:
        add_bullet(doc, d, new=True)
    # 收集
    elems_5b = doc.paragraphs[-len(ds_items):]
    elems_5 += [p._p for p in elems_5b]

    add_heading(doc, "五、编译与运行", level=1)
    elems_6 = []
    compile_lines = [
        "编译器：MinGW-w64 GCC 14.2.0  (C:\\bin\\gcc.exe)",
        "编译命令：gcc -std=c99 -Wall -Wextra -O2 -o sports.exe main.c",
        "一键构建：mingw32-make  （或直接 make）",
        "自动测试：mingw32-make test",
        "编码要求：源文件保存为 UTF-8（无 BOM），Windows 终端自动启用 VT100 颜色",
    ]
    for c in compile_lines:
        add_bullet(doc, c, new=True)
    elems_6b = doc.paragraphs[-len(compile_lines):]
    elems_6 = [p._p for p in elems_6b]

    end_p = doc.add_paragraph()
    r = end_p.add_run("—— V2.0 版本说明页结束 ——")
    set_font(r, size=10, color=ORANGE, highlight="yellow")
    end_p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    elems.append(end_p._p)

    page_break = doc.add_paragraph()
    from docx.enum.text import WD_BREAK
    r = page_break.add_run()
    r.add_break(WD_BREAK.PAGE)
    elems.append(page_break._p)

    # 所有新增的元素（从 elems + 中间段落）其实已经在文档末尾；需要把它们移动到最前
    # 收集所有我们刚添加的段落（从 title 到 page_break 之间的所有元素）
    all_new = []
    all_new.extend(elems[:2])          # title, sub
    for p in elems_2:
        all_new.append(p)
    for p in elems_3:
        all_new.append(p)
    for p in elems_4:
        all_new.append(p)
    for p in elems_5:
        all_new.append(p)
    for p in elems_6:
        all_new.append(p)
    all_new.append(elems[-2])          # end_p
    all_new.append(elems[-1])          # page_break

    # 把 new 元素按顺序移动到 first_child 之前
    for elem in reversed(all_new):
        body.remove(elem)
    for elem in all_new:
        if first_child is not None:
            first_child.addprevious(elem)
        else:
            body.append(elem)


def update_existing_sections(doc):
    """对原文中涉及技术规格的章节做橙色高亮修改提示。"""
    keywords = {
        "#include": ["V2.0 中头文件保持 <stdio.h>/<stdlib.h>/<string.h> 等标准 C 头，新增 <stdarg.h> 变长参数用于彩色打印，"
                     "通过 #ifdef _WIN32 兼容 Windows.h 与 unistd.h。"],
        "#define": ["V2.0 新增 DATA_MAGIC=\"SPRT\"、DATA_VERSION=2、INIT_CAPACITY=8、GROWTH_FACTOR=2 等宏；"
                    "原 MAX_SCHOOLS/MAX_PROJECTS/MAX_RESULTS 固定上限宏已被动态内存取代。"],
        "struct": ["V2.0 保留 School/Project/Result 结构体字段，新增 DataFileHeader（含 magic 与 version）以支持文件校验。"],
        "main": ["V2.0 中 main() 函数支持命令行参数解析（argc/argv），新增 --help/-f/-t/--stats 四种非交互模式。"],
        "save": ["V2.0 的保存函数以二进制写入魔数+版本头，再顺序写入动态数组内容；新增写进度条反馈。"],
        "load": ["V2.0 的加载函数先校验魔数/版本/计数合法性，再按需 realloc 读入数据；加载完成后自动重算各校总分。"],
        "菜单": ["V2.0 菜单采用彩色边框+数字选择+OK/ERROR 彩色状态行，新增 \"9. 统计概览\" 选项。"],
        "score": ["V2.0 计分规则保持不变（TOP3=5,3,2；TOP5=7,5,3,2,1），实现统一为 get_rank_score() 函数。"],
    }
    for para in doc.paragraphs:
        text = para.text.strip()
        for key, notes in keywords.items():
            if key in text and len(text) < 80:
                # 在该段后插入一个橙色说明段落
                note = notes[0]
                new_p = insert_paragraph_after(para)
                new_p.paragraph_format.left_indent = Cm(0.5)
                new_p.paragraph_format.space_after = Pt(4)
                r = new_p.add_run("▶ [V2.0 升级提示] " + note)
                set_font(r, size=10, color=ORANGE, highlight="yellow")
                break


def main():
    if not os.path.exists(DOCX_NAME):
        print(f"[ERROR] 找不到 {DOCX_NAME}")
        sys.exit(1)

    # 备份原文件
    if not os.path.exists(BACKUP_NAME):
        shutil.copy2(DOCX_NAME, BACKUP_NAME)
        print(f"[OK] 已备份原文件到 {BACKUP_NAME}")

    doc = Document(DOCX_NAME)

    # 设置默认字体
    style = doc.styles['Normal']
    style.font.name = FONT_NAME
    style.font.size = Pt(11)
    rPr = style.element.get_or_add_rPr()
    rFonts = rPr.find(qn('w:rFonts'))
    if rFonts is None:
        rFonts = OxmlElement('w:rFonts')
        rPr.append(rFonts)
    rFonts.set(qn('w:eastAsia'), FONT_NAME)
    rFonts.set(qn('w:ascii'), FONT_NAME)
    rFonts.set(qn('w:hAnsi'), FONT_NAME)

    print("[..] 正在构建 V2.0 版本更新页 ...")
    build_v2_page(doc)
    print("[..] 正在更新原有章节的 V2.0 高亮提示 ...")
    update_existing_sections(doc)

    doc.save(DOCX_NAME)
    print(f"[OK] 文档已更新：{DOCX_NAME}")
    print(f"[OK] 备份文件：{BACKUP_NAME}")


if __name__ == "__main__":
    main()
