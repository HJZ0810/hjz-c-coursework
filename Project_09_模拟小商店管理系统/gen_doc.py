# -*- coding: utf-8 -*-
"""
模拟小商店管理系统 V2.0 文档生成器
V2.0新增/修改内容用蓝色字体高亮标注
"""

from docx import Document
from docx.shared import Pt, Inches, Cm, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT, WD_ALIGN_VERTICAL
from docx.oxml.ns import qn, nsdecls
from docx.oxml import parse_xml
import os
import shutil

COLOR_V2 = RGBColor(0x00, 0x00, 0xFF)
COLOR_PRIMARY = RGBColor(0x1A, 0x56, 0xDB)
COLOR_DARK = RGBColor(0x1F, 0x29, 0x37)
COLOR_GRAY = RGBColor(0x6B, 0x72, 0x80)
COLOR_WHITE = RGBColor(0xFF, 0xFF, 0xFF)
COLOR_SUCCESS = RGBColor(0x21, 0x96, 0x53)
COLOR_DANGER = RGBColor(0xDC, 0x35, 0x45)

def set_cell_shading(cell, color_hex):
    shading = parse_xml(f'<w:shd {nsdecls("w")} w:fill="{color_hex}" w:val="clear"/>')
    cell._tc.get_or_add_tcPr().append(shading)

def add_styled_heading(doc, text, level=1, color=None, v2=False):
    heading = doc.add_heading(text, level=level)
    for run in heading.runs:
        run.font.name = '微软雅黑'
        run._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
        run.font.color.rgb = color if color else (COLOR_V2 if v2 else COLOR_PRIMARY)
    return heading

def add_para(doc, text, bold=False, size=11, color=None, indent_first=True, v2=False):
    p = doc.add_paragraph()
    run = p.add_run(text)
    run.font.name = '微软雅黑'
    run._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    run.font.size = Pt(size)
    run.bold = bold
    if v2:
        run.font.color.rgb = COLOR_V2
    elif color:
        run.font.color.rgb = color
    else:
        run.font.color.rgb = COLOR_DARK
    pf = p.paragraph_format
    pf.space_after = Pt(6)
    pf.line_spacing = 1.5
    if indent_first:
        pf.first_line_indent = Cm(0.74)
    return p

def add_v2_label(doc, text):
    p = doc.add_paragraph()
    run = p.add_run("【V2.0新增】" + text)
    run.font.name = '微软雅黑'
    run._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    run.font.size = Pt(11)
    run.bold = True
    run.font.color.rgb = COLOR_V2
    pf = p.paragraph_format
    pf.space_after = Pt(6)
    pf.line_spacing = 1.5
    return p

def add_bullet(doc, text, v2=False, bold_prefix=""):
    p = doc.add_paragraph(style='List Bullet')
    p.paragraph_format.left_indent = Cm(1.2)
    p.paragraph_format.space_after = Pt(3)
    p.paragraph_format.line_spacing = 1.4
    if bold_prefix:
        r1 = p.add_run(bold_prefix)
        r1.bold = True
        r1.font.name = '微软雅黑'
        r1._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
        r1.font.size = Pt(11)
        r1.font.color.rgb = COLOR_PRIMARY if not v2 else COLOR_V2
    r2 = p.add_run(text)
    r2.font.name = '微软雅黑'
    r2._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    r2.font.size = Pt(11)
    r2.font.color.rgb = COLOR_V2 if v2 else COLOR_DARK
    return p

def add_code_block(doc, code_text):
    table = doc.add_table(rows=1, cols=1)
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    cell = table.cell(0, 0)
    set_cell_shading(cell, "1E293B")
    p = cell.paragraphs[0]
    p.paragraph_format.space_before = Pt(4)
    p.paragraph_format.space_after = Pt(4)
    lines = code_text.split('\n')
    for i, line in enumerate(lines):
        if i > 0:
            p = cell.add_paragraph()
            p.paragraph_format.space_before = Pt(0)
            p.paragraph_format.space_after = Pt(0)
        run = p.add_run(line)
        run.font.name = 'Consolas'
        run.font.size = Pt(9.5)
        run.font.color.rgb = RGBColor(0xE2, 0xE8, 0xF0)
    doc.add_paragraph()

