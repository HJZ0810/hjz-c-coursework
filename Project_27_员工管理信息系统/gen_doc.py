# -*- coding: utf-8 -*-
"""
员工管理信息系统 V2.0 文档生成/更新脚本
功能：
1. 备份原文档为 _V1_backup.docx
2. 在原文档基础上添加V2.0更新章节，蓝色高亮显示新特性
"""

import os
import shutil
from docx import Document
from docx.shared import Pt, RGBColor, Inches
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.style import WD_STYLE_TYPE
from docx.oxml.ns import qn
from docx.oxml import OxmlElement

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DOCX_NAME = "Project_27_员工管理信息系统.docx"
BACKUP_NAME = "Project_27_员工管理信息系统_V1_backup.docx"
DOCX_PATH = os.path.join(SCRIPT_DIR, DOCX_NAME)
BACKUP_PATH = os.path.join(SCRIPT_DIR, BACKUP_NAME)

BLUE_COLOR = RGBColor(0x00, 0x70, 0xC0)
DARK_BLUE = RGBColor(0x00, 0x20, 0x60)
HIGHLIGHT_YELLOW = "yellow"


def set_cell_shading(cell, color_hex):
    """设置单元格背景色"""
    shading = OxmlElement('w:shd')
    shading.set(qn('w:fill'), color_hex)
    cell._tc.get_or_add_tcPr().append(shading)


def add_blue_heading(doc, text, level=1):
    """添加蓝色标题"""
    h = doc.add_heading(text, level=level)
    for run in h.runs:
        run.font.color.rgb = BLUE_COLOR
    return h


def add_highlighted_paragraph(doc, label, content, highlight=False):
    """添加带标签的段落，标签蓝色高亮"""
    p = doc.add_paragraph()
    if label:
        run_label = p.add_run(label)
        run_label.bold = True
        run_label.font.color.rgb = BLUE_COLOR
        run_label.font.size = Pt(11)
    run_content = p.add_run(content)
    run_content.font.size = Pt(11)
    if highlight:
        run_content.font.highlight_color = 7  # WD_COLOR_INDEX.YELLOW
    return p


def add_feature_table(doc, features):
    """添加V2.0新特性表格"""
    table = doc.add_table(rows=1, cols=3)
    table.style = 'Table Grid'
    hdr = table.rows[0].cells
    hdr[0].text = "编号"
    hdr[1].text = "V2.0新特性"
    hdr[2].text = "说明"
    for cell in hdr:
        set_cell_shading(cell, "0070C0")
        for para in cell.paragraphs:
            for run in para.runs:
                run.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)
                run.bold = True
    for i, (name, desc) in enumerate(features, 1):
        row = table.add_row().cells
        row[0].text = str(i)
        row[1].text = name
        row[2].text = desc
        for cell in row:
            for para in cell.paragraphs:
                for run in para.runs:
                    run.font.color.rgb = BLUE_COLOR if cell is row[1] else None
                    run.font.size = Pt(10)
    return table


def backup_document():
    """备份原文档"""
    if os.path.exists(DOCX_PATH) and not os.path.exists(BACKUP_PATH):
        shutil.copy2(DOCX_PATH, BACKUP_PATH)
        print(f"[备份] 原文档已备份至: {BACKUP_NAME}")
    elif os.path.exists(BACKUP_PATH):
        print(f"[备份] 备份文件已存在: {BACKUP_NAME}")


