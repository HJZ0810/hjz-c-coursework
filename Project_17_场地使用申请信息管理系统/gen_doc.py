#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
场地使用申请信息管理系统 V2.0 文档生成脚本
功能：
1. 备份原docx文档为 _V1_backup.docx
2. 更新原文档，添加V2.0新特性章节并用蓝色高亮标记
"""

import os
import shutil
import sys

def backup_document(original_path, backup_path):
    """备份原文档"""
    if os.path.exists(backup_path):
        print(f"[提示] 备份文件已存在: {backup_path}")
        return True
    if not os.path.exists(original_path):
        print(f"[错误] 原文档不存在: {original_path}")
        return False
    shutil.copy2(original_path, backup_path)
    print(f"[成功] 已备份原文档到: {backup_path}")
    return True

def update_docx_with_python_docx(original_path):
    """使用python-docx更新文档，添加V2.0内容并蓝色高亮"""
    try:
        from docx import Document
        from docx.shared import RGBColor, Pt
        from docx.enum.text import WD_COLOR_INDEX
    except ImportError:
        print("[提示] python-docx未安装，尝试使用pip安装...")
        import subprocess
        try:
            subprocess.check_call([sys.executable, "-m", "pip", "install", "python-docx", "-q"])
            from docx import Document
            from docx.shared import RGBColor, Pt
            from docx.enum.text import WD_COLOR_INDEX
        except Exception as e:
            print(f"[警告] 无法安装python-docx: {e}")
            print("[提示] 文档将使用基础方式更新（手动模式提示）")
            return False

    doc = Document(original_path)

    blue_color = RGBColor(0x00, 0x70, 0xC0)

    doc.add_page_break()

    heading = doc.add_heading('V2.0 版本更新说明', level=1)
    for run in heading.runs:
        run.font.color.rgb = blue_color

    doc.add_paragraph()

    p = doc.add_paragraph()
    run = p.add_run('【升级到 V2.0 标准 - 新增特性一览】')
    run.bold = True
    run.font.color.rgb = blue_color
    run.font.size = Pt(12)

    features = [
        ("1. UTF-8 BOM 编码支持", "文件使用UTF-8 BOM编码，配合SetConsoleOutputCP(65001)实现Windows控制台中文正常显示"),
        ("2. ANSI 彩色终端输出", "支持8色ANSI转义序列，成功/错误/提示信息使用不同颜色区分，界面更友好"),
        ("3. CLI 命令行参数", "支持 -h/--help 显示帮助，-t/--test 运行内置自测框架"),
        ("4. 动态数组内存管理", "场地(Venue)和申请(Apply)使用动态数组存储，自动扩容，替代V1.0链表实现"),
        ("5. 统一错误码枚举", "使用 VenueStatus 枚举统一管理所有错误/成功状态码，错误处理更规范"),
        ("6. VENUV2 Magic Header持久化", "二进制文件格式，以\"VENUV2\"为魔数开头，含版本号校验，数据更安全"),
        ("7. 内置自测框架", "包含28项自动化测试用例，覆盖增删改查、持久化、扩容等所有核心功能"),
        ("8. 申请审批流程", "新增审批功能，申请状态分为待审批/已批准/已拒绝三种，支持审批备注"),
        ("9. 统计报表功能", "提供数据统计：基础数据统计、审批状态统计、场地容量与价格统计"),
        ("10. 完整中文注释", "所有函数和关键代码段均有详细中文注释说明"),
    ]

    for title, desc in features:
        p = doc.add_paragraph()
        run = p.add_run(title)
        run.bold = True
        run.font.color.rgb = blue_color
        p.add_run(f"：{desc}")

    doc.add_paragraph()

    p = doc.add_paragraph()
    run = p.add_run('【编译方式】')
    run.bold = True
    run.font.color.rgb = blue_color

    p = doc.add_paragraph()
    p.add_run('    使用 GCC 编译（零警告）：').bold = True
    code_p = doc.add_paragraph()
    code_run = code_p.add_run('    D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o venue.exe main.c')
    code_run.font.name = 'Consolas'

    doc.add_paragraph()

    p = doc.add_paragraph()
    run = p.add_run('【测试验证】')
    run.bold = True
    run.font.color.rgb = blue_color

    test_items = [
        "运行 venue.exe --test，所有28项测试应全部PASS通过",
        "运行 venue.exe -h，显示完整帮助信息",
        "运行 venue.exe（无参数），进入交互菜单模式",
    ]
    for item in test_items:
        p = doc.add_paragraph(item, style='List Bullet')
        for run in p.runs:
            run.font.color.rgb = blue_color

    doc.add_paragraph()

    p = doc.add_paragraph()
    run = p.add_run('【菜单结构（主菜单）】')
    run.bold = True
    run.font.color.rgb = blue_color

    menu_items = [
        "1. 申请管理 - 提交场地使用申请",
        "2. 审批管理 - 审批待处理申请（批准/拒绝+备注）",
        "3. 查询功能 - 按单位/场地/申请编号多条件查询",
        "4. 统计报表 - 数据统计分析（单位数、场地数、审批统计、容量统计）",
        "5. 单位管理 - 添加/删除/列表单位",
        "6. 场地管理 - 添加/删除/列表场地",
        "7. 全部数据 - 浏览所有记录",
        "8. 保存数据 - 手动保存到VENUV2二进制文件",
        "0. 退出系统（自动保存数据）",
    ]
    for item in menu_items:
        doc.add_paragraph(item, style='List Bullet')

    doc.save(original_path)
    print(f"[成功] 文档已更新，V2.0新特性已用蓝色高亮标记")
    return True

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    original_docx = os.path.join(script_dir, "Project_17_场地使用申请信息管理系统.docx")
    backup_path = os.path.join(script_dir, "Project_17_场地使用申请信息管理系统_V1_backup.docx")

    print("=" * 60)
    print("  场地使用申请信息管理系统 V2.0 文档处理工具")
    print("=" * 60)
    print()

    print("[步骤1] 备份V1文档...")
    if not backup_document(original_docx, backup_path):
        print("[错误] 备份失败，终止")
        return 1

    print()
    print("[步骤2] 更新文档并添加蓝色高亮V2.0内容...")
    if not update_docx_with_python_docx(original_docx):
        print("[提示] 已完成备份，文档更新请手动在Word中添加V2.0章节")
        print("       V2.0新增特性列表：")
        print("       - UTF-8 BOM + SetConsoleOutputCP(65001)")
        print("       - ANSI彩色终端")
        print("       - CLI (-h/-t) 参数")
        print("       - 动态数组（场地+申请）")
        print("       - VenueStatus 统一错误码枚举")
        print("       - \"VENUV2\" Magic Header二进制持久化")
        print("       - 28项内置自测")
        print("       - 申请审批流程（待审批/已批准/已拒绝）")
        print("       - 统计报表功能")
        print("       - 完整中文注释")

    print()
    print("=" * 60)
    print("  处理完成！")
    print("=" * 60)
    return 0

if __name__ == "__main__":
    sys.exit(main())