def add_table_with_header(doc, headers, rows, v2_rows=None):
    if v2_rows is None:
        v2_rows = set()
    table = doc.add_table(rows=1 + len(rows), cols=len(headers))
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    table.style = 'Table Grid'
    hdr = table.rows[0].cells
    for i, h in enumerate(headers):
        set_cell_shading(hdr[i], "1A56DB")
        p = hdr[i].paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r = p.add_run(h)
        r.bold = True
        r.font.name = '微软雅黑'
        r._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
        r.font.size = Pt(10.5)
        r.font.color.rgb = COLOR_WHITE
        hdr[i].vertical_alignment = WD_ALIGN_VERTICAL.CENTER
    for ri, row in enumerate(rows):
        cells = table.rows[ri + 1].cells
        bg = "F9FAFB" if ri % 2 == 0 else "FFFFFF"
        is_v2 = ri in v2_rows
        for ci, val in enumerate(row):
            set_cell_shading(cells[ci], bg)
            p = cells[ci].paragraphs[0]
            r = p.add_run(str(val))
            r.font.name = '微软雅黑'
            r._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
            r.font.size = Pt(10)
            r.font.color.rgb = COLOR_V2 if is_v2 else COLOR_DARK
            cells[ci].vertical_alignment = WD_ALIGN_VERTICAL.CENTER
    doc.add_paragraph()
    return table

def setup_styles(doc):
    style = doc.styles['Normal']
    style.font.name = '微软雅黑'
    style._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    style.font.size = Pt(11)
    style.font.color.rgb = COLOR_DARK
    style.paragraph_format.line_spacing = 1.5
    for lvl in range(1, 4):
        hs = doc.styles[f'Heading {lvl}']
        hs.font.name = '微软雅黑'
        hs._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
        hs.font.color.rgb = COLOR_PRIMARY
        if lvl == 1:
            hs.font.size = Pt(20)
        elif lvl == 2:
            hs.font.size = Pt(16)
        else:
            hs.font.size = Pt(13)
    for section in doc.sections:
        section.top_margin = Cm(2.5)
        section.bottom_margin = Cm(2.5)
        section.left_margin = Cm(2.8)
        section.right_margin = Cm(2.8)

def add_separator(doc):
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = p.add_run("━" * 60)
    r.font.color.rgb = COLOR_PRIMARY
    r.font.size = Pt(8)

