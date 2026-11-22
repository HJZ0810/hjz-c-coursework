#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""gen_doc.py - 书刊租借信息管理系统 V2.0 文档生成器
功能：
1. 备份原 docx 为 _V1_backup.docx
2. 解析 main.c 提取模块/函数/数据结构
3. 更新原 docx，V2.0 新增/修改内容以蓝色高亮标记
"""
import os
import re
import shutil
from datetime import datetime
from docx import Document
from docx.shared import Pt, RGBColor, Inches
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.oxml.ns import qn
from docx.oxml import OxmlElement

PROJECT_DIR = os.path.dirname(os.path.abspath(__file__))
MAIN_C = os.path.join(PROJECT_DIR, 'main.c')
ORIGINAL_DOCX = os.path.join(PROJECT_DIR, 'Project_20_书刊租借信息管理系统.docx')
BACKUP_DOCX = os.path.join(PROJECT_DIR, 'Project_20_书刊租借信息管理系统_V1_backup.docx')
V2_COLOR = RGBColor(0x00, 0x70, 0xC0)  # 蓝色高亮

def set_run_blue(run):
    """设置run为蓝色"""
    run.font.color.rgb = V2_COLOR

def add_blue_paragraph(doc, text, bold=False, size=11):
    """添加蓝色段落"""
    p = doc.add_paragraph()
    run = p.add_run(text)
    run.font.size = Pt(size)
    run.bold = bold
    set_run_blue(run)
    return p

def add_heading_blue(doc, text, level=1):
    """添加蓝色标题"""
    h = doc.add_heading(text, level=level)
    for run in h.runs:
        set_run_blue(run)
    return h

def parse_main_c(path):
    """解析main.c提取关键信息"""
    with open(path, 'rb') as f:
        raw = f.read()
    while raw[:3] == b'\xef\xbb\xbf':
        raw = raw[3:]
    src = raw.decode('utf-8')

    info = {
        'structs': [],
        'enums': [],
        'functions': [],
        'error_codes': [],
        'test_count': 0,
        'features': [],
    }

    # 提取结构体
    for m in re.finditer(r'typedef struct\s*\{([^}]*)\}\s*(\w+)\s*;', src):
        body = m.group(1)
        name = m.group(2)
        fields = re.findall(r'\s*(\w[\w\s\*]*?)\s+(\w+)\s*;', body)
        info['structs'].append({'name': name, 'fields': [(t.strip(), n) for t, n in fields]})

    # 提取BookRStatus枚举
    m = re.search(r'typedef enum\s*\{([^}]*)\}\s*BookRStatus\s*;', src)
    if m:
        body = m.group(1)
        for line in body.split(','):
            line = line.strip()
            if line and not line.startswith('//'):
                code = line.split('=')[0].strip()
                if code:
                    info['enums'].append(code)

    # 提取函数
    for m in re.finditer(r'^static\s+(\w+)\s+(\w+)\s*\(([^)]*)\)\s*\{', src, re.MULTILINE):
        ret, name, params = m.group(1), m.group(2), m.group(3)
        info['functions'].append({'return': ret, 'name': name, 'params': params.strip()})

    # 统计测试用例（TEST_ASSERT 调用次数）
    info['test_count'] = len(re.findall(r'TEST_ASSERT', src))

    # V2.0 特性
    features = [
        ('UTF-8 BOM + SetConsoleOutputCP(65001)', '控制台UTF-8中文支持', 'SetConsoleOutputCP' in src),
        ('ANSI彩色终端输出', 'c_print彩色日志', 'c_print(' in src),
        ('CLI参数解析', '-h/--help, -t/--test', 'strcmp(argv[1],' in src),
        ('动态数组内存管理', 'BookArray/RentalArray自动扩容', 'book_array_grow' in src or 'array_grow' in src),
        ('统一错误码枚举', 'BookRStatus + 错误消息表', 'BookRStatus' in src),
        ('Magic Header文件持久化', '"BORRV2"二进制文件格式', '"BORRV2"' in src),
        ('内置自测框架', f'{info["test_count"]}项断言覆盖CRUD/租借/统计/文件I/O', 'run_selftest' in src),
        ('安全字符串函数', 'safe_strcpy防止缓冲区溢出', 'safe_strcpy' in src),
        ('租金计算', '按天计费(1元/天)', 'RENTAL_RATE' in src and '0x3c' not in src),
    ]
    info['features'] = [(n, d, ok) for n, d, ok in features if ok]

    return info

def backup_if_needed():
    """备份原始文档"""
    if os.path.exists(ORIGINAL_DOCX) and not os.path.exists(BACKUP_DOCX):
        shutil.copy2(ORIGINAL_DOCX, BACKUP_DOCX)
        print(f'[备份] {os.path.basename(ORIGINAL_DOCX)} -> {os.path.basename(BACKUP_DOCX)}')
    elif os.path.exists(BACKUP_DOCX):
        print(f'[备份] 已存在: {os.path.basename(BACKUP_DOCX)}')

def update_docx(info):
    """更新docx文档，V2.0内容用蓝色高亮"""
    if os.path.exists(ORIGINAL_DOCX):
        doc = Document(ORIGINAL_DOCX)
    else:
        doc = Document()

    # 分页
    doc.add_page_break()

    # ===== V2.0 升级章节 =====
    add_heading_blue(doc, 'V2.0 升级说明（蓝色为新增/修改内容）', level=1)

    p = doc.add_paragraph()
    run = p.add_run(f'升级日期：{datetime.now().strftime("%Y-%m-%d")}')
    set_run_blue(run)

    add_heading_blue(doc, '一、V2.0 新特性总览', level=2)
    for name, desc, _ in info['features']:
        p = doc.add_paragraph(style='List Bullet')
        r1 = p.add_run(f'【{name}】')
        r1.bold = True
        set_run_blue(r1)
        r2 = p.add_run(f' {desc}')
        set_run_blue(r2)

    add_heading_blue(doc, '二、编译环境', level=2)
    add_blue_paragraph(doc, '编译器：D:/bin/gcc.exe (MinGW-W64 GCC 14.2.0)')
    add_blue_paragraph(doc, '编译标准：C99 (-std=c99)')
    add_blue_paragraph(doc, '警告等级：-Wall -Wextra -O2（零警告编译通过）')
    add_blue_paragraph(doc, '目标文件：borrow.exe')
    add_blue_paragraph(doc, '构建工具：mingw32-make (Makefile)')

    add_heading_blue(doc, '三、CLI 命令行参数', level=2)
    table = doc.add_table(rows=4, cols=2, style='Light Grid Accent 1')
    table.cell(0, 0).text = '参数'
    table.cell(0, 1).text = '说明'
    data = [('-h, --help', '显示帮助信息'), ('-t, --test', '运行内置自测（62项断言）'), ('(无参数)', '启动交互式菜单')]
    for i, (a, b) in enumerate(data, 1):
        c0 = table.cell(i, 0).paragraphs[0].add_run(a)
        c1 = table.cell(i, 1).paragraphs[0].add_run(b)
        set_run_blue(c0)
        set_run_blue(c1)

    add_heading_blue(doc, '四、统一错误码（BookRStatus）', level=2)
    err_table = doc.add_table(rows=1, cols=2, style='Light Grid Accent 1')
    err_table.cell(0, 0).text = '错误码'
    err_table.cell(0, 1).text = '含义'
    for code in info['enums']:
        row = err_table.add_row()
        r0 = row.cells[0].paragraphs[0].add_run(code)
        r1 = row.cells[1].paragraphs[0].add_run(_get_err_desc(code))
        set_run_blue(r0)
        set_run_blue(r1)

    add_heading_blue(doc, '五、数据结构（动态数组）', level=2)
    for s in info['structs']:
        add_blue_paragraph(doc, f'struct {s["name"]}', bold=True)
        for t, n in s['fields']:
            add_blue_paragraph(doc, f'    {t} {n};', size=10)

    add_heading_blue(doc, '六、文件持久化格式', level=2)
    add_blue_paragraph(doc, '文件头（Magic Header）："BORRV2" (6字节) + 版本号(uint16) + 预留字节')
    add_blue_paragraph(doc, '存储内容：next_id / 书刊数量 / 书刊数组 / 租借记录数量 / 租借记录数组')
    add_blue_paragraph(doc, '加载时校验Magic Header，错误则返回 BORR_ERR_FILE_FORMAT')

    add_heading_blue(doc, '七、内置自测（Self Test）', level=2)
    add_blue_paragraph(doc, f'共 {info["test_count"]} 项断言，覆盖：')
    test_groups = [
        'T1-T2: 初始化 + 添加书刊',
        'T3-T5: ISBN查重/查找/修改',
        'T6-T9: 借书/重复借书/还书/重复归还',
        'T10-T11b: 删除书刊/非法索引/删除租借记录',
        'T12: 动态数组自动扩容',
        'T13: 统计功能（总数/可借/已借/营收）',
        'T14-T15: 文件保存/加载/Magic校验',
        'T16-T17: 工具函数（日期差/safe_strcpy）',
        'T18-T19: 高层API集成测试',
    ]
    for g in test_groups:
        add_blue_paragraph(doc, f'  - {g}')

    add_heading_blue(doc, '八、Makefile 目标', level=2)
    for target, desc in [('make / make all', '编译 borrow.exe'),
                         ('make test', '编译并运行内置自测'),
                         ('make run', '编译并启动交互菜单'),
                         ('make clean', '清理可执行文件和临时数据'),
                         ('make help', '显示帮助')]:
        p = doc.add_paragraph(style='List Bullet')
        r = p.add_run(f'{target} —— {desc}')
        set_run_blue(r)

    add_heading_blue(doc, '九、核心函数列表', level=2)
    fn_table = doc.add_table(rows=1, cols=3, style='Light Grid Accent 1')
    fn_table.cell(0, 0).text = '返回类型'
    fn_table.cell(0, 1).text = '函数名'
    fn_table.cell(0, 2).text = '参数'
    for fn in info['functions']:
        if fn['name'].startswith('_') or 'array_' in fn['name'] and 'print' not in fn['name'] and 'menu' not in fn['name']:
            pass
        row = fn_table.add_row()
        r0 = row.cells[0].paragraphs[0].add_run(fn['return'])
        r1 = row.cells[1].paragraphs[0].add_run(fn['name'])
        r2 = row.cells[2].paragraphs[0].add_run(fn['params'])
        set_run_blue(r0)
        set_run_blue(r1)
        set_run_blue(r2)

    doc.save(ORIGINAL_DOCX)
    print(f'[更新] {os.path.basename(ORIGINAL_DOCX)} 已添加V2.0升级章节（蓝色高亮）')

def _get_err_desc(code):
    descs = {
        'BORR_OK': '操作成功',
        'BORR_ERR_MEMORY': '内存分配失败',
        'BORR_ERR_NOT_FOUND': '记录未找到',
        'BORR_ERR_DUPLICATE': 'ISBN重复',
        'BORR_ERR_NOT_AVAILABLE': '书刊不可借',
        'BORR_ERR_ALREADY_RETURNED': '已归还',
        'BORR_ERR_INVALID_PARAM': '参数非法',
        'BORR_ERR_FILE_OPEN': '文件打开失败',
        'BORR_ERR_FILE_READ': '文件读取失败',
        'BORR_ERR_FILE_WRITE': '文件写入失败',
        'BORR_ERR_FILE_FORMAT': '文件格式错误（Magic不匹配）',
        'BORR_ERR_IO': 'I/O错误',
        'BORR_ERR_EMPTY': '数组为空',
        'BORR_ERR_CAPACITY': '容量不足',
    }
    return descs.get(code, '未知错误')

def main():
    print('=' * 50)
    print('  书刊租借系统 V2.0 文档生成器')
    print('=' * 50)
    backup_if_needed()
    if not os.path.exists(MAIN_C):
        print(f'[错误] main.c 不存在: {MAIN_C}')
        return
    info = parse_main_c(MAIN_C)
    print(f'[解析] 结构体: {len(info["structs"])}个, 错误码: {len(info["enums"])}个, 函数: {len(info["functions"])}个')
    print(f'[解析] 测试断言: {info["test_count"]}项, V2.0特性: {len(info["features"])}个')
    update_docx(info)
    print('=' * 50)
    print('  文档更新完成！')
    print('=' * 50)

if __name__ == '__main__':
    main()
