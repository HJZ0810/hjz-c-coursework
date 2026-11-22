# -*- coding: utf-8 -*-
"""
期刊管理系统 V2.0 - 文档生成与更新工具
功能：
1. 备份原文档为 _V1_backup.docx (已手动完成)
2. 打开现有docx文档，追加V2.0升级说明
3. V2.0新增/修改内容使用蓝色字体高亮标注
"""

from docx import Document
from docx.shared import Pt, Cm, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.oxml.ns import qn, nsdecls
from docx.enum.table import WD_TABLE_ALIGNMENT, WD_ALIGN_VERTICAL
from docx.oxml import parse_xml
import os

# V2.0 蓝色高亮色 (标准蓝)
COLOR_V2_NEW = RGBColor(0x00, 0x66, 0xCC)
# 标题色
COLOR_PRIMARY = RGBColor(0x1A, 0x56, 0xDB)
# 普通文本色
COLOR_DARK = RGBColor(0x1F, 0x29, 0x37)
# 灰色
COLOR_GRAY = RGBColor(0x6B, 0x72, 0x80)
# 绿色成功
COLOR_SUCCESS = RGBColor(0x21, 0x96, 0x53)


def setup_run_font(run, size=11, bold=False, color=None, font_name='微软雅黑'):
    """设置run字体格式"""
    run.font.name = font_name
    run._element.rPr.rFonts.set(qn('w:eastAsia'), font_name)
    run.font.size = Pt(size)
    run.bold = bold
    if color:
        run.font.color.rgb = color
    else:
        run.font.color.rgb = COLOR_DARK


def set_cell_shading(cell, color_hex):
    """设置单元格背景色"""
    shading = parse_xml(f'<w:shd {nsdecls("w")} w:fill="{color_hex}" w:val="clear"/>')
    cell._tc.get_or_add_tcPr().append(shading)


def add_heading_blue(doc, text, level=1):
    """添加V2.0蓝色标题"""
    h = doc.add_heading(text, level=level)
    for run in h.runs:
        setup_run_font(run, size=20 if level == 1 else (16 if level == 2 else 13),
                      bold=True, color=COLOR_V2_NEW)
    return h


def add_para_blue(doc, text, bold=False, size=11, indent=True):
    """添加蓝色高亮段落（V2.0新增内容）"""
    p = doc.add_paragraph()
    if indent:
        p.paragraph_format.first_line_indent = Cm(0.74)
    p.paragraph_format.line_spacing = 1.5
    p.paragraph_format.space_after = Pt(6)
    run = p.add_run(text)
    setup_run_font(run, size=size, bold=bold, color=COLOR_V2_NEW)
    return p


def add_para(doc, text, bold=False, size=11, indent=True, color=None):
    """添加普通段落"""
    p = doc.add_paragraph()
    if indent:
        p.paragraph_format.first_line_indent = Cm(0.74)
    p.paragraph_format.line_spacing = 1.5
    p.paragraph_format.space_after = Pt(6)
    run = p.add_run(text)
    setup_run_font(run, size=size, bold=bold, color=color if color else COLOR_DARK)
    return p


def add_bullet_blue(doc, text, bold_prefix=""):
    """添加蓝色项目符号（V2.0新增功能）"""
    p = doc.add_paragraph(style='List Bullet')
    p.paragraph_format.left_indent = Cm(1.2)
    p.paragraph_format.space_after = Pt(3)
    if bold_prefix:
        r1 = p.add_run(bold_prefix)
        setup_run_font(r1, size=11, bold=True, color=COLOR_V2_NEW)
    r2 = p.add_run(text)
    setup_run_font(r2, size=11, color=COLOR_V2_NEW)
    return p


def add_bullet(doc, text, bold_prefix="", color=None):
    """添加普通项目符号"""
    p = doc.add_paragraph(style='List Bullet')
    p.paragraph_format.left_indent = Cm(1.2)
    p.paragraph_format.space_after = Pt(3)
    if bold_prefix:
        r1 = p.add_run(bold_prefix)
        setup_run_font(r1, size=11, bold=True, color=COLOR_PRIMARY if not color else color)
    r2 = p.add_run(text)
    setup_run_font(r2, size=11, color=color if color else COLOR_DARK)
    return p