def main():
    os.chdir(SCRIPT_DIR)

    print("=" * 60)
    print("  员工管理信息系统 V2.0 文档更新工具")
    print("=" * 60)

    backup_document()

    if os.path.exists(DOCX_PATH):
        doc = Document(DOCX_PATH)
        print(f"[加载] 已加载现有文档: {DOCX_NAME}")
    else:
        doc = Document()
        print("[创建] 创建新文档")

    doc.add_page_break()

    add_blue_heading(doc, "V2.0 版本升级说明", level=1)

    p = doc.add_paragraph()
    run = p.add_run("本文档记录员工管理信息系统从V1.0升级到V2.0的所有改进内容。"
                   "V2.0版本采用全新架构，遵循现代C语言开发规范，提供更稳定、更易用的员工管理功能。")
    run.font.size = Pt(11)

    add_blue_heading(doc, "V2.0 技术架构升级", level=2)

    features = [
        ("UTF-8 BOM编码支持", "源文件采用UTF-8 BOM编码，彻底解决中文乱码问题"),
        ("Windows控制台UTF-8", "调用SetConsoleOutputCP(65001)设置控制台代码页为UTF-8"),
        ("ANSI彩色终端输出", "支持ANSI转义序列，提供彩色终端界面（成功/错误/提示分色显示）"),
        ("CLI命令行参数", "支持 -h/--help 帮助、-t/--test 自测、无参数启动交互菜单"),
        ("动态数组存储", "使用动态扩容数组（Dynamic Array）替代链表，随机访问O(1)，缓存友好"),
        ("统一错误码枚举", "定义EmpStatus枚举类型，统一管理所有错误码（13种错误类型）"),
        ("EMPV2魔数文件格式", "二进制持久化文件以\"EMPV2\"为魔数头，包含版本号校验，防止格式错误"),
        ("内置20项自测", "覆盖CRUD、部门管理、薪资计算、统计、文件I/O等20项自动化测试"),
        ("交互菜单系统", "完整的菜单驱动交互界面，职位/部门/员工/薪资/统计分模块管理"),
        ("数据完整性保护", "删除时检查外键关联，员工删除级联删除薪资记录，防止数据不一致"),
        ("级联删除", "删除员工时自动删除其所有薪资记录，保证数据引用完整性"),
        ("零警告编译", "使用gcc -std=c99 -Wall -Wextra -O2编译，零警告通过"),
    ]
    add_feature_table(doc, features)

    add_blue_heading(doc, "V2.0 核心功能模块", level=2)

    modules = [
        ("职位管理", "职位的增删查，职位与员工关联保护（有员工使用的职位不允许删除）"),
        ("部门管理", "部门信息管理，包含负责人、联系电话，部门与员工关联保护"),
        ("员工管理", "员工完整信息管理（姓名、性别、年龄、职位、部门、电话），支持级联删除"),
        ("薪资管理", "薪资记录管理，自动计算实发工资（基本工资+奖金-扣除），按月/年记录"),
        ("统计分析", "员工总数、男女比例、薪资总额/平均/最高/最低、各部门人数统计"),
        ("数据持久化", "EMPV2二进制格式保存/加载，启动时自动加载，退出时自动保存"),
        ("登录验证", "3次尝试机会的登录系统，默认账号admin/123456"),
    ]

    for mod_name, mod_desc in modules:
        add_highlighted_paragraph(doc, f"■ {mod_name}：", mod_desc, highlight=True)

    add_blue_heading(doc, "V2.0 文件格式说明（EMPV2）", level=2)

    p = doc.add_paragraph()
    p.add_run("数据文件：").bold = True
    p.add_run("empv2.dat（二进制格式）")

    format_items = [
        "魔数头：\"EMPV2\"（5字节），用于文件格式校验",
        "版本号：int类型（当前版本为2），防止版本不兼容导致数据损坏",
        "数据计数：四个int分别存储职位/部门/员工/薪资记录数量",
        "ID序列：四个int存储下一个可用ID，保证ID自增连续性",
        "数据区：连续存储Job[]、Dept[]、Emp[]、Income[]结构体二进制数据",
    ]
    for item in format_items:
        p = doc.add_paragraph(item, style='List Bullet')
        for run in p.runs:
            run.font.size = Pt(10)

    add_blue_heading(doc, "V2.0 编译与运行", level=2)

    add_highlighted_paragraph(doc, "编译器：", "D:/bin/gcc.exe (MinGW GCC)")
    add_highlighted_paragraph(doc, "编译标准：", "C99 (-std=c99)")
    add_highlighted_paragraph(doc, "编译选项：", "-Wall -Wextra -O2（零警告）")
    add_highlighted_paragraph(doc, "目标文件：", "emp.exe")
    add_highlighted_paragraph(doc, "Makefile：", "支持 make / make test / make run / make clean")

    add_blue_heading(doc, "V2.0 命令行用法", level=2)

    usage_table = doc.add_table(rows=1, cols=2)
    usage_table.style = 'Table Grid'
    hdr = usage_table.rows[0].cells
    hdr[0].text = "命令"
    hdr[1].text = "功能"
    for cell in hdr:
        set_cell_shading(cell, "0070C0")
        for para in cell.paragraphs:
            for run in para.runs:
                run.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)
                run.bold = True

    usages = [
        ("emp.exe", "启动交互菜单系统（需登录）"),
        ("emp.exe -h / --help", "显示帮助信息"),
        ("emp.exe -t / --test", "运行20项内置自测"),
        ("make", "编译项目"),
        ("make test", "编译并运行自测"),
        ("make run", "编译并运行"),
        ("make clean", "清理生成文件"),
    ]
    for cmd, desc in usages:
        row = usage_table.add_row().cells
        row[0].text = cmd
        row[1].text = desc
        for cell in row:
            for para in cell.paragraphs:
                for run in para.runs:
                    run.font.size = Pt(10)
                    if cell is row[0]:
                        run.font.color.rgb = BLUE_COLOR
                        run.font.name = 'Consolas'

    add_blue_heading(doc, "V2.0 测试覆盖（共20项）", level=2)

    test_cases = [
        "T01-T03: 基础CRUD - 添加职位、部门、员工",
        "T04-T05: 参数校验 - 无效年龄、无效职位ID",
        "T06-T07: 薪资功能 - 添加薪资记录、实发工资计算验证",
        "T08-T10: 查询功能 - 按ID查找职位、部门、员工",
        "T11-T13: 统计功能 - 多条薪资添加、员工数统计、平均薪资统计",
        "T14-T16: 文件I/O - 保存文件、加载文件、EMPV2魔数验证",
        "T17: 关联保护 - 有员工关联的职位不允许删除",
        "T18: 级联删除 - 删除员工时自动删除相关薪资记录",
        "T19: 边界处理 - 删除不存在的记录返回错误码",
        "T20: 动态扩容 - 批量添加20条数据测试动态数组自动扩容",
    ]
    for tc in test_cases:
        p = doc.add_paragraph(tc, style='List Bullet')
        for run in p.runs:
            run.font.size = Pt(10)
            run.font.color.rgb = DARK_BLUE

    add_blue_heading(doc, "V2.0 错误码说明", level=2)

    err_table = doc.add_table(rows=1, cols=3)
    err_table.style = 'Table Grid'
    hdr = err_table.rows[0].cells
    hdr[0].text = "错误码"
    hdr[1].text = "枚举值"
    hdr[2].text = "含义"
    for cell in hdr:
        set_cell_shading(cell, "0070C0")
        for para in cell.paragraphs:
            for run in para.runs:
                run.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)
                run.bold = True

    errors = [
        ("0", "EMP_OK", "操作成功"),
        ("1", "EMP_ERR_MEMORY", "内存分配失败"),
        ("2", "EMP_ERR_NOT_FOUND", "未找到记录"),
        ("3", "EMP_ERR_DUPLICATE", "存在关联记录无法删除"),
        ("4", "EMP_ERR_INVALID_PARAM", "参数无效"),
        ("5", "EMP_ERR_FILE_OPEN", "无法打开文件"),
        ("6", "EMP_ERR_FILE_READ", "文件读取失败"),
        ("7", "EMP_ERR_FILE_WRITE", "文件写入失败"),
        ("8", "EMP_ERR_FILE_MAGIC", "文件魔数不匹配（格式错误）"),
        ("9", "EMP_ERR_FILE_VERSION", "文件版本不兼容"),
        ("10", "EMP_ERR_FILE_CORRUPT", "文件损坏"),
        ("12", "EMP_ERR_LOGIN_FAILED", "登录失败"),
    ]
    for code, name, desc in errors:
        row = err_table.add_row().cells
        row[0].text = code
        row[1].text = name
        row[2].text = desc
        for cell in row:
            for para in cell.paragraphs:
                for run in para.runs:
                    run.font.size = Pt(9)
                    if cell is row[1]:
                        run.font.color.rgb = BLUE_COLOR
                        run.font.name = 'Consolas'

    doc.add_paragraph()
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = p.add_run("—— 员工管理信息系统 V2.0 ——")
    run.font.color.rgb = BLUE_COLOR
    run.bold = True
    run.font.size = Pt(14)

    doc.save(DOCX_PATH)
    print(f"[保存] 文档已更新并保存: {DOCX_NAME}")
    print()
    print("=" * 60)
    print("  文档更新完成！")
    print(f"  - 原文件: {DOCX_NAME}")
    print(f"  - 备份文件: {BACKUP_NAME}")
    print(f"  - V2.0新内容已蓝色高亮标记")
    print("=" * 60)


if __name__ == "__main__":
    main()
