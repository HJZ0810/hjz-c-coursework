# -*- coding: utf-8 -*-
"""
gen_doc.py — Warehouse Management System V2.0 文档更新脚本
在原 docx 开头插入 V2.0 版本说明页（蓝色文字浅蓝底高亮），
并更新技术章节以反映动态内存/ANSI颜色/错误码等新特性。
使用 python-docx 操作，字体：微软雅黑；新增内容蓝色高亮。
"""
import os, sys, shutil
from docx import Document
from docx.shared import Pt, RGBColor, Cm
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.oxml.ns import qn
from docx.oxml import OxmlElement

DOCX_NAME = "Project_21_仓库管理信息系统.docx"
BACKUP_NAME = "Project_21_仓库管理信息系统_V1_backup.docx"
FONT_NAME = "微软雅黑"
BLUE = RGBColor(0x00, 0x00, 0xFF)
LIGHT_BLUE_BG = "DCE6F1"


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
        shd = OxmlElement('w:shd')
        shd.set(qn('w:val'), 'clear')
        shd.set(qn('w:color'), 'auto')
        shd.set(qn('w:fill'), highlight)
        rPr.append(shd)


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
    set_font(run, size=sizes.get(level, 12), bold=True, color=BLUE, highlight=LIGHT_BLUE_BG)
    return p


