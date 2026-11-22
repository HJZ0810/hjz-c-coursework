#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
学生宿舍管理信息系统 V2.0 文档生成器
生成 _V1_backup.docx 文档，V2.0 新特性使用蓝色高亮
"""

from docx import Document
from docx.shared import Pt, RGBColor, Inches
from docx.enum.text import WD_COLOR_INDEX
from docx.oxml.ns import qn
import os

def set_chinese_font(run, font_name="宋体", size=12, bold=False, color=None, highlight=None):
    run.font.name = font_name
    run._element.rPr.rFonts.set(qn('w:eastAsia'), font_name)
    run.font.size = Pt(size)
    run.font.bold = bold
    if color:
        run.font.color.rgb = color
    if highlight:
        run.font.highlight_color = highlight

def add_heading(doc, text, level=1):
    h = doc.add_heading(text, level=level)
    for run in h.runs:
        set_chinese_font(run, "黑体", 16 if level == 1 else 14, bold=True)

def add_para(doc, text, bold=False, blue_highlight=False):
    p = doc.add_paragraph()
    run = p.add_run(text)
    if blue_highlight:
        set_chinese_font(run, "宋体", 12, bold=bold,
                        color=RGBColor(0, 0, 255),
                        highlight=WD_COLOR_INDEX.BRIGHT_GREEN)
    else:
        set_chinese_font(run, "宋体", 12, bold=bold)
    return p

def add_code(doc, code):
    p = doc.add_paragraph()
    run = p.add_run(code)
    set_chinese_font(run, "Consolas", 10)
    p.paragraph_format.left_indent = Inches(0.5)

def main():
    doc = Document()

    style = doc.styles['Normal']
    style.font.name = '宋体'
    style._element.rPr.rFonts.set(qn('w:eastAsia'), '宋体')
    style.font.size = Pt(12)

    add_heading(doc, "学生宿舍管理信息系统 V2.0", level=0)

    add_para(doc, "")
    p = doc.add_paragraph()
    r = p.add_run("【V2.0 升级说明 - 蓝色高亮为新增/升级特性】")
    set_chinese_font(r, "黑体", 14, bold=True, color=RGBColor(0, 0, 255))

    add_heading(doc, "1. 项目概述", level=1)
    add_para(doc, "学生宿舍管理信息系统 V2.0 是一个用 C99 标准编写的控制台应用程序，用于管理学生宿舍的入住、退房、缴费等信息。")
    add_para(doc, "")
    add_para(doc, "V2.0 新特性：", bold=True, blue_highlight=True)
    features = [
        "UTF-8 BOM 编码支持 + Windows 控制台 UTF-8 代码页设置 (SetConsoleOutputCP(65001))",
        "ANSI 彩色终端输出，支持错误/成功/信息不同颜色显示",
        "动态数组存储（宿舍 + 学生）替代原链表结构，自动扩容",
        "统一错误码枚举 DormStatus，包含 13 种状态码",
        "Magic header \"DORMV2\" 二进制文件持久化，带版本号校验",
        "CLI 命令行参数支持：-h/--help 帮助，-t/--test 自测",
        "内置 27 项自测（CRUD + 入住/退房 + 缴费 + 统计 + 文件 I/O）",
        "交互菜单模式，支持 8 项主要功能",
        "完整中文注释",
        "零警告编译（gcc -std=c99 -Wall -Wextra -O2）"
    ]
    for i, f in enumerate(features, 1):
        add_para(doc, f"  {i}. {f}", blue_highlight=True)

    add_heading(doc, "2. 编译方法", level=1)
    add_para(doc, "使用指定编译器编译：")
    add_code(doc, "D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o dorm.exe main.c")
    add_para(doc, "")
    add_para(doc, "或使用 Makefile：")
    add_code(doc, "make          # 编译\nmake test     # 编译并运行自测\nmake clean    # 清理")

    add_heading(doc, "3. 使用方法", level=1)
    add_para(doc, "命令行参数：", bold=True)
    add_code(doc, "dorm.exe -h       # 显示帮助\ndorm.exe --help   # 显示帮助\ndorm.exe -t       # 运行内置自测\ndorm.exe --test   # 运行内置自测\ndorm.exe          # 启动交互菜单模式")

    add_heading(doc, "4. 功能菜单", level=1)
    menu_items = [
        "1. 查看宿舍列表",
        "2. 添加宿舍",
        "3. 删除宿舍",
        "4. 查看学生列表",
        "5. 学生入住",
        "6. 学生退房",
        "7. 学生缴费",
        "8. 统计信息",
        "0. 退出系统"
    ]
    for item in menu_items:
        add_para(doc, f"  {item}")

    add_heading(doc, "5. 错误码说明", level=1)
    add_para(doc, "DormStatus 统一错误码枚举：", blue_highlight=True)
    error_codes = [
        ("DORM_OK", 0, "操作成功"),
        ("DORM_ERR_MEMORY", 1, "内存分配失败"),
        ("DORM_ERR_NOT_FOUND", 2, "未找到记录"),
        ("DORM_ERR_DUPLICATE", 3, "重复记录"),
        ("DORM_ERR_FULL", 4, "床位已满/无法删除"),
        ("DORM_ERR_EMPTY", 5, "空状态"),
        ("DORM_ERR_FILE_IO", 6, "文件读写错误"),
        ("DORM_ERR_INVALID_PARAM", 7, "无效参数"),
        ("DORM_ERR_ALREADY_CHECKED_IN", 8, "已入住"),
        ("DORM_ERR_NOT_CHECKED_IN", 9, "未入住"),
        ("DORM_ERR_ALREADY_PAID", 10, "已缴费"),
        ("DORM_ERR_MAGIC_MISMATCH", 11, "Magic header不匹配"),
        ("DORM_ERR_VERSION", 12, "版本不兼容")
    ]
    for name, code, desc in error_codes:
        add_para(doc, f"  {name} ({code}) - {desc}", blue_highlight=True)

    add_heading(doc, "6. 文件格式", level=1)
    add_para(doc, "V2.0 使用二进制文件 dormv2.dat 持久化数据，格式如下：", blue_highlight=True)
    add_code(doc, "[0-5]   Magic header: \"DORMV2\" (6 bytes)\n[6-9]   Version: 2 (int)\n[10-13] Room count: n (int)\n[14..]  Room array: n * sizeof(Room)\n[...]   Student count: m (int)\n[...]   Student array: m * sizeof(Student)")
    add_para(doc, "")
    add_para(doc, "数据结构：", bold=True)
    add_code(doc, "Room: id[64] + building[64] + floor(int) + capacity(int)\n      + occupied(int) + type[64]\n\nStudent: stu_id[64] + name[32] + room_id[64] + phone[64]\n         + check_in[64] + check_out[64] + fee(double)\n         + fee_paid(int)")

    add_heading(doc, "7. 自测说明", level=1)
    add_para(doc, "内置 27 项测试覆盖：", bold=True, blue_highlight=True)
    test_cases = [
        "T01-T02: 初始状态验证",
        "T03-T05: 添加宿舍/重复添加",
        "T06-T07: 查找宿舍（存在/不存在）",
        "T08-T10: 学生入住/重复入住/不存在宿舍",
        "T11-T12: 入住人数/满员验证",
        "T13-T14: 学生缴费/重复缴费",
        "T15-T17: 学生退房/重复退房/不存在学生",
        "T18: 删除有人宿舍失败",
        "T19-T22: 文件保存/加载/数据一致性",
        "T23-T24: 统计信息验证",
        "T25: 删除空宿舍成功",
        "T26: 错误 Magic header 验证",
        "T27: 无效参数验证"
    ]
    for t in test_cases:
        add_para(doc, f"  {t}", blue_highlight=True)

    add_heading(doc, "8. V1 备份说明", level=1)
    add_para(doc, "此文档同时作为 V1 版本备份文档，原有链表实现代码已完全重构为动态数组实现。")
    add_para(doc, "")
    add_para(doc, "编译器：D:/bin/gcc.exe", bold=True)
    add_para(doc, "输出文件名：dorm.exe", bold=True)
    add_para(doc, "编译标准：C99", bold=True)
    add_para(doc, "警告级别：-Wall -Wextra -O2，零警告通过", bold=True, blue_highlight=True)

    out_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "_V1_backup.docx")
    doc.save(out_path)
    print(f"文档已生成: {out_path}")
    return 0

if __name__ == "__main__":
    exit(main())
