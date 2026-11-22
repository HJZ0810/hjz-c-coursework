# -*- coding: utf-8 -*-
"""
gen_doc.py - 研究生管理信息系统 V2.0 文档生成脚本
功能:
  1. 读取现有 docx 文档
  2. 在文档末尾追加 V2.0 更新章节
  3. 所有更新内容使用蓝色高亮字体标记
  4. 保留 V1 备份文件不变
"""

import os
import sys
import shutil
from docx import Document
from docx.shared import Pt, RGBColor, Cm, Inches
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.oxml.ns import qn
from docx.oxml import OxmlElement

PROJECT_DIR = os.path.dirname(os.path.abspath(__file__))
DOCX_NAME = "Project_22_研究生管理信息系统.docx"
BACKUP_NAME = "Project_22_研究生管理信息系统_V1_backup.docx"
DOCX_PATH = os.path.join(PROJECT_DIR, DOCX_NAME)
BACKUP_PATH = os.path.join(PROJECT_DIR, BACKUP_NAME)

BLUE = RGBColor(0x00, 0x70, 0xC0)
DARK_BLUE = RGBColor(0x00, 0x40, 0x80)
BLACK = RGBColor(0x00, 0x00, 0x00)


def set_cell_shading(cell, color_hex):
    """设置表格单元格底色"""
    shading = OxmlElement('w:shd')
    shading.set(qn('w:fill'), color_hex)
    shading.set(qn('w:val'), 'clear')
    cell._tc.get_or_add_tcPr().append(shading)


def add_blue_heading(doc, text, level=1):
    """添加蓝色标题段落"""
    h = doc.add_heading(text, level=level)
    for run in h.runs:
        run.font.color.rgb = DARK_BLUE
        run.font.name = '微软雅黑'
        run._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    return h


def add_blue_para(doc, text, bold=False, size=10.5):
    """添加蓝色正文段落"""
    p = doc.add_paragraph()
    run = p.add_run(text)
    run.font.color.rgb = BLUE
    run.font.size = Pt(size)
    run.font.name = '宋体'
    run._element.rPr.rFonts.set(qn('w:eastAsia'), '宋体')
    run.bold = bold
    return p


def add_blue_bullet(doc, text, level=0):
    """添加蓝色项目符号"""
    p = doc.add_paragraph(style='List Bullet')
    p.paragraph_format.left_indent = Cm(1.0 + level * 0.8)
    run = p.add_run(text)
    run.font.color.rgb = BLUE
    run.font.size = Pt(10.5)
    run.font.name = '宋体'
    run._element.rPr.rFonts.set(qn('w:eastAsia'), '宋体')
    return p


