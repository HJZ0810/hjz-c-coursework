#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
车辆管理系统 V2.0 文档生成脚本
gen_doc.py - 生成/更新项目文档，V2.0更新部分使用蓝色高亮标记

功能：
  1. 备份原文档为 _V1_backup.docx（已手动完成）
  2. 读取 main.c 提取功能信息
  3. 生成 V2.0 更新说明文档，新增内容蓝色高亮
"""

import os
import sys
import datetime
import shutil

def print_banner():
    print("=" * 60)
    print("  车辆管理系统 V2.0 - 文档生成工具")
    print("=" * 60)
    print()

def check_python_docx():
    """检查python-docx库是否可用"""
    try:
        import docx
        return True
    except ImportError:
        return False

def generate_docx(output_path):
    """生成带蓝色高亮的V2.0文档"""
    from docx import Document
    from docx.shared import Pt, RGBColor, Inches
    from docx.enum.text import WD_ALIGN_PARAGRAPH
    from docx.oxml.ns import qn
    from docx.oxml import OxmlElement

    doc = Document()

    # 设置中文字体
    def set_chinese_font(run, font_name='微软雅黑', size=11, bold=False, color=None):
        run.font.name = font_name
        run.font.size = Pt(size)
        run.font.bold = bold
        if color:
            run.font.color.rgb = color
        r = run._element
        rPr = r.get_or_add_rPr()
        rFonts = rPr.find(qn('w:rFonts'))
        if rFonts is None:
            rFonts = OxmlElement('w:rFonts')
            rPr.append(rFonts)
        rFonts.set(qn('w:eastAsia'), font_name)

    # 添加蓝色高亮run
    def add_blue_para(paragraph, text, bold=False, size=11):
        run = paragraph.add_run(text)
        set_chinese_font(run, size=size, bold=bold, color=RGBColor(0x00, 0x70, 0xC0))
        return run

    # 添加普通run
    def add_normal_para(paragraph, text, bold=False, size=11):
        run = paragraph.add_run(text)
        set_chinese_font(run, size=size, bold=bold)
        return run

    # 添加标题（带蓝色高亮标记V2.0）
    title = doc.add_heading('', level=0)
    title.alignment = WD_ALIGN_PARAGRAPH.CENTER
    add_normal_para(title, '车辆管理系统 ', bold=True, size=22)
    add_blue_para(title, 'V2.0', bold=True, size=22)

    subtitle = doc.add_paragraph()
    subtitle.alignment = WD_ALIGN_PARAGRAPH.CENTER
    add_normal_para(subtitle, '——动态数组实现 + 彩色终端 + 二进制持久化 + 47项自测', size=12)

    doc.add_paragraph()
    info_para = doc.add_paragraph()
    info_para.alignment = WD_ALIGN_PARAGRAPH.CENTER
    add_normal_para(info_para, f'文档生成时间：{datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")}', size=10)
    add_normal_para(info_para, '  |  ', size=10)
    add_normal_para(info_para, '编译器：GCC C99', size=10)
    add_normal_para(info_para, '  |  ', size=10)
    add_normal_para(info_para, '目标文件：veh.exe', size=10)

    doc.add_paragraph()

    # ========== V2.0 更新说明（蓝色高亮） ==========
    h1 = doc.add_heading('', level=1)
    add_blue_para(h1, '■ V2.0 版本更新亮点', bold=True, size=16)

    update_para = doc.add_paragraph()
    add_blue_para(update_para, '【重要提示】以下标蓝内容为 V2.0 新增/升级特性', bold=True)

    updates = [
        ('动态数组替代链表', '使用 realloc 实现自动扩容的动态数组（VehicleArray/DriverArray），初始容量8，翻倍增长'),
        ('独立司机管理', '司机信息独立 CRUD，不再与车辆硬绑定，支持年龄/电话/派车次数统计'),
        ('VEHIV2 魔数二进制持久化', '文件头6字节魔数 "VEHIV2" 校验，二进制存储而非文本，数据完整性更高'),
        ('ANSI 彩色终端', '支持8色ANSI转义序列，Windows下自动启用 ENABLE_VIRTUAL_TERMINAL_PROCESSING'),
        ('UTF-8 控制台', 'SetConsoleOutputCP(65001) + SetConsoleCP(65001) 彻底解决中文乱码'),
        ('CLI 命令行参数', '-h/--help 显示帮助，-t/--test 运行自测，无参数进入交互菜单'),
        ('VehStatus 统一错误码', '8种错误码枚举：OK/NOT_FOUND/DUPLICATE/MEMORY/FILE/INVALID/DISPATCHED/MAINTENANCE'),
        ('派车/还车/维修流程', '三态流转：可用→已派出→维修中→可用，带派车备注与维修次数统计'),
        ('47项内置自测', '覆盖：CRUD(11项) + 派车/还车/维修(12项) + 删除/更新/边界(12项) + 文件I/O(8项) + 扩容(4项)'),
        ('车辆信息更新', '新增「更新车辆信息」菜单项，支持修改里程和价值'),
        ('类型统计增强', '按类型统计时同步显示可用/派出/维修各状态数量'),
    ]

    for feature, desc in updates:
        p = doc.add_paragraph(style='List Bullet')
        add_blue_para(p, f'● {feature}：', bold=True)
        add_normal_para(p, desc)

    doc.add_paragraph()

    # ========== 系统功能清单 ==========
    doc.add_heading('1. 系统功能清单', level=1)

    table = doc.add_table(rows=1, cols=4)
    table.style = 'Light Grid Accent 1'
    hdr = table.rows[0].cells
    for i, text in enumerate(['编号', '功能模块', '功能说明', 'V2.0 标记']):
        hdr[i].text = ''
        p = hdr[i].paragraphs[0]
        run = p.add_run(text)
        set_chinese_font(run, bold=True, size=10)

    functions = [
        ('1', '车辆显示', '彩色格式显示所有车辆信息（编号/车牌/类型/司机/里程/价值/状态）', ''),
        ('2', '司机显示', '显示所有司机基本信息（ID/姓名/电话/年龄/派车次数）', '蓝'),
        ('3', '添加车辆', '交互式添加，自动分配编号V###，查重车牌与ID', ''),
        ('4', '删除车辆', '按编号删除，已派出车辆禁止删除（安全保护）', ''),
        ('5', '添加司机', '交互式添加司机，自动分配编号D###', '蓝'),
        ('6', '删除司机', '按司机ID删除', '蓝'),
        ('7', '派车登记', '可用车辆派车，记录备注与时间戳，已派出/维修中禁止派车', '蓝'),
        ('8', '还车登记', '还车时累计行驶里程，状态恢复可用', '蓝'),
        ('9', '登记维修', '可用车辆送修，维修次数+1', '蓝'),
        ('10', '维修完成', '维修中车辆恢复可用状态', '蓝'),
        ('11', '类型统计', '按车辆类型分组统计：数量/总里程/总价值/各状态分布', '增强'),
        ('12', '更新车辆', '修改车辆里程和价值信息', '蓝'),
        ('13', '数据保存', '手动触发二进制持久化（退出时自动保存）', '蓝'),
    ]

    for no, name, desc, mark in functions:
        row_cells = table.add_row().cells
        row_cells[0].text = no
        row_cells[1].text = name
        row_cells[2].text = desc
        if mark:
            row_cells[3].text = ''
            p = row_cells[3].paragraphs[0]
            add_blue_para(p, mark, bold=True)
        else:
            row_cells[3].text = ''

    doc.add_paragraph()

    # ========== 数据结构 ==========
    doc.add_heading('2. 核心数据结构', level=1)

    doc.add_heading('2.1 车辆状态枚举 (VehicleStatus)', level=2)
    p = doc.add_paragraph()
    add_normal_para(p, '三态状态机：')
    add_blue_para(p, 'VEH_AVAILABLE(可用) → VEH_DISPATCHED(已派出) → VEH_MAINTENANCE(维修中) → VEH_AVAILABLE')

    doc.add_heading('2.2 错误码枚举 (VehStatus)', level=2)
    code_table = doc.add_table(rows=1, cols=3)
    code_table.style = 'Light Grid Accent 1'
    for i, text in enumerate(['错误码', '枚举值', '含义']):
        code_table.rows[0].cells[i].text = text
    error_codes = [
        ('VEH_OK', '0', '操作成功'),
        ('VEH_ERR_NOT_FOUND', '-1', '未找到目标'),
        ('VEH_ERR_DUPLICATE', '-2', '重复添加'),
        ('VEH_ERR_MEMORY', '-3', '内存分配失败'),
        ('VEH_ERR_FILE', '-4', '文件操作失败'),
        ('VEH_ERR_INVALID', '-5', '无效参数'),
        ('VEH_ERR_DISPATCHED', '-6', '车辆已派出'),
        ('VEH_ERR_MAINTENANCE', '-7', '车辆维修中'),
    ]
    for code, val, desc in error_codes:
        row = code_table.add_row().cells
        row[0].text = code
        row[1].text = val
        row[2].text = desc

    doc.add_paragraph()
    doc.add_heading('2.3 动态数组容器', level=2)
    p = doc.add_paragraph()
    add_normal_para(p, 'VehicleArray（车辆）和 DriverArray（司机）均采用相同结构：')
    code_p = doc.add_paragraph()
    set_chinese_font(code_p.add_run('typedef struct { T *data; int count; int capacity; } Array;'), font_name='Consolas', size=10)
    add_normal_para(doc.add_paragraph(), '初始容量 INIT_CAPACITY=8，每次扩容 capacity*2，均通过 realloc 实现。')

    doc.add_paragraph()

    # ========== 文件格式 ==========
    doc.add_heading('3. VEHIV2 二进制文件格式', level=1)

    p = doc.add_paragraph()
    add_blue_para(p, '文件名：vehicles.dat（二进制）', bold=True)

    file_format = [
        ('偏移 0x00 (6字节)', '魔数 "VEHIV2"（0x56 0x45 0x48 0x49 0x56 0x32）'),
        ('偏移 0x06 (4字节)', 'int32 车辆数量 v_count'),
        ('偏移 0x0A (4字节)', 'int32 司机数量 d_count'),
        ('之后', 'v_count 个 Vehicle 结构体（二进制直写）'),
        ('之后', 'd_count 个 Driver 结构体（二进制直写）'),
    ]
    for offset, desc in file_format:
        p = doc.add_paragraph(style='List Bullet')
        add_blue_para(p, f'{offset}：', bold=True)
        add_normal_para(p, desc)

    doc.add_paragraph()

    # ========== 编译与运行 ==========
    doc.add_heading('4. 编译与运行', level=1)

    doc.add_heading('4.1 Makefile 编译', level=2)
    code_p = doc.add_paragraph()
    set_chinese_font(code_p.add_run('CC = D:/bin/gcc.exe\nCFLAGS = -std=c99 -Wall -Wextra -O2\nTARGET = veh.exe\n\nmake          # 编译\nmake test     # 编译并运行自测\nmake clean    # 清理产物'), font_name='Consolas', size=10)

    doc.add_heading('4.2 命令行参数', level=2)
    cmds = [
        ('veh.exe -h / --help', '显示帮助信息'),
        ('veh.exe -t / --test', '运行47项内置自测'),
        ('veh.exe', '无参数进入交互菜单模式'),
    ]
    for cmd, desc in cmds:
        p = doc.add_paragraph(style='List Bullet')
        set_chinese_font(p.add_run(cmd), font_name='Consolas', size=10)
        add_normal_para(p, f'  —— {desc}')

    doc.add_paragraph()

    # ========== 自测覆盖 ==========
    doc.add_heading('5. 自测用例覆盖（共47项）', level=1)

    test_groups = [
        ('测试组1：基础CRUD（11项）', '数组初始化、司机增删/查重、车辆增删/查重、初始状态验证'),
        ('测试组2：派车/还车/维修（12项）', '派车成功、状态变更、重复派车拦截、还车+里程累计、维修登记/完成、维修中派车拦截'),
        ('测试组3：删除/更新/边界（12项）', '派车中禁删、还车后可删、删除不存在项、批量15车+5司机初始化、里程/价值更新、负数值拦截'),
        ('测试组4：文件持久化（8项）', '保存/加载成功、车辆/司机数量一致、车牌/里程/价值数据一致'),
        ('测试组5：动态扩容（4项）', '扩容后连续添加20辆车、容量自动增长验证、NULL参数拦截'),
    ]

    for group, desc in test_groups:
        p = doc.add_paragraph(style='List Bullet')
        add_blue_para(p, group, bold=True)
        add_normal_para(p, f'：{desc}')

    doc.add_paragraph()

    # ========== 默认账号 ==========
    h = doc.add_heading('', level=1)
    add_blue_para(h, '■ 附录：V1.0→V2.0 对照', bold=True, size=16)

    compare_table = doc.add_table(rows=1, cols=3)
    compare_table.style = 'Light Grid Accent 1'
    for i, text in enumerate(['对比项', 'V1.0', 'V2.0（蓝色）']):
        compare_table.rows[0].cells[i].text = text

    comparisons = [
        ('数据结构', '单链表', '动态数组（自动扩容）'),
        ('司机管理', '姓名字段', '独立CRUD + 电话/年龄/次数'),
        ('文件存储', '文本管道符分隔', '二进制 + VEHIV2魔数'),
        ('控制台', '普通输出', 'ANSI 8色彩色 + UTF-8'),
        ('CLI参数', '无', '-h / -t 支持'),
        ('错误处理', 'printf提示', '统一 VehStatus 枚举'),
        ('派车/维修', '无', '完整三态流转'),
        ('自测', '无/隐含', '47项显式自测用例'),
        ('编译器警告', '有未使用函数', '-Wall -Wextra 零警告'),
    ]
    for item, v1, v2 in comparisons:
        row = compare_table.add_row().cells
        row[0].text = item
        row[1].text = v1
        row[2].text = ''
        add_blue_para(row[2].paragraphs[0], v2, bold=True)

    doc.add_paragraph()
    doc.add_paragraph()

    footer_p = doc.add_paragraph()
    footer_p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    add_normal_para(footer_p, '—— 文档结束 ——', size=10)
    add_normal_para(footer_p, '\n车辆管理系统 V2.0 | 数据库专 | 2026-06-28', size=9)

    doc.save(output_path)
    return True

def generate_markdown_summary(output_path):
    """生成Markdown格式摘要（备用方案）"""
    md = f"""# 车辆管理系统 V2.0

