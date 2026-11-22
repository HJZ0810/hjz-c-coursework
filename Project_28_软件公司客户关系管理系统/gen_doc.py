# -*- coding: utf-8 -*-
"""
CRM V2.0 文档生成脚本
将V1文档备份后，生成V2.0说明文档
V2.0 新增/修改部分以蓝色高亮标注

使用方法：
    python gen_doc.py
依赖：
    pip install python-docx
"""

import os
import shutil
from docx import Document
from docx.shared import Pt, RGBColor, Inches
from docx.enum.text import WD_ALIGN_PARAGRAPH

PROJECT_DIR = os.path.dirname(os.path.abspath(__file__))
V1_DOCX = os.path.join(PROJECT_DIR, "Project_28_软件公司客户关系管理系统.docx")
V1_BACKUP = os.path.join(PROJECT_DIR, "Project_28_软件公司客户关系管理系统_V1_backup.docx")
V2_DOCX = V1_DOCX  # 覆盖原文件

BLUE = RGBColor(0x00, 0x70, 0xC0)
BLACK = RGBColor(0x00, 0x00, 0x00)
RED = RGBColor(0xC0, 0x00, 0x00)


def blue_run(p, text, bold=False, size=11):
    """添加蓝色高亮文字"""
    r = p.add_run(text)
    r.font.color.rgb = BLUE
    r.font.size = Pt(size)
    r.bold = bold
    return r


def black_run(p, text, bold=False, size=11):
    """添加普通黑色文字"""
    r = p.add_run(text)
    r.font.color.rgb = BLACK
    r.font.size = Pt(size)
    r.bold = bold
    return r


def add_heading(doc, text, level=1):
    h = doc.add_heading(text, level=level)
    for run in h.runs:
        run.font.color.rgb = BLUE
    return h


def add_code_block(doc, code):
    """添加代码块"""
    p = doc.add_paragraph()
    p.paragraph_format.left_indent = Inches(0.3)
    r = p.add_run(code)
    r.font.name = "Consolas"
    r.font.size = Pt(9)
    r.font.color.rgb = RGBColor(0x33, 0x33, 0x33)


