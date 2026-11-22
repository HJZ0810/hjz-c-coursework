# -*- coding: utf-8 -*-
"""
家庭理财管理信息系统 V2.0 文档生成脚本
运行: python gen_doc.py
功能: 生成 Project_25_理财管理信息系统.docx, V2.0 新特性用蓝色高亮标注
"""

import os
import sys

try:
    from docx import Document
    from docx.shared import Pt, RGBColor
    from docx.enum.text import WD_ALIGN_PARAGRAPH
    from docx.oxml.ns import qn
    from docx.oxml import OxmlElement
except ImportError:
    print("请先安装 python-docx: pip install python-docx")
    sys.exit(1)

BLUE = RGBColor(0x1E, 0x90, 0xFF)
DARK_BLUE = RGBColor(0x00, 0x33, 0x99)
BLACK = RGBColor(0, 0, 0)
GRAY = RGBColor(0x66, 0x66, 0x66)


def set_cell_shading(cell, color_hex):
    tc_pr = cell._tc.get_or_add_tcPr()
    shd = OxmlElement('w:shd')
    shd.set(qn('w:fill'), color_hex)
    tc_pr.append(shd)


def set_font(run, name='微软雅黑', size=11, color=BLACK, bold=False):
    run.font.name = name
    run._element.rPr.rFonts.set(qn('w:eastAsia'), name)
    run.font.size = Pt(size)
    run.font.color.rgb = color
    run.bold = bold


def add_highlight(p, text, size=11):
    r = p.add_run(text)
    set_font(r, '微软雅黑', size, BLUE, True)
    return r


def add_plain(p, text, size=11, bold=False, color=BLACK):
    r = p.add_run(text)
    set_font(r, '微软雅黑', size, color, bold)
    return r


def add_code(p, text):
    r = p.add_run(text)
    set_font(r, 'Consolas', 9, DARK_BLUE, False)
    return r


def add_heading(doc, text, level=1):
    h = doc.add_heading('', level=level)
    r = h.add_run(text)
    set_font(r, '微软雅黑', 18 if level == 1 else (14 if level == 2 else 12),
             DARK_BLUE if level != 2 else BLUE, True)
    return h


