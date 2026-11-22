# -*- coding: utf-8 -*-
"""
Project_04 V2.0 文档生成脚本
更新航空客运订票系统.docx，添加V2.0升级说明页
"""

from docx import Document
from docx.shared import Pt, RGBColor, Inches, Cm
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.oxml.ns import qn, nsdecls
from docx.oxml import parse_xml
import os
import sys

DOC_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                        "Project_04_航空客运订票系统.docx")

FONT_CN = "微软雅黑"
FONT_EN = "Microsoft YaHei"

ORANGE = RGBColor(0xFF, 0x6B, 0x00)
DARK_BLUE = RGBColor(0x1A, 0x3C, 0x6E)
BLACK = RGBColor(0x00, 0x00, 0x00)
GRAY = RGBColor(0x55, 0x55, 0x55)
WHITE = RGBColor(0xFF, 0xFF, 0xFF)
RED = RGBColor(0xCC, 0x00, 0x00)
GREEN = RGBColor(0x00, 0x88, 0x00)

YELLOW_BG = "FFFACD"
HEADER_BG = "1A3C6E"
LIGHT_BLUE_BG = "E8F0FE"
LIGHT_GREEN_BG = "E8F5E9"
LIGHT_RED_BG = "FFEBEE"


def set_font(run, name=FONT_CN, size=11, color=BLACK, bold=False):
    run.font.name = name
    run.font.size = Pt(size)
    run.font.color.rgb = color
    run.bold = bold
    r = run._element
    r.rPr.rFonts.set(qn('w:eastAsia'), name)


def set_paragraph_shading(paragraph, color_hex):
    pPr = paragraph._p.get_or_add_pPr()
    shd = parse_xml(f'<w:shd {nsdecls("w")} w:fill="{color_hex}" w:val="clear"/>')
    pPr.append(shd)


def set_cell_shading(cell, color_hex):
    tcPr = cell._tc.get_or_add_tcPr()
    shd = parse_xml(f'<w:shd {nsdecls("w")} w:fill="{color_hex}" w:val="clear"/>')
    tcPr.append(shd)


def add_heading_styled(doc, text, level=1, color=DARK_BLUE):
    h = doc.add_heading(level=level)
    h.alignment = WD_ALIGN_PARAGRAPH.LEFT
    run = h.add_run(text)
    sizes = {1: 20, 2: 16, 3: 13}
    set_font(run, size=sizes.get(level, 12), color=color, bold=True)
    return h


def add_para(doc, text, size=11, color=BLACK, bold=False, align=None, indent=None):
    p = doc.add_paragraph()
    if align:
        p.alignment = align
    if indent:
        p.paragraph_format.left_indent = Cm(indent)
    run = p.add_run(text)
    set_font(run, size=size, color=color, bold=bold)
    return p


def add_bullet(doc, text, size=11, color=BLACK, bold=False, level=0):
    p = doc.add_paragraph(style='List Bullet')
    p.paragraph_format.left_indent = Cm(1.2 + level * 0.8)
    run = p.add_run(text)
    set_font(run, size=size, color=color, bold=bold)
    return p


def add_code_block(doc, code_text):
    p = doc.add_paragraph()
    p.paragraph_format.left_indent = Cm(1.0)
    p.paragraph_format.space_before = Pt(4)
    p.paragraph_format.space_after = Pt(4)
    set_paragraph_shading(p, "F5F5F5")
    run = p.add_run(code_text)
    run.font.name = "Consolas"
    run.font.size = Pt(9)
    run.font.color.rgb = RGBColor(0x33, 0x33, 0x33)
    r = run._element
    r.rPr.rFonts.set(qn('w:eastAsia'), FONT_CN)


def add_separator(doc):
    p = doc.add_paragraph()
    run = p.add_run("─" * 60)
    set_font(run, size=10, color=GRAY)
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER


def add_v2_banner(doc):
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p.paragraph_format.space_before = Pt(24)
    p.paragraph_format.space_after = Pt(12)
    set_paragraph_shading(p, YELLOW_BG)

    run1 = p.add_run("★ ")
    set_font(run1, size=28, color=ORANGE, bold=True)

    run2 = p.add_run("V2.0 增强版升级说明")
    set_font(run2, size=28, color=ORANGE, bold=True)

    run3 = p.add_run(" ★")
    set_font(run3, size=28, color=ORANGE, bold=True)

    p2 = doc.add_paragraph()
    p2.alignment = WD_ALIGN_PARAGRAPH.CENTER
    set_paragraph_shading(p2, YELLOW_BG)
    run_sub = p2.add_run("Airline Ticket Booking System - Enhanced Edition")
    set_font(run_sub, size=14, color=ORANGE, bold=False)

    for _ in range(2):
        p_space = doc.add_paragraph()
        set_paragraph_shading(p_space, YELLOW_BG)