def generate_v2_doc():
    doc = Document()

    # 标题
    title = doc.add_heading("软件公司客户关系管理系统 V2.0", level=0)
    title.alignment = WD_ALIGN_PARAGRAPH.CENTER

    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    blue_run(p, "Customer Relationship Management System V2.0", bold=True, size=12)

    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    black_run(p, "2025-2026学年第二学期算法课程实习  |  Project 28", size=10)

    doc.add_paragraph()

    # === V2.0 升级说明 ===
    add_heading(doc, "V2.0 升级说明（蓝色高亮为V2新增/变更）", level=1)
    p = doc.add_paragraph()
    blue_run(p, "★ 本版本（V2.0）相对于 V1 链表版进行了全面重写，主要升级点：\n", bold=True)
    blue_run(p, "  1. 【编码】文件采用 UTF-8 BOM 编码，调用 SetConsoleOutputCP(65001) 彻底解决Windows中文乱码\n")
    blue_run(p, "  2. 【终端】引入 ANSI 彩色终端输出（红/绿/黄/蓝/青/紫/灰），界面更美观\n")
    blue_run(p, "  3. 【CLI】支持命令行参数：-h/--help 帮助，-t/--test 运行内置自测\n")
    blue_run(p, "  4. 【存储】核心数据结构由单链表升级为动态数组（Dynamic Array），缓存友好，支持随机访问\n")
    blue_run(p, "  5. 【错误码】统一错误码枚举 CrmStatus（13种状态），替换V1中void/零散int返回\n")
    blue_run(p, "  6. 【持久化】Magic Header \"CRMV2\" 二进制文件格式，含版本校验，防止读取旧版本/损坏文件\n")
    blue_run(p, "  7. 【自测】内置18项自动化测试，覆盖CRUD、跟进记录、统计、文件I/O、魔数校验等\n")
    blue_run(p, "  8. 【架构】使用单一 CrmContext 上下文结构替代V1的10个全局链表头指针\n")
    blue_run(p, "  9. 【实体】V2聚焦核心三实体：客户(Customer)+联系人(Contact)+跟进记录(FollowUp)\n")
    blue_run(p, " 10. 【级联】删除客户时自动级联删除其关联的联系人和跟进记录，保证数据一致性\n")

    # === 目录 ===
    add_heading(doc, "目录", level=1)
    doc.add_paragraph("1. 项目概述", style="List Number")
    doc.add_paragraph("2. V2.0 新特性详解", style="List Number")
    doc.add_paragraph("3. 系统架构", style="List Number")
    doc.add_paragraph("4. 数据结构", style="List Number")
    doc.add_paragraph("5. 统一错误码 CrmStatus", style="List Number")
    doc.add_paragraph("6. 文件持久化格式", style="List Number")
    doc.add_paragraph("7. 内置自测套件", style="List Number")
    doc.add_paragraph("8. 编译与运行", style="List Number")
    doc.add_paragraph("9. 功能菜单", style="List Number")
    doc.add_paragraph("10. 总结", style="List Number")

    # === 1. 项目概述 ===
    add_heading(doc, "1. 项目概述", level=1)
    p = doc.add_paragraph()
    black_run(p, "客户关系管理（CRM）系统是软件公司日常运营的核心系统之一。本系统采用 C 语言（C99标准）开发，为软件公司提供客户信息、联系人、跟进记录的统一管理平台，支持数据持久化、统计分析、交互菜单等功能。")

    p = doc.add_paragraph()
    blue_run(p, "V2.0版本重点提升工程质量：统一错误处理、动态数组替代链表、UTF-8全链路中文支持、ANSI彩色界面、自动化测试覆盖。", bold=True)

    add_heading(doc, "1.1 功能模块", level=2)
    table = doc.add_table(rows=1, cols=3)
    table.style = "Light Grid Accent 1"
    hdr = table.rows[0].cells
    hdr[0].text = "序号"
    hdr[1].text = "功能模块"
    hdr[2].text = "说明"
    modules = [
        ("1", "客户信息管理", "客户CRUD、按等级/状态/区域分类管理"),
        ("2", "联系人管理", "【V2】每个客户可多个联系人，支持增删"),
        ("3", "跟进记录", "【V2】记录跟进方式/内容/下次跟进日期"),
        ("4", "数据统计", "按等级/状态分布统计，联系人TOP排行"),
        ("5", "文件持久化", "【V2】CRMV2二进制格式+魔数校验"),
        ("6", "交互菜单", "【V2】ANSI彩色菜单界面"),
        ("7", "内置自测", "【V2】18项自动化测试套件"),
        ("8", "CLI参数", "【V2】-h帮助/-t测试命令行支持"),
    ]
    for no, name, desc in modules:
        row = table.add_row().cells
        row[0].text = no
        row[1].text = name
        p = row[2].paragraphs[0]
        if desc.startswith("【V2】"):
            blue_run(p, desc, size=10)
        else:
            black_run(p, desc, size=10)

    # === 2. V2.0 新特性详解 ===
    add_heading(doc, "2. V2.0 新特性详解", level=1)

    add_heading(doc, "2.1 UTF-8 BOM + 控制台编码", level=2)
    p = doc.add_paragraph()
    blue_run(p, "文件以 UTF-8 BOM (EF BB BF) 开头保存，确保编译器和编辑器正确识别中文。Windows下通过以下API设置控制台为UTF-8模式：", bold=True)
    add_code_block(doc, """SetConsoleOutputCP(65001);
SetConsoleCP(65001);
// 启用ANSI转义序列
HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
DWORD mode = 0;
GetConsoleMode(hOut, &mode);
mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
SetConsoleMode(hOut, mode);""")

    add_heading(doc, "2.2 ANSI彩色终端", level=2)
    p = doc.add_paragraph()
    blue_run(p, "定义了完整的ANSI颜色宏：", bold=True)
    add_code_block(doc, """#define C_RESET     "\\033[0m"
#define C_RED       "\\033[31m"   /* 错误/失败 */
#define C_GREEN     "\\033[32m"   /* 成功 */
#define C_YELLOW    "\\033[33m"   /* 警告 */
#define C_BLUE      "\\033[34m"   /* 信息 */
#define C_MAGENTA   "\\033[35m"   /* 标题 */
#define C_CYAN      "\\033[36m"   /* 边框/分隔 */
#define C_BOLD      "\\033[1m"    /* 粗体 */""")

    add_heading(doc, "2.3 动态数组（Dynamic Array）", level=2)
    p = doc.add_paragraph()
    black_run(p, "V1使用单链表存储数据，V2改用动态数组。动态数组通过宏实现类型通用：")
    add_code_block(doc, """typedef struct {
    Type *data;       // 数据指针
    int    count;     // 当前元素数
    int    capacity;  // 已分配容量
} Name##Array;
// 初始容量=4，扩容倍数=2""")
    blue_run(p, "优势：缓存友好（连续内存）、O(1)随机访问、无指针额外开销、内存局部性好。", bold=True)

    add_heading(doc, "2.4 统一错误码 CrmStatus", level=2)
    p = doc.add_paragraph()
    black_run(p, "V1中函数多为void返回或使用零散int值，错误处理不统一。V2定义了统一的错误码枚举：")
    add_code_block(doc, """typedef enum {
    CRM_OK = 0,              // 操作成功
    CRM_ERR_MEMORY,          // 内存分配失败
    CRM_ERR_NOT_FOUND,       // 未找到记录
    CRM_ERR_DUPLICATE,       // 重复记录
    CRM_ERR_INVALID_PARAM,   // 参数非法
    CRM_ERR_FILE_OPEN,       // 文件打开失败
    CRM_ERR_FILE_READ,       // 文件读取失败
    CRM_ERR_FILE_WRITE,      // 文件写入失败
    CRM_ERR_FILE_MAGIC,      // 魔数不匹配
    CRM_ERR_FILE_VERSION,    // 版本不兼容
    CRM_ERR_FULL,            // 容量已满
    CRM_ERR_EMPTY,           // 数据为空
    CRM_ERR_IO               // 通用I/O错误
} CrmStatus;""")

    add_heading(doc, "2.5 Magic Header 文件持久化", level=2)
    p = doc.add_paragraph()
    black_run(p, "V1使用'|'分隔的文本文件，V2升级为带魔数的二进制格式：")
    add_code_block(doc, """文件格式:
  [0..4]   Magic "CRMV2"    (5字节标识)
  [5]      版本号 uint8      (=2)
  [6..9]   保留字节
  [10..21] ID计数器(3个int)
  [22..33] 数组大小(3个int:客户/联系人/跟进)
  [34..]   依次写入三个数组的二进制数据""")
    blue_run(p, "加载时校验魔数\"CRMV2\"，如果不是该格式则返回CRM_ERR_FILE_MAGIC，防止读取错误文件或V1旧文件。", bold=True)

    add_heading(doc, "2.6 CLI命令行参数", level=2)
    add_code_block(doc, """crm.exe              启动交互菜单
crm.exe -h/--help    显示帮助信息
crm.exe -t/--test    运行内置18项自测""")

    # === 3. 系统架构 ===
    add_heading(doc, "3. 系统架构", level=1)
    add_code_block(doc, """┌─────────────────────────────────────────────┐
│  表示层 (UI Layer)                           │
│  ├─ ANSI彩色菜单  ├─ 帮助信息  ├─ 提示信息  │
├─────────────────────────────────────────────┤
│  业务逻辑层 (Business Layer)                 │
│  ├─ CRUD操作  ├─ 级联删除  ├─ 统计分析      │
│  └─ 输入校验  └─ ID自动生成                 │
├─────────────────────────────────────────────┤
│  数据层 (Data Layer)                         │
│  ├─ 动态数组(Customer/Contact/FollowUp)      │
│  └─ CRMV2二进制文件持久化                    │
└─────────────────────────────────────────────┘""")

    add_heading(doc, "3.1 上下文结构", level=2)
    p = doc.add_paragraph()
    black_run(p, "V2 使用单一全局上下文结构替代V1的10个分散全局指针：")
    add_code_block(doc, """typedef struct {
    custArray   customers;     // 客户动态数组
    contArray   contacts;      // 联系人动态数组
    folArray    followups;     // 跟进记录动态数组
    int         next_cust_id;  // 下一个客户ID
    int         next_cont_id;  // 下一个联系人ID
    int         next_fol_id;   // 下一个跟进ID
} CrmContext;""")

    # === 4. 数据结构 ===
    add_heading(doc, "4. 数据结构", level=1)

    add_heading(doc, "4.1 客户实体 Customer", level=2)
    add_code_block(doc, """typedef struct {
    int    id;                      // 客户ID，自增
    char   name[MAX_STR];           // 公司名称
    char   contact_person[MAX_STR]; // 首要联系人
    char   region[MAX_STR];         // 区域
    char   industry[MAX_STR];       // 行业
    char   status[MAX_STR];         // 状态:潜在/活跃/暂停/流失
    char   grade[MAX_STR];          // 等级:A/B/C/D
    char   phone[MAX_STR];          // 电话
    char   address[MAX_LONG];       // 地址
    char   create_time[MAX_STR];    // 创建时间
} Customer;""")

    add_heading(doc, "4.2 联系人实体 Contact【V2新增】", level=2)
    p = doc.add_paragraph()
    blue_run(p, "通过cust_id外键关联客户，支持一对多关系（一个客户多个联系人）")
    add_code_block(doc, """typedef struct {
    int    id;              // 联系人ID
    int    cust_id;         // 所属客户ID（外键）
    char   name[MAX_STR];   // 姓名
    char   title[MAX_STR];  // 职位
    char   phone[MAX_STR];  // 电话
    char   email[MAX_STR];  // 邮箱
    char   remark[MAX_LONG];// 备注
} Contact;""")

    add_heading(doc, "4.3 跟进记录实体 FollowUp【V2新增】", level=2)
    p = doc.add_paragraph()
    blue_run(p, "记录每次客户跟进详情，支持下次跟进日期提醒")
    add_code_block(doc, """typedef struct {
    int    id;                  // 跟进ID
    int    cust_id;             // 所属客户ID（外键）
    char   date[MAX_STR];       // 跟进日期
    char   method[MAX_STR];     // 方式:电话/拜访/邮件/微信
    char   content[MAX_LONG];   // 内容
    char   next_date[MAX_STR];  // 下次跟进日期
} FollowUp;""")

    # === 5. 错误码 ===
    add_heading(doc, "5. 统一错误码 CrmStatus", level=1)
    table = doc.add_table(rows=1, cols=3)
    table.style = "Light Grid Accent 1"
    hdr = table.rows[0].cells
    hdr[0].text = "错误码"
    hdr[1].text = "枚举值"
    hdr[2].text = "中文描述"
    errors = [
        ("0", "CRM_OK", "操作成功"),
        ("1", "CRM_ERR_MEMORY", "内存分配失败"),
        ("2", "CRM_ERR_NOT_FOUND", "未找到记录"),
        ("3", "CRM_ERR_DUPLICATE", "重复记录"),
        ("4", "CRM_ERR_INVALID_PARAM", "参数非法"),
        ("5", "CRM_ERR_FILE_OPEN", "文件打开失败"),
        ("6", "CRM_ERR_FILE_READ", "文件读取失败"),
        ("7", "CRM_ERR_FILE_WRITE", "文件写入失败"),
        ("8", "CRM_ERR_FILE_MAGIC", "文件格式错误(魔数不匹配)"),
        ("9", "CRM_ERR_FILE_VERSION", "文件版本不兼容"),
        ("10", "CRM_ERR_FULL", "容量已满"),
        ("11", "CRM_ERR_EMPTY", "数据为空"),
        ("12", "CRM_ERR_IO", "I/O错误"),
    ]
    for code, enum_val, desc in errors:
        row = table.add_row().cells
        row[0].text = code
        row[1].text = enum_val
        row[2].text = desc

    # === 6. 文件格式 ===
    add_heading(doc, "6. 文件持久化格式（CRMV2）", level=1)
    p = doc.add_paragraph()
    black_run(p, "持久化文件名为 ")
    blue_run(p, "crmv2.dat", bold=True)
    black_run(p, "，采用二进制格式存储。打开文件首先校验魔数\"CRMV2\"，如果不匹配立即返回错误。文件头部结构：")

    table = doc.add_table(rows=1, cols=4)
    table.style = "Light Grid Accent 1"
    hdr = table.rows[0].cells
    hdr[0].text = "偏移(字节)"
    hdr[1].text = "长度"
    hdr[2].text = "字段"
    hdr[3].text = "说明"
    fields = [
        ("0", "5", "Magic Header", "必须为\"CRMV2\""),
        ("5", "1", "Version", "版本号=2(uint8)"),
        ("6", "4", "Reserved", "保留字段"),
        ("10", "4", "next_cust_id", "下一个客户ID(int32)"),
        ("14", "4", "next_cont_id", "下一个联系人ID(int32)"),
        ("18", "4", "next_fol_id", "下一个跟进ID(int32)"),
        ("22", "4", "cust_count", "客户数组大小(int32)"),
        ("26", "4", "cont_count", "联系人数组大小(int32)"),
        ("30", "4", "fol_count", "跟进数组大小(int32)"),
        ("34", "N", "Customer[]", "客户数组二进制数据"),
        ("34+Cc*SzC", "M", "Contact[]", "联系人数组二进制数据"),
        ("...", "K", "FollowUp[]", "跟进数组二进制数据"),
    ]
    for off, ln, name, desc in fields:
        row = table.add_row().cells
        row[0].text = off
        row[1].text = ln
        row[2].text = name
        row[3].text = desc

    # === 7. 自测 ===
    add_heading(doc, "7. 内置自测套件（18项）", level=1)
    p = doc.add_paragraph()
    blue_run(p, "通过 crm.exe --test 运行，覆盖以下18项测试：", bold=True)
    tests = [
        ("T1", "test_init", "初始化上下文，验证初始状态为0"),
        ("T2", "test_add_customer", "添加客户成功，返回CRM_OK"),
        ("T3", "test_customer_count", "客户数量正确"),
        ("T4", "test_id_increment", "客户ID自增正确(1,2,3...)"),
        ("T5", "test_duplicate_phone", "重复电话返回DUPLICATE错误"),
        ("T6", "test_invalid_param", "NULL/空参数返回INVALID_PARAM"),
        ("T7", "test_update_customer", "更新客户信息，ID保持不变"),
        ("T8", "test_add_contact", "添加联系人并关联客户"),
        ("T9", "test_contact_count", "联系人数量统计正确"),
        ("T10", "test_add_followup", "添加跟进记录成功"),
        ("T11", "test_followup_count", "跟进数量统计正确"),
        ("T12", "test_cascade_delete", "删除客户级联删除联系人和跟进"),
        ("T13", "test_grade_stats", "等级统计A/B/C/D正确"),
        ("T14", "test_status_stats", "状态统计潜在/活跃/暂停/流失正确"),
        ("T15", "test_file_save", "保存文件成功"),
        ("T16", "test_file_load", "加载文件数据完整性验证"),
        ("T17", "test_magic_header", "错误魔数文件返回MAGIC错误"),
        ("T18", "test_not_found", "查找/删除不存在记录返回NOT_FOUND"),
    ]
    table = doc.add_table(rows=1, cols=3)
    table.style = "Light Grid Accent 1"
    hdr = table.rows[0].cells
    hdr[0].text = "编号"
    hdr[1].text = "测试函数"
    hdr[2].text = "验证目标"
    for tid, fn, desc in tests:
        row = table.add_row().cells
        row[0].text = tid
        row[1].text = fn
        row[2].text = desc

    p2 = doc.add_paragraph()
    blue_run(p2, "测试框架使用自定义断言宏 TEST_ASSERT / TEST_ASSERT_EQ_INT / TEST_ASSERT_EQ_STR，失败时输出红色[FAIL]，成功输出绿色[PASS]。", size=10)

    # === 8. 编译运行 ===
    add_heading(doc, "8. 编译与运行", level=1)
    add_heading(doc, "8.1 环境要求", level=2)
    doc.add_paragraph("操作系统：Windows 10/11 (64位)", style="List Bullet")
    doc.add_paragraph("编译器：D:/bin/gcc.exe (MinGW GCC, 支持C99)", style="List Bullet")
    doc.add_paragraph("标准：C99 (-std=c99)", style="List Bullet")

    add_heading(doc, "8.2 编译命令", level=2)
    blue_run(doc.add_paragraph(), "使用Makefile编译（推荐）：", bold=True)
    add_code_block(doc, "make            # 编译生成 crm.exe\nmake test       # 编译并运行自测\nmake run        # 编译并启动交互菜单\nmake clean      # 清理生成文件")

    black_run(doc.add_paragraph(), "或直接使用gcc命令：")
    add_code_block(doc, "D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o crm.exe main.c")

    p = doc.add_paragraph()
    blue_run(p, "★ 编译目标：零警告（-Wall -Wextra 全开，无任何warning）", bold=True)

    add_heading(doc, "8.3 Makefile配置", level=2)
    add_code_block(doc, """CC      = D:/bin/gcc.exe
CFLAGS  = -std=c99 -Wall -Wextra -O2
TARGET  = crm.exe
SRC     = main.c""")

    # === 9. 功能菜单 ===
    add_heading(doc, "9. 交互菜单功能", level=1)
    add_heading(doc, "9.1 主菜单", level=2)
    table = doc.add_table(rows=1, cols=2)
    table.style = "Light Grid Accent 1"
    hdr = table.rows[0].cells
    hdr[0].text = "选项"
    hdr[1].text = "功能"
    menus = [
        ("1", "客户列表 - 彩色显示所有客户，等级按A红/B黄/C绿着色"),
        ("2", "查看客户详情 - 显示客户信息+联系人列表+跟进记录"),
        ("3", "添加客户 - 交互式输入客户信息"),
        ("4", "删除客户 - 确认后删除，自动级联删除联系人和跟进"),
        ("5", "添加联系人 - 为指定客户添加联系人"),
        ("6", "添加跟进记录 - 为指定客户添加跟进，支持今日默认日期"),
        ("7", "数据统计 - 等级分布/状态分布/联系人TOP排行"),
        ("8", "保存数据 - 手动保存到crmv2.dat"),
        ("9", "删除联系人【V2】- 列出所有联系人并选择删除"),
        ("10", "删除跟进记录【V2】- 列出所有跟进并选择删除"),
        ("0", "保存并退出"),
    ]
    for opt, desc in menus:
        row = table.add_row().cells
        row[0].text = opt
        p = row[1].paragraphs[0]
        if "【V2】" in desc:
            blue_run(p, desc, size=10)
        else:
            black_run(p, desc, size=10)

    # === 10. 总结 ===
    add_heading(doc, "10. 总结", level=1)
    p = doc.add_paragraph()
    black_run(p, "CRM V2.0在V1链表版本基础上进行了全面的工程化升级：")
    doc.add_paragraph("动态数组替代链表：提升缓存性能，支持随机访问，代码更简洁", style="List Bullet")
    p = doc.add_paragraph(style="List Bullet")
    blue_run(p, "UTF-8全链路：从源文件BOM到控制台代码页，彻底解决中文乱码")
    doc.add_paragraph("彩色UI：ANSI转义序列打造专业级终端界面，重要信息一目了然", style="List Bullet")
    p = doc.add_paragraph(style="List Bullet")
    blue_run(p, "统一错误处理：CrmStatus枚举让错误处理标准化，不再静默失败")
    p = doc.add_paragraph(style="List Bullet")
    blue_run(p, "Magic Header持久化：二进制格式+魔数校验+版本号，防止数据损坏和版本混乱")
    p = doc.add_paragraph(style="List Bullet")
    blue_run(p, "18项自动化测试：覆盖核心功能，代码质量有保障，-Wall -Wextra零警告")
    p = doc.add_paragraph(style="List Bullet")
    blue_run(p, "级联删除：删除客户自动清理关联数据，保持数据库一致性")

    p2 = doc.add_paragraph()
    blue_run(p2, "V2.0代码行数约1350行，编译后crm.exe为独立可执行文件，无外部依赖，可直接部署使用。", bold=True)

    # 页脚信息
    doc.add_paragraph()
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    black_run(p, "─── CRM V2.0 | 软件公司客户关系管理系统 | 2026 ───", size=9)

    doc.save(V2_DOCX)
    print(f"[OK] V2.0文档已生成: {V2_DOCX}")
    print(f"[OK] V1备份文件: {V1_BACKUP}")


def backup_v1():
    """备份V1文档"""
    if os.path.exists(V1_DOCX) and not os.path.exists(V1_BACKUP):
        shutil.copy2(V1_DOCX, V1_BACKUP)
        print(f"[OK] 已备份V1文档: {V1_BACKUP}")
    elif os.path.exists(V1_BACKUP):
        print(f"[INFO] V1备份已存在: {V1_BACKUP}")


if __name__ == "__main__":
    backup_v1()
    generate_v2_doc()