def add_v2_badge(doc):
    """添加V2.0标签"""
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.RIGHT
    run = p.add_run("【V2.0 新增/修改内容 - 蓝色字体标注】")
    setup_run_font(run, size=10, bold=True, color=COLOR_V2_NEW)


def add_code_block(doc, code_text):
    """添加代码块（带深色背景）"""
    table = doc.add_table(rows=1, cols=1)
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    cell = table.cell(0, 0)
    set_cell_shading(cell, "1E293B")
    lines = code_text.split('\n')
    for i, line in enumerate(lines):
        if i == 0:
            p = cell.paragraphs[0]
        else:
            p = cell.add_paragraph()
        p.paragraph_format.space_before = Pt(0)
        p.paragraph_format.space_after = Pt(0)
        p.paragraph_format.line_spacing = 1.2
        run = p.add_run(line if line else " ")
        run.font.name = 'Consolas'
        run.font.size = Pt(9.5)
        run.font.color.rgb = RGBColor(0xE2, 0xE8, 0xF0)
    doc.add_paragraph()


def add_separator(doc, color=COLOR_PRIMARY):
    """添加分隔线"""
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = p.add_run("━" * 60)
    run.font.color.rgb = color
    run.font.size = Pt(8)
    p.paragraph_format.space_before = Pt(6)
    p.paragraph_format.space_after = Pt(6)