def add_version_table(doc):
    table = doc.add_table(rows=7, cols=2, style='Table Grid')
    table.alignment = WD_ALIGN_PARAGRAPH.CENTER

    headers = ["项目", "说明"]
    data = [
        ("版本号", "V2.0 (Enhanced Edition)"),
        ("魔数标识", "AIRS (0x41495253)"),
        ("文件版本", "0x0002 (小端序)"),
        ("编码标准", "UTF-8 / C99"),
        ("编译器", "gcc (MinGW) -std=c99 -Wall -O2"),
        ("构建系统", "Makefile (含 all/clean/run 目标)"),
    ]

    for i, h in enumerate(headers):
        cell = table.rows[0].cells[i]
        cell.text = ""
        set_cell_shading(cell, HEADER_BG)
        run = cell.paragraphs[0].add_run(h)
        set_font(run, size=11, color=WHITE, bold=True)
        cell.paragraphs[0].alignment = WD_ALIGN_PARAGRAPH.CENTER

    for row_idx, (key, val) in enumerate(data, start=1):
        bg = LIGHT_BLUE_BG if row_idx % 2 == 1 else "FFFFFF"
        for col_idx, text in enumerate([key, val]):
            cell = table.rows[row_idx].cells[col_idx]
            cell.text = ""
            set_cell_shading(cell, bg)
            run = cell.paragraphs[0].add_run(text)
            bold = (col_idx == 0)
            set_font(run, size=10, color=BLACK, bold=bold)


def add_features_table(doc):
    add_heading_styled(doc, "V2.0 新增特性一览", level=2)
    table = doc.add_table(rows=11, cols=3, style='Table Grid')
    table.alignment = WD_ALIGN_PARAGRAPH.CENTER

    hdr = ["分类", "特性", "说明"]
    features = [
        ("界面", "ANSI 彩色终端输出", "OK绿色/ERR红色/INFO青色/WARN黄色，Windows自动启用VT100"),
        ("内存", "动态航班数组", "替代静态MAX_FLIGHTS=50，realloc自动扩容（初始8，倍增）"),
        ("内存", "动态候补队列", "循环队列自动扩容，支持无限候补乘客"),
        ("错误", "统一错误码", "AirStatus枚举：OK/ERR_NOMEM/ERR_NOTFOUND/ERR_FULL等"),
        ("CLI", "命令行参数", "--help/--load/--save支持启动时指定数据文件"),
        ("持久化", "魔数文件头", "AIRS魔数+版本号+校验和，防止无效文件加载"),
        ("统计", "运营统计面板", "总航班/已订座/营收/载客率/候补人数/满员航班数"),
        ("显示", "座位分配可视化", "航班列表显示座位条[####....]直观展示占用状态"),
        ("体验", "预置测试数据", "一键加载5条测试航班，方便快速体验"),
        ("健壮性", "空指针/内存检查", "所有malloc/realloc返回值检查，OOM优雅处理"),
    ]

    for i, h in enumerate(hdr):
        cell = table.rows[0].cells[i]
        cell.text = ""
        set_cell_shading(cell, HEADER_BG)
        run = cell.paragraphs[0].add_run(h)
        set_font(run, size=10, color=WHITE, bold=True)
        cell.paragraphs[0].alignment = WD_ALIGN_PARAGRAPH.CENTER

    cat_colors = {
        "界面": LIGHT_BLUE_BG,
        "内存": LIGHT_GREEN_BG,
        "错误": LIGHT_RED_BG,
        "CLI": LIGHT_BLUE_BG,
        "持久化": LIGHT_GREEN_BG,
        "统计": LIGHT_BLUE_BG,
        "显示": LIGHT_BLUE_BG,
        "体验": LIGHT_GREEN_BG,
        "健壮性": LIGHT_RED_BG,
    }

    for row_idx, (cat, feat, desc) in enumerate(features, start=1):
        bg = cat_colors.get(cat, "FFFFFF")
        for col_idx, text in enumerate([cat, feat, desc]):
            cell = table.rows[row_idx].cells[col_idx]
            cell.text = ""
            set_cell_shading(cell, bg if col_idx == 0 else "FFFFFF")
            run = cell.paragraphs[0].add_run(text)
            set_font(run, size=9, color=BLACK, bold=(col_idx < 2))


