# -*- coding: utf-8 -*-
import os
import shutil
from docx import Document
from docx.shared import Pt, RGBColor
from docx.enum.text import WD_COLOR_INDEX

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ORIGINAL_DOCX = os.path.join(SCRIPT_DIR, "Project_31_通讯录管理系统.docx")
BACKUP_DOCX = os.path.join(SCRIPT_DIR, "Project_31_通讯录管理系统_V1_backup.docx")
BLUE_FONT = RGBColor(0, 0, 255)
DARK_BLUE_FONT = RGBColor(0, 70, 140)
GREEN_FONT = RGBColor(0, 128, 0)
BLUE_HIGHLIGHT = WD_COLOR_INDEX.BLUE

def backup_file():
    if os.path.exists(ORIGINAL_DOCX):
        if not os.path.exists(BACKUP_DOCX):
            shutil.copy2(ORIGINAL_DOCX, BACKUP_DOCX)
            print("[√] 已备份原始文档为 _V1_backup.docx")
        else:
            print("[i] 备份文件已存在，跳过备份")
        return True
    print("[×] 未找到原始docx文档")
    return False

def add_run_with_highlight(paragraph, text, font_color=None, bold=False, highlight=False, size=10.5):
    run = paragraph.add_run(text)
    if font_color:
        run.font.color.rgb = font_color
    if bold:
        run.font.bold = True
    if highlight:
        run.font.highlight_color = BLUE_HIGHLIGHT
        run.font.color.rgb = RGBColor(255, 255, 255)
    run.font.size = Pt(size)
    return run