def add_bullet(doc, text, new=False):
    p = doc.add_paragraph(style='List Bullet')
    pf = p.paragraph_format
    pf.space_after = Pt(3)
    pf.line_spacing = 1.4
    run = p.add_run(text)
    color = BLUE if new else None
    hl = LIGHT_BLUE_BG if new else None
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
    body = doc.element.body
    first_child = body[0] if len(body) > 0 else None

    elems = []

    title = doc.add_paragraph()
    title.alignment = WD_ALIGN_PARAGRAPH.CENTER
    title.paragraph_format.space_after = Pt(6)
    r = title.add_run("V2.0 完全体升级版说明")
    set_font(r, size=24, bold=True, color=BLUE, highlight=LIGHT_BLUE_BG)
    elems.append(title._p)

    sub = doc.add_paragraph()
    sub.alignment = WD_ALIGN_PARAGRAPH.CENTER
    sub.paragraph_format.space_after = Pt(12)
    r = sub.add_run("Warehouse Management System — Enhanced Edition  |  2025-2026 第二学期")
    set_font(r, size=11, color=BLUE, highlight=LIGHT_BLUE_BG)
    elems.append(sub._p)

    sep = doc.add_paragraph()
    elems.append(sep._p)

    add_heading(doc, "一、V2.0 版本重要变更总览", level=1)
    elems_2 = []
    changes = [
        ("编码与平台", "UTF-8 BOM 编码；C99 标准；Windows 控制台通过 SetConsoleOutputCP(65001) 设置UTF-8输出，支持ANSI彩色终端。"),
        ("动态内存管理", "彻底移除固定数组上限，所有商品/入库记录/出库记录均使用动态数组(DynArray)存储，按倍增策略(INIT_CAP=16, GROW_FACTOR=2)自动扩容。"),
        ("统一错误码", "引入 WareStatus 枚举（WARE_OK / WARE_ERR_MEMORY / WARE_ERR_NOT_FOUND / WARE_ERR_MAGIC / WARE_ERR_VERSION 等14种状态），所有核心函数返回状态码替代原int返回。"),
        ("ANSI 彩色终端 UI", "新增彩色菜单、[√]成功/[×]错误/[i]信息/[!]警告状态标记，标题/边框/表格高亮显示。"),
        ("命令行参数支持", "新增 --help/-h 显示帮助，-t/--test 运行内置17项自测套件（覆盖CRUD、出入库、库存统计、文件I/O）。"),
        ("文件持久化 Magic Header", "数据文件以魔数 \"WAREV2\" + 版本号2开头，加载时校验魔数/版本完整性，防止误读损坏文件。"),
        ("功能增强", "新增库存预警功能（阈值自定义）、商品关键字搜索、自动日期填充、入库/出库独立子菜单。"),
        ("代码质量提升", "全量重构：函数分块清晰、静态函数化、完整中文注释、D:/bin/gcc.exe -Wall -Wextra 零警告通过。"),
        ("Makefile 构建", "新增 Makefile，使用 D:/bin/gcc.exe 一键编译为 warehouse.exe，make test 自动运行内置测试。"),
        ("内置自测", "17项测试用例共391个断言全部通过，覆盖初始化/增删改查/出入库组合/库存统计/文件保存加载/魔数校验/版本校验/动态扩容/空操作/记录完整性。"),
    ]
    for title_text, desc in changes:
        p = doc.add_paragraph()
        p.paragraph_format.space_after = Pt(4)
        p.paragraph_format.line_spacing = 1.4
        p.paragraph_format.left_indent = Cm(0.5)
        r1 = p.add_run(f"● [{title_text}] ")
        set_font(r1, size=11, bold=True, color=BLUE, highlight=LIGHT_BLUE_BG)
        r2 = p.add_run(desc)
        set_font(r2, size=11, color=BLUE, highlight=LIGHT_BLUE_BG)
        elems_2.append(p._p)

    add_heading(doc, "二、保留的原有功能", level=1)
    elems_3 = []
    keeps = [
        "商品信息管理（编号、名称、规格、单位、库存）",
        "商品入库管理（记录入库商品、数量、日期）",
        "商品出库管理（记录出库商品、数量、日期，库存不足自动拦截）",
        "库存总览统计（商品总数、总库存）",
        "数据文件持久化保存 / 读取",
        "交互式菜单操作界面",
    ]
    for k in keeps:
        add_bullet(doc, k, new=False)
    elems_3 = doc.paragraphs[-len(keeps):]
    elems_3 = [p._p for p in elems_3]

    add_heading(doc, "三、命令行用法", level=1)
    elems_4 = []
    usage_lines = [
        "warehouse.exe                   进入交互菜单模式（首次运行自动初始化示例数据）",
        "warehouse.exe --help / -h       显示帮助信息",
        "warehouse.exe -t / --test       运行内置自测套件（17项测试全通过）",
    ]
    for line in usage_lines:
        p = doc.add_paragraph()
        p.paragraph_format.space_after = Pt(2)
        p.paragraph_format.left_indent = Cm(0.7)
        r = p.add_run("  " + line)
        set_font(r, size=10, color=BLUE, highlight=LIGHT_BLUE_BG)
        elems_4.append(p._p)

    add_heading(doc, "四、数据结构升级说明", level=1)
    elems_5 = []
    ds_intro = doc.add_paragraph()
    ds_intro.paragraph_format.line_spacing = 1.5
    r = ds_intro.add_run(
        "V2.0 中 Material/InRecord/OutRecord 结构体保持原有核心字段语义，"
        "新增通用动态数组 DynArray 替代固定长度数组存储，统一管理内存扩容："
    )
    set_font(r, size=11, color=BLUE, highlight=LIGHT_BLUE_BG)
    elems_5.append(ds_intro._p)
    ds_items = [
        "DynArray: void* data + size_t count/capacity/elem_size 通用动态数组结构",
        "初始容量 INIT_CAP = 16，扩容因子 GROW_FACTOR = 2（倍增策略）",
        "dynarray_init/dynarray_free/dynarray_push/dynarray_get/dynarray_remove 统一操作接口",
        "ware_shutdown() 在退出/重载时释放全部堆内存，无内存泄漏",
        "文件魔数 MAGIC_HEADER=\"WAREV2\" + FILE_VERSION=2 实现安全加载校验",
    ]
    for d in ds_items:
        add_bullet(doc, d, new=True)
    elems_5b = doc.paragraphs[-len(ds_items):]
    elems_5 += [p._p for p in elems_5b]

    add_heading(doc, "五、编译与运行", level=1)
    elems_6 = []
    compile_lines = [
        "编译器：MinGW-w64 GCC  (D:\\bin\\gcc.exe)",
        "编译命令：gcc -std=c99 -Wall -Wextra -O2 -o warehouse.exe main.c",
        "一键构建：make all",
        "自动测试：make test",
        "编码要求：源文件保存为 UTF-8 BOM，Windows 终端自动设置代码页65001",
    ]
    for c in compile_lines:
        add_bullet(doc, c, new=True)
    elems_6b = doc.paragraphs[-len(compile_lines):]
    elems_6 = [p._p for p in elems_6b]

    end_p = doc.add_paragraph()
    r = end_p.add_run("—— V2.0 版本说明页结束 ——")
    set_font(r, size=10, color=BLUE, highlight=LIGHT_BLUE_BG)
    end_p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    elems.append(end_p._p)

    page_break = doc.add_paragraph()
    from docx.enum.text import WD_BREAK
    r = page_break.add_run()
    r.add_break(WD_BREAK.PAGE)
    elems.append(page_break._p)

    all_new = []
    all_new.extend(elems[:2])
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
    all_new.append(elems[-2])
    all_new.append(elems[-1])

    for elem in reversed(all_new):
        body.remove(elem)
    for elem in all_new:
        if first_child is not None:
            first_child.addprevious(elem)
        else:
            body.append(elem)