def generate_doc():
    doc = Document()
    setup_styles(doc)

    for _ in range(4):
        doc.add_paragraph()
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = p.add_run("模拟小商店管理系统")
    r.bold = True
    r.font.name = '微软雅黑'
    r._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    r.font.size = Pt(32)
    r.font.color.rgb = COLOR_PRIMARY
    
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = p.add_run("V2.0 版本升级说明文档")
    r.font.name = '微软雅黑'
    r._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    r.font.size = Pt(16)
    r.font.color.rgb = COLOR_V2
    
    for _ in range(3):
        doc.add_paragraph()
    
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = p.add_run("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    r.font.color.rgb = COLOR_PRIMARY
    r.font.size = Pt(12)
    
    info_lines = [
        "项目：Project_09（模拟小商店管理系统）",
        "作者：HJZ（惠俊卓）",
        "文档类型：V2.0升级说明",
        "日期：2026年",
        "版本：V2.0"
    ]
    for line in info_lines:
        p = doc.add_paragraph()
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r = p.add_run(line)
        r.font.name = '微软雅黑'
        r._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
        r.font.size = Pt(12)
        r.font.color.rgb = COLOR_DARK
    
    doc.add_page_break()

    add_styled_heading(doc, "📑 文档目录", level=1)
    toc_items = [
        "第一章  版本概述",
        "第二章  V2.0新增功能（蓝色标注）",
        "第三章  V1.0原有功能",
        "第四章  系统架构与数据结构",
        "第五章  编译与使用说明",
        "第六章  文件格式说明",
        "第七章  内置自测用例",
        "附录A   错误码对照表",
        "附录B   CLI命令行参数",
    ]
    for item in toc_items:
        p = doc.add_paragraph()
        p.paragraph_format.left_indent = Cm(1)
        r = p.add_run(item)
        r.font.name = '微软雅黑'
        r._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
        r.font.size = Pt(12)
        r.font.color.rgb = COLOR_DARK
    doc.add_page_break()

    add_styled_heading(doc, "第一章  版本概述", level=1)
    add_separator(doc)
    
    add_styled_heading(doc, "1.1 版本信息", level=2)
    add_para(doc, "本文档描述模拟小商店管理系统从V1.0升级到V2.0的完整变更。V2.0参照Project_39（哈夫曼编译码器）的代码质量标准进行了全面重构和增强。")
    
    add_styled_heading(doc, "1.2 V1.0原有功能回顾", level=2)
    v1_features = [
        "商品管理：添加、删除、查询商品",
        "销售管理：商品销售、库存扣减",
        "进货管理：库存补充",
        "商品列表：显示所有商品信息",
        "销售统计：基本统计功能",
        "菜单交互：文本菜单界面",
        "文本文件存储：CSV格式数据持久化"
    ]
    for f in v1_features:
        add_bullet(doc, f)

    doc.add_page_break()

    add_styled_heading(doc, "第二章  V2.0新增功能（蓝色标注）", level=1, v2=True)
    add_separator(doc)
    
    add_v2_label(doc, "以下所有功能均为V2.0版本新增，使用蓝色字体标注")
    
    add_styled_heading(doc, "2.1 UTF-8 BOM 文件支持", level=2, v2=True)
    add_para(doc, "新增UTF-8 BOM检测功能，能够自动识别并跳过UTF-8编码文件开头的BOM标记（0xEF 0xBB 0xBF），确保中文内容正确读取。同时支持V1.0文本格式和V2.0二进制格式的自动识别与兼容加载。", v2=True)
    
    add_styled_heading(doc, "2.2 ANSI 彩色终端输出", level=2, v2=True)
    add_para(doc, "在Windows 10+系统上启用VT100转义序列支持，实现彩色终端输出：", v2=True)
    add_bullet(doc, "绿色 [OK]：操作成功提示", v2=True)
    add_bullet(doc, "红色 [ERR]：错误信息提示", v2=True)
    add_bullet(doc, "青色 === 标题 ===：章节标题", v2=True)
    add_code_block(doc, '''#ifdef _WIN32
static void enableVT100(void) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) return;
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, mode);
}
#endif''')

    add_styled_heading(doc, "2.3 CLI 命令行参数支持", level=2, v2=True)
    add_para(doc, "新增完整的命令行参数解析，支持以下选项：", v2=True)
    cli_data = [
        ["-h, --help", "显示帮助信息"],
        ["-t, --test", "运行内置自测用例"],
        ["-f <file>", "指定自定义数据文件路径"]
    ]
    add_table_with_header(doc, ["参数", "说明"], cli_data, v2_rows={0,1,2})

    add_styled_heading(doc, "2.4 动态内存管理（数组动态扩容）", level=2, v2=True)
    add_para(doc, "摒弃V1.0的固定大小限制，采用动态数组实现商品存储：", v2=True)
    add_bullet(doc, "初始容量：16个商品", v2=True)
    add_bullet(doc, "扩容策略：容量不足时自动翻倍（16→32→64→128...）", v2=True)
    add_bullet(doc, "内存释放：程序退出时自动释放所有动态分配内存，无内存泄漏", v2=True)
    add_code_block(doc, '''static ShopStatus dbEnsureCapacity(ShopDB *db, int needed) {
    if (db->count + needed <= db->capacity) return SHOP_OK;
    int newCap = db->capacity == 0 ? INIT_CAPACITY : db->capacity;
    while (newCap < db->count + needed) newCap *= 2;
    Goods *p = (Goods*)realloc(db->items, (size_t)newCap * sizeof(Goods));
    if (!p) return SHOP_ERR_MEMORY;
    db->items = p;
    db->capacity = newCap;
    return SHOP_OK;
}''')

    add_styled_heading(doc, "2.5 统一错误码枚举", level=2, v2=True)
    add_para(doc, "定义统一的ShopStatus错误码枚举，所有函数返回标准化的状态码：", v2=True)
    add_code_block(doc, '''typedef enum {
    SHOP_OK = 0,
    SHOP_ERR_OPEN_FILE,
    SHOP_ERR_READ_FILE,
    SHOP_ERR_WRITE_FILE,
    SHOP_ERR_MAGIC,
    SHOP_ERR_VERSION,
    SHOP_ERR_MEMORY,
    SHOP_ERR_NOT_FOUND,
    SHOP_ERR_DUPLICATE,
    SHOP_ERR_INVALID_INPUT
} ShopStatus;''')

    add_styled_heading(doc, "2.6 文件持久化带 Magic Header", level=2, v2=True)
    add_para(doc, "V2.0采用二进制文件格式存储，文件开头包含魔数标识\"SHOPV2\"和版本号，支持格式校验和未来版本兼容：", v2=True)
    add_bullet(doc, "魔数：\"SHOPV2\"（6字节）", v2=True)
    add_bullet(doc, "版本号：2（4字节int）", v2=True)
    add_bullet(doc, "商品数量：n（4字节int）", v2=True)
    add_bullet(doc, "商品数据：n个Goods结构体", v2=True)
    add_bullet(doc, "向后兼容：自动识别并加载V1.0的CSV文本格式", v2=True)
    
    add_styled_heading(doc, "2.7 内置自测用例", level=2, v2=True)
    add_para(doc, "内置32个自动化测试用例，覆盖：", v2=True)
    add_bullet(doc, "商品CRUD操作测试（添加、删除、查询）", v2=True)
    add_bullet(doc, "销售功能测试（正常销售、超库存拒绝）", v2=True)
    add_bullet(doc, "进货功能测试", v2=True)
    add_bullet(doc, "文件I/O测试（保存和加载）", v2=True)
    add_bullet(doc, "动态扩容测试（批量添加20个商品）", v2=True)
    add_bullet(doc, "边界条件测试（空名称、负价格、负库存）", v2=True)
    add_bullet(doc, "运行方式：shop.exe --test 或 shop.exe -t", v2=True)

    add_styled_heading(doc, "2.8 详细注释与代码规范", level=2, v2=True)
    add_para(doc, "代码遵循严格的C99标准，使用 -Wall -Wextra 编译选项实现零警告。所有函数和关键代码段都有清晰注释，代码结构模块化。", v2=True)

    doc.add_page_break()

    add_styled_heading(doc, "第三章  V1.0原有功能", level=1)
    add_separator(doc)
    
    add_styled_heading(doc, "3.1 商品管理", level=2)
    add_para(doc, "商品信息包含以下字段：")
    v1_data = [
        ["ID", "整数", "商品唯一标识，自动递增"],
        ["名称", "字符串", "商品名称，最长50字符"],
        ["单价", "浮点数", "商品单价"],
        ["库存", "整数", "当前库存数量"],
        ["销量", "整数", "累计销售数量"]
    ]
    add_table_with_header(doc, ["字段", "类型", "说明"], v1_data)
    
    add_styled_heading(doc, "3.2 功能菜单", level=2)
    menu_data = [
        ["1", "添加商品", "输入名称、单价、库存，自动分配ID"],
        ["2", "删除商品", "按ID删除商品"],
        ["3", "商品销售", "选择商品并输入销售数量，自动扣减库存、增加销量"],
        ["4", "商品进货", "选择商品并输入进货数量，增加库存"],
        ["5", "查询商品", "按ID查询商品详细信息"],
        ["6", "商品列表", "表格形式显示所有商品"],
        ["7", "销售统计", "显示总商品数、总库存、总销量、总销售额"],
        ["8", "按销售率排序", "按销售率从高到低排序显示商品"],
        ["0", "退出系统", "保存数据并退出"]
    ]
    add_table_with_header(doc, ["选项", "功能", "说明"], menu_data)

    doc.add_page_break()

    add_styled_heading(doc, "第四章  系统架构与数据结构", level=1)
    add_separator(doc)
    
    add_styled_heading(doc, "4.1 核心数据结构", level=2)
    
    add_para(doc, "Goods 商品结构体：", bold=True)
    add_code_block(doc, '''typedef struct {
    int   id;
    char  name[MAX_NAME_LEN];
    float price;
    int   stock;
    int   sold;
} Goods;''')
    
    add_para(doc, "ShopDB 数据库结构体（V2.0新增动态数组管理）：", bold=True, v2=True)
    add_code_block(doc, '''typedef struct {
    Goods  *items;
    int     count;
    int     capacity;
    char    filepath[256];
} ShopDB;''')
    
    add_styled_heading(doc, "4.2 核心函数清单", level=2)
    func_data = [
        ["dbInit", "初始化数据库", ""],
        ["dbFree", "释放数据库内存", "V2"],
        ["dbEnsureCapacity", "确保数组容量足够", "V2"],
        ["dbAdd", "添加商品", ""],
        ["dbDelete", "删除商品", ""],
        ["dbSell", "销售商品", ""],
        ["dbRestock", "进货", ""],
        ["dbFindById", "按ID查找商品", ""],
        ["dbSave", "保存到文件", "V2重写（二进制格式）"],
        ["dbLoad", "从文件加载", "V2重写（支持BOM、自动识别格式）"],
        ["runSelfTest", "运行自测", "V2新增"],
        ["interactiveLoop", "交互主循环", ""]
    ]
    add_table_with_header(doc, ["函数名", "功能", "版本"], func_data, v2_rows={1,2,8,9,10})

    doc.add_page_break()

    add_styled_heading(doc, "第五章  编译与使用说明", level=1)
    add_separator(doc)
    
    add_styled_heading(doc, "5.1 编译环境", level=2)
    add_bullet(doc, "编译器：D:/bin/gcc.exe (MinGW GCC)", v2=True)
    add_bullet(doc, "标准：C99", v2=True)
    add_bullet(doc, "编译选项：-std=c99 -Wall -Wextra -O2（零警告）", v2=True)
    
    add_styled_heading(doc, "5.2 Makefile构建（V2.0新增）", level=2, v2=True)
    add_para(doc, "V2.0提供Makefile支持简单构建：", v2=True)
    add_code_block(doc, '''CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = shop.exe
SRC = main.c

.PHONY: all clean test run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	-del /f /q $(TARGET) goods.dat test_shop.dat 2>nul

test: $(TARGET)
	.\\$(TARGET) --test

run: $(TARGET)
	.\\$(TARGET)''')
    
    add_para(doc, "编译命令：", bold=True)
    add_code_block(doc, "make          # 编译\nmake test     # 编译并运行测试\nmake run      # 编译并运行\nmake clean    # 清理")
    
    add_styled_heading(doc, "5.3 手动编译命令", level=2)
    add_code_block(doc, "D:\\bin\\gcc.exe -std=c99 -Wall -Wextra -O2 -o shop.exe main.c")
    
    add_styled_heading(doc, "5.4 运行方式", level=2)
    add_bullet(doc, "shop.exe              启动交互模式")
    add_bullet(doc, "shop.exe -h           显示帮助", v2=True)
    add_bullet(doc, "shop.exe -t           运行自测", v2=True)
    add_bullet(doc, "shop.exe -f mydata.dat 使用指定数据文件", v2=True)

    doc.add_page_break()

    add_styled_heading(doc, "第六章  文件格式说明", level=1)
    add_separator(doc)
    
    add_styled_heading(doc, "6.1 V2.0二进制格式", level=2, v2=True)
    add_para(doc, "V2.0采用二进制格式存储，结构如下：", v2=True)
    format_data = [
        ["0-5", "6字节", "魔数 \"SHOPV2\"", "文件标识"],
        ["6-9", "4字节", "版本号 = 2", "int类型"],
        ["10-13", "4字节", "商品数量 n", "int类型"],
        ["14+", "n*sizeof(Goods)", "商品数组", "连续存储n个Goods结构体"]
    ]
    add_table_with_header(doc, ["偏移", "长度", "内容", "说明"], format_data, v2_rows={0,1,2,3})
    
    add_styled_heading(doc, "6.2 V1.0文本格式（兼容读取）", level=2)
    add_para(doc, "V2.0可以自动读取V1.0的CSV文本格式：")
    add_code_block(doc, "商品名称,单价,库存,销量\\n可乐,3.50,100,0\\n面包,5.00,50,0")

    doc.add_page_break()

    add_styled_heading(doc, "第七章  内置自测用例", level=1, v2=True)
    add_separator(doc)
    
    add_para(doc, "V2.0内置32个测试用例，使用TEST_ASSERT宏进行断言：", v2=True)
    test_data = [
        ["1-3", "添加商品测试", "添加可乐、面包、矿泉水3个商品"],
        ["4-6", "ID自动分配测试", "验证ID=1,2,3"],
        ["7-9", "销售测试", "销售30瓶可乐，验证库存70、销量30"],
        ["10", "超库存销售测试", "验证销售100瓶可乐失败"],
        ["11-12", "进货测试", "进货面包20个，验证库存70"],
        ["13-14", "查找测试", "查找存在/不存在的ID"],
        ["15-17", "删除测试", "删除矿泉水，验证数量和顺序"],
        ["18", "删除不存在测试", "删除ID=999应失败"],
        ["19-24", "文件I/O测试", "保存、重新加载并验证数据一致性"],
        ["25-27", "边界条件测试", "空名称、负价格、负库存"],
        ["28-29", "动态扩容测试", "批量添加20个商品，验证自动扩容"],
        ["30-32", "结果统计", "通过/失败计数"]
    ]
    add_table_with_header(doc, ["测试编号", "测试类别", "测试内容"], test_data, v2_rows=set(range(12)))

    doc.add_page_break()

    add_styled_heading(doc, "附录A   错误码对照表", level=1, v2=True)
    add_separator(doc)
    
    error_data = [
        ["SHOP_OK (0)", "操作成功"],
        ["SHOP_ERR_OPEN_FILE (1)", "无法打开文件"],
        ["SHOP_ERR_READ_FILE (2)", "读取文件失败"],
        ["SHOP_ERR_WRITE_FILE (3)", "写入文件失败"],
        ["SHOP_ERR_MAGIC (4)", "无效的文件格式（magic不匹配）"],
        ["SHOP_ERR_VERSION (5)", "不支持的文件版本"],
        ["SHOP_ERR_MEMORY (6)", "内存分配失败"],
        ["SHOP_ERR_NOT_FOUND (7)", "商品未找到"],
        ["SHOP_ERR_DUPLICATE (8)", "商品ID重复"],
        ["SHOP_ERR_INVALID_INPUT (9)", "输入无效"]
    ]
    add_table_with_header(doc, ["错误码", "说明"], error_data, v2_rows=set(range(10)))

    doc.add_page_break()

    add_styled_heading(doc, "附录B   CLI命令行参数", level=1, v2=True)
    add_separator(doc)
    
    add_para(doc, "帮助信息输出（shop.exe -h）：", v2=True)
    add_code_block(doc, '''模拟小商店管理系统 V2.0
用法：
  shop.exe              启动交互模式
  shop.exe -h, --help   显示帮助
  shop.exe -t, --test   运行内置自测
  shop.exe -f <file>    指定数据文件（默认 goods.dat）''')

    doc.save("Project_09_模拟小商店管理系统.docx")
    print("文档生成成功：Project_09_模拟小商店管理系统.docx")

if __name__ == "__main__":
    generate_doc()
