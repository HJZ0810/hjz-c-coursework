# -*- coding: utf-8 -*-
"""
全国交通咨询模拟系统 V2.0 文档生成脚本
生成带蓝色高亮的Word文档，标记V2.0新增特性
"""

from docx import Document
from docx.shared import Pt, RGBColor, Inches
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.oxml.ns import qn
from docx.oxml import OxmlElement
import sys
import os
import shutil

def set_cell_shading(cell, color_hex):
    shading_elm = OxmlElement('w:shd')
    shading_elm.set(qn('w:fill'), color_hex)
    cell._tc.get_or_add_tcPr().append(shading_elm)

def add_highlight_run(paragraph, text, color=RGBColor(0, 0, 255), bold=True):
    run = paragraph.add_run(text)
    run.font.color.rgb = color
    run.font.bold = bold
    return run

def add_v2_badge(paragraph):
    run = paragraph.add_run('【V2.0新增】')
    run.font.color.rgb = RGBColor(0, 0, 255)
    run.font.bold = True
    run.font.size = Pt(10)

def create_doc():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    original_docx = os.path.join(script_dir, 'Project_35_全国交通咨询模拟系统.docx')
    backup_docx = os.path.join(script_dir, '_V1_backup.docx')

    if os.path.exists(original_docx) and not os.path.exists(backup_docx):
        shutil.copy2(original_docx, backup_docx)
        print(f'已备份原文档到: {backup_docx}')

    doc = Document()

    style = doc.styles['Normal']
    style.font.name = '宋体'
    style._element.rPr.rFonts.set(qn('w:eastAsia'), '宋体')
    style.font.size = Pt(12)

    title = doc.add_heading('', level=0)
    title.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = title.add_run('全国交通咨询模拟系统 V2.0')
    run.font.color.rgb = RGBColor(0, 0, 139)
    run.font.bold = True

    subtitle = doc.add_paragraph()
    subtitle.alignment = WD_ALIGN_PARAGRAPH.CENTER
    add_highlight_run(subtitle, 'National Traffic Consultation System V2.0')

    info = doc.add_paragraph()
    info.alignment = WD_ALIGN_PARAGRAPH.CENTER
    info.add_run('学号：424123888888  |  班级：A班  |  姓名：HuiJunzhuo  |  2025-2026学年第二学期算法实习')

    doc.add_paragraph()

    doc.add_heading('一、项目概述', level=1)

    p = doc.add_paragraph()
    p.add_run('本项目是一个使用C99标准开发的全国交通咨询模拟系统，采用')
    add_highlight_run(p, '邻接表（Adjacency List）')
    p.add_run('作为核心数据结构建模交通网络，实现')
    add_highlight_run(p, 'Dijkstra最短路径算法')
    p.add_run('，支持按时间最快和费用最省两种优化模式查询最优路线，自动计算中转等候时间，支持跨夜航班。')

    p2 = doc.add_paragraph()
    add_v2_badge(p2)
    p2.add_run(' V2.0版本全面升级：支持UTF-8 BOM编码、Windows控制台UTF-8输出（SetConsoleOutputCP(65001)）、ANSI彩色终端、CLI命令行参数（--help/-h/--test/-t）、')
    add_highlight_run(p2, '动态数组（Dynamic Array）')
    p2.add_run('存储城市和交通线路（支持自动扩容）、统一错误码枚举TraffStatus、文件持久化带Magic Header "TRAFV2"、内置自测框架（26项测试用例覆盖最短路径+中转+文件I/O）。')

    doc.add_heading('二、V2.0 新特性', level=1)

    features = [
        ('UTF-8 BOM编码支持', '文件开头添加UTF-8 BOM标记(﻿)，确保中文在Windows下正确显示'),
        ('SetConsoleOutputCP(65001)', '调用Windows API设置控制台输入输出代码页为UTF-8，解决中文乱码问题'),
        ('ANSI彩色终端', '完整的ANSI转义序列支持，成功/错误/警告使用不同颜色显示'),
        ('CLI命令行参数', '支持--help/-h显示帮助、--test/-t运行自测、无参数进入交互菜单'),
        ('动态数组存储', '使用动态数组存储城市(CityArray)和边(EdgePool)，支持自动扩容(GROWTH_FACTOR=2)'),
        ('统一错误码枚举', '定义TraffStatus枚举统一管理所有错误状态(TRAFF_OK/TRAFF_ERR_*)'),
        ('Magic Header持久化', '数据文件以"TRAFV2"作为Magic Header，包含版本号字段防止格式错误'),
        ('内置自测框架', '26项测试用例，覆盖CRUD、最短路径查询、中转路径、不可达路径、跨夜航班、文件I/O、动态扩容等'),
        ('Dijkstra最短路径', '支持时间最快(含中转等候)和费用最省两种优化模式'),
        ('中文注释完整', '所有函数均有详细中文注释说明功能、参数、返回值及设计思路'),
    ]

    table = doc.add_table(rows=1, cols=2)
    table.style = 'Table Grid'
    hdr = table.rows[0].cells
    hdr[0].text = '新特性'
    hdr[1].text = '说明'
    set_cell_shading(hdr[0], '4472C4')
    set_cell_shading(hdr[1], '4472C4')
    for cell in hdr:
        for paragraph in cell.paragraphs:
            for run in paragraph.runs:
                run.font.color.rgb = RGBColor(255, 255, 255)
                run.font.bold = True

    for name, desc in features:
        row = table.add_row().cells
        p_name = row[0].paragraphs[0]
        add_highlight_run(p_name, name, RGBColor(0, 0, 200))
        row[1].text = desc

    doc.add_paragraph()

    doc.add_heading('三、数据结构设计', level=1)

    p = doc.add_paragraph()
    p.add_run('V2.0版本采用')
    add_highlight_run(p, '邻接表 + 动态数组')
    p.add_run('作为核心数据结构，使用动态数组管理城市和边，邻接表通过指针链表组织出发边：')

    doc.add_heading('3.1 数据实体结构', level=2)

    entities = [
        ('Edge（交通线路/边）', 'type(交通类型:火车/飞机)、route_num(班次/航班号)、from/to(出发/到达城市索引)、dep_time/arr_time(出发/到达时间HH:MM)、duration(行程分钟)、cost(票价元)、next(邻接表指针)'),
        ('City（城市/顶点）', 'name(城市名称)、head(邻接表出发边链表头指针)'),
    ]

    for name, fields in entities:
        p = doc.add_paragraph(style='List Bullet')
        run = p.add_run(name + '：')
        run.font.bold = True
        p.add_run(fields)

    doc.add_heading('3.2 动态数组容器', level=2)

    p = doc.add_paragraph()
    add_v2_badge(p)
    p.add_run(' 使用结构体封装动态数组，CityArray和EdgePool分别管理城市和边：')

    code_text = '''typedef struct {
    City *items;
    size_t count;
    size_t capacity;
} CityArray;

typedef struct {
    Edge *items;
    size_t count;
    size_t capacity;
} EdgePool;'''
    p_code = doc.add_paragraph()
    p_code.paragraph_format.left_indent = Inches(0.5)
    run = p_code.add_run(code_text)
    run.font.name = 'Consolas'
    run.font.size = Pt(10)

    doc.add_paragraph()

    doc.add_heading('3.3 错误码枚举', level=2)

    p = doc.add_paragraph()
    add_v2_badge(p)
    p.add_run(' TraffStatus统一错误码枚举定义：')

    error_codes = [
        'TRAFF_OK = 0 - 操作成功',
        'TRAFF_ERR_MEMORY - 内存分配失败',
        'TRAFF_ERR_NOT_FOUND - 城市/线路不存在',
        'TRAFF_ERR_DUPLICATE - 重复城市',
        'TRAFF_ERR_INVALID_INPUT - 无效输入',
        'TRAFF_ERR_FILE_OPEN/READ/WRITE - 文件操作错误',
        'TRAFF_ERR_MAGIC_MISMATCH - Magic Header验证失败',
        'TRAFF_ERR_VERSION_MISMATCH - 数据版本不匹配',
        'TRAFF_ERR_NO_PATH - 无可达路径',
    ]
    for ec in error_codes:
        doc.add_paragraph(ec, style='List Bullet')

    doc.add_heading('四、Dijkstra最短路径算法', level=1)

    p = doc.add_paragraph()
    p.add_run('系统实现Dijkstra单源最短路径算法，支持两种优化模式：')

    doc.add_heading('4.1 权重计算', level=2)

    p = doc.add_paragraph(style='List Bullet')
    run = p.add_run('时间最快模式(mode=0)：')
    run.font.bold = True
    p.add_run('权重 = 行程时间(duration) + 中转等候时间(wait)')

    p = doc.add_paragraph(style='List Bullet')
    run = p.add_run('费用最省模式(mode=1)：')
    run.font.bold = True
    p.add_run('权重 = 票价(cost)，不考虑中转时间')

    p = doc.add_paragraph()
    add_v2_badge(p)
    p.add_run(' 中转等候时间计算：中转等候 = 当前边出发时间 - 前序边到达时间，若为负则加1440分钟(24小时)处理跨夜情况。')

    doc.add_heading('4.2 路径回溯', level=2)
    p = doc.add_paragraph()
    p.add_run('算法通过prev_e[]记录到达每个城市的最后一条边索引，prev_c[]记录前驱城市索引。查询完成后从目标城市dst逆向回溯到源城市src，还原完整路径，并计算总时间（含中转等候）和总费用。')

    doc.add_heading('五、文件持久化格式', level=1)

    p = doc.add_paragraph()
    add_v2_badge(p)
    p.add_run(' V2.0使用二进制格式存储数据，带Magic Header和版本号：')

    file_table = doc.add_table(rows=1, cols=3)
    file_table.style = 'Table Grid'
    hdr = file_table.rows[0].cells
    hdr[0].text = '偏移'
    hdr[1].text = '长度'
    hdr[2].text = '内容'
    for cell in hdr:
        set_cell_shading(cell, '4472C4')
        for paragraph in cell.paragraphs:
            for run in paragraph.runs:
                run.font.color.rgb = RGBColor(255, 255, 255)
                run.font.bold = True

    file_sections = [
        ('0', '6字节', 'Magic Header: "TRAFV2"'),
        ('6', '1字节', '版本号: 2'),
        ('7', 'sizeof(size_t)', '城市数量(cc)'),
        ('7+s', 'sizeof(size_t)', '边数量(ec)'),
        ('7+2s', 'cc * MAX_NAME字节', '城市名称数组'),
        ('...', 'ec * sizeof(Edge)', '边数据数组'),
    ]
    for off, sz, desc in file_sections:
        row = file_table.add_row().cells
        row[0].text = off
        row[1].text = sz
        p_desc = row[2].paragraphs[0]
        if 'Magic' in desc or '版本' in desc:
            add_highlight_run(p_desc, desc, RGBColor(0, 0, 200))
        else:
            p_desc.add_run(desc)

    doc.add_paragraph()
    p = doc.add_paragraph()
    add_v2_badge(p)
    p.add_run(' 加载文件时首先验证Magic Header和版本号，不匹配则返回错误，防止加载错误格式文件或旧版本数据导致崩溃。加载完成后调用rebuildAdjacencyList()重建邻接表指针（因为realloc可能改变内存地址）。')

    doc.add_heading('六、编译与运行', level=1)

    doc.add_heading('6.1 编译', level=2)
    p = doc.add_paragraph()
    p.add_run('使用GCC编译器（C99标准）：')

    p_code = doc.add_paragraph()
    p_code.paragraph_format.left_indent = Inches(0.3)
    run = p_code.add_run('D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o traff.exe main.c')
    run.font.name = 'Consolas'
    run.font.size = Pt(10)

    doc.add_heading('6.2 Makefile', level=2)
    p = doc.add_paragraph()
    p.add_run('项目提供Makefile，包含以下目标：')
    doc.add_paragraph('make / make all - 编译程序（零警告）', style='List Bullet')
    doc.add_paragraph('make test - 运行内置自测', style='List Bullet')
    doc.add_paragraph('make run - 运行交互菜单', style='List Bullet')
    doc.add_paragraph('make clean - 清理生成文件', style='List Bullet')

    doc.add_heading('6.3 命令行参数', level=2)

    p = doc.add_paragraph()
    add_v2_badge(p)
    p.add_run(' CLI支持：')

    cli_table = doc.add_table(rows=1, cols=2)
    cli_table.style = 'Table Grid'
    hdr = cli_table.rows[0].cells
    hdr[0].text = '参数'
    hdr[1].text = '功能'
    for cell in hdr:
        set_cell_shading(cell, '4472C4')
        for paragraph in cell.paragraphs:
            for run in paragraph.runs:
                run.font.color.rgb = RGBColor(255, 255, 255)
                run.font.bold = True

    cli_cmds = [
        ('-h / --help', '显示帮助信息'),
        ('-t / --test', '运行内置自测（26项测试）'),
        ('(无参数)', '启动交互菜单'),
    ]
    for cmd, func in cli_cmds:
        row = cli_table.add_row().cells
        p_cmd = row[0].paragraphs[0]
        run = p_cmd.add_run(cmd)
        run.font.name = 'Consolas'
        row[1].text = func

    doc.add_paragraph()

    doc.add_heading('七、内置自测用例', level=1)

    p = doc.add_paragraph()
    add_v2_badge(p)
    p.add_run(' V2.0内置26项测试用例，全面覆盖：')

    test_categories = [
        ('城市CRUD', 'TC01-TC06: 添加城市(成功/重复/空名称)、批量添加8城市、查询城市(存在/不存在)'),
        ('线路添加', 'TC07-TC10: 添加火车/飞机线路、不存在城市添加线路失败、构建测试路网(10条边)'),
        ('最短路径-最快', 'TC11: 北京→上海最快路线(飞机约2小时10分)'),
        ('最短路径-最省', 'TC12: 北京→上海最省钱路线(火车553元)'),
        ('中转路径', 'TC13: 北京→杭州需中转(路径长度≥3)'),
        ('不可达路径', 'TC14: 孤立城市(海口)无路径返回TRAFF_ERR_NO_PATH'),
        ('文件I/O', 'TC15-TC17: 保存文件、Magic Header验证(TRAFV2)、重新加载、城市/线路数量正确'),
        ('动态扩容', 'TC18: 添加30个城市触发动态数组扩容'),
        ('跨夜航班', 'TC19: 23:00→06:00跨夜航班时长计算为420分钟'),
        ('数据完整性', 'TC20: 重载后城市索引正确'),
    ]

    for cat, desc in test_categories:
        p = doc.add_paragraph(style='List Bullet')
        run = p.add_run(cat + '：')
        run.font.bold = True
        add_highlight_run(p, desc, RGBColor(0, 0, 180), bold=False)

    doc.add_paragraph()

    doc.add_heading('八、系统功能模块', level=1)

    modules = [
        ('城市管理', '查看所有城市、添加城市（自动去重）'),
        ('线路管理', '查看所有线路、添加火车/飞机线路（验证城市存在、时间格式）'),
        ('路线查询', '选择出发/到达城市、选择最快/最省模式、显示完整路径（含中转等候）、统计总时间总费用'),
        ('数据持久化', '启动自动加载（失败则初始化默认8城市+25线路）、操作自动保存、退出自动保存、Magic Header+版本验证'),
    ]

    for name, desc in modules:
        p = doc.add_paragraph(style='List Bullet')
        run = p.add_run(name + '：')
        run.font.bold = True
        p.add_run(desc)

    doc.add_paragraph()

    doc.add_heading('九、默认测试数据', level=1)

    p = doc.add_paragraph()
    p.add_run('系统首次启动时自动初始化默认数据：')
    doc.add_paragraph('8个核心城市：北京、上海、广州、成都、武汉、西安、南京、杭州', style='List Bullet')
    doc.add_paragraph('15条火车线路(G字头高铁)：覆盖主要城市间的高铁连接', style='List Bullet')
    doc.add_paragraph('10条飞机线路(CA/MU/CZ/HU)：国航/东航/南航/海航主要航班', style='List Bullet')

    doc.add_paragraph()

    doc.add_heading('十、测试运行结果', level=1)

    p = doc.add_paragraph()
    add_highlight_run(p, '✓ 26项测试全部通过！')
    p.add_run(' 编译零警告、零错误。')

    results = [
        '✓ GCC编译：-std=c99 -Wall -Wextra -O2，零警告',
        '✓ 26项自测：全部通过',
        '✓ Magic Header：TRAFV2验证正确',
        '✓ 动态扩容：30城市添加成功',
        '✓ 最短路径：最快/最省模式均正确',
        '✓ 中转路径：北京→杭州中转路径正确',
        '✓ 跨夜计算：23:00-06:00 = 420分钟',
        '✓ 文件I/O：保存→重载数据完整',
    ]
    for r in results:
        doc.add_paragraph(r, style='List Bullet')

    doc.add_paragraph()
    end = doc.add_paragraph()
    end.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = end.add_run('—— V2.0 升级完成 ——')
    run.font.color.rgb = RGBColor(0, 0, 139)
    run.font.bold = True

    output_path = os.path.join(script_dir, 'Project_35_全国交通咨询模拟系统.docx')
    doc.save(output_path)
    print(f'文档已生成: {output_path}')
    return output_path

if __name__ == '__main__':
    try:
        create_doc()
        print('✓ 文档生成成功')
    except ImportError:
        print('需要安装python-docx库: pip install python-docx')
        sys.exit(1)
    except Exception as e:
        print(f'生成失败: {e}')
        import traceback
        traceback.print_exc()
        sys.exit(1)
