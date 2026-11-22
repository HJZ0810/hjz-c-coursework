# -*- coding: utf-8 -*-
"""
机动车违章信息管理系统 V2.0 文档生成器
生成升级说明文档，新增内容使用蓝色高亮
"""

from docx import Document
from docx.shared import Pt, Cm, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT, WD_ALIGN_VERTICAL
from docx.oxml.ns import qn, nsdecls
from docx.oxml import parse_xml
import os

# ======================== 颜色主题 ========================
COLOR_PRIMARY   = RGBColor(0x1A, 0x56, 0xDB)   # 深蓝主色
COLOR_V2_BLUE   = RGBColor(0x25, 0x63, 0xEB)   # V2新增内容蓝色高亮
COLOR_SUCCESS   = RGBColor(0x21, 0x96, 0x53)   # 绿色成功
COLOR_DANGER    = RGBColor(0xDC, 0x35, 0x45)   # 红色错误
COLOR_DARK      = RGBColor(0x1F, 0x29, 0x37)   # 深色文本
COLOR_GRAY      = RGBColor(0x6B, 0x72, 0x80)   # 灰色辅助
COLOR_WHITE     = RGBColor(0xFF, 0xFF, 0xFF)

def set_cell_shading(cell, color_hex):
    shading = parse_xml(f'<w:shd {nsdecls("w")} w:fill="{color_hex}" w:val="clear"/>')
    cell._tc.get_or_add_tcPr().append(shading)

def add_styled_heading(doc, text, level=1, color=None, is_v2=False):
    heading = doc.add_heading(text, level=level)
    for run in heading.runs:
        run.font.name = '微软雅黑'
        run._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
        if is_v2:
            run.font.color.rgb = COLOR_V2_BLUE
        elif color:
            run.font.color.rgb = color
        else:
            run.font.color.rgb = COLOR_PRIMARY if level <= 2 else COLOR_DARK
    return heading

def add_para(doc, text, bold=False, color=None, is_v2=False, indent_first=True):
    p = doc.add_paragraph()
    run = p.add_run(text)
    run.font.name = '微软雅黑'
    run._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    run.font.size = Pt(11)
    run.bold = bold
    if is_v2:
        run.font.color.rgb = COLOR_V2_BLUE
        run.bold = True
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

def add_bullet(doc, text, is_v2=False, color=None):
    p = doc.add_paragraph(style='List Bullet')
    p.paragraph_format.left_indent = Cm(1.2)
    p.paragraph_format.space_after = Pt(3)
    run = p.add_run(text)
    run.font.name = '微软雅黑'
    run._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    run.font.size = Pt(11)
    if is_v2:
        run.font.color.rgb = COLOR_V2_BLUE
        run.bold = True
    elif color:
        run.font.color.rgb = color
    else:
        run.font.color.rgb = COLOR_DARK
    return p

def add_code_block(doc, code_text):
    table = doc.add_table(rows=1, cols=1)
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    cell = table.cell(0, 0)
    set_cell_shading(cell, "1E293B")
    p = cell.paragraphs[0]
    p.paragraph_format.space_before = Pt(4)
    p.paragraph_format.space_after = Pt(4)
    for i, line in enumerate(code_text.split('\n')):
        if i > 0:
            p = cell.add_paragraph()
            p.paragraph_format.space_before = Pt(0)
            p.paragraph_format.space_after = Pt(0)
        run = p.add_run(line)
        run.font.name = 'Consolas'
        run.font.size = Pt(9.5)
        run.font.color.rgb = RGBColor(0xE2, 0xE8, 0xF0)
    doc.add_paragraph()
    return table

def add_table_with_header(doc, headers, rows, col_widths=None, v2_rows=None):
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
        is_v2_row = ri in v2_rows
        for ci, val in enumerate(row):
            set_cell_shading(cells[ci], bg)
            p = cells[ci].paragraphs[0]
            r = p.add_run(str(val))
            r.font.name = '微软雅黑'
            r._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
            r.font.size = Pt(10)
            if is_v2_row:
                r.font.color.rgb = COLOR_V2_BLUE
                r.bold = True
            else:
                r.font.color.rgb = COLOR_DARK
            cells[ci].vertical_alignment = WD_ALIGN_VERTICAL.CENTER
    if col_widths:
        for row in table.rows:
            for i, w in enumerate(col_widths):
                row.cells[i].width = Cm(w)
    doc.add_paragraph()
    return table