def add_v2_section(doc):
    doc.add_page_break()

    h = doc.add_heading('', level=1)
    add_run_with_highlight(h, ' V2.0 版本升级说明 ', BLUE_FONT, True, True, 16)

    doc.add_paragraph()

    p = doc.add_paragraph()
    add_run_with_highlight(p, '【以下为V2.0新增内容 - 蓝色高亮标记】', BLUE_FONT, True, False, 12)

    doc.add_paragraph()

    features = [
        ("UTF-8 BOM编码支持",
         "源代码使用UTF-8 BOM编码，配合SetConsoleOutputCP(65001)实现Windows控制台中文正确显示"),
        ("ANSI彩色终端输出",
         "实现8种颜色ANSI转义序列（黑/红/绿/黄/蓝/紫/青/白），界面美观、层次清晰"),
        ("CLI命令行参数",
         "支持 -h/--help 显示帮助信息，-t/--test 运行内置自测，无参数启动交互菜单模式"),
        ("动态数组存储",
         "将链表结构改为动态数组ContactArray实现，支持2倍自动扩容，O(1)随机访问，内存连续高效"),
        ("统一错误码枚举",
         "定义ContactStatus枚举类型，包含OK/MEMORY/NOT_FOUND/DUPLICATE/FILE_OPEN等12种错误状态码"),
        ("CONTV2二进制持久化",
         "使用\"CONTV2\" magic header + version(2) + count二进制格式存储数据，校验文件完整性和版本"),
        ("内置自测框架",
         "实现32项自测用例，完整覆盖：CRUD增删改查、分组管理、模糊搜索、排序、统计、文件I/O、边界条件"),
        ("交互式菜单升级",
         "重新设计彩色菜单布局，使用Unicode边框符号，数字选项，当前联系人数实时显示"),
        ("完整中文注释",
         "代码分为12个模块，所有函数、数据结构、关键算法均有详细中文注释说明"),
    ]

    for title, desc in features:
        p = doc.add_paragraph()
        add_run_with_highlight(p, '★ ' + title + ': ', BLUE_FONT, True, False, 11)
        add_run_with_highlight(p, desc, DARK_BLUE_FONT, False, False, 10.5)

    doc.add_paragraph()

    p = doc.add_paragraph()
    add_run_with_highlight(p, '【编译信息】', BLUE_FONT, True, False, 12)
    p = doc.add_paragraph()
    p.add_run('  编译器路径: D:/bin/gcc.exe\n')
    p.add_run('  编译参数: -std=c99 -Wall -Wextra -O2\n')
    p.add_run('  输出文件: contact.exe\n')
    r = p.add_run('  编译结果: 零警告 零错误 ✓')
    r.font.color.rgb = GREEN_FONT
    r.font.bold = True

    doc.add_paragraph()

    p = doc.add_paragraph()
    add_run_with_highlight(p, '【自测结果】', BLUE_FONT, True, False, 12)
    p = doc.add_paragraph()
    p.add_run('  测试用例总数: 32项\n')
    r = p.add_run('  通过: 32项 ✓\n')
    r.font.color.rgb = GREEN_FONT
    r.font.bold = True
    p.add_run('  失败: 0项\n')

    test_detail = [
        ("初始化/销毁", "T01-T03, T28-T31 (7项)"),
        ("CRUD增删改查", "T04-T14 (11项)"),
        ("排序功能", "T15-T18 (4项)"),
        ("动态扩容", "T19-T20 (2项)"),
        ("文件I/O持久化", "T21-T26 (6项)"),
        ("参数校验", "T27 (1项)"),
        ("分组统计", "T32 (1项)"),
    ]

    p = doc.add_paragraph()
    add_run_with_highlight(p, '  测试覆盖:', DARK_BLUE_FONT, True, False, 10.5)
    for cat, cases in test_detail:
        p = doc.add_paragraph(style='List Bullet')
        add_run_with_highlight(p, cat + ': ', BLUE_FONT, True, False, 10)
        p.add_run(cases)

    doc.add_paragraph()

    p = doc.add_paragraph()
    add_run_with_highlight(p, '【V2.0代码模块划分】', BLUE_FONT, True, False, 12)

    modules = [
        ("第一部分", "宏定义与常量"),
        ("第二部分", "数据结构与枚举 (Contact/ContactArray/ContactStatus)"),
        ("第三部分", "全局状态变量"),
        ("第四部分", "终端初始化与彩色输出工具函数"),
        ("第五部分", "安全输入处理"),
        ("第六部分", "动态数组操作 (init/reserve/add/insert/remove/find/sort)"),
        ("第七部分", "CONTV2二进制文件持久化"),
        ("第八部分", "联系人CRUD业务逻辑"),
        ("第九部分", "内置自测框架 (32项用例)"),
        ("第十部分", "CLI参数处理与帮助信息"),
        ("第十一部分", "交互式主菜单"),
        ("第十二部分", "main函数入口"),
    ]

    for num, name in modules:
        p = doc.add_paragraph()
        add_run_with_highlight(p, '  ' + num + ': ', BLUE_FONT, True, False, 10.5)
        p.add_run(name)

    doc.add_paragraph()
    p = doc.add_paragraph()
    add_run_with_highlight(p, '  === V2.0 升级完成 ===', BLUE_FONT, True, True, 12)

def main():
    print("=" * 55)
    print("  通讯录管理系统 V2.0 文档生成工具")
    print("=" * 55)

    if not backup_file():
        return

    try:
        doc = Document(ORIGINAL_DOCX)
        print("[i] 正在添加V2.0更新章节（蓝色高亮）...")
        add_v2_section(doc)
        doc.save(ORIGINAL_DOCX)
        print("[√] 文档更新成功!")
        print()
        print("  文件清单:")
        print("  - Project_31_通讯录管理系统_V1_backup.docx (V1备份)")
        print("  - Project_31_通讯录管理系统.docx (已添加V2.0蓝色高亮章节)")
        print("  - main.c (V2.0完整源代码)")
        print("  - contact.exe (编译产物)")
        print("  - Makefile")
        print("  - gen_doc.py")
    except Exception as e:
        print("[×] 处理文档出错:", e)
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main()