def build_doc():
    doc = Document()

    style = doc.styles['Normal']
    style.font.name = '微软雅黑'
    style.font.size = Pt(11)
    style.font.color.rgb = BLACK
    style._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')

    # 标题
    title = doc.add_heading('', level=0)
    title.alignment = WD_ALIGN_PARAGRAPH.CENTER
    tr = title.add_run('家庭理财管理信息系统')
    set_font(tr, '微软雅黑', 26, DARK_BLUE, True)

    sub = doc.add_paragraph()
    sub.alignment = WD_ALIGN_PARAGRAPH.CENTER
    add_highlight(sub, 'V2.0 升级版', 16)

    info = doc.add_paragraph()
    info.alignment = WD_ALIGN_PARAGRAPH.CENTER
    add_plain(info, '课程: C语言算法实习  |  学期: 2025-2026学年第二学期', 10, False, GRAY)

    doc.add_paragraph()

    # ======== V2.0 升级亮点 ========
    add_heading(doc, '一、V2.0 升级亮点', 1)

    features = [
        [('UTF-8 BOM 文件编码 + ', True), ('SetConsoleOutputCP(65001)', True),
         (' 原生中文支持, 彻底解决控制台乱码问题', False)],
        [('ANSI 彩色终端输出 (', False), ('VT100 虚拟终端序列', True),
         ('): 收入绿色、支出红色、标题青色、提示黄色', False)],
        [('CLI 命令行参数支持: ', False), ('-h/--help', True), (' 显示帮助、', False),
         ('-t/--test', True), (' 运行内置自测、无参数进入交互菜单', False)],
        [('动态内存管理: 从链表全面升级为', False),
         ('动态数组 (Dynamic Array)', True),
         (', AccountList + TransactionList, 自动扩容(倍增策略)', False)],
        [('统一错误码体系: ', False), ('enum FinStatus', True),
         (' (FIN_OK / FIN_ERR_MEMORY / FIN_ERR_FILE_* / FIN_ERR_NOT_FOUND 等10种状态)', False)],
        [('文件持久化: 单文件存储, ', False), ('Magic Header "FIN V2"', True),
         (' 文件头校验, 防止读取非法/旧版文件', False)],
        [('内置自测套件: ', False), ('26 项自动化测试', True),
         (' (>=15项要求), 覆盖 CRUD、收支统计、余额、文件I/O、错误码路径', False)],
        [('编译零警告: ', False), ('gcc -std=c99 -Wall -Wextra -O2', True),
         (' 严格编译通过, 零警告零错误', False)],
    ]
    for parts in features:
        p = doc.add_paragraph()
        for text, is_v2 in parts:
            if is_v2:
                add_highlight(p, text)
            else:
                add_plain(p, text)

    doc.add_paragraph()

    # ======== V1 vs V2 对比表 ========
    add_heading(doc, '二、V1.0 vs V2.0 对比', 1)

    table = doc.add_table(rows=1, cols=3)
    table.style = 'Light Grid Accent 1'
    hdr = table.rows[0].cells
    headers = ['对比项', 'V1.0 (旧版)', 'V2.0 (新版)']
    for i, text in enumerate(headers):
        hdr[i].text = ''
        p = hdr[i].paragraphs[0]
        add_plain(p, text, 10, True)
        set_cell_shading(hdr[i], 'D9E2F3')

    rows = [
        ('数据结构', '单向链表 (5个独立链表)', '动态数组 (Account + Transaction 双列表)'),
        ('文件存储', '5个独立 .dat 文件', '单文件 finance.dat, Magic Header "FIN V2"'),
        ('错误处理', '无统一机制, printf 提示', 'enum FinStatus 统一错误码, 10种状态'),
        ('终端显示', '纯文本, 无颜色', 'ANSI 彩色输出 (VT100)'),
        ('中文编码', 'GBK/ANSI, 乱码风险', 'UTF-8 BOM + SetConsoleOutputCP(65001)'),
        ('CLI 参数', '无', '-h/--help, -t/--test'),
        ('自测框架', '无', '26项内置自动化测试'),
        ('登录认证', '硬编码 admin/123456', '(简化, 聚焦核心理财功能)'),
        ('功能模块', '成员/分类/记录/储蓄/借贷', '账户/交易/收支统计/余额/文件持久化'),
        ('编译警告', '未严格控制', '-Wall -Wextra -O2 零警告'),
    ]
    for row_data in rows:
        cells = table.add_row().cells
        for i, text in enumerate(row_data):
            cells[i].text = ''
            p = cells[i].paragraphs[0]
            if i == 2:
                add_highlight(p, text, 10)
            else:
                add_plain(p, text, 10)

    doc.add_paragraph()

    # ======== 系统架构 ========
    add_heading(doc, '三、系统架构', 1)
    add_heading(doc, '3.1 核心数据结构', 2)

    structs = [
        ('Account (账户): ', 'id(int), name(char[64]), balance(double)'),
        ('Transaction (交易): ', 'id, account_id, type(收入/支出), amount, category, date, desc'),
        ('Database (数据库): ', 'AccountList + TransactionList + next_acc_id + next_tx_id'),
    ]
    for name, desc in structs:
        p = doc.add_paragraph()
        add_plain(p, name, 11, True, DARK_BLUE)
        add_plain(p, desc)

    p = doc.add_paragraph()
    add_plain(p, '动态数组自动扩容: ', 11, False)
    add_highlight(p, '初始容量4, 每次倍增(x2), 均摊O(1)追加')

    add_heading(doc, '3.2 错误码枚举 (FinStatus)', 2)
    code_p = doc.add_paragraph()
    add_code(code_p,
             'typedef enum {\n'
             '    FIN_OK = 0,\n'
             '    FIN_ERR_MEMORY,\n'
             '    FIN_ERR_FILE_OPEN,\n'
             '    FIN_ERR_FILE_READ,\n'
             '    FIN_ERR_FILE_WRITE,\n'
             '    FIN_ERR_MAGIC,\n'
             '    FIN_ERR_NOT_FOUND,\n'
             '    FIN_ERR_INVALID_PARAM,\n'
             '    FIN_ERR_DUPLICATE,\n'
             '    FIN_ERR_IO\n'
             '} FinStatus;')

    add_heading(doc, '3.3 文件格式', 2)
    file_parts = [
        ('文件头: ', False), ('"FIN V2" (6字节 Magic Header)', True),
        ('\n元数据: ', False), ('next_acc_id, next_tx_id, acc_cnt, tx_cnt', False),
        ('\n数据区: ', False), ('Account[] 数组 + Transaction[] 数组 (二进制连续存储)', False),
    ]
    p = doc.add_paragraph()
    for text, is_v2 in file_parts:
        if is_v2:
            add_highlight(p, text)
        else:
            add_plain(p, text)

    # ======== 功能说明 ========
    add_heading(doc, '四、功能说明', 1)
    functions = [
        ('账户管理', '添加/删除/查看账户, 初始余额设定, 删除前检查交易引用'),
        ('交易记录', '添加收入/支出, 自动更新余额, 删除时自动回滚余额'),
        ('收支统计', '总收入、总支出、净结余、账户总资产、交易笔数统计'),
        ('文件持久化', '启动自动加载、退出自动保存、操作实时保存, Magic校验'),
        ('彩色交互菜单', '8个主菜单选项, 彩色提示, 输入校验与错误提示'),
    ]
    for name, desc in functions:
        p = doc.add_paragraph(style='List Bullet')
        add_plain(p, name + ': ', 11, True, DARK_BLUE)
        add_plain(p, desc)

    # ======== 编译与运行 ========
    add_heading(doc, '五、编译与运行', 1)
    p = doc.add_paragraph()
    add_plain(p, '编译器: ', 11, True); add_highlight(p, 'D:/bin/gcc.exe (MinGW GCC)')
    p = doc.add_paragraph()
    add_plain(p, '编译选项: ', 11, True); add_highlight(p, '-std=c99 -Wall -Wextra -O2')
    p = doc.add_paragraph()
    add_plain(p, '目标文件: ', 11, True); add_highlight(p, 'finance.exe (非main.exe)')
    p = doc.add_paragraph()
    add_plain(p, '构建工具: ', 11, True); add_plain(p, 'Makefile (make / make test / make run / make clean)')

    add_heading(doc, '5.1 使用示例', 2)
    cmds = [
        ('make               ', '编译 finance.exe'),
        ('./finance.exe -h   ', '显示帮助信息'),
        ('./finance.exe -t   ', '运行 26 项内置自测'),
        ('./finance.exe      ', '启动交互式菜单'),
    ]
    for cmd, desc in cmds:
        p = doc.add_paragraph()
        r = p.add_run('  ' + cmd)
        set_font(r, 'Consolas', 10, DARK_BLUE, False)
        add_plain(p, '   # ' + desc, 10, False, GRAY)

    # ======== 自测清单 ========
    add_heading(doc, '六、内置自测清单 (26项)', 1)
    ttable = doc.add_table(rows=1, cols=2)
    ttable.style = 'Light Grid Accent 1'
    th = ttable.rows[0].cells
    for i, text in enumerate(['编号', '测试内容']):
        th[i].text = ''
        p = th[i].paragraphs[0]
        add_plain(p, text, 10, True)
        set_cell_shading(th[i], 'D9E2F3')

    tests = [
        ('T1-T3', '初始状态验证 (账户数=0、交易数=0、ID起始值=1)'),
        ('T4-T7', '账户 CRUD (添加2个账户、查询余额、数量校验)'),
        ('T8-T11', '交易 CRUD (收入+500、支出-200、余额联动更新)'),
        ('T12-T14', '收支统计 (总收入=500、总支出=200、净结余=300)'),
        ('T15', '删除交易余额回滚验证'),
        ('T16', '按类型统计交易笔数'),
        ('T17-T18', '账户更名 (update) 功能验证'),
        ('T19-T24', '文件I/O: 保存->加载->数据一致性校验 (Magic+count+余额)'),
        ('T25', '错误路径: 查询不存在账户返回 NULL'),
        ('T26', '错误路径: 有交易的账户禁止删除'),
    ]
    for tid, desc in tests:
        cells = ttable.add_row().cells
        cells[0].text = ''
        cells[1].text = ''
        add_highlight(cells[0].paragraphs[0], tid, 10)
        add_plain(cells[1].paragraphs[0], desc, 10)

    doc.add_paragraph()
    p = doc.add_paragraph()
    add_plain(p, '测试结果: ', 11, True)
    add_highlight(p, '26 PASS / 0 FAIL')
    add_plain(p, ', 全部通过。')

    doc.add_paragraph()
    foot = doc.add_paragraph()
    foot.alignment = WD_ALIGN_PARAGRAPH.CENTER
    add_plain(foot, '—— 家庭理财管理信息系统 V2.0 ——', 10, False, GRAY)

    out_path = 'Project_25_理财管理信息系统.docx'
    doc.save(out_path)
    print('文档已生成: ' + os.path.abspath(out_path))
    print('V2.0 升级内容已用蓝色 (#1E90FF) 高亮标注。')


if __name__ == '__main__':
    build_doc()