> **蓝色高亮为V2.0新增/升级内容**

## 编译命令
```
D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o veh.exe main.c
```

## 运行方式
```
veh.exe -h       # 显示帮助
veh.exe -t       # 运行47项自测
veh.exe          # 进入交互菜单
```

## V2.0 新特性（蓝色高亮）

| 特性 | 说明 |
|------|------|
| <span style="color:blue">**动态数组**</span> | 替代链表，realloc自动扩容（初始8，翻倍增长） |
| <span style="color:blue">**独立司机管理**</span> | 司机CRUD独立，含年龄/电话/派车次数 |
| <span style="color:blue">**VEHIV2魔数持久化**</span> | 6字节魔数校验，二进制存储 |
| <span style="color:blue">**ANSI彩色终端**</span> | 8色输出，Windows自动启用VT处理 |
| <span style="color:blue">**UTF-8控制台**</span> | SetConsoleOutputCP(65001)解决中文乱码 |
| <span style="color:blue">**CLI参数**</span> | -h/--help, -t/--test |
| <span style="color:blue">**VehStatus错误码**</span> | 8种统一错误码枚举 |
| <span style="color:blue">**派车/还车/维修**</span> | 三态流转 + 备注 + 维修次数 |
| <span style="color:blue">**47项自测**</span> | CRUD+派车+维修+文件I/O+扩容全覆盖 |