def update_existing_sections(doc):
    keywords = {
        "#include": ["V2.0 中头文件先包含 windows.h（_WIN32条件编译），再包含标准头文件，新增SetConsoleOutputCP/SetConsoleCP设置UTF-8输出。"],
        "#define": ["V2.0 新增 MAGIC_HEADER=\"WAREV2\"、FILE_VERSION=2、INIT_CAP=16、GROW_FACTOR=2 等宏；固定数组上限已被动态内存取代。"],
        "struct": ["V2.0 保留 Material/InRecord/OutRecord 核心字段，新增通用 DynArray 动态数组结构和 Warehouse 顶层聚合结构。"],
        "main": ["V2.0 中 main() 函数支持命令行参数解析（argc/argv），新增 -h/--help 帮助和 -t/--test 自测模式。"],
        "save": ["V2.0 的保存函数先写入6字节魔数 WAREV2 和版本号2，再顺序写入next_id和三个动态数组的全部内容。"],
        "load": ["V2.0 的加载函数先校验魔数和版本号，再初始化Warehouse、恢复next_id，最后读入商品/入库/出库记录。"],
        "菜单": ["V2.0 菜单采用彩色边框+数字选择+[√]/[×]/[i]/[!]彩色状态行，新增库存预警和商品搜索功能。"],
        "入库": ["V2.0 入库操作自动更新库存，自动填充当天日期（可手动指定），写入InRecord并自动保存。"],
        "出库": ["V2.0 出库操作检查库存充足性（库存不足返回 WARE_ERR_STOCK_INSUFFICIENT），防止负库存。"],
    }
    for para in doc.paragraphs:
        text = para.text.strip()
        for key, notes in keywords.items():
            if key in text and len(text) < 80:
                note = notes[0]
                new_p = insert_paragraph_after(para)
                new_p.paragraph_format.left_indent = Cm(0.5)
                new_p.paragraph_format.space_after = Pt(4)
                r = new_p.add_run("▶ [V2.0 升级提示] " + note)
                set_font(r, size=10, color=BLUE, highlight=LIGHT_BLUE_BG)
                break


def main():
    if not os.path.exists(DOCX_NAME):
        print(f"[ERROR] 找不到 {DOCX_NAME}")
        sys.exit(1)

    if not os.path.exists(BACKUP_NAME):
        shutil.copy2(DOCX_NAME, BACKUP_NAME)
        print(f"[OK] 已备份原文件到 {BACKUP_NAME}")

    doc = Document(DOCX_NAME)

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