def add_tech_section(doc):
    add_heading_styled(doc, "技术架构说明（V2.0更新）", level=2)

    add_heading_styled(doc, "1. 数据结构升级", level=3, color=DARK_BLUE)

    add_para(doc, "航班存储从静态数组升级为动态数组：", bold=True)
    add_code_block(doc, """/* V1.0 静态数组 - 固定容量50 */
#define MAX_FLIGHTS 50
Flight flights[MAX_FLIGHTS];
int flightCount = 0;

/* V2.0 动态数组 - 自动扩容 */
static Flight *flights = NULL;
static int flightCount = 0;
static int flightCap = 0;     // 当前容量（2的幂次）

static int ensureFlightCap(int need) {
    if (need <= flightCap) return 1;
    int newCap = flightCap == 0 ? 8 : flightCap;
    while (newCap < need) newCap *= 2;  // 倍增策略
    Flight *nf = (Flight *)realloc(flights,
                       (size_t)newCap * sizeof(Flight));
    if (!nf) return 0;
    flights = nf;
    flightCap = newCap;
    return 1;
}""")

    add_para(doc, "")
    add_para(doc, "候补队列实现为可扩容循环队列：", bold=True)
    add_code_block(doc, """typedef struct {
    char id[ID_LEN];
    char name[NAME_LEN];
    int seatNum;
} WaitItem;

typedef struct {
    WaitItem *items;
    int front, rear, cap, count;
} WaitQueue;

static WaitQueue *waitQueueCreate(void) {
    WaitQueue *q = (WaitQueue *)calloc(1, sizeof(WaitQueue));
    q->cap = 8;
    q->items = (WaitItem *)calloc(8, sizeof(WaitItem));
    return q;
}""")

    add_heading_styled(doc, "2. ANSI彩色输出系统", level=3, color=DARK_BLUE)
    add_code_block(doc, """#define C_RESET     "\\033[0m"
#define C_RED       "\\033[31m"
#define C_GREEN     "\\033[32m"
#define C_YELLOW    "\\033[33m"
#define C_BLUE      "\\033[34m"
#define C_CYAN      "\\033[36m"
#define C_DIM       "\\033[2m"
#define C_BOLD      "\\033[1m"

static void cPrint(const char *color, const char *fmt, ...) {
    va_list args; va_start(args, fmt);
    if (colorEnabled && color) fputs(color, stdout);
    vprintf(fmt, args);
    if (colorEnabled && color) fputs(C_RESET, stdout);
    va_end(args);
}""")

    add_para(doc, "Windows平台通过SetConsoleMode启用VT100支持，非TTY环境自动禁用颜色。")

    add_heading_styled(doc, "3. 二进制持久化格式", level=3, color=DARK_BLUE)
    add_code_block(doc, """typedef struct {
    char magic[4];           // "AIRS" 魔数标识
    unsigned short version;  // 0x0002 = V2.0
    unsigned int flightCount;// 航班总数
    unsigned int totalBooked;// 总订票数
    unsigned int checksum;   // 校验和 (count*31 + booked*7)
} AirHeader;  // 16字节文件头""")
    add_para(doc, "文件结构：AirHeader(16B) → Flight[0] → Customer链表 → WaitQueue → Flight[1] → ...",
             color=GRAY, size=10)

    add_heading_styled(doc, "4. 统一错误码", level=3, color=DARK_BLUE)
    add_code_block(doc, """typedef enum {
    AIR_OK = 0,
    AIR_ERR_NOMEM,      // 内存不足
    AIR_ERR_NOTFOUND,   // 未找到
    AIR_ERR_FULL,       // 已满员
    AIR_ERR_DUPLICATE,  // 重复
    AIR_ERR_IO,         // IO错误
    AIR_ERR_INVALID,    // 无效参数
    AIR_ERR_EMPTY       // 空数据
} AirStatus;""")