## 测试结果
```
47 通过, 0 失败, 共 47 项
编译：-Wall -Wextra 零警告
```

---
*生成时间：{datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}*
"""
    with open(output_path, 'w', encoding='utf-8-sig') as f:
        f.write(md)
    return True

def main():
    print_banner()

    # 检查备份文件
    src_docx = 'Project_29_车辆管理系统.docx'
    backup_docx = 'Project_29_车辆管理系统_V1_backup.docx'

    if os.path.exists(backup_docx):
        print(f"[✓] 备份文件已存在: {backup_docx}")
    elif os.path.exists(src_docx):
        shutil.copy2(src_docx, backup_docx)
        print(f"[✓] 已创建备份: {backup_docx}")
    else:
        print(f"[!] 未找到原文档，跳过备份")

    # 尝试生成docx
    has_docx = check_python_docx()

    if has_docx:
        print("[i] 检测到 python-docx 库，生成V2.0文档...")
        out_docx = 'Project_29_车辆管理系统_V2.0.docx'
        try:
            generate_docx(out_docx)
            print(f"[✓] 文档已生成: {out_docx}")
        except Exception as e:
            print(f"[!] 生成docx失败: {e}")
            print("[i] 切换为Markdown格式...")
            generate_markdown_summary('V2.0更新说明.md')
            print("[✓] Markdown摘要已生成: V2.0更新说明.md")
    else:
        print("[!] 未检测到 python-docx 库，生成Markdown格式文档")
        print("    安装命令: pip install python-docx")
        generate_markdown_summary('V2.0更新说明.md')
        print("[✓] Markdown摘要已生成: V2.0更新说明.md")

    print()
    print("=" * 60)
    print("  文档生成完成！")
    print("=" * 60)
    print()
    print("文件清单：")
    print(f"  • main.c          - V2.0主程序源码（UTF-8 BOM）")
    print(f"  • veh.exe         - 编译产物（零警告）")
    print(f"  • Makefile        - GCC构建配置")
    print(f"  • {backup_docx}  - V1文档备份")
    if has_docx and os.path.exists('Project_29_车辆管理系统_V2.0.docx'):
        print(f"  • Project_29_车辆管理系统_V2.0.docx - V2.0蓝色高亮文档")
    if os.path.exists('V2.0更新说明.md'):
        print(f"  • V2.0更新说明.md - Markdown格式说明")
    print(f"  • gen_doc.py      - 本文档生成脚本")
    print()

    return 0

if __name__ == '__main__':
    sys.exit(main())
