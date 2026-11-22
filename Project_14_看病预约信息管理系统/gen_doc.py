#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
看病预约信息管理系统 V2.0 文档生成脚本
功能：生成/更新 docx 文档，将 V2.0 新特性用蓝色高亮标记
"""

import os
import sys
import shutil
from datetime import datetime

try:
    from docx import Document
    from docx.shared import Pt, RGBColor, Inches
    from docx.enum.text import WD_ALIGN_PARAGRAPH
    from docx.oxml.ns import qn
    from docx.oxml import OxmlElement
except ImportError:
    print("请先安装 python-docx: pip install python-docx")
    sys.exit(1)

PROJECT_DIR = os.path.dirname(os.path.abspath(__file__))
V1_BACKUP = os.path.join(PROJECT_DIR, "_V1_backup.docx")
V2_OUTPUT = os.path.join(PROJECT_DIR, "Project_14_看病预约信息管理系统.docx")

BLUE_COLOR = RGBColor(0x00, 0x70, 0xC0)
BLUE_HIGHLIGHT = "blue"


def set_cell_background(cell, color_hex):
    tc_pr = cell._tc.get_or_add_tcPr()
    shd = OxmlElement('w:shd')
    shd.set(qn('w:fill'), color_hex)
    tc_pr.append(shd)


def add_blue_text(paragraph, text, bold=False, size=12):
    run = paragraph.add_run(text)
    run.font.color.rgb = BLUE_COLOR
    run.font.name = '宋体'
    run._element.rPr.rFonts.set(qn('w:eastAsia'), '宋体')
    run.font.size = Pt(size)
    run.bold = bold
    return run


def add_normal_text(paragraph, text, bold=False, size=12):
    run = paragraph.add_run(text)
    run.font.name = '宋体'
    run._element.rPr.rFonts.set(qn('w:eastAsia'), '宋体')
    run.font.size = Pt(size)
    run.bold = bold
    return run


def add_heading(doc, text, level=1, blue=False):
    h = doc.add_heading(level=level)
    h.alignment = WD_ALIGN_PARAGRAPH.LEFT
    run = h.add_run(text)
    run.font.name = '黑体'
    run._element.rPr.rFonts.set(qn('w:eastAsia'), '黑体')
    if blue:
        run.font.color.rgb = BLUE_COLOR
    return h


def generate_v2_doc():
    doc = Document()

    style = doc.styles['Normal']
    style.font.name = '宋体'
    style._element.rPr.rFonts.set(qn('w:eastAsia'), '宋体')
    style.font.size = Pt(12)

    title = doc.add_paragraph()
    title.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = title.add_run('看病预约信息管理系统 V2.0')
    run.font.size = Pt(22)
    run.bold = True
    run.font.name = '黑体'
    run._element.rPr.rFonts.set(qn('w:eastAsia'), '黑体')
    run.font.color.rgb = BLUE_COLOR

    subtitle = doc.add_paragraph()
    subtitle.alignment = WD_ALIGN_PARAGRAPH.CENTER
    add_normal_text(subtitle, 'Medical Appointment Information Management System V2.0', size=12)

    doc.add_paragraph()

    info_p = doc.add_paragraph()
    info_p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    add_normal_text(info_p, '课程名称：数据结构    作者：HuiJunzhuo    班级：计算机科学与技术A班\n')
    add_normal_text(info_p, '学号：424123888888    学年学期：2025-2026学年第二学期')

    doc.add_paragraph()

    add_heading(doc, '一、V2.0 版本升级说明（蓝色为新特性）', level=1, blue=True)

    p = doc.add_paragraph()
    add_normal_text(p, '本系统在 V1.0 链表实现基础上，全面升级为 V2.0 版本，', size=12)
    add_blue_text(p, '新增以下核心特性：', bold=True, size=12)

    features = [
        ('UTF-8 BOM 编码 + SetConsoleOutputCP(65001)',
         '解决Windows中文控制台乱码问题，统一使用UTF-8编码输出'),
        ('ANSI 彩色终端输出',
         '成功(绿)/失败(红)/警告(黄)/信息(青)/菜单(紫) 多色区分，界面更友好'),
        ('CLI 命令行参数支持',
         '-h/--help 显示帮助；-t/--test 运行内置自测套件；无参数进入交互模式'),
        ('动态数组存储',
         '将V1.0链表替换为动态数组(DynArray)，支持自动扩容，内存访问更高效'),
        ('统一错误码枚举 HospStatus',
         'HOSP_OK/HOSP_ERR_MEMORY/HOSP_ERR_NOT_FOUND 等14种标准错误码'),
        ('Magic Header "HOSPV2" 二进制持久化',
         '二进制文件存储带魔数校验+版本号，防止加载错误文件'),
        ('内置自测框架',
         '18组测试用例、129+断言，覆盖动态数组/CRUD/边界条件/持久化全流程'),
        ('交互菜单升级',
         '预约/取消/查询/统计/保存 八大主菜单选项，操作更直观'),
    ]

    for feat_name, feat_desc in features:
        p = doc.add_paragraph(style='List Bullet')
        add_blue_text(p, '● ' + feat_name + '：', bold=True, size=11)
        add_normal_text(p, feat_desc, size=11)

    doc.add_paragraph()

    add_heading(doc, '二、系统架构', level=1)
    arch_items = [
        '控制台层：ANSI彩色输出 + UTF-8编码 + CLI参数解析',
        '交互菜单层：主菜单/患者管理/医生管理/预约管理/数据统计',
        '业务逻辑层：Patient/Doctor/Appointment CRUD + 状态校验',
        '容器层：DynArray动态数组（自动扩容2倍）',
        '持久化层："HOSPV2"魔数 + 版本号 + 二进制序列化',
        '测试层：18组单元测试 + TEST_ASSERT断言宏',
    ]
    for item in arch_items:
        doc.add_paragraph(item, style='List Bullet')

    doc.add_paragraph()

    add_heading(doc, '三、数据结构（动态数组版）', level=1)

    add_heading(doc, '3.1 动态数组 DynArray', level=2)
    t = doc.add_table(rows=5, cols=2)
    t.style = 'Table Grid'
    headers = ['字段', '说明']
    for i, h in enumerate(headers):
        cell = t.rows[0].cells[i]
        cell.text = h
        set_cell_background(cell, '0070C0')
        for paragraph in cell.paragraphs:
            for run in paragraph.runs:
                run.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)
                run.bold = True

    rows_data = [
        ('void **data', '元素指针数组（存储各元素拷贝）'),
        ('size_t size', '当前元素个数'),
        ('size_t capacity', '当前容量（自动扩容为2倍）'),
        ('size_t elemSize', '单个元素字节大小'),
    ]
    for i, (k, v) in enumerate(rows_data):
        t.rows[i + 1].cells[0].text = k
        t.rows[i + 1].cells[1].text = v

    doc.add_paragraph()

    add_heading(doc, '3.2 业务结构体', level=2)
    p = doc.add_paragraph()
    add_normal_text(p, 'Patient(患者): id/name/age/gender/job/addr\n', size=11)
    add_normal_text(p, 'Doctor(医生): id/name/specialty/dept/years\n', size=11)
    add_normal_text(p, 'Appointment(预约): id/patientId/doctorId/datetime/desc/note/status', size=11)

    doc.add_paragraph()

    add_heading(doc, '四、HospStatus 统一错误码', level=1)
    err_table = doc.add_table(rows=15, cols=2)
    err_table.style = 'Table Grid'
    err_headers = ['错误码', '含义']
    for i, h in enumerate(err_headers):
        cell = err_table.rows[0].cells[i]
        cell.text = h
        set_cell_background(cell, '0070C0')
        for paragraph in cell.paragraphs:
            for run in paragraph.runs:
                run.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)
                run.bold = True

    errors = [
        ('HOSP_OK (0)', '操作成功'),
        ('HOSP_ERR_MEMORY (-1)', '内存分配失败'),
        ('HOSP_ERR_NOT_FOUND (-2)', '记录未找到'),
        ('HOSP_ERR_DUPLICATE (-3)', '编号重复'),
        ('HOSP_ERR_INVALID_PARAM (-4)', '无效参数'),
        ('HOSP_ERR_FILE_OPEN (-5)', '无法打开文件'),
        ('HOSP_ERR_FILE_READ (-6)', '文件读取错误'),
        ('HOSP_ERR_FILE_WRITE (-7)', '文件写入错误'),
        ('HOSP_ERR_FILE_MAGIC (-8)', '文件魔数不匹配'),
        ('HOSP_ERR_FILE_VERSION (-9)', '文件版本不兼容'),
        ('HOSP_ERR_EMPTY (-10)', '数据为空'),
        ('HOSP_ERR_PATIENT_EXISTS (-11)', '患者已存在'),
        ('HOSP_ERR_DOCTOR_EXISTS (-12)', '医生已存在'),
        ('HOSP_ERR_INVALID_ID (-13)', '无效编号'),
    ]
    for i, (k, v) in enumerate(errors):
        err_table.rows[i + 1].cells[0].text = k
        err_table.rows[i + 1].cells[1].text = v

    doc.add_paragraph()

    add_heading(doc, '五、文件持久化格式（V2 二进制）', level=1)
    p = doc.add_paragraph()
    add_blue_text(p, 'Magic Header: ', bold=True)
    add_normal_text(p, '"HOSPV2" (6字节)，用于校验文件合法性\n')
    add_blue_text(p, 'Version: ', bold=True)
    add_normal_text(p, 'int (4字节)，值为 2\n')
    add_blue_text(p, 'Counts: ', bold=True)
    add_normal_text(p, '3个size_t，分别记录患者/医生/预约数量\n')
    add_blue_text(p, 'NextApptId: ', bold=True)
    add_normal_text(p, 'int，下一个预约号\n')
    add_blue_text(p, 'Data: ', bold=True)
    add_normal_text(p, '依次写入所有Patient、Doctor、Appointment结构体二进制数据')

    doc.add_paragraph()

    add_heading(doc, '六、编译与运行', level=1)
    p = doc.add_paragraph()
    add_blue_text(p, '编译（零警告）：', bold=True)
    add_normal_text(p, 'make 或 D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o hosp_v2.exe main.c\n')
    add_blue_text(p, '运行自测：', bold=True)
    add_normal_text(p, './hosp_v2.exe --test （或 make test）\n')
    add_blue_text(p, '显示帮助：', bold=True)
    add_normal_text(p, './hosp_v2.exe --help\n')
    add_blue_text(p, '交互模式：', bold=True)
    add_normal_text(p, './hosp_v2.exe （或 make run）')

    doc.add_paragraph()

    add_heading(doc, '七、自测用例清单 (18项)', level=1)
    tests = [
        '动态数组基础操作',
        '动态数组自动扩容',
        '系统初始化与释放',
        '患者添加',
        '患者编号重复检测',
        '患者查找',
        '患者删除',
        '医生添加',
        '医生编号重复检测',
        '医生查找',
        '医生删除',
        '有效预约创建',
        '无效患者预约拒绝',
        '无效医生预约拒绝',
        '预约取消',
        '预约查找',
        '文件持久化（保存+重载）',
        '错误码字符串',
    ]
    for t in tests:
        doc.add_paragraph(t, style='List Number')

    doc.add_paragraph()

    add_heading(doc, '八、Makefile 配置', level=1)
    p = doc.add_paragraph()
    add_normal_text(p, 'CC = D:/bin/gcc.exe\n')
    add_normal_text(p, 'CFLAGS = -std=c99 -Wall -Wextra -O2\n')
    add_normal_text(p, 'TARGET = hosp_v2.exe\n')
    add_normal_text(p, '\n')
    add_normal_text(p, '目标：all / test / run / clean')

    doc.add_paragraph()
    footer = doc.add_paragraph()
    footer.alignment = WD_ALIGN_PARAGRAPH.CENTER
    add_blue_text(footer, 'V2.0 升级完成 - 看病预约信息管理系统', bold=True, size=10)

    doc.save(V2_OUTPUT)
    print(f"[OK] 已生成V2.0文档: {V2_OUTPUT}")
    return True


def backup_v1():
    if not os.path.exists(V1_BACKUP):
        src = os.path.join(PROJECT_DIR, "Project_14_看病预约信息管理系统.docx")
        if os.path.exists(src):
            shutil.copy2(src, V1_BACKUP)
            print(f"[OK] 已备份V1文档为: {V1_BACKUP}")


def main():
    print("=" * 60)
    print("  看病预约系统 V2.0 文档生成工具")
    print("=" * 60)

    backup_v1()
    generate_v2_doc()

    print("\n[完成] 文档生成成功！")
    return 0


if __name__ == '__main__':
    sys.exit(main())
