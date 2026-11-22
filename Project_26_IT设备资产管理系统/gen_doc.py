#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
gen_doc.py - IT设备资产管理系统 V2.0 文档生成脚本
将原docx文档更新为V2.0内容，新增/升级部分用蓝色字体高亮标注。
"""

from docx import Document
from docx.shared import Pt, RGBColor, Inches
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.oxml.ns import qn
from docx.oxml import OxmlElement
import os

DOCX_PATH = os.path.join(os.path.dirname(__file__), "Project_26_IT设备资产管理系统.docx")
BLUE = RGBColor(0x00, 0x70, 0xC0)
BLACK = RGBColor(0x00, 0x00, 0x00)
RED = RGBColor(0xC0, 0x00, 0x00)
GREEN = RGBColor(0x00, 0x80, 0x00)


def set_run_font(run, name="宋体", size=11, bold=False, color=BLACK):
    run.font.name = name
    run.font.size = Pt(size)
    run.font.bold = bold
    run.font.color.rgb = color
    rPr = run._element.get_or_add_rPr()
    rFonts = rPr.find(qn('w:rFonts'))
    if rFonts is None:
        rFonts = OxmlElement('w:rFonts')
        rPr.insert(0, rFonts)
    rFonts.set(qn('w:eastAsia'), name)
    rFonts.set(qn('w:ascii'), name)
    rFonts.set(qn('w:hAnsi'), name)


def add_h(doc, text, level=1, v2_new=False):
    p = doc.add_heading(text, level=level)
    for run in p.runs:
        set_run_font(run, "黑体", 16 - level * 2, bold=True,
                     color=BLUE if v2_new else BLACK)
    return p


def add_p(doc, text, bold=False, v2_new=False, size=11):
    p = doc.add_paragraph()
    run = p.add_run(text)
    set_run_font(run, "宋体", size, bold=bold, color=BLUE if v2_new else BLACK)
    return p


def add_bullet(doc, text, v2_new=False):
    p = doc.add_paragraph(style='List Bullet')
    run = p.add_run(text)
    set_run_font(run, "宋体", 11, color=BLUE if v2_new else BLACK)
    return p


def add_code(doc, text):
    p = doc.add_paragraph()
    run = p.add_run(text)
    set_run_font(run, "Consolas", 10, color=RGBColor(0x33, 0x33, 0x33))
    p.paragraph_format.left_indent = Inches(0.3)
    return p


def add_table(doc, headers, rows, v2_new_cols=None):
    table = doc.add_table(rows=1, cols=len(headers))
    table.style = 'Light Grid Accent 1'
    hdr_cells = table.rows[0].cells
    for i, h in enumerate(headers):
        hdr_cells[i].text = ""
        run = hdr_cells[i].paragraphs[0].add_run(h)
        is_new = v2_new_cols and i in v2_new_cols
        set_run_font(run, "黑体", 10, bold=True,
                     color=BLUE if is_new else BLACK)
    for row in rows:
        cells = table.add_row().cells
        for i, val in enumerate(row):
            cells[i].text = ""
            run = cells[i].paragraphs[0].add_run(str(val))
            is_new = v2_new_cols and i in v2_new_cols
            set_run_font(run, "宋体", 10, color=BLUE if is_new else BLACK)
    return table


def main():
    print("生成V2.0文档...")
    doc = Document()

    style = doc.styles['Normal']
    style.font.name = "宋体"
    style.font.size = Pt(11)
    style.element.rPr.rFonts.set(qn('w:eastAsia'), '宋体')

    # 标题
    title = doc.add_paragraph()
    title.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = title.add_run("IT设备资产管理系统 V2.0")
    set_run_font(run, "黑体", 22, bold=True, color=BLUE)

    sub = doc.add_paragraph()
    sub.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = sub.add_run("IT Equipment Asset Management System")
    set_run_font(run, "Times New Roman", 12, color=BLUE)

    doc.add_paragraph()

    info = doc.add_paragraph()
    info.alignment = WD_ALIGN_PARAGRAPH.CENTER
    for txt, color in [
        ("作者：HuiJunzhuo    ", BLACK),
        ("版本：V2.0    ", BLUE),
        ("日期：2026", BLACK),
    ]:
        run = info.add_run(txt)
        set_run_font(run, "宋体", 11, color=color)

    doc.add_paragraph()

    # V2.0升级亮点
    add_h(doc, "★ V2.0 升级亮点 ★", level=1, v2_new=True)
    add_p(doc, "V2.0相比V1进行了全面重写与架构升级，核心改进点如下（蓝色字体标注均为V2新增特性）：", v2_new=True)
    upgrades = [
        ("UTF-8 BOM编码 + SetConsoleOutputCP(65001)" , "彻底解决Windows控制台中文乱码问题"),
        ("ANSI彩色终端输出", "彩色状态提示，视觉层次清晰，交互体验升级"),
        ("CLI命令行参数", "支持 -h/--help 帮助、-t/--test 自测模式，可脚本化调用"),
        ("动态数组替代链表", "内存连续、缓存友好、随机访问O(1)，自动2倍扩容"),
        ("统一错误码枚举 AssetStatus", "规范化错误处理，10种错误码覆盖所有场景"),
        ("Magic Header \"ASSTV2\" 二进制持久化", "文件格式自描述，防误读，含版本号校验"),
        ("内置 ≥15项自测用例（实际18项）", "全功能自动化验证：CRUD/领用归还/统计/文件I/O/扩容/魔数校验/压力测试"),
        ("交互菜单重设计", "清晰的功能分区，操作流程引导友好"),
        ("代码精简聚焦", "核心代码~1160行，去除V1复杂的RBAC/软件/服务模块，聚焦资产管理核心"),
        ("中文注释", "全代码中文注释，模块/函数/关键逻辑均有说明"),
    ]
    for title_text, desc in upgrades:
        p = doc.add_paragraph()
        r1 = p.add_run("  ▸ ")
        set_run_font(r1, "宋体", 11, color=BLUE, bold=True)
        r2 = p.add_run(f"{title_text}：")
        set_run_font(r2, "黑体", 11, color=BLUE, bold=True)
        r3 = p.add_run(desc)
        set_run_font(r3, "宋体", 11, color=BLUE)

    doc.add_paragraph()

    # 1. 项目概述
    add_h(doc, "1. 项目概述", level=1)

    add_h(doc, "1.1 项目背景", level=2)
    add_p(doc, "针对企业信息化管理中IT设备资产登记混乱、状态追踪困难、统计不便等问题，"
              "本系统提供一个轻量级的命令行IT设备资产管理解决方案。V2.0采用C99标准开发，"
              "纯命令行交互，支持Windows/Linux跨平台运行，编译产物单一可执行文件，零依赖部署。")

    add_h(doc, "1.2 编译与运行环境", level=2)
    add_table(doc,
        ["项目", "规格/要求"],
        [
            ["操作系统", "Windows 10/11 64位 / Linux (跨平台C99)"],
            ["编译器", "GCC (D:\\bin\\gcc.exe 或 MinGW-w64 GCC)"],
            ["编译标准", "C99 (-std=c99)"],
            ["编译选项", "-Wall -Wextra -O2（开启严格警告，零警告通过）"],
            ["目标文件", "asset.exe"],
            ["数据文件", "assets.dat（Magic Header: \"ASSTV2\"）"],
            ["编码", "UTF-8 with BOM"],
        ]
    )

    add_p(doc, "编译命令：", bold=True)
    add_code(doc, "    D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o asset.exe main.c")
    add_p(doc, "或使用Makefile：", bold=True)
    add_code(doc, "    make          # 编译\n    make test     # 编译并运行自测\n    make run      # 编译并运行\n    make clean    # 清理产物")

    doc.add_paragraph()

    # 2. 命令行接口
    add_h(doc, "2. 命令行接口 (CLI)", level=1, v2_new=True)
    add_table(doc,
        ["参数", "说明"],
        [
            ["(无参数)", "启动交互菜单模式"],
            ["-h, --help", "显示帮助信息"],
            ["-t, --test", "运行内置自测套件（18项全功能测试）"],
        ],
        v2_new_cols=[0, 1]
    )
    add_p(doc, "示例：", v2_new=True)
    add_code(doc, "    asset.exe --help\n    asset.exe --test\n    asset.exe")

    doc.add_paragraph()

    # 3. 核心数据结构
    add_h(doc, "3. 核心数据结构", level=1)

    add_h(doc, "3.1 统一错误码 AssetStatus", level=2, v2_new=True)
    add_p(doc, "V2.0定义了统一的错误码枚举，所有业务函数均返回AssetStatus状态值，"
              "0表示成功，负数表示各类错误，便于统一错误处理：", v2_new=True)
    add_table(doc,
        ["错误码", "值", "含义"],
        [
            ["ASSET_OK", "0", "操作成功"],
            ["ASSET_ERR_NOT_FOUND", "-1", "设备未找到"],
            ["ASSET_ERR_DUPLICATE", "-2", "序列号重复"],
            ["ASSET_ERR_MEMORY", "-3", "内存分配失败"],
            ["ASSET_ERR_FILE_IO", "-4", "文件读写错误"],
            ["ASSET_ERR_INVALID_ARG", "-5", "参数无效"],
            ["ASSET_ERR_ALREADY_OUT", "-6", "设备已被领用"],
            ["ASSET_ERR_NOT_OUT", "-7", "设备未领用，无法归还"],
            ["ASSET_ERR_MAGIC", "-8", "文件魔数不匹配（格式错误）"],
            ["ASSET_ERR_FULL", "-9", "容量不足（内部使用，自动扩容）"],
        ],
        v2_new_cols=[0, 1, 2]
    )

    add_h(doc, "3.2 设备状态 DeviceStatus", level=2)
    add_table(doc,
        ["状态码", "值", "颜色", "含义"],
        [
            ["DEV_IDLE", "0", "绿色", "闲置：在库可用"],
            ["DEV_IN_USE", "1", "蓝色", "使用中：已被领用"],
            ["DEV_REPAIR", "2", "黄色", "维修中"],
            ["DEV_SCRAPPED", "3", "红色", "已报废"],
        ]
    )

    add_h(doc, "3.3 Asset 设备结构体", level=2)
    add_table(doc,
        ["字段", "类型", "说明", "V2新增"],
        [
            ["id", "int", "设备ID，自增唯一", ""],
            ["name", "char[64]", "设备名称", ""],
            ["type", "char[64]", "设备类型", ""],
            ["brand", "char[64]", "品牌", ""],
            ["model", "char[64]", "型号", ""],
            ["serial", "char[64]", "序列号(SN)，唯一约束", ""],
            ["purchase_date", "char[64]", "购置日期 YYYY-MM-DD", ""],
            ["price", "double", "购置价值(元)", ""],
            ["status", "int", "当前状态(DeviceStatus)", ""],
            ["location", "char[64]", "存放位置", ""],
            ["current_user", "char[64]", "当前领用人，空串表示未领用", "★"],
            ["remark", "char[256]", "备注信息", "★"],
        ],
        v2_new_cols=[3]
    )

    add_h(doc, "3.4 AssetArray 动态数组容器", level=2, v2_new=True)
    add_table(doc,
        ["字段", "类型", "说明"],
        [
            ["items", "Asset*", "设备数组指针（连续内存）"],
            ["count", "int", "当前设备数量"],
            ["capacity", "int", "当前分配容量（自动2倍扩容）"],
            ["next_id", "int", "下一个可用自增ID"],
        ],
        v2_new_cols=[0, 1]
    )
    add_p(doc, "V2.0采用动态数组替代V1的单链表，优势对比：", v2_new=True)
    add_bullet(doc, "内存连续，CPU缓存友好，遍历速度显著提升", v2_new=True)
    add_bullet(doc, "随机访问O(1)，按索引定位高效", v2_new=True)
    add_bullet(doc, "初始容量8，容量不足时按2倍扩容，避免频繁realloc", v2_new=True)
    add_bullet(doc, "更适合资产系统\"读多写少\"的使用场景", v2_new=True)

    doc.add_paragraph()

    # 4. 文件持久化格式
    add_h(doc, "4. 文件持久化 (Magic Header ASSTV2)", level=1, v2_new=True)
    add_p(doc, "V2.0使用自定义二进制格式持久化数据，文件头部包含Magic Header进行格式自检：", v2_new=True)
    add_table(doc,
        ["偏移", "长度", "内容", "说明"],
        [
            ["0", "6字节", "\"ASSTV2\"", "Magic Header魔数（不含'\\0'）"],
            ["6", "4字节", "version=2", "版本号(uint32)"],
            ["10", "4字节", "count", "设备数量(int32)"],
            ["14", "4字节", "next_id", "下一个ID(int32)"],
            ["18", "count*sizeof(Asset)", "Asset记录", "连续存储设备结构"],
        ],
        v2_new_cols=[0, 1, 2, 3]
    )
    add_p(doc, "安全设计：", bold=True, v2_new=True)
    add_bullet(doc, "读取前验证Magic Header，防止误读非本系统文件", v2_new=True)
    add_bullet(doc, "验证设备数量(0~100000)，防止损坏文件导致内存溢出", v2_new=True)
    add_bullet(doc, "读取失败时自动回滚初始化，保证程序不崩溃", v2_new=True)

    doc.add_paragraph()

    # 5. 功能模块
    add_h(doc, "5. 功能模块", level=1)

    modules = [
        ("设备CRUD", [
            "添加设备：名称/序列号必填，序列号唯一性校验，自动分配ID",
            "查询设备：支持按ID精确查找+关键词模糊搜索（名称/品牌/序列号/领用人/型号）",
            "修改设备：保持ID不变，可改所有字段（含状态），修改时仍校验序列号唯一性",
            "删除设备：二次确认（yes/no），防止误删",
            "浏览全部：彩色表格列出所有设备，状态用颜色区分",
        ]),
        ("领用/归还", [
            "领用登记：输入设备ID+领用人姓名，状态自动切换为\"使用中\"，记录领用人",
            "归还登记：输入设备ID，状态自动切换回\"闲置\"，清空领用人",
            "状态约束：已领用设备不可重复领用，未领用设备不可归还，报废设备不可领用",
        ]),
        ("统计报表", [
            "设备总数、闲置/使用中/维修中/报废各状态数量统计",
            "按状态统计资产价值（闲置价值、使用中价值、资产总价值）",
            "资产利用率自动计算（使用中/总数），并给出利用率评级（高/中/低）",
            "彩色边框报表，视觉效果清晰",
        ]),
    ]

    for mod_name, funcs in modules:
        add_h(doc, f"5.{modules.index((mod_name, funcs))+1} {mod_name}", level=2)
        for f in funcs:
            add_bullet(doc, f)

    doc.add_paragraph()

    # 6. ANSI彩色终端
    add_h(doc, "6. ANSI彩色终端", level=1, v2_new=True)
    add_table(doc,
        ["颜色", "用途", "ANSI码"],
        [
            ["绿色", "成功提示、闲置状态、闲置价值", "\\033[32m"],
            ["红色", "错误提示、已报废状态、低利用率警告", "\\033[31m"],
            ["黄色", "警告信息、维修中状态", "\\033[33m"],
            ["蓝色", "V2新特性标识、使用中状态、领用人、使用中价值", "\\033[34m"],
            ["青色", "标题边框、信息提示、菜单标题", "\\033[36m"],
            ["粗体白", "菜单边框、表头", "\\033[1;37m"],
            ["灰色", "辅助信息（序列号、位置等）", "\\033[2m"],
        ],
        v2_new_cols=[0, 1, 2]
    )
    add_p(doc, "Windows平台自动通过SetConsoleMode开启ENABLE_VIRTUAL_TERMINAL_PROCESSING，"
              "原生支持ANSI转义序列（Windows 10 1511+）。", v2_new=True)

    doc.add_paragraph()

    # 7. 内置自测
    add_h(doc, "7. 内置自测套件（18项）", level=1, v2_new=True)
    add_p(doc, "运行 asset.exe --test 即可自动执行全部18项测试，覆盖：", v2_new=True)
    add_table(doc,
        ["编号", "测试项", "覆盖模块"],
        [
            ["T01", "初始化空数组", "动态数组"],
            ["T02", "添加单个设备", "CRUD-Create"],
            ["T03", "按ID查找设备", "CRUD-Read"],
            ["T04", "ID自增验证", "ID管理"],
            ["T05", "重复序列号拒绝添加", "唯一性约束"],
            ["T06", "修改设备信息", "CRUD-Update"],
            ["T07", "删除设备", "CRUD-Delete"],
            ["T08", "删除不存在设备返回错误", "错误处理"],
            ["T09", "设备领用流程", "领用/归还"],
            ["T10", "已领用设备拒绝重复领用", "状态约束"],
            ["T11", "设备归还流程", "领用/归还"],
            ["T12", "未领用设备拒绝归还", "状态约束"],
            ["T13", "动态数组自动扩容", "动态数组"],
            ["T14", "文件保存并加载（Magic Header）", "文件I/O"],
            ["T15", "魔数错误返回ERR_MAGIC", "格式校验"],
            ["T16", "统计功能计算正确", "统计报表"],
            ["T17", "批量添加1000台设备（压力测试）", "性能/稳定性"],
            ["T18", "空名称/序列号拒绝添加", "参数校验"],
        ],
        v2_new_cols=[0, 1, 2]
    )
    add_p(doc, "自测结果以彩色PASS/FAIL标识，最终汇总通过/失败数量，全部通过返回值0。", v2_new=True)

    doc.add_paragraph()

    # 8. 交互菜单
    add_h(doc, "8. 交互菜单", level=1)
    add_p(doc, "无参数启动进入交互模式，主菜单如下：")
    add_code(doc,
        "  ============================================================\n"
        "       * IT设备资产管理系统 V2.0 *\n"
        "  ============================================================\n"
        "  当前共 N 台设备 [闲置:x] [使用:y] [维修:z] [报废:w]\n"
        "\n"
        "  ┌──────────────────────────────┐\n"
        "  │  1.  添加设备                 │\n"
        "  │  2.  查询设备                 │\n"
        "  │  3.  修改设备                 │\n"
        "  │  4.  删除设备                 │\n"
        "  │  5.  浏览全部设备             │\n"
        "  │  6.  领用登记                 │\n"
        "  │  7.  归还登记                 │\n"
        "  │  8.  统计报表                 │\n"
        "  │  0.  保存并退出               │\n"
        "  └──────────────────────────────┘\n"
    )

    doc.add_paragraph()

    # 9. 测试结果
    add_h(doc, "9. 编译与自测验证结果", level=1, v2_new=True)
    add_p(doc, "使用 D:\\bin\\gcc.exe -std=c99 -Wall -Wextra -O2 编译：", v2_new=True, bold=True)
    add_bullet(doc, "零警告、零错误通过编译", v2_new=True)
    add_p(doc, "运行 asset.exe --test 自测结果：", v2_new=True, bold=True)
    add_bullet(doc, "18项测试全部PASS，0项FAIL", v2_new=True)

    doc.add_paragraph()

    # 10. 文件清单
    add_h(doc, "10. 项目文件清单", level=1)
    add_table(doc,
        ["文件名", "说明", "版本"],
        [
            ["main.c", "主程序源代码（UTF-8 BOM，C99，约1160行）", "V2.0 全新"],
            ["Makefile", "GCC编译脚本", "V2.0 新增"],
            ["asset.exe", "编译产物（可执行文件）", "V2.0"],
            ["assets.dat", "数据持久化文件（首次运行自动生成）", "V2.0 新格式"],
            ["gen_doc.py", "本文档自动生成脚本", "V2.0 新增"],
            ["README.md", "项目说明", "V2.0 更新"],
            ["Project_26_IT设备资产管理系统.docx", "本文档（V2.0）", "V2.0 更新"],
            ["Project_26_IT设备资产管理系统_V1_backup.docx", "V1文档备份", "备份"],
        ]
    )

    doc.save(DOCX_PATH)
    print(f"文档已保存: {DOCX_PATH}")
    print("完成!")


if __name__ == "__main__":
    main()