def add_v2_changelog(doc):
    """在文档末尾添加 V2.0 更新日志章节"""
    doc.add_page_break()

    add_blue_heading(doc, "V2.0 版本更新说明", level=1)

    add_blue_para(doc, "【V2.0 升级概述】", bold=True, size=12)
    add_blue_para(doc,
        "本章记录从 V1.0 到 V2.0 的全部架构升级与功能增强。"
        "所有 V2.0 新增/修改内容均以蓝色字体标识。")

    add_blue_heading(doc, "一、架构升级", level=2)

    table = doc.add_table(rows=1, cols=3, style='Table Grid')
    table.alignment = WD_ALIGN_PARAGRAPH.CENTER
    hdr = table.rows[0].cells
    hdr[0].text = "升级项"
    hdr[1].text = "V1.0"
    hdr[2].text = "V2.0"
    for cell in hdr:
        set_cell_shading(cell, "0070C0")
        for para in cell.paragraphs:
            para.alignment = WD_ALIGN_PARAGRAPH.CENTER
            for run in para.runs:
                run.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)
                run.bold = True
                run.font.name = '微软雅黑'
                run._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')

    rows_data = [
        ("数据结构", "单链表 (Singly Linked List)", "动态数组 (Dynamic Array)"),
        ("持久化格式", "管道分隔文本文件 (.dat)", "二进制文件 + Magic Header \"GRADV2\""),
        ("错误处理", "分散 printf 提示", "统一 GradStatus 枚举错误码 (15种)"),
        ("终端输出", "单色文本", "ANSI 彩色终端 (VT100/VT520)"),
        ("编码支持", "无编码设置", "UTF-8 BOM + SetConsoleOutputCP(65001)"),
        ("CLI 参数", "无", "-h/--help 帮助, -t/--test 自测"),
        ("数据模型", "专业/课程/学生/成绩/用户", "导师/研究生/成绩 (聚焦导师制)"),
        ("测试覆盖", "约12项基础测试", "44项内置自测 (CRUD+分配+成绩+文件I/O+边界)"),
        ("内存管理", "链表 malloc/free", "泛型动态数组 (自动扩容+收缩安全)"),
    ]
    for item, v1, v2 in rows_data:
        row = table.add_row().cells
        row[0].text = item
        row[1].text = v1
        row[2].text = v2
        for i, cell in enumerate(row):
            for para in cell.paragraphs:
                for run in para.runs:
                    run.font.size = Pt(9)
                    run.font.name = '宋体'
                    run._element.rPr.rFonts.set(qn('w:eastAsia'), '宋体')
                    if i == 2:
                        run.font.color.rgb = BLUE
                        run.bold = True

    doc.add_paragraph()

    add_blue_heading(doc, "二、核心功能增强", level=2)

    add_blue_para(doc, "1. 动态数组容器 (DynArray)", bold=True)
    add_blue_bullet(doc, "泛型实现: 使用 void* + elem_size 支持任意类型元素")
    add_blue_bullet(doc, "自动扩容: 容量不足时按 2 倍因子增长 (初始容量 8)")
    add_blue_bullet(doc, "内存连续: 缓存友好，随机访问 O(1)")
    add_blue_bullet(doc, "安全删除: 使用 memmove 移动元素，无内存泄漏")

    add_blue_para(doc, "2. 统一错误码 (GradStatus)", bold=True)
    error_items = [
        "GRAD_OK (0) - 操作成功",
        "GRAD_ERR_NULL_PTR (-1) - 空指针错误",
        "GRAD_ERR_MEMORY (-2) - 内存分配失败",
        "GRAD_ERR_NOT_FOUND (-3) - 目标未找到",
        "GRAD_ERR_DUPLICATE (-4) - ID重复冲突",
        "GRAD_ERR_INVALID_INPUT (-5) - 输入参数无效",
        "GRAD_ERR_FILE_IO (-6) - 文件I/O错误",
        "GRAD_ERR_INVALID_SCORE (-7) - 成绩超出0~100范围",
        "GRAD_ERR_ADVISOR_NOT_FOUND (-8) - 导师不存在",
        "GRAD_ERR_STUDENT_NOT_FOUND (-9) - 研究生不存在",
        "GRAD_ERR_ALREADY_ASSIGNED (-10) - 已分配导师(不可重复分配)",
        "GRAD_ERR_NOT_ASSIGNED (-11) - 未分配导师(无法解除)",
        "GRAD_ERR_FILE_CORRUPT (-12) - 文件数据损坏",
        "GRAD_ERR_FILE_VERSION (-13) - 文件版本不兼容",
        "GRAD_ERR_FILE_MAGIC (-14) - 文件魔数不匹配(非GRADV2文件)",
    ]
    for item in error_items:
        add_blue_bullet(doc, item)

    add_blue_para(doc, "3. 文件持久化 (GRADV2 Binary Format)", bold=True)
    add_blue_bullet(doc, "Magic Header: \"GRADV2\" (6字节) 用于文件类型校验")
    add_blue_bullet(doc, "Version字段: 支持未来格式升级的向后兼容性检查")
    add_blue_bullet(doc, "二进制存储: 直接读写结构体，比文本格式更高效")
    add_blue_bullet(doc, "完整性校验: 魔数+版本+记录数三重校验，防止误读损坏文件")

    add_blue_para(doc, "4. ANSI彩色终端", bold=True)
    add_blue_bullet(doc, "Windows 10+ 自动启用 ENABLE_VIRTUAL_TERMINAL_PROCESSING")
    add_blue_bullet(doc, "绿色 [√] 表示成功，红色 [×] 表示失败")
    add_blue_bullet(doc, "成绩颜色分级: <60红色, 60-79白色, 80-89青色, >=90绿色")
    add_blue_bullet(doc, "菜单标题黄色高亮，数据保存/加载蓝色提示")

    add_blue_para(doc, "5. 导师分配业务", bold=True)
    add_blue_bullet(doc, "导师-研究生 1:N 关系 (通过 advisor_id 外键关联)")
    add_blue_bullet(doc, "分配导师: 需先验证导师和学生均存在，且学生未分配导师")
    add_blue_bullet(doc, "解除分配: 学生必须已分配导师才可解除")
    add_blue_bullet(doc, "级联处理: 删除导师时自动清空其指导学生的导师ID")
    add_blue_bullet(doc, "导师列表显示每位导师指导的研究生人数")

    add_blue_para(doc, "6. 成绩管理", bold=True)
    add_blue_bullet(doc, "成绩范围严格校验: 0.0 ~ 100.0")
    add_blue_bullet(doc, "学生+课程名联合唯一约束，防止重复录入")
    add_blue_bullet(doc, "彩色分级显示: 不及格红色、优秀绿色")
    add_blue_bullet(doc, "自动计算单科平均分和全局统计(平均分/及格率/最高最低)")

    add_blue_heading(doc, "三、内置自测 (44项)", level=2)
    add_blue_para(doc,
        "运行 grad.exe -t 或 grad.exe --test 执行全部测试用例，"
        "覆盖以下场景:")

    test_table = doc.add_table(rows=1, cols=3, style='Table Grid')
    test_table.alignment = WD_ALIGN_PARAGRAPH.CENTER
    th = test_table.rows[0].cells
    th[0].text = "测试分类"
    th[1].text = "测试项"
    th[2].text = "数量"
    for cell in th:
        set_cell_shading(cell, "0070C0")
        for para in cell.paragraphs:
            para.alignment = WD_ALIGN_PARAGRAPH.CENTER
            for run in para.runs:
                run.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)
                run.bold = True

    test_rows = [
        ("导师CRUD", "添加/重复添加/查找/修改/删除/空ID校验/不存在删除", "T01-T08, T35, T39, T41 (11项)"),
        ("研究生CRUD", "添加/重复学号/查找/修改/删除/空ID校验/不存在删除", "T09-T12, T36-T37, T40, T42 (8项)"),
        ("导师分配", "分配/重复分配/分配不存在导师/解除/重复解除", "T13-T19 (7项)"),
        ("成绩管理", "录入/无效成绩/重复成绩/修改/平均分", "T20-T29 (10项)"),
        ("文件I/O", "保存/加载(数据完整性)/错误魔数校验", "T30-T34, T44 (6项)"),
        ("动态扩容", "批量添加20条(超过初始容量8触发扩容)", "T43 (1项)"),
        ("边界与安全", "空输入/不存在实体/级联删除", "T37-T42 (融入上述分类)"),
    ]
    for cat, items, count in test_rows:
        row = test_table.add_row().cells
        row[0].text = cat
        row[1].text = items
        row[2].text = count
        for cell in row:
            for para in cell.paragraphs:
                for run in para.runs:
                    run.font.size = Pt(9)
                    run.font.name = '宋体'
                    run._element.rPr.rFonts.set(qn('w:eastAsia'), '宋体')
                    run.font.color.rgb = BLUE

    doc.add_paragraph()
    add_blue_heading(doc, "四、编译与使用", level=2)
    add_blue_para(doc, "编译命令:", bold=True)
    p = doc.add_paragraph()
    run = p.add_run("    gcc -std=c99 -Wall -Wextra -O2 -o grad.exe main.c")
    run.font.name = 'Consolas'
    run.font.size = Pt(10)
    run.font.color.rgb = BLUE

    add_blue_para(doc, "Makefile:", bold=True)
    add_blue_bullet(doc, "make / make all  - 编译生成 grad.exe")
    add_blue_bullet(doc, "make test       - 编译并运行自测")
    add_blue_bullet(doc, "make run        - 编译并启动交互式菜单")
    add_blue_bullet(doc, "make clean      - 清理生成文件")

    add_blue_para(doc, "CLI参数:", bold=True)
    add_blue_bullet(doc, "grad.exe           - 启动交互式菜单(自动加载数据文件)")
    add_blue_bullet(doc, "grad.exe -h        - 显示帮助信息")
    add_blue_bullet(doc, "grad.exe --help    - 显示帮助信息")
    add_blue_bullet(doc, "grad.exe -t        - 运行内置自测")
    add_blue_bullet(doc, "grad.exe --test    - 运行内置自测")

    add_blue_heading(doc, "五、文件清单", level=2)
    file_table = doc.add_table(rows=1, cols=2, style='Table Grid')
    fh = file_table.rows[0].cells
    fh[0].text = "文件名"
    fh[1].text = "说明"
    for cell in fh:
        set_cell_shading(cell, "0070C0")
        for para in cell.paragraphs:
            for run in para.runs:
                run.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)
                run.bold = True

    files = [
        ("main.c", "V2.0主程序源代码 (UTF-8 BOM, C99标准)"),
        ("Makefile", "GNU Make构建脚本 (指定D:/bin/gcc.exe)"),
        ("grad.exe", "编译生成的可执行文件"),
        ("gradv2.dat", "V2.0二进制数据文件(运行时自动生成)"),
        ("gen_doc.py", "本文档生成/更新脚本"),
        ("README.md", "项目说明文档"),
        (BACKUP_NAME, "V1.0原始文档备份(升级前自动备份)"),
    ]
    for fname, desc in files:
        row = file_table.add_row().cells
        row[0].text = fname
        row[1].text = desc
        for cell in row:
            for para in cell.paragraphs:
                for run in para.runs:
                    run.font.size = Pt(9)
                    run.font.name = '宋体'
                    run._element.rPr.rFonts.set(qn('w:eastAsia'), '宋体')
                    run.font.color.rgb = BLUE

    doc.add_paragraph()
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = p.add_run("—— V2.0 升级完成 ——")
    run.font.color.rgb = DARK_BLUE
    run.font.size = Pt(12)
    run.bold = True
    run.font.name = '微软雅黑'
    run._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')

    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = p.add_run("升级日期: 2026-06-28 | 编译器: GCC (D:/bin/gcc.exe) | 标准: C99")
    run.font.color.rgb = BLUE
    run.font.size = Pt(9)
    run.font.name = '宋体'
    run._element.rPr.rFonts.set(qn('w:eastAsia'), '宋体')


def main():
    print("=" * 60)
    print("  研究生管理信息系统 V2.0 - 文档更新脚本")
    print("=" * 60)

    if not os.path.exists(DOCX_PATH):
        print(f"[ERROR] 未找到文档: {DOCX_PATH}")
        sys.exit(1)

    if not os.path.exists(BACKUP_PATH):
        print(f"[INFO] 创建V1备份: {BACKUP_NAME}")
        shutil.copy2(DOCX_PATH, BACKUP_PATH)
    else:
        print(f"[INFO] V1备份已存在: {BACKUP_NAME}")

    print(f"[INFO] 正在读取文档: {DOCX_NAME}")
    doc = Document(DOCX_PATH)

    print("[INFO] 添加 V2.0 更新章节(蓝色高亮)...")
    add_v2_changelog(doc)

    print(f"[INFO] 保存文档...")
    doc.save(DOCX_PATH)

    print("[OK] 文档更新完成!")
    print(f"     - V1备份: {BACKUP_NAME}")
    print(f"     - 更新文档: {DOCX_NAME}")
    print("=" * 60)


if __name__ == "__main__":
    main()
