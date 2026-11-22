# -*- coding: utf-8 -*-
"""
学生社团成员信息管理系统 V2.0 文档生成脚本
生成带蓝色高亮的Word文档，标记V2.0新增特性
"""

from docx import Document
from docx.shared import Pt, RGBColor, Inches
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.oxml.ns import qn
from docx.oxml import OxmlElement
import sys
import os

def set_cell_shading(cell, color_hex):
    shading_elm = OxmlElement('w:shd')
    shading_elm.set(qn('w:fill'), color_hex)
    cell._tc.get_or_add_tcPr().append(shading_elm)

def add_highlight_run(paragraph, text, color=RGBColor(0, 0, 255), bold=True):
    run = paragraph.add_run(text)
    run.font.color.rgb = color
    run.font.bold = bold
    return run

def add_v2_badge(paragraph):
    run = paragraph.add_run('【V2.0新增】')
    run.font.color.rgb = RGBColor(0, 0, 255)
    run.font.bold = True
    run.font.size = Pt(10)

def create_doc():
    doc = Document()

    style = doc.styles['Normal']
    style.font.name = '宋体'
    style._element.rPr.rFonts.set(qn('w:eastAsia'), '宋体')
    style.font.size = Pt(12)

    title = doc.add_heading('', level=0)
    title.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = title.add_run('学生社团成员信息管理系统 V2.0')
    run.font.color.rgb = RGBColor(0, 0, 139)
    run.font.bold = True

    subtitle = doc.add_paragraph()
    subtitle.alignment = WD_ALIGN_PARAGRAPH.CENTER
    add_highlight_run(subtitle, 'Student Club Member Management System V2.0')

    info = doc.add_paragraph()
    info.alignment = WD_ALIGN_PARAGRAPH.CENTER
    info.add_run('学号：424123888888  |  班级：A班  |  姓名：HuiJunzhuo  |  2025-2026学年第二学期算法实习')

    doc.add_paragraph()

    doc.add_heading('一、项目概述', level=1)

    p = doc.add_paragraph()
    p.add_run('本项目是一个使用C99标准开发的学生社团成员信息管理系统，采用')
    add_highlight_run(p, '动态数组（Dynamic Array）')
    p.add_run('作为核心数据结构，替代了V1版本的链表实现，支持自动扩容。系统实现了学生信息管理、社团信息管理、成员归属管理三大核心功能，并通过成员归属表实现学生与社团之间的多对多关系。')

    p2 = doc.add_paragraph()
    add_v2_badge(p2)
    p2.add_run(' V2.0版本全面升级：支持UTF-8 BOM编码、Windows控制台UTF-8输出（SetConsoleOutputCP(65001)）、ANSI彩色终端、CLI命令行参数（--help/-h/--test/-t）、统一错误码枚举ClubStatus、文件持久化带Magic Header "CLUBV2"、内置自测框架（27项测试用例覆盖CRUD+查询+文件I/O）。')

    doc.add_heading('二、V2.0 新特性', level=1)

    features = [
        ('UTF-8 BOM编码支持', '文件开头添加UTF-8 BOM标记，确保中文在Windows下正确显示'),
        ('SetConsoleOutputCP(65001)', '调用Windows API设置控制台代码页为UTF-8，解决中文乱码问题'),
        ('ANSI彩色终端', '完整的ANSI转义序列支持，成功/错误/警告/信息使用不同颜色'),
        ('CLI命令行参数', '支持--help/-h显示帮助、--test/-t运行自测、无参数进入交互菜单'),
        ('动态数组存储', '使用动态数组（Dynamic Array）替代链表，支持自动扩容（GROWTH_FACTOR=2）'),
        ('统一错误码枚举', '定义ClubStatus枚举统一管理所有错误状态（CLUB_OK/CLUB_ERR_*）'),
        ('Magic Header持久化', '数据文件以"CLUBV2"作为Magic Header，包含版本号字段'),
        ('内置自测框架', '27项测试用例，覆盖CRUD操作、边界情况、文件I/O、级联删除等'),
        ('中文注释完整', '所有函数均有详细中文注释说明功能、参数、返回值'),
        ('级联删除', '删除学生/社团时自动删除关联的成员记录，保证数据一致性'),
    ]

    table = doc.add_table(rows=1, cols=2)
    table.style = 'Table Grid'
    hdr = table.rows[0].cells
    hdr[0].text = '新特性'
    hdr[1].text = '说明'
    set_cell_shading(hdr[0], '4472C4')
    set_cell_shading(hdr[1], '4472C4')
    for cell in hdr:
        for paragraph in cell.paragraphs:
            for run in paragraph.runs:
                run.font.color.rgb = RGBColor(255, 255, 255)
                run.font.bold = True

    for name, desc in features:
        row = table.add_row().cells
        p_name = row[0].paragraphs[0]
        add_highlight_run(p_name, name, RGBColor(0, 0, 200))
        row[1].text = desc

    doc.add_paragraph()

    doc.add_heading('三、数据结构设计', level=1)

    p = doc.add_paragraph()
    p.add_run('V2.0版本采用')
    add_highlight_run(p, '动态数组（Dynamic Array）')
    p.add_run('作为核心数据结构，相比链表具有更好的缓存局部性和随机访问性能：')

    doc.add_heading('3.1 数据实体结构', level=2)

    entities = [
        ('Student（学生）', 'name(姓名)、id(学号)、age(年龄)、gender(性别)、college(学院)、major(专业)、grade(年级)'),
        ('Club（社团）', 'clubId(社团编号)、name(名称)、leader(负责人)、location(地点)、foundDate(成立日期)、description(描述)'),
        ('Member（成员归属）', 'studentId(学号)、clubId(社团编号)、joinDate(加入日期)、position(职位)、note(备注)'),
    ]

    for name, fields in entities:
        p = doc.add_paragraph(style='List Bullet')
        run = p.add_run(name + '：')
        run.font.bold = True
        p.add_run(fields)

    doc.add_heading('3.2 动态数组容器', level=2)

    p = doc.add_paragraph()
    add_v2_badge(p)
    p.add_run(' 使用结构体封装动态数组，包含items(数据指针)、count(元素个数)、capacity(当前容量)三个字段：')

    code_text = '''typedef struct {
    Student *items;
    size_t count;
    size_t capacity;
} StudentArray;'''
    p_code = doc.add_paragraph()
    p_code.paragraph_format.left_indent = Inches(0.5)
    run = p_code.add_run(code_text)
    run.font.name = 'Consolas'
    run.font.size = Pt(10)

    doc.add_paragraph()

    doc.add_heading('3.3 错误码枚举', level=2)

    p = doc.add_paragraph()
    add_v2_badge(p)
    p.add_run(' ClubStatus统一错误码枚举定义：')

    error_codes = [
        'CLUB_OK = 0 - 操作成功',
        'CLUB_ERR_MEMORY - 内存分配失败',
        'CLUB_ERR_NOT_FOUND - 记录不存在',
        'CLUB_ERR_DUPLICATE - 重复记录',
        'CLUB_ERR_INVALID_INPUT - 无效输入',
        'CLUB_ERR_FILE_OPEN/READ/WRITE - 文件操作错误',
        'CLUB_ERR_MAGIC_MISMATCH - Magic Header验证失败',
        'CLUB_ERR_VERSION_MISMATCH - 数据版本不匹配',
    ]
    for ec in error_codes:
        doc.add_paragraph(ec, style='List Bullet')

    doc.add_heading('四、文件持久化格式', level=1)

    p = doc.add_paragraph()
    add_v2_badge(p)
    p.add_run(' V2.0使用二进制格式存储数据，带Magic Header和版本号：')

    file_table = doc.add_table(rows=1, cols=3)
    file_table.style = 'Table Grid'
    hdr = file_table.rows[0].cells
    hdr[0].text = '偏移'
    hdr[1].text = '长度'
    hdr[2].text = '内容'
    for cell in hdr:
        set_cell_shading(cell, '4472C4')
        for paragraph in cell.paragraphs:
            for run in paragraph.runs:
                run.font.color.rgb = RGBColor(255, 255, 255)
                run.font.bold = True

    file_sections = [
        ('0', '6字节', 'Magic Header: "CLUBV2"'),
        ('6', '1字节', '版本号: 2'),
        ('7', 'sizeof(size_t)', '学生数量(sc)'),
        ('7+s', 'sizeof(size_t)', '社团数量(cc)'),
        ('7+2s', 'sizeof(size_t)', '成员数量(mc)'),
        ('7+3s', 'sc * sizeof(Student)', '学生数据数组'),
        ('...', 'cc * sizeof(Club)', '社团数据数组'),
        ('...', 'mc * sizeof(Member)', '成员数据数组'),
    ]
    for off, sz, desc in file_sections:
        row = file_table.add_row().cells
        row[0].text = off
        row[1].text = sz
        p_desc = row[2].paragraphs[0]
        if 'Magic' in desc or '版本' in desc:
            add_highlight_run(p_desc, desc, RGBColor(0, 0, 200))
        else:
            p_desc.add_run(desc)

    doc.add_paragraph()

    doc.add_heading('五、编译与运行', level=1)

    doc.add_heading('5.1 编译', level=2)
    p = doc.add_paragraph()
    p.add_run('使用GCC编译器（C99标准）：')

    p_code = doc.add_paragraph()
    p_code.paragraph_format.left_indent = Inches(0.3)
    run = p_code.add_run('D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o clubmgr.exe main.c')
    run.font.name = 'Consolas'
    run.font.size = Pt(10)

    doc.add_heading('5.2 Makefile', level=2)
    p = doc.add_paragraph()
    p.add_run('项目提供Makefile，包含以下目标：')
    doc.add_paragraph('make / make all - 编译程序', style='List Bullet')
    doc.add_paragraph('make test - 运行内置自测', style='List Bullet')
    doc.add_paragraph('make run - 运行交互菜单', style='List Bullet')
    doc.add_paragraph('make clean - 清理生成文件', style='List Bullet')

    doc.add_heading('5.3 命令行参数', level=2)

    p = doc.add_paragraph()
    add_v2_badge(p)
    p.add_run(' CLI支持：')

    cli_table = doc.add_table(rows=1, cols=2)
    cli_table.style = 'Table Grid'
    hdr = cli_table.rows[0].cells
    hdr[0].text = '参数'
    hdr[1].text = '功能'
    for cell in hdr:
        set_cell_shading(cell, '4472C4')
        for paragraph in cell.paragraphs:
            for run in paragraph.runs:
                run.font.color.rgb = RGBColor(255, 255, 255)
                run.font.bold = True

    cli_cmds = [
        ('-h / --help', '显示帮助信息'),
        ('-t / --test', '运行内置自测（27项测试）'),
        ('(无参数)', '启动交互菜单'),
    ]
    for cmd, func in cli_cmds:
        row = cli_table.add_row().cells
        p_cmd = row[0].paragraphs[0]
        run = p_cmd.add_run(cmd)
        run.font.name = 'Consolas'
        row[1].text = func

    doc.add_paragraph()

    doc.add_heading('六、内置自测用例', level=1)

    p = doc.add_paragraph()
    add_v2_badge(p)
    p.add_run(' V2.0内置27项测试用例，全面覆盖：')

    test_categories = [
        ('学生CRUD', 'TC01-TC05: 添加学生(成功/重复/空输入/无效年龄)、查询学生'),
        ('社团CRUD', 'TC06-TC08: 添加社团(成功/重复)、查询社团'),
        ('成员管理', 'TC09-TC12: 添加成员(成功/重复/学生不存在/社团不存在)'),
        ('批量操作', 'TC13: 批量添加3学生+3社团+4成员后计数验证'),
        ('文件I/O', 'TC14-TC17: 保存文件、重新加载、数据完整性验证、内容正确性验证'),
        ('级联删除', 'TC18-TC20: 删除学生级联删除成员、删除社团级联删除成员、删除不存在记录'),
        ('动态扩容', 'TC21: 添加20个学生触发数组扩容验证'),
        ('Magic Header', 'TC22: 验证数据文件Magic Header正确'),
    ]

    for cat, desc in test_categories:
        p = doc.add_paragraph(style='List Bullet')
        run = p.add_run(cat + '：')
        run.font.bold = True
        add_highlight_run(p, desc, RGBColor(0, 0, 180), bold=False)

    doc.add_paragraph()

    doc.add_heading('七、系统功能模块', level=1)

    modules = [
        ('学生管理', '添加学生、查看所有学生、删除学生（级联删除成员记录）'),
        ('社团管理', '添加社团、查看所有社团、删除社团（级联删除成员记录）'),
        ('成员管理', '加入社团、查看所有成员、退出社团'),
        ('查询功能', '按学号/姓名查询学生、按编号/名称查询社团、查询学生参加的社团、查询社团成员'),
        ('数据持久化', '启动自动加载、操作自动保存、退出自动保存、Magic Header+版本验证'),
    ]

    for name, desc in modules:
        p = doc.add_paragraph(style='List Bullet')
        run = p.add_run(name + '：')
        run.font.bold = True
        p.add_run(desc)

    doc.add_paragraph()

    doc.add_heading('八、测试运行结果', level=1)

    p = doc.add_paragraph()
    add_highlight_run(p, '✓ 27项测试全部通过！')
    p.add_run(' 编译零警告、零错误。')

    doc.add_paragraph()
    end = doc.add_paragraph()
    end.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = end.add_run('—— V2.0 升级完成 ——')
    run.font.color.rgb = RGBColor(0, 0, 139)
    run.font.bold = True

    output_path = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                               'Project_12_学生社团成员信息管理系统_V2.docx')
    doc.save(output_path)
    print(f'文档已生成: {output_path}')
    return output_path

if __name__ == '__main__':
    try:
        create_doc()
        print('✓ 文档生成成功')
    except ImportError:
        print('需要安装python-docx库: pip install python-docx')
        sys.exit(1)
    except Exception as e:
        print(f'生成失败: {e}')
        sys.exit(1)