def add_compile_section(doc):
    add_heading_styled(doc, "编译与运行", level=2)

    add_para(doc, "环境要求", bold=True, size=12)
    add_bullet(doc, "GCC (MinGW-w64) 支持 C99 标准")
    add_bullet(doc, "Windows 10/11 （ANSI颜色需要VT100终端支持）")
    add_bullet(doc, "python-docx （仅文档生成需要）")

    add_para(doc, "")
    add_para(doc, "编译命令", bold=True, size=12)
    add_code_block(doc, """# 使用Makefile
make            # 编译
make run        # 编译并运行
make clean      # 清理

# 或直接编译
D:\\bin\\gcc.exe -std=c99 -Wall -O2 -o airline.exe main.c""")

    add_para(doc, "")
    add_para(doc, "命令行参数", bold=True, size=12)
    add_code_block(doc, """airline.exe --help              # 显示帮助
airline.exe                     # 默认加载 airline.dat
airline.exe --load data.bin     # 指定加载文件
airline.exe --save backup.dat   # 指定保存文件""")


def add_data_section(doc):
    add_heading_styled(doc, "功能模块保留说明", level=2)

    modules = [
        ("航班信息录入", "录入航班号/飞机号/终点站/飞行日/乘员定额，自动分配座位"),
        ("航线查询", "按终点站名模糊匹配查询，支持子串搜索"),
        ("客票预订", "客户姓名订票，余票>0直接分配座位，满员自动进入候补队列"),
        ("退票处理", "退票后自动从候补队列补位，FIFO顺序分配座位，通知新客户"),
        ("航班浏览", "彩色表格显示所有航班，含座位可视化条[####....]"),
        ("客户名单", "按航班号查看已订票客户链表，显示座位号"),
        ("测试数据", "一键加载5条预置航班（北京/上海/广州/成都/深圳）"),
        ("运营统计", "总航班/总座位/已订座/余票/营收/载客率/满员数/候补数"),
        ("数据持久化", "二进制保存/加载，魔数校验防误读，自动保存退出"),
    ]

    table = doc.add_table(rows=len(modules) + 1, cols=2, style='Table Grid')
    table.alignment = WD_ALIGN_PARAGRAPH.CENTER

    for i, h in enumerate(["功能模块", "说明"]):
        cell = table.rows[0].cells[i]
        cell.text = ""
        set_cell_shading(cell, HEADER_BG)
        run = cell.paragraphs[0].add_run(h)
        set_font(run, size=10, color=WHITE, bold=True)

    for idx, (name, desc) in enumerate(modules, start=1):
        bg = LIGHT_BLUE_BG if idx % 2 == 1 else "FFFFFF"
        for col, text in enumerate([name, desc]):
            cell = table.rows[idx].cells[col]
            cell.text = ""
            set_cell_shading(cell, bg)
            run = cell.paragraphs[0].add_run(text)
            set_font(run, size=9, color=BLACK, bold=(col == 0))


def main():
    print("=" * 60)
    print("  Project_04 V2.0 文档更新脚本")
    print("=" * 60)

    if os.path.exists(DOC_PATH):
        print(f"[INFO] 打开已有文档: {DOC_PATH}")
        doc = Document(DOC_PATH)
    else:
        print(f"[WARN] 未找到原文档，创建新文档")
        doc = Document()

    style = doc.styles['Normal']
    style.font.name = FONT_CN
    style.font.size = Pt(11)
    style.element.rPr.rFonts.set(qn('w:eastAsia'), FONT_CN)

    doc.add_page_break()

    add_v2_banner(doc)

    add_heading_styled(doc, "版本信息", level=2)
    add_version_table(doc)

    doc.add_paragraph()
    add_separator(doc)
    doc.add_paragraph()

    add_features_table(doc)

    doc.add_paragraph()
    add_separator(doc)

    add_tech_section(doc)

    doc.add_paragraph()
    add_separator(doc)

    add_compile_section(doc)

    doc.add_paragraph()
    add_separator(doc)

    add_data_section(doc)

    doc.add_paragraph()
    p_end = doc.add_paragraph()
    p_end.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run_end = p_end.add_run("—— V2.0 文档更新完成 ——")
    set_font(run_end, size=12, color=ORANGE, bold=True)

    doc.save(DOC_PATH)
    print(f"[OK] 文档已保存: {DOC_PATH}")
    print(f"[OK] V2.0 升级页面已添加（黄底橙字标识）")
    print("=" * 60)


if __name__ == "__main__":
    main()
