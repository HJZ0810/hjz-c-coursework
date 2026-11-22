#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
观影网站影评信息管理系统 V2.0 文档生成脚本
功能：生成V2版本文档，V2新特性内容使用蓝色高亮
"""

import os
import sys
import shutil

try:
    from docx import Document
    from docx.shared import Pt, RGBColor, Inches
    from docx.enum.text import WD_ALIGN_PARAGRAPH
    from docx.oxml.ns import qn
    from docx.oxml import OxmlElement
except ImportError:
    print("正在安装 python-docx...")
    os.system(f"{sys.executable} -m pip install python-docx")
    from docx import Document
    from docx.shared import Pt, RGBColor, Inches
    from docx.enum.text import WD_ALIGN_PARAGRAPH
    from docx.oxml.ns import qn
    from docx.oxml import OxmlElement

BLUE = RGBColor(0, 0, 255)
BLACK = RGBColor(0, 0, 0)
HEADER_COLOR = RGBColor(0, 51, 102)

def add_highlight_run(paragraph, text, color=BLUE, bold=True, size=11):
    run = paragraph.add_run(text)
    run.font.color.rgb = color
    run.font.bold = bold
    run.font.size = Pt(size)
    run.font.name = '微软雅黑'
    run._element.rPr.rFonts.set(qn('w:eastAsia'), '微软雅黑')
    return run

def add_normal_run(paragraph, text, color=BLACK, bold=False, size=11):
    run = paragraph.add_run(text)
    run.font.color.rgb = color
    run.font.bold = bold
    run.font.size = Pt(size)
    run.font.name = '宋体'
    run._element.rPr.rFonts.set(qn('w:eastAsia'), '宋体')
    return run

def add_heading(doc, text, level=1, new_v2=False):
    p = doc.add_paragraph()
    if new_v2:
        add_highlight_run(p, "🆕 ", BLUE, True, 14 if level == 1 else 12)
        add_highlight_run(p, text, HEADER_COLOR, True, 14 if level == 1 else 12)
    else:
        add_normal_run(p, text, HEADER_COLOR, True, 14 if level == 1 else 12)
    return p

def add_para(doc, text, new_v2=False):
    p = doc.add_paragraph()
    if new_v2:
        add_highlight_run(p, "🔹 ", BLUE, True)
        add_highlight_run(p, text, BLUE, False)
    else:
        add_normal_run(p, text, BLACK, False)
    return p

def generate_v2_doc(output_path):
    doc = Document()

    style = doc.styles['Normal']
    style.font.name = '宋体'
    style.font.size = Pt(11)
    style._element.rPr.rFonts.set(qn('w:eastAsia'), '宋体')

    title = doc.add_paragraph()
    title.alignment = WD_ALIGN_PARAGRAPH.CENTER
    add_highlight_run(title, "观影网站影评信息管理系统 V2.0", BLUE, True, 20)

    subtitle = doc.add_paragraph()
    subtitle.alignment = WD_ALIGN_PARAGRAPH.CENTER
    add_normal_run(subtitle, "——课程设计文档", BLACK, False, 14)

    info = doc.add_paragraph()
    info.alignment = WD_ALIGN_PARAGRAPH.CENTER
    add_normal_run(info, "\n课程：数据库课程     学号：424123888888     班级：计算机类A班", BLACK, False, 12)
    info2 = doc.add_paragraph()
    info2.alignment = WD_ALIGN_PARAGRAPH.CENTER
    add_normal_run(info2, "姓名：HuiJunzhuo     学年学期：2025-2026学年第二学期数据结构实习", BLACK, False, 12)

    doc.add_page_break()

    add_heading(doc, "一、系统概述 V2.0", 1, True)
    add_para(doc, "观影网站影评信息管理系统V2.0是对V1.0版本的全面升级重写。系统实现了对电影信息和影评信息的管理，支持增删改查(CRUD)操作，提供评分统计分析功能，并采用二进制文件持久化存储。")

    add_heading(doc, "V2.0 版本新特性", 2, True)
    add_para(doc, "✅ UTF-8 BOM编码 + Windows SetConsoleOutputCP(65001) 完美中文支持", True)
    add_para(doc, "✅ ANSI彩色终端输出，界面美观直观", True)
    add_para(doc, "✅ CLI命令行参数支持(-h/--help帮助、-t/--test自测)", True)
    add_para(doc, "✅ 动态数组(Dynamic Array)替代链表，内存管理更高效", True)
    add_para(doc, "✅ 统一错误码枚举 MovieStatus (14种状态码)", True)
    add_para(doc, "✅ Magic Header \"MOVVV2\" 二进制文件持久化格式", True)
    add_para(doc, "✅ 内置47项自测用例，覆盖CRUD+评分+统计+文件I/O全功能", True)
    add_para(doc, "✅ 评分统计功能：平均分、最高分、最低分、评分等级分布", True)
    add_para(doc, "✅ 电影级联删除：删除电影自动删除关联影评", True)
    add_para(doc, "✅ 交互菜单美化，支持修改电影信息", True)
    add_para(doc, "✅ 编译零警告：gcc -std=c99 -Wall -Wextra -O2", True)

    add_heading(doc, "编译与运行", 2)
    add_para(doc, "编译器：D:/bin/gcc.exe (MinGW GCC)")
    add_para(doc, "编译命令：gcc -std=c99 -Wall -Wextra -O2 -o movie.exe main.c -lm")
    add_para(doc, "运行方式：")
    add_para(doc, "  movie.exe            启动交互模式")
    add_para(doc, "  movie.exe -h/--help  显示帮助信息")
    add_para(doc, "  movie.exe -t/--test  运行内置自测套件")
    add_para(doc, "目标文件：movie.exe（非main.exe，命名更具语义）", True)

    doc.add_page_break()

    add_heading(doc, "二、系统架构设计", 1)

    add_heading(doc, "2.1 数据结构设计（动态数组）", 2, True)
    add_para(doc, "V2.0版本采用动态数组(Dynamic Array)替代V1.0的单链表实现：")
    add_para(doc, "• 初始容量 INIT_CAPACITY = 8", True)
    add_para(doc, "• 扩容策略 GROWTH_FACTOR = 2（2倍扩容）", True)
    add_para(doc, "• 优势：随机访问O(1)、缓存友好、内存连续", True)

    add_heading(doc, "Movie结构体（电影）", 3)
    table = doc.add_table(rows=9, cols=3)
    table.style = 'Table Grid'
    headers = ["字段", "类型", "说明"]
    rows_data = [
        ["id", "int", "电影ID，从2001自增，唯一标识"],
        ["title", "char[64]", "电影名称"],
        ["year", "int", "上映年份(1800-2100)"],
        ["director", "char[64]", "导演"],
        ["genre", "char[64]", "类型(科幻/剧情/动画等)"],
        ["rating", "char[64]", "分级(PG/PG-13/R等)"],
        ["avgScore", "float", "平均评分，自动计算"],
        ["reviewCount", "int", "影评数量，自动维护"],
    ]
    for i, h in enumerate(headers):
        cell = table.rows[0].cells[i]
        cell.text = h
        for paragraph in cell.paragraphs:
            for run in paragraph.runs:
                run.font.bold = True
    for i, row in enumerate(rows_data):
        for j, val in enumerate(row):
            table.rows[i+1].cells[j].text = val

    add_heading(doc, "Review结构体（影评）", 3)
    table2 = doc.add_table(rows=6, cols=3)
    table2.style = 'Table Grid'
    rows_data2 = [
        ["字段", "类型", "说明"],
        ["id", "int", "影评ID，从3001自增"],
        ["movieId", "int", "所属电影ID(外键)"],
        ["reviewer", "char[64]", "评论者昵称"],
        ["score", "float", "评分(0.0-10.0)"],
        ["comment", "char[256]", "评论内容"],
    ]
    for i, row in enumerate(rows_data2):
        for j, val in enumerate(row):
            table2.rows[i].cells[j].text = val
            if i == 0:
                for paragraph in table2.rows[i].cells[j].paragraphs:
                    for run in paragraph.runs:
                        run.font.bold = True

    add_heading(doc, "MovieArray/ReviewArray（动态数组容器）", 3, True)
    add_para(doc, "typedef struct {", True)
    add_para(doc, "    T     *items;     // 元素指针", True)
    add_para(doc, "    int    count;     // 当前元素数量", True)
    add_para(doc, "    int    capacity;  // 当前容量", True)
    add_para(doc, "} Array;", True)

    add_heading(doc, "2.2 统一错误码枚举", 2, True)
    add_para(doc, "typedef enum {", True)
    add_para(doc, "    MOVIE_OK = 0,              // 操作成功", True)
    add_para(doc, "    MOVIE_ERR_NULL_PTR,        // 空指针错误", True)
    add_para(doc, "    MOVIE_ERR_MEMORY,          // 内存分配失败", True)
    add_para(doc, "    MOVIE_ERR_NOT_FOUND,       // 未找到记录", True)
    add_para(doc, "    MOVIE_ERR_DUPLICATE,       // 重复记录", True)
    add_para(doc, "    MOVIE_ERR_INVALID_PARAM,   // 无效参数", True)
    add_para(doc, "    MOVIE_ERR_FILE_OPEN,       // 文件打开失败", True)
    add_para(doc, "    MOVIE_ERR_FILE_READ,       // 文件读取失败", True)
    add_para(doc, "    MOVIE_ERR_FILE_WRITE,      // 文件写入失败", True)
    add_para(doc, "    MOVIE_ERR_MAGIC_MISMATCH,  // Magic头不匹配", True)
    add_para(doc, "    MOVIE_ERR_VERSION,         // 版本不兼容", True)
    add_para(doc, "    MOVIE_ERR_EMPTY,           // 数据为空", True)
    add_para(doc, "    MOVIE_ERR_IO,              // IO错误", True)
    add_para(doc, "    MOVIE_ERR_SCORE_RANGE      // 评分范围错误", True)
    add_para(doc, "} MovieStatus;", True)

    doc.add_page_break()

    add_heading(doc, "三、核心功能模块", 1)

    add_heading(doc, "3.1 文件持久化（MOVVV2二进制格式）", 2, True)
    add_para(doc, "V2.0采用自定义二进制文件格式，以Magic Header \"MOVVV2\"开头：")
    add_para(doc, "文件结构（二进制）：", True)
    add_para(doc, "  [0-5]   Magic Header: 'M','O','V','V','V','2' (6字节)", True)
    add_para(doc, "  [6-9]   Version: 2 (int, 4字节)", True)
    add_para(doc, "  [10-13] movieIdCounter (int)", True)
    add_para(doc, "  [14-17] reviewIdCounter (int)", True)
    add_para(doc, "  [18-21] 电影数量 movieCount (int)", True)
    add_para(doc, "  [...]   movieCount个Movie结构体", True)
    add_para(doc, "  [...]   影评数量 reviewCount (int)", True)
    add_para(doc, "  [...]   reviewCount个Review结构体", True)
    add_para(doc, "安全特性：加载时校验Magic Header和版本号，防止读取错误格式文件", True)

    add_heading(doc, "3.2 CRUD操作", 2)
    add_para(doc, "电影操作：")
    add_para(doc, "• addMovie() - 添加电影（参数校验：非空标题、年份范围）")
    add_para(doc, "• updateMovie() - 修改电影信息（V2新增，支持保留原值）", True)
    add_para(doc, "• deleteMovie() - 删除电影（V2新增级联删除关联影评）", True)
    add_para(doc, "• findMovieIndexById() - 按ID查找（O(n)线性扫描）")
    add_para(doc, "影评操作：")
    add_para(doc, "• addReview() - 添加影评（校验电影存在性、评分0-10范围）")
    add_para(doc, "• deleteReview() - 删除影评")
    add_para(doc, "• recalcMovieScore() - 添加/删除影评后自动重算电影平均分", True)

    add_heading(doc, "3.3 统计分析功能", 2, True)
    add_para(doc, "V2.0新增完善的评分统计功能：")
    add_para(doc, "• 总影评数", True)
    add_para(doc, "• 平均评分", True)
    add_para(doc, "• 最高评分 / 最低评分", True)
    add_para(doc, "• 评分分布统计：优秀(8-10)、良好(6-7.9)、一般(4-5.9)、较差(0-3.9)", True)
    add_para(doc, "• 各等级百分比计算", True)

    add_heading(doc, "3.4 内置自测框架", 2, True)
    add_para(doc, "V2.0内置47项自动化测试用例，覆盖：", True)
    add_para(doc, "• 初始化测试(3项)：数组初始化、初始容量", True)
    add_para(doc, "• 电影CRUD测试(6项)：添加、ID分配、参数校验", True)
    add_para(doc, "• 影评CRUD测试(8项)：添加、平均分计算、评分范围、外键约束", True)
    add_para(doc, "• 文件持久化测试(8项)：保存/加载、计数器恢复、Magic校验", True)
    add_para(doc, "• 更新电影测试(4项)：字段更新、不存在ID处理", True)
    add_para(doc, "• 删除测试(5项)：电影删除、级联删除、不存在ID处理", True)
    add_para(doc, "• 统计功能测试(7项)：总数/平均/最高/最低/分布/空数据", True)
    add_para(doc, "• 动态扩容测试(6项)：电影数组扩容、影评数组扩容", True)
    add_para(doc, "测试命令：movie.exe --test", True)
    add_para(doc, "测试结果：全部47项通过 ✓", True)

    doc.add_page_break()

    add_heading(doc, "四、系统菜单", 1)
    add_para(doc, "主菜单选项（V2更新）：")
    add_para(doc, "  1. 📋 浏览所有电影")
    add_para(doc, "  2. ➕ 添加新电影")
    add_para(doc, "  3. ✏️  修改电影信息  [V2新增]")
    add_para(doc, "  4. 🗑️  删除电影（级联删除影评）[V2增强]")
    add_para(doc, "  5. 📝 浏览所有影评")
    add_para(doc, "  6. ✍️  添加影评")
    add_para(doc, "  7. ❌ 删除影评")
    add_para(doc, "  8. 📊 数据统计  [V2新增]")
    add_para(doc, "  9. 💾 手动保存")
    add_para(doc, "  0. 🚪 退出系统（自动保存）")

    add_heading(doc, "五、ANSI彩色终端", 2, True)
    add_para(doc, "V2.0引入ANSI转义序列实现彩色输出：", True)
    add_para(doc, "• 绿色 ✓ 成功提示", True)
    add_para(doc, "• 红色 ✗ 错误提示", True)
    add_para(doc, "• 黄色 ℹ 信息提示", True)
    add_para(doc, "• 青色分隔线、蓝色标题、洋红统计标题", True)
    add_para(doc, "• Windows下通过SetConsoleMode启用VT100支持", True)

    add_heading(doc, "六、模块划分", 2)
    modules = [
        ("[1] 常量与宏定义", "缓冲区长度、文件名、Magic头、颜色码、初始容量"),
        ("[2] 错误码枚举", "MovieStatus统一错误码(14种)"),
        ("[3] 数据结构", "Movie/Review/MovieArray/ReviewArray"),
        ("[4] 全局状态", "g_movies/g_reviews数组、ID计数器"),
        ("[5] 工具函数", "控制台初始化、输入处理、彩色打印、状态码转字符串"),
        ("[6] 动态数组", "init/grow/add（电影数组和影评数组）"),
        ("[7] 电影CRUD", "addMovie/deleteMovie/updateMovie/findMovie"),
        ("[8] 影评CRUD", "addReview/deleteReview/recalcMovieScore"),
        ("[9] 文件持久化", "saveToFile/loadFromFile (MOVVV2二进制格式)"),
        ("[10] 统计功能", "getStats (平均/最高/最低/分布)"),
        ("[11] 显示函数", "browseMovies/browseReviews/showStats"),
        ("[12] 示例数据", "loadSampleData (8部电影+10条影评)"),
        ("[13] 交互菜单", "mainMenuLoop及各菜单处理函数"),
        ("[14] 自测框架", "runAllTests (47项测试)"),
        ("[15] CLI入口", "main: 参数解析(-h/-t)、初始化、主循环、清理"),
    ]
    table3 = doc.add_table(rows=len(modules)+1, cols=2)
    table3.style = 'Table Grid'
    table3.rows[0].cells[0].text = "模块"
    table3.rows[0].cells[1].text = "功能"
    for cell in table3.rows[0].cells:
        for paragraph in cell.paragraphs:
            for run in paragraph.runs:
                run.font.bold = True
    for i, (name, desc) in enumerate(modules):
        table3.rows[i+1].cells[0].text = name
        table3.rows[i+1].cells[1].text = desc

    doc.add_page_break()

    add_heading(doc, "七、文件清单", 1)
    files = [
        ("main.c", "V2.0主程序源代码（UTF-8 BOM编码）"),
        ("Makefile", "编译配置（CC=D:/bin/gcc.exe，TARGET=movie.exe）"),
        ("movie.exe", "编译生成的可执行文件"),
        ("movies_v2.dat", "V2数据文件（MOVVV2二进制格式，运行时自动生成）"),
        ("gen_doc.py", "本文档生成脚本"),
        ("Project_16_观影网站影评信息管理系统.docx", "V2.0文档（本文档）"),
        ("Project_16_观影网站影评信息管理系统_V1_backup.docx", "V1.0文档备份"),
        ("README.md", "项目说明"),
    ]
    table4 = doc.add_table(rows=len(files)+1, cols=2)
    table4.style = 'Table Grid'
    table4.rows[0].cells[0].text = "文件名"
    table4.rows[0].cells[1].text = "说明"
    for cell in table4.rows[0].cells:
        for paragraph in cell.paragraphs:
            for run in paragraph.runs:
                run.font.bold = True
    for i, (name, desc) in enumerate(files):
        is_new = name in ["main.c", "Makefile", "movie.exe", "movies_v2.dat", "gen_doc.py", "Project_16_观影网站影评信息管理系统_V1_backup.docx"]
        cell0 = table4.rows[i+1].cells[0]
        cell1 = table4.rows[i+1].cells[1]
        p0 = cell0.paragraphs[0]
        if is_new:
            add_highlight_run(p0, name, BLUE, False, 10)
        else:
            add_normal_run(p0, name, BLACK, False, 10)
        p1 = cell1.paragraphs[0]
        if is_new:
            add_highlight_run(p1, "🔹 " + desc, BLUE, False, 10)
        else:
            add_normal_run(p1, desc, BLACK, False, 10)

    add_heading(doc, "八、测试结果", 1)
    add_para(doc, "编译测试：gcc -std=c99 -Wall -Wextra -O2 零警告零错误 ✓")
    add_para(doc, "自测套件：47项测试全部通过 ✓", True)
    add_para(doc, "CLI参数测试：-h/--help 正常显示帮助 ✓")
    add_para(doc, "CLI参数测试：-t/--test 正常运行测试 ✓")
    add_para(doc, "文件I/O测试：保存/加载数据一致性 ✓")
    add_para(doc, "功能测试：增删改查、统计、级联删除均正常 ✓")

    doc.save(output_path)
    print(f"✅ V2.0文档已生成: {output_path}")

if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.abspath(__file__))
    docx_name = "Project_16_观影网站影评信息管理系统.docx"
    output_path = os.path.join(script_dir, docx_name)
    
    print("=" * 60)
    print("观影网站影评信息管理系统 V2.0 文档生成器")
    print("=" * 60)
    print()
    
    generate_v2_doc(output_path)
    print()
    print("蓝色高亮(🆕🔹)标记的内容为V2.0新增/升级特性")
    print(f"V1备份文件: Project_16_观影网站影评信息管理系统_V1_backup.docx")