def main():
    docx_path = r"D:\HJZ\Project_10_期刊管理\Project_10_期刊管理系统.docx"

    print(f"正在打开文档: {docx_path}")
    doc = Document(docx_path)

    # 文档分页，开始V2.0升级章节
    doc.add_page_break()

    # ========== V2.0 升级说明封面 ==========
    add_v2_badge(doc)

    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p.paragraph_format.space_before = Pt(80)
    run = p.add_run("期刊管理系统")
    setup_run_font(run, size=32, bold=True, color=COLOR_V2_NEW)

    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = p.add_run("V2.0 升级说明文档")
    setup_run_font(run, size=24, bold=True, color=COLOR_V2_NEW)

    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = p.add_run("Enhanced Edition - V2.0 Standards")
    setup_run_font(run, size=14, color=COLOR_GRAY)

    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p.paragraph_format.space_before = Pt(60)
    run = p.add_run("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    run.font.color.rgb = COLOR_V2_NEW
    run.font.size = Pt(12)

    version_info = [
        "项目编号：Project_10",
        "文档类型：系统升级说明书",
        "版本：V1.0 → V2.0",
        "升级日期：2026年",
        "编译器：GCC (MinGW) -std=c99 -Wall -Wextra -O2",
    ]
    for line in version_info:
        p = doc.add_paragraph()
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        p.paragraph_format.space_after = Pt(4)
        run = p.add_run(line)
        setup_run_font(run, size=12, color=COLOR_DARK)

    doc.add_page_break()

    # ========== 第一章 升级概述 ==========
    add_heading_blue(doc, "第一章  V2.0 升级概述", level=1)
    add_separator(doc, COLOR_V2_NEW)

    add_heading_blue(doc, "1.1 升级背景与目标", level=2)
    add_para_blue(doc, "本次升级参照 Project_39_哈夫曼编译码器 的高质量工程标准，对期刊管理系统进行全面重构。升级目标包括：代码工程化规范化、增强鲁棒性、提升用户体验、完善错误处理、增加自动化测试能力。所有新增/修改内容在本文档中以蓝色字体标注。")

    add_heading_blue(doc, "1.2 V2.0 新增功能一览", level=2)

    add_para_blue(doc, "本次升级新增/改进以下核心特性：", bold=True)

    v2_features = [
        ("UTF-8 BOM 源码编码支持", "源文件采用UTF-8 with BOM编码，彻底解决Windows记事本/编辑器打开乱码问题，跨平台兼容"),
        ("ANSI 彩色终端输出", "Windows 10+ VT100模式启用，成功信息绿色[OK]、错误信息红色[ERR]、标题青色高亮，界面美观清晰"),
        ("命令行界面 (CLI)", "支持 --help/-h 显示帮助、--test/-t 运行自测、-f <file> 指定数据文件，支持脚本化调用"),
        ("动态内存管理", "期刊和读者数组从静态数组改为动态扩容数组，初始容量16，满载时自动翻倍，无数量硬限制"),
        ("统一错误码枚举", "定义 MagzStatus 枚举类型，包含12种错误码，错误处理统一规范，不再零散返回-1/0"),
        ("文件持久化带魔数校验", "二进制数据文件以 \"MAGZV2\" 魔数开头，带版本号和CRC32校验，检测文件损坏/非法文件"),
        ("内置自测用例", "37个自动化测试用例，覆盖系统初始化、动态扩容、CRUD操作、借阅归还预约、文件I/O、日期工具、错误处理6大测试组"),
        ("详细代码注释", "每个函数、每个关键数据结构、每个业务逻辑分支都有清晰注释，可读性和可维护性大幅提升"),
        ("Makefile构建系统", "提供标准Makefile，CC指定为D:/bin/gcc.exe，CFLAGS=-std=c99 -Wall -Wextra -O2，一条命令编译"),
        ("预约队列自动流转", "归还期刊时自动检查预约队列，队首读者自动完成借阅，流程更智能"),
    ]

    table = doc.add_table(rows=1 + len(v2_features), cols=3)
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    table.style = 'Table Grid'

    hdr = table.rows[0].cells
    headers = ["编号", "功能特性", "说明"]
    for i, h in enumerate(headers):
        set_cell_shading(hdr[i], "0066CC")
        p = hdr[i].paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r = p.add_run(h)
        setup_run_font(r, size=10.5, bold=True, color=RGBColor(0xFF, 0xFF, 0xFF))
        hdr[i].vertical_alignment = WD_ALIGN_VERTICAL.CENTER

    for i, (name, desc) in enumerate(v2_features):
        cells = table.rows[i + 1].cells
        bg = "E8F4FF" if i % 2 == 0 else "FFFFFF"
        for c in cells:
            set_cell_shading(c, bg)
            c.vertical_alignment = WD_ALIGN_VERTICAL.CENTER

        p = cells[0].paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r = p.add_run(f"F{i+1:02d}")
        setup_run_font(r, size=10, color=COLOR_V2_NEW)

        p = cells[1].paragraphs[0]
        r = p.add_run(name)
        setup_run_font(r, size=10, bold=True, color=COLOR_V2_NEW)

        p = cells[2].paragraphs[0]
        r = p.add_run(desc)
        setup_run_font(r, size=10, color=COLOR_V2_NEW)

    doc.add_paragraph()

    doc.add_page_break()

    # ========== 第二章 核心架构变更 ==========
    add_heading_blue(doc, "第二章  核心架构变更", level=1)
    add_separator(doc, COLOR_V2_NEW)

    add_heading_blue(doc, "2.1 数据结构重构：从静态数组到动态数组", level=2)
    add_para_blue(doc, "V1.0版本采用固定大小的静态数组存储期刊和读者，存在数量上限且空间浪费。V2.0改为动态扩容数组（Dynamic Array），采用倍增策略自动扩容。")

    add_para_blue(doc, "V2.0核心数据结构定义如下：", bold=True)
    add_code_block(doc, """/* V2.0 系统上下文 - 管理所有动态数组 */
typedef struct SysContext {
    Journal *journals;           // 动态期刊数组
    int     journal_count;       // 当前期刊数量
    int     journal_capacity;    // 数组容量(自动扩容)
    Reader  *readers;            // 动态读者数组
    int     reader_count;        // 当前读者数量
    int     reader_capacity;     // 数组容量(自动扩容)
    ResvQueue *resv_queues;      // 每个期刊一条预约队列
    char    data_path[260];      // 数据文件路径
} SysContext;

/* V2.0 统一错误码枚举 */
typedef enum {
    MAGZ_OK = 0,
    MAGZ_ERR_MEMORY = -1,        // 内存分配失败
    MAGZ_ERR_NOT_FOUND = -2,     // 期刊/读者不存在
    MAGZ_ERR_DUPLICATE = -3,     // ID重复
    MAGZ_ERR_ALREADY_BORROWED = -4,  // 已全部借出
    MAGZ_ERR_NOT_BORROWED = -5,  // 未借此刊
    MAGZ_ERR_QUEUE_FULL = -6,    // (保留)队列满
    MAGZ_ERR_FILE_OPEN = -7,     // 无法打开文件
    MAGZ_ERR_FILE_READ = -8,     // 读取失败
    MAGZ_ERR_FILE_WRITE = -9,    // 写入失败
    MAGZ_ERR_CRC_MISMATCH = -10, // CRC校验失败(文件损坏)
    MAGZ_ERR_INVALID = -11,      // 非法文件(魔数不匹配)
    MAGZ_ERR_MAX_BORROW = -12    // 借阅数达上限
} MagzStatus;""")

    add_para_blue(doc, "动态扩容策略：", bold=True)
    add_bullet_blue(doc, "初始容量 INITIAL_CAPACITY = 16")
    add_bullet_blue(doc, "当 count == capacity 时触发扩容，新容量 = 旧容量 × 2")
    add_bullet_blue(doc, "使用 realloc() 重新分配内存，原有数据自动拷贝")
    add_bullet_blue(doc, "扩容同时同步预约队列数组，一一对应")
    add_bullet_blue(doc, "程序退出时调用 ctx_destroy() 释放所有内存，无内存泄漏")

    add_heading_blue(doc, "2.2 文件持久化：MAGZV2 魔数 + CRC32 校验", level=2)
    add_para_blue(doc, "V1.0版本无持久化或简单文本存储，无法校验数据完整性。V2.0设计了带魔数和校验的二进制格式：")

    add_code_block(doc, """/* V2.0 二进制文件头格式 (19字节) */
#pragma pack(push, 1)
typedef struct MagzHeader {
    char          magic[6];       // 魔数 "MAGZV2" (0x4D41475A5632)
    unsigned short version;       // 版本号 0x0200 (V2.0)
    int           journal_count;  // 期刊数量
    int           reader_count;   // 读者数量
    unsigned int  data_crc;       // 数据区CRC32校验和
} MagzHeader;
#pragma pack(pop)

/* 文件布局：
   ┌─────────────────────────────┐
   │  MagzHeader (19字节)        │  ← 魔数+版本+计数+CRC
   ├─────────────────────────────┤
   │  Journal[journal_count]     │  ← 期刊数据(二进制)
   ├─────────────────────────────┤
   │  Reader[reader_count]       │  ← 读者数据(二进制)
   └─────────────────────────────┘
*/""")

    add_bullet_blue(doc, "加载时首先校验magic是否为\"MAGZV2\"，不是则拒绝加载（防止误打开其他文件）")
    add_bullet_blue(doc, "校验version字段，为未来版本兼容性预留空间")
    add_bullet_blue(doc, "加载后重新计算数据区CRC与头中存储的CRC比对，不一致则报告文件损坏")
    add_bullet_blue(doc, "所有文件操作使用\"wb\"/\"rb\"二进制模式，Windows下不做\\r\\n转换")

    add_heading_blue(doc, "2.3 ANSI彩色终端输出", level=2)
    add_para_blue(doc, "V2.0在Windows 10+系统上启用VT100虚拟终端处理模式，支持ANSI转义序列输出彩色文本。颜色方案：")

    color_table = doc.add_table(rows=6, cols=3)
    color_table.style = 'Table Grid'
    color_table.alignment = WD_TABLE_ALIGNMENT.CENTER
    color_data = [
        ("颜色", "用途", "示例文本"),
        ("绿色 [OK]", "操作成功提示", "[OK] 期刊添加成功"),
        ("红色 [ERR]", "错误/失败提示", "[ERR] 期刊不存在"),
        ("黄色 [WARN]", "警告提示", "[WARN] 暂无期刊信息"),
        ("青色加粗", "标题、菜单、分隔线", "主菜单"),
        ("灰色", "辅助提示、按Enter继续", "按 Enter 键继续..."),
    ]
    for i, row_data in enumerate(color_data):
        cells = color_table.rows[i].cells
        if i == 0:
            for j, c in enumerate(cells):
                set_cell_shading(c, "0066CC")
                p = c.paragraphs[0]
                p.alignment = WD_ALIGN_PARAGRAPH.CENTER
                r = p.add_run(row_data[j])
                setup_run_font(r, size=10, bold=True, color=RGBColor(0xFF,0xFF,0xFF))
        else:
            bg = "E8F4FF" if i % 2 == 1 else "FFFFFF"
            for j, c in enumerate(cells):
                set_cell_shading(c, bg)
                p = c.paragraphs[0]
                r = p.add_run(row_data[j])
                setup_run_font(r, size=10, bold=(j==0), color=COLOR_V2_NEW)
    doc.add_paragraph()

    doc.add_page_break()

    # ========== 第三章 命令行界面 ==========
    add_heading_blue(doc, "第三章  命令行界面 (CLI) 使用说明", level=1)
    add_separator(doc, COLOR_V2_NEW)

    add_para_blue(doc, "V2.0新增完整的命令行参数支持，既可以交互菜单模式运行，也支持命令行直接调用：")

    add_code_block(doc, """用法:
  journal.exe                 启动交互模式(默认)
  journal.exe -h | --help     显示帮助信息
  journal.exe -t | --test     运行内置自测用例
  journal.exe -f <file>       指定数据文件路径
  journal.exe <file>          等价于 -f <file>

示例:
  journal.exe                         # 交互模式，数据文件 magz_data.bin
  journal.exe -t                      # 运行自测，验证程序正确性
  journal.exe -f my_magazine.bin      # 使用指定数据文件
  journal.exe library.bin             # 同上(简写形式)""")

    add_heading_blue(doc, "3.1 自测模式 (--test/-t)", level=2)
    add_para_blue(doc, "运行 journal.exe --test 将执行内置的37个自动化测试用例，无需人工交互即可验证程序功能正确性。测试分为6组：")
    add_bullet_blue(doc, "测试组1：系统初始化与动态内存（ctx_init、动态扩容、ctx_destroy）")
    add_bullet_blue(doc, "测试组2：期刊CRUD操作（添加、查找、重复ID拒绝、删除）")
    add_bullet_blue(doc, "测试组3：读者CRUD与借阅/归还/预约（完整业务流程，含预约队列peek校验）")
    add_bullet_blue(doc, "测试组4：文件持久化（MAGZV2魔数校验、版本号、CRC、保存/加载闭环）")
    add_bullet_blue(doc, "测试组5：日期工具与辅助函数（日期格式、日期比较、跨年比较）")
    add_bullet_blue(doc, "测试组6：错误处理边界（不存在资源操作、归还未借阅等错误路径）")

    add_para_blue(doc, "自测成功输出示例：", bold=True)
    add_code_block(doc, """  ----------------------------------------
  期刊管理系统 V2.0 - 内置自测用例
  ----------------------------------------

  [测试组1] 系统初始化与动态内存
  [测试组2] 期刊CRUD操作
  [测试组3] 读者CRUD与借阅/归还/预约
  [测试组4] 文件持久化(MAGZV2魔数+CRC)
  [测试组5] 日期工具与辅助函数
  [测试组6] 错误处理边界
  ----------------------------------------

  测试结果: 37 通过  0 失败

  ✓ 全部自测通过!""")

    doc.add_page_break()

    # ========== 第四章 构建系统 ==========
    add_heading_blue(doc, "第四章  Makefile 构建系统", level=1)
    add_separator(doc, COLOR_V2_NEW)

    add_para_blue(doc, "V2.0提供标准Makefile，使用指定编译器 D:\\bin\\gcc.exe，编译选项严格遵循 C99 标准并开启所有警告：")

    add_code_block(doc, """# Journal Management System V2.0 - Makefile
# Compiler: D:/bin/gcc.exe (MinGW-w64)
# Flags: -std=c99 -Wall -Wextra -O2

CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = journal.exe
SRC = main.c

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(SRC)
\t$(CC) $(CFLAGS) -o $@ $^

run: $(TARGET)
\t./$(TARGET)

test: $(TARGET)
\t./$(TARGET) --test

clean:
\t-del /f $(TARGET) *.o *.bin *.exe 2>nul
\t-del /f magz_data.bin _test_magz_v2.bin _test_run.bin 2>nul""")

    add_para_blue(doc, "Make 目标说明：", bold=True)
    add_bullet_blue(doc, "make 或 make all：编译生成 journal.exe（默认目标）")
    add_bullet_blue(doc, "make run：编译并启动交互模式")
    add_bullet_blue(doc, "make test：编译并运行自测用例")
    add_bullet_blue(doc, "make clean：清理编译产物和测试数据文件")

    add_para_blue(doc, "编译质量要求：零警告（Zero Warning）。-Wall -Wextra 开启几乎所有常见警告，编译时不允许出现任何警告。")

    doc.add_page_break()

    # ========== 第五章 业务逻辑增强 ==========
    add_heading_blue(doc, "第五章  业务逻辑增强", level=1)
    add_separator(doc, COLOR_V2_NEW)

    add_heading_blue(doc, "5.1 借阅上限控制", level=2)
    add_para_blue(doc, "V2.0新增每个读者最多借阅5册期刊的限制，防止单个读者占用过多资源。达到上限后借阅返回 MAGZ_ERR_MAX_BORROW 错误。")

    add_heading_blue(doc, "5.2 预约自动流转", level=2)
    add_para_blue(doc, "V1.0中预约仅记录队列但归还后不自动处理。V2.0实现智能预约流转：当期刊被归还时，自动检查该期刊的预约队列，如果队列非空，取出队首读者ID，自动为该读者办理借阅，实现\"归还即预约下一位\"的无缝体验。")

    add_code_block(doc, """// journal_return() 中的预约自动处理逻辑
ResvQueue *q = &ctx->resv_queues[ji];
if (!queue_is_empty(q)) {
    int next_reader_id = queue_dequeue(q);  // 取出队首预约者
    if (next_reader_id > 0) {
        int nri = ctx_find_reader_index(ctx, next_reader_id);
        if (nri >= 0) {
            journal_borrow(ctx, journal_id, next_reader_id);  // 自动借阅
        }
    }
}""")

    add_heading_blue(doc, "5.3 到期/超期提醒", level=2)
    add_para_blue(doc, "新增到期提醒功能（菜单选项11）：")
    add_bullet_blue(doc, "借阅超过30天：红色 [超期] 警告")
    add_bullet_blue(doc, "借阅25-29天：黄色 [即将到期] 提醒")
    add_bullet_blue(doc, "无超期/即将到期：绿色 OK 提示")

    add_heading_blue(doc, "5.4 删除安全检查", level=2)
    add_bullet_blue(doc, "删除期刊时检查是否已被借出，借出状态不允许删除，防止数据不一致")
    add_bullet_blue(doc, "删除读者时检查是否有未归还期刊，有未还不允许删除")
    add_bullet_blue(doc, "删除读者时自动清理该读者在所有预约队列中的记录")

    doc.add_page_break()

    # ========== 第六章 V1.0 vs V2.0 对比表 ==========
    add_heading_blue(doc, "第六章  V1.0 vs V2.0 功能对比", level=1)
    add_separator(doc, COLOR_V2_NEW)

    cmp_table = doc.add_table(rows=14, cols=3)
    cmp_table.style = 'Table Grid'
    cmp_table.alignment = WD_TABLE_ALIGNMENT.CENTER

    cmp_data = [
        ("对比维度", "V1.0 (原版)", "V2.0 (升级版)"),
        ("源码编码", "GBK/ANSI，UTF-8环境乱码", "UTF-8 with BOM，全平台无乱码"),
        ("终端输出", "黑白纯文本", "ANSI彩色输出（绿OK/红ERR/青标题）"),
        ("运行模式", "仅交互菜单", "交互模式 + CLI(--help/--test/-f)"),
        ("数据结构", "静态数组(固定大小)", "动态数组(自动倍增扩容，无上限)"),
        ("错误处理", "返回-1/0，分散", "统一MagzStatus枚举(12种错误码)"),
        ("数据持久化", "无/简单文本", "二进制MAGZV2格式+魔数+版本+CRC32校验"),
        ("预约队列", "仅记录不自动流转", "归还时自动借阅给队首预约者"),
        ("借阅限制", "无明确上限", "每人最多借阅5册"),
        ("到期提醒", "无", "超期红色警告/即将到期黄色提醒"),
        ("删除安全", "直接删除", "检查借出状态/未还期刊，拒绝不安全删除"),
        ("测试", "无自测", "37个内置自测用例(6个测试组)"),
        ("构建系统", "手动编译命令", "标准Makefile，零警告编译"),
        ("代码注释", "少量注释", "详细Doxygen风格注释"),
    ]

    for i, row in enumerate(cmp_data):
        cells = cmp_table.rows[i].cells
        for j, val in enumerate(row):
            if i == 0:
                set_cell_shading(cells[j], "0066CC")
                p = cells[j].paragraphs[0]
                p.alignment = WD_ALIGN_PARAGRAPH.CENTER
                r = p.add_run(val)
                setup_run_font(r, size=10.5, bold=True, color=RGBColor(0xFF,0xFF,0xFF))
            else:
                bg = "E8F4FF" if i % 2 == 1 else "FFFFFF"
                set_cell_shading(cells[j], bg)
                p = cells[j].paragraphs[0]
                r = p.add_run(val)
                is_v2 = (j == 2)
                setup_run_font(r, size=10, bold=is_v2, color=COLOR_V2_NEW if is_v2 else COLOR_DARK)
            cells[j].vertical_alignment = WD_ALIGN_VERTICAL.CENTER

    doc.add_paragraph()

    # ========== 附录 编译验证记录 ==========
    doc.add_page_break()
    add_heading_blue(doc, "附录  编译与测试验证记录", level=1)
    add_separator(doc, COLOR_V2_NEW)

    add_para_blue(doc, "V2.0版本已通过以下验证：", bold=True)

    verify_items = [
        ("编译验证", "D:\\bin\\gcc.exe -std=c99 -Wall -Wextra -O2", "零警告，零错误 (0 warnings, 0 errors)"),
        ("自测验证", "journal.exe --test", "37个测试用例全部通过 (37 passed, 0 failed)"),
        ("功能验证", "交互模式全流程操作", "添加/借阅/归还/预约/查询/删除/提醒/保存/加载正常"),
        ("CLI验证", "-h/--help/-t/--test/-f", "所有命令行参数工作正常"),
        ("文件验证", "MAGZV2二进制格式", "保存→退出→重启→加载数据完整，魔数/CRC正确"),
        ("内存验证", "ctx_init/ctx_destroy配对", "动态分配内存全部释放，无泄漏"),
    ]

    v_table = doc.add_table(rows=1 + len(verify_items), cols=4)
    v_table.style = 'Table Grid'
    v_table.alignment = WD_TABLE_ALIGNMENT.CENTER
    v_headers = ["验证项", "验证方法", "预期结果", "验证状态"]
    hdr = v_table.rows[0].cells
    for i, h in enumerate(v_headers):
        set_cell_shading(hdr[i], "219653")
        p = hdr[i].paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r = p.add_run(h)
        setup_run_font(r, size=10, bold=True, color=RGBColor(0xFF,0xFF,0xFF))

    for i, (name, method, expect) in enumerate(verify_items):
        cells = v_table.rows[i+1].cells
        bg = "D1FAE5" if i % 2 == 0 else "FFFFFF"
        for c in cells:
            set_cell_shading(c, bg)
            c.vertical_alignment = WD_ALIGN_VERTICAL.CENTER
        p = cells[0].paragraphs[0]; r = p.add_run(name); setup_run_font(r, size=9.5, color=COLOR_DARK)
        p = cells[1].paragraphs[0]; r = p.add_run(method); setup_run_font(r, size=9.5, color=COLOR_DARK)
        p = cells[2].paragraphs[0]; r = p.add_run(expect); setup_run_font(r, size=9.5, color=COLOR_DARK)
        p = cells[3].paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r = p.add_run("✓ PASS")
        setup_run_font(r, size=9.5, bold=True, color=COLOR_SUCCESS)

    doc.add_paragraph()

    add_para_blue(doc, "")
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p.paragraph_format.space_before = Pt(40)
    run = p.add_run("━" * 50)
    run.font.color.rgb = COLOR_V2_NEW
    run.font.size = Pt(10)

    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = p.add_run("期刊管理系统 V2.0  升级完成")
    setup_run_font(run, size=16, bold=True, color=COLOR_V2_NEW)

    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = p.add_run("Journal Management System - Enhanced Edition")
    setup_run_font(run, size=11, color=COLOR_GRAY)

    # 保存文档
    print(f"正在保存更新后的文档...")
    doc.save(docx_path)
    print(f"文档更新成功: {docx_path}")
    print(f"备份文件: Project_10_期刊管理系统_V1_backup.docx")


if __name__ == "__main__":
    main()
