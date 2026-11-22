#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
更新课程设计docx文档，添加V2.0蓝色高亮更新章节
"""

from docx import Document
from docx.shared import Pt, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
import os
import shutil

BLUE = RGBColor(0, 0, 255)
DARK_BLUE = RGBColor(0, 51, 153)

def add_blue_heading(doc, text, level=1):
    h = doc.add_heading(text, level=level)
    for run in h.runs:
        run.font.color.rgb = DARK_BLUE
    return h

def add_blue_para(doc, text, bold=False):
    p = doc.add_paragraph()
    run = p.add_run(text)
    run.font.color.rgb = BLUE
    if bold:
        run.font.bold = True
    return p

def add_blue_bullet(doc, text):
    p = doc.add_paragraph(style='List Bullet')
    run = p.add_run(text)
    run.font.color.rgb = BLUE
    return p

def add_blue_table(doc, headers, rows):
    table = doc.add_table(rows=1, cols=len(headers))
    table.style = 'Table Grid'
    hdr_cells = table.rows[0].cells
    for i, h in enumerate(headers):
        p = hdr_cells[i].paragraphs[0]
        run = p.add_run(h)
        run.font.bold = True
        run.font.color.rgb = DARK_BLUE
    for row_data in rows:
        row_cells = table.add_row().cells
        for i, val in enumerate(row_data):
            p = row_cells[i].paragraphs[0]
            run = p.add_run(str(val))
            run.font.color.rgb = BLUE
    return table

def main():
    docx_path = "Project_37_学校超市选址问题.docx"
    backup_path = "_V1_backup.docx"
    
    print("=" * 56)
    print("  学校超市选址问题 V2.0 Docx更新工具")
    print("=" * 56)
    print()
    
    if not os.path.exists(docx_path):
        print(f"[错误] 未找到文件: {docx_path}")
        return
    
    if not os.path.exists(backup_path):
        print(f"[备份] 创建V1备份: {backup_path}")
        shutil.copy2(docx_path, backup_path)
    else:
        print(f"[备份] V1备份已存在: {backup_path}")
    
    print(f"[加载] 打开文档: {docx_path}")
    doc = Document(docx_path)
    
    print(f"[添加] V2.0蓝色高亮更新章节...")
    doc.add_page_break()
    
    add_blue_heading(doc, "V2.0 版本更新（蓝色高亮）", level=1)
    
    add_blue_para(doc, "本节为V2.0版本相对于V1.0的全部更新内容，以蓝色字体标注。", bold=True)
    doc.add_paragraph()
    
    add_blue_heading(doc, "1. 编码与终端升级", level=2)
    add_blue_bullet(doc, "源文件采用UTF-8 BOM编码（首字节0xEF 0xBB 0xBF）")
    add_blue_bullet(doc, "Windows下调用SetConsoleOutputCP(65001)设置控制台UTF-8输出")
    add_blue_bullet(doc, "启用VT100虚拟终端模式，支持ANSI彩色输出")
    add_blue_bullet(doc, "彩色终端：红色错误、绿色成功、黄色提示、蓝色标题、洋红菜单")
    
    add_blue_heading(doc, "2. 命令行接口（CLI）", level=2)
    add_blue_bullet(doc, "新增-h/--help参数：显示帮助信息")
    add_blue_bullet(doc, "新增-t/--test参数：运行内置38项自测用例")
    add_blue_bullet(doc, "无参数时启动交互式菜单")
    add_blue_bullet(doc, "可执行文件名从main.exe改为loc.exe")
    
    add_blue_heading(doc, "3. 算法增强", level=2)
    add_blue_para(doc, "V2.0保留Floyd-Warshall全源最短路径算法（O(n^3)），并新增中心点选址：")
    add_blue_bullet(doc, "中位点(Median)：最小化加权距离总和 cost(i)=Σdist[i][j]×weight[j]")
    add_blue_bullet(doc, "中心点(Center)：最小化最大偏心距 ecc(i)=max{dist[i][j]}")
    add_blue_bullet(doc, "选址结果显示全部地点排名，标注最优解")
    
    add_blue_heading(doc, "4. 动态内存管理", level=2)
    add_blue_para(doc, "彻底替换V1.0的静态数组（MAX=50）为动态数组：")
    add_blue_bullet(doc, "初始容量：地点8个、道路16条")
    add_blue_bullet(doc, "容量不足时自动realloc翻倍扩容")
    add_blue_bullet(doc, "邻接矩阵、距离矩阵、next_hop矩阵同步扩容")
    add_blue_bullet(doc, "graph_free()完全释放所有内存，无泄漏")
    add_blue_bullet(doc, "突破50个地点限制，支持任意规模校园图")
    
    add_blue_heading(doc, "5. 统一错误码", level=2)
    add_blue_para(doc, "定义LocStatus枚举类型，替代V1.0的零散int返回值：")
    err_table = add_blue_table(doc,
        ["错误码", "值", "含义"],
        [
            ["LOC_OK", "0", "成功"],
            ["LOC_ERR_MEMORY", "-1", "内存分配失败"],
            ["LOC_ERR_FILE", "-2", "文件操作失败"],
            ["LOC_ERR_MAGIC", "-3", "魔数校验失败"],
            ["LOC_ERR_VERSION", "-4", "版本不兼容"],
            ["LOC_ERR_FULL", "-5", "容量已满"],
            ["LOC_ERR_NOT_FOUND", "-6", "未找到"],
            ["LOC_ERR_INVALID_PARAM", "-7", "参数无效"],
            ["LOC_ERR_IO", "-8", "I/O错误"],
            ["LOC_ERR_DISCONNECTED", "-9", "图不连通"],
        ]
    )
    doc.add_paragraph()
    
    add_blue_heading(doc, "6. 文件持久化升级（LOCV2格式）", level=2)
    add_blue_para(doc, "V1.0的二进制格式无标识、无版本，V2.0设计为带魔数头的结构化格式：")
    add_blue_bullet(doc, "[0-4] 'LOCV2' 魔数标识（Magic Header）")
    add_blue_bullet(doc, "[5-8] 版本号（int = 2）")
    add_blue_bullet(doc, "[9-12] 地点数量（int n）")
    add_blue_bullet(doc, "[13-16] 道路数量（int m）")
    add_blue_bullet(doc, "地点数据：id(4B)+name(48B)+weight(4B)+type(4B)+active(4B)")
    add_blue_bullet(doc, "道路数据：from(4B)+to(4B)+distance(4B)")
    add_blue_bullet(doc, "加载时验证魔数和版本，不兼容文件拒绝加载")
    add_blue_bullet(doc, "默认数据文件名为campus.locv2")
    
    add_blue_heading(doc, "7. 内置自测（38项全自动化）", level=2)
    add_blue_para(doc, "V2.0内置38项自动化测试，覆盖10个测试组，loc.exe --test一键运行：")
    test_table = add_blue_table(doc,
        ["测试组", "测试内容", "数量"],
        [
            ["图初始化", "graph_init/初始计数/初始容量", "3"],
            ["添加地点", "添加A/B/C栋/数量/ID分配", "5"],
            ["添加道路", "A-B/B-C/A-C/道路计数", "4"],
            ["Floyd最短路", "计算/自环/直达/绕路/连通性", "6"],
            ["中位点选址", "计算成功/正确性/数值/正值", "4"],
            ["中心点选址", "计算成功/偏心距/最优ID", "3"],
            ["文件持久化", "保存/加载/数量一致/距离正确", "5"],
            ["默认校园", "12建筑/28道路/Floyd/连通性", "4"],
            ["动态扩容", "添加50点/总数62/容量增长", "3"],
            ["边界条件", "无边不连通检测", "1"],
            ["总计", "", "38"],
        ]
    )
    doc.add_paragraph()
    
    add_blue_heading(doc, "8. 编译与构建", level=2)
    add_blue_bullet(doc, "编译器：D:/bin/gcc.exe (MinGW-w64 GCC 14.2.0)")
    add_blue_bullet(doc, "编译标准：-std=c99 -Wall -Wextra -O2")
    add_blue_bullet(doc, "零警告编译（-Wall -Wextra无任何warning）")
    add_blue_bullet(doc, "提供Makefile：make/make test/make run/make clean")
    
    add_blue_heading(doc, "9. 源代码结构", level=2)
    add_blue_para(doc, "main.c共约1125行，分为12个部分：")
    add_blue_bullet(doc, "第一部分：常量定义与LocStatus错误码枚举")
    add_blue_bullet(doc, "第二部分：Location/Road/Graph数据结构（动态数组）")
    add_blue_bullet(doc, "第三部分：终端工具（UTF-8初始化、彩色输出）")
    add_blue_bullet(doc, "第四部分：图内存管理（初始化/扩容/添加/释放）")
    add_blue_bullet(doc, "第五部分：Floyd-Warshall算法与连通性检测")
    add_blue_bullet(doc, "第六部分：中位点+中心点双选址算法")
    add_blue_bullet(doc, "第七部分：LOCV2文件保存/加载")
    add_blue_bullet(doc, "第八部分：默认校园数据（12建筑28道路）")
    add_blue_bullet(doc, "第九部分：彩色显示函数（地点/道路/矩阵/选址结果）")
    add_blue_bullet(doc, "第十部分：交互式菜单系统")
    add_blue_bullet(doc, "第十一部分：run_self_test() 38项自测")
    add_blue_bullet(doc, "第十二部分：main() CLI参数解析与模式分发")
    
    add_blue_heading(doc, "10. V2.0文件清单", level=2)
    file_table = add_blue_table(doc,
        ["文件名", "说明"],
        [
            ["main.c", "V2.0完整源代码（~1125行）"],
            ["Makefile", "GCC编译配置（CC=D:/bin/gcc.exe）"],
            ["loc.exe", "编译后的可执行文件（支持CLI参数）"],
            ["campus.locv2", "数据持久化文件（自动生成）"],
            ["gen_doc.py", "README文档生成脚本"],
            ["update_docx.py", "本docx更新脚本"],
            ["README.md", "项目说明文档"],
            ["_V1_backup.docx", "V1.0原文档备份"],
            ["_V1_backup_notes.txt", "V1备份说明"],
            ["_code_structure.txt", "代码结构说明"],
        ]
    )
    doc.add_paragraph()
    
    add_blue_para(doc, "—— V2.0更新章节结束 ——", bold=True)
    
    print(f"[保存] 写入更新后的文档: {docx_path}")
    doc.save(docx_path)
    
    print()
    print("[√] Docx更新完成！")
    print()
    print("已完成:")
    print(f"  - V1备份: {backup_path}")
    print(f"  - 原文档已更新，添加蓝色高亮V2.0章节")
    print()

if __name__ == "__main__":
    main()