def add_info_box(doc, title, content, box_type="info"):
    colors = {
        "v2":      ("DBEAFE", "1D4ED8", "🔷 V2.0 升级要点"),
        "info":    ("DBEAFE", "1E40AF", "ℹ️ 信息提示"),
        "success": ("D1FAE5", "065F46", "✅ 功能说明"),
    }
    bg, fg, icon = colors.get(box_type, colors["info"])
    if title:
        icon = title
    table = doc.add_table(rows=1, cols=1)
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    cell = table.cell(0, 0)
    set_cell_shading(cell, bg)
    p = cell.paragraphs[0]
    p.paragraph_format.space_after = Pt(2)
    r = p.add_run(icon)
    r.bold = True
    r.font.name = '微软雅黑'
    r._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    r.font.size = Pt(11)
    r.font.color.rgb = RGBColor(int(fg[:2],16), int(fg[2:4],16), int(fg[4:6],16))
    p2 = cell.add_paragraph()
    p2.paragraph_format.space_after = Pt(2)
    r2 = p2.add_run(content)
    r2.font.name = '微软雅黑'
    r2._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    r2.font.size = Pt(10.5)
    r2.font.color.rgb = COLOR_DARK
    doc.add_paragraph()

def add_cover_page(doc):
    for _ in range(5):
        doc.add_paragraph()
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = p.add_run("机动车违章信息管理系统")
    r.bold = True
    r.font.name = '微软雅黑'
    r._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    r.font.size = Pt(32)
    r.font.color.rgb = COLOR_PRIMARY

    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = p.add_run("V2.0 完全增强版 · 升级说明文档")
    r.font.name = '微软雅黑'
    r._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    r.font.size = Pt(18)
    r.font.color.rgb = COLOR_V2_BLUE
    r.bold = True

    for _ in range(4):
        doc.add_paragraph()

    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = p.add_run("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    r.font.color.rgb = COLOR_PRIMARY
    r.font.size = Pt(12)

    info_lines = [
        "项目：Project_11（机动车违章信息管理系统）",
        "作者：数据库男神（HuiJunzhuo）",
        "文档类型：V2.0 升级说明文档",
        "日期：2026年6月28日",
        "版本：V2.0（参照哈夫曼编译码器质量标准）",
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


def generate_doc():
    doc = Document()
    setup_styles(doc)
    add_cover_page(doc)

    # V2.0 升级概览
    add_styled_heading(doc, "📋 V2.0 升级概览", level=1, is_v2=True)
    add_info_box(doc, "🔷 V2.0 升级要点",
        "本版本为完全重写版本，参照Project_39哈夫曼编译码器的代码质量标准进行升级。"
        "新增内容在本文档中以蓝色加粗字体高亮显示。", "v2")

    add_para(doc, "本系统V2.0版本在V1.0的基础上进行了完全重写，主要升级点如下：", is_v2=False)
    v2_features = [
        ("UTF-8 BOM编码 + SetConsoleOutputCP(65001)", "彻底解决Windows中文控制台乱码问题"),
        ("ANSI彩色终端界面", "绿色[OK]、红色[ERROR]、青色标题、蓝色菜单，提升可读性"),
        ("命令行模式(CLI)", "支持--help/-h、--test/-t、-f <文件>、-o <输出>参数"),
        ("动态内存管理", "违章记录使用动态数组，容量翻倍策略自动扩容"),
        ("统一错误码枚举", "VioStatus枚举类型，所有函数返回标准错误码"),
        ("V2二进制文件格式", "magic header \"VIO V2\"(6字节)+版本号，数据完整性校验"),
        ("内置18+全面自测用例", "覆盖CRUD、查询、统计、文件I/O、错误处理全场景"),
        ("完善交互菜单系统", "三级菜单结构，输入安全验证，删除确认"),
        ("详尽Doxygen中文注释", "每个函数都有完整功能说明和使用注释"),
        ("统计功能增强", "违章次数统计、处理/未处理分类统计"),
    ]
    add_table_with_header(doc,
        ["升级特性", "说明"],
        v2_features,
        col_widths=[6, 10],
        v2_rows=set(range(len(v2_features)))
    )

    # 第一章：系统架构
    doc.add_page_break()
    add_styled_heading(doc, "第一章  系统架构与数据结构", level=1)

    add_styled_heading(doc, "1.1 数据结构升级", level=2)
    add_para(doc, "V1.0版本使用链表存储数据，V2.0版本改为动态数组存储，使用容量翻倍策略实现自动扩容，均摊时间复杂度O(1)。")
    add_info_box(doc, "🔷 V2.0 新数据结构",
        "使用宏DEFINE_DYN_ARRAY定义类型安全的通用动态数组模板，支持自动初始化、扩容、添加、释放。"
        "初始容量为8条记录，每次容量不足时翻倍扩容。", "v2")

    add_code_block(doc, """/* V2.0 动态数组核心宏定义 */
#define DEFINE_DYN_ARRAY(Type, Name) \\
typedef struct {
    Type *data;
    int count;
    int capacity;
} Name##Array;
static VioStatus Name##Array_init(Name##Array *arr);
static VioStatus Name##Array_add(Name##Array *arr, const Type *item);
static void Name##Array_free(Name##Array *arr);

DEFINE_DYN_ARRAY(Vehicle, Vehicle)
DEFINE_DYN_ARRAY(Driver, Driver)
DEFINE_DYN_ARRAY(Violation, Violation)""")

    add_styled_heading(doc, "1.2 VioHeader V2文件头", level=2)
    add_para(doc, "V2.0引入二进制文件格式，使用魔数和版本号确保数据完整性和兼容性：", is_v2=True)
    add_code_block(doc, """typedef struct {
    char magic[6];           // "VIO V2" 魔数标识
    unsigned short version;  // 版本号(当前=2)
    unsigned int vehicleCount;
    unsigned int driverCount;
    unsigned int violationCount;
} VioHeader;""")

    # 第二章：功能模块
    doc.add_page_break()
    add_styled_heading(doc, "第二章  功能模块说明", level=1)

    add_styled_heading(doc, "2.1 车辆信息管理", level=2)
    add_para(doc, "支持车辆信息的添加、查看、查询、删除操作，车牌号作为唯一标识。")
    vehicle_funcs = [
        ("addVehicle", "添加车辆，自动检测重复车牌"),
        ("deleteVehicle", "删除车辆，需用户确认"),
        ("findVehicleByPlate", "按车牌号精确查找"),
        ("listVehicles", "列出所有车辆信息"),
    ]
    add_table_with_header(doc, ["函数名", "功能说明"], vehicle_funcs, col_widths=[5, 11])

    add_styled_heading(doc, "2.2 驾驶员信息管理", level=2)
    add_para(doc, "支持驾驶员信息的添加、查看、查询、删除操作，身份证号作为唯一标识。")

    add_styled_heading(doc, "2.3 违章记录管理", level=2)
    add_para(doc, "支持违章记录的添加、查看、处理、删除、多条件查询，这是系统的核心功能。")
    add_para(doc, "V2.0新增违章筛选功能：可按全部/未处理/已处理三种状态筛选显示。", is_v2=True)

    add_styled_heading(doc, "2.4 查询与统计", level=2)
    add_para(doc, "V2.0增强了查询统计功能：", is_v2=True)
    add_bullet(doc, "按车牌号查询违章记录，显示该车辆违章次数统计", is_v2=True)
    add_bullet(doc, "按驾驶员身份证查询违章记录，显示该驾驶员违章统计", is_v2=True)
    add_bullet(doc, "快速查询所有未处理违章", is_v2=True)
    add_bullet(doc, "统计面板显示车辆总数、驾驶员总数、违章总数、已处理/未处理数量", is_v2=True)

    # 第三章：命令行与自测
    doc.add_page_break()
    add_styled_heading(doc, "第三章  命令行模式与自测用例", level=1)

    add_styled_heading(doc, "3.1 命令行参数", level=2)
    add_para(doc, "V2.0新增命令行支持，方便脚本集成和快速操作：", is_v2=True)
    add_code_block(doc, """用法:
  vio.exe                 交互模式（菜单）
  vio.exe -h, --help      显示帮助
  vio.exe -t, --test      运行内置自测用例
  vio.exe -f <数据文件>   指定加载的数据文件
  vio.exe -o <输出文件>   指定导出的文件""")

    add_styled_heading(doc, "3.2 内置自测用例 (18个)", level=2)
    add_para(doc, "V2.0内置全面自测用例，运行--test即可执行所有测试：", is_v2=True)
    tests = [
        ("T01", "系统初始化", "initSystem成功"),
        ("T02", "添加车辆", "addVehicle成功"),
        ("T03", "重复车牌检测", "返回VIO_ERR_DUPLICATE"),
        ("T04", "按车牌查找", "findVehicleByPlate成功"),
        ("T05", "查找不存在车辆", "返回-1"),
        ("T06", "添加驾驶员", "addDriver成功"),
        ("T07", "重复身份证检测", "返回VIO_ERR_DUPLICATE"),
        ("T08", "添加违章记录", "addViolation成功"),
        ("T09", "动态数组扩容", "添加11条记录触发自动扩容"),
        ("T10", "按车牌违章统计", "countViolationsByPlate正确"),
        ("T11", "处理违章", "handleViolation成功"),
        ("T12", "处理后统计更新", "handled/unhandled计数正确"),
        ("T13", "保存到文件", "saveToFile文件创建成功"),
        ("T14", "从文件加载", "loadFromFile成功"),
        ("T15", "数据完整性验证", "车辆/驾驶员/违章数据全部正确"),
        ("T16", "魔数错误检测", "返回VIO_ERR_MAGIC"),
        ("T17", "删除车辆", "deleteVehicle成功"),
        ("T18", "删除不存在记录", "返回VIO_ERR_NOT_FOUND"),
    ]
    add_table_with_header(doc,
        ["编号", "测试项", "验证点"],
        tests,
        col_widths=[2, 5, 9],
        v2_rows=set(range(len(tests)))
    )

    # 第四章：编译与运行
    doc.add_page_break()
    add_styled_heading(doc, "第四章  编译与使用说明", level=1)

    add_styled_heading(doc, "4.1 编译方法", level=2)
    add_para(doc, "使用指定路径的GCC编译器进行编译，要求零警告零错误：")
    add_code_block(doc, """# 使用Makefile编译（推荐）
make

# 或直接使用gcc命令
D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o vio.exe main.c""")

    add_styled_heading(doc, "4.2 Makefile目标", level=2)
    add_info_box(doc, "🔷 V2.0 Makefile",
        "V2.0提供标准Makefile，CC固定为D:/bin/gcc.exe，CFLAGS使用-std=c99 -Wall -Wextra -O2。", "v2")
    make_targets = [
        ("make all", "编译生成vio.exe（默认目标）"),
        ("make run", "编译后运行交互模式"),
        ("make test", "编译并运行自测用例"),
        ("make clean", "清除编译产物和临时文件"),
        ("make help", "显示帮助信息"),
    ]
    add_table_with_header(doc, ["命令", "说明"], make_targets, col_widths=[4, 12], v2_rows=set(range(5)))

    add_styled_heading(doc, "4.3 运行自测", level=2)
    add_code_block(doc, """# 运行所有自测用例
.\\vio.exe --test

# 预期输出（所有测试PASS）
  ===== 运行内置自测用例 =====
  [PASS] 测试1: 系统初始化成功
  ...
  [PASS] 测试18: 删除不存在记录正确返回错误
  ============================================================================
  所有测试通过! 21/18 ✓""")

    # 第五章：错误码说明
    doc.add_page_break()
    add_styled_heading(doc, "第五章  VioStatus错误码说明", level=1)
    add_para(doc, "V2.0使用统一的错误码枚举，所有核心函数都返回VioStatus类型：", is_v2=True)
    error_codes = [
        ("VIO_OK", 0, "操作成功"),
        ("VIO_ERR_OPEN_FILE", 1, "无法打开文件"),
        ("VIO_ERR_READ_FILE", 2, "读取文件失败"),
        ("VIO_ERR_WRITE_FILE", 3, "写入文件失败"),
        ("VIO_ERR_MEMORY", 4, "内存分配失败"),
        ("VIO_ERR_CORRUPT", 5, "数据文件损坏"),
        ("VIO_ERR_MAGIC", 6, "魔数不匹配（非VIO格式）"),
        ("VIO_ERR_VERSION", 7, "不支持的文件版本"),
        ("VIO_ERR_EMPTY", 8, "空数据"),
        ("VIO_ERR_NOT_FOUND", 9, "记录未找到"),
        ("VIO_ERR_DUPLICATE", 10, "重复记录（车牌/身份证已存在）"),
        ("VIO_ERR_INVALID_INPUT", 11, "无效输入"),
        ("VIO_ERR_CANCEL", 12, "用户取消操作"),
        ("VIO_ERR_CAPACITY", 13, "容量不足"),
    ]
    add_table_with_header(doc,
        ["错误码", "值", "说明"],
        error_codes,
        col_widths=[5, 2, 9],
        v2_rows=set(range(len(error_codes)))
    )

    # 附录
    doc.add_page_break()
    add_styled_heading(doc, "附录  文件清单", level=1)
    files = [
        ("main.c", "主程序源代码（UTF-8 BOM编码）"),
        ("Makefile", "构建配置文件"),
        ("gen_doc.py", "本文档生成脚本"),
        ("vio.exe", "编译后的可执行文件"),
        ("violations.vio", "默认数据文件（运行后生成）"),
        ("Project_11_机动车违章信息管理系统_V1_backup.docx", "V1版本文档备份"),
    ]
    add_table_with_header(doc, ["文件名", "说明"], files, col_widths=[8, 8])

    # 保存文档
    output_file = "Project_11_机动车违章信息管理系统.docx"
    doc.save(output_file)
    print(f"文档已生成: {output_file}")
    return output_file

if __name__ == "__main__":
    generate_doc()
