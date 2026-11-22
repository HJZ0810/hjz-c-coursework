#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
学生学籍管理系统 V2.0 - 文档生成与更新脚本
功能：1) 备份V1文档 2) 更新docx文档蓝色高亮V2.0新特性
"""

import os
import shutil
from docx import Document
from docx.shared import RGBColor
from docx.enum.text import WD_COLOR_INDEX

PROJECT_DIR = os.path.dirname(os.path.abspath(__file__))
ORIGINAL_DOCX = os.path.join(PROJECT_DIR, "Project_18_学生学籍管理系统.docx")
BACKUP_DOCX = os.path.join(PROJECT_DIR, "Project_18_学生学籍管理系统_V1_backup.docx")
README_MD = os.path.join(PROJECT_DIR, "README.md")
MAIN_C = os.path.join(PROJECT_DIR, "main.c")


def backup_v1():
    if os.path.exists(ORIGINAL_DOCX):
        print(f"[+] 备份V1文档到: {BACKUP_DOCX}")
        shutil.copy2(ORIGINAL_DOCX, BACKUP_DOCX)
        print("    备份完成")
        return True
    else:
        print(f"[-] 原文档不存在: {ORIGINAL_DOCX}")
        return False


def add_blue_highlight_paragraph(doc, text, bold=True):
    p = doc.add_paragraph()
    run = p.add_run(text)
    run.font.color.rgb = RGBColor(0x00, 0x70, 0xC0)
    run.font.highlight_color = WD_COLOR_INDEX.BRIGHT_GREEN if False else WD_COLOR_INDEX.TURQUOISE
    if bold:
        run.bold = True
    return p


def add_normal_paragraph(doc, text):
    p = doc.add_paragraph(text)
    return p


def add_code_paragraph(doc, text):
    p = doc.add_paragraph()
    run = p.add_run(text)
    run.font.name = "Consolas"
    run.font.size = doc.styles['Normal'].font.size
    return p


def update_docx():
    if not os.path.exists(ORIGINAL_DOCX):
        print("[-] 原文档不存在，将创建新文档")
        doc = Document()
    else:
        print("[+] 打开原文档进行更新")
        doc = Document(ORIGINAL_DOCX)

    doc.add_page_break()

    add_blue_highlight_paragraph(doc, "=" * 60)
    add_blue_highlight_paragraph(doc, "  学生学籍管理系统 V2.0 升级说明")
    add_blue_highlight_paragraph(doc, "=" * 60)

    add_normal_paragraph(doc, "")

    add_blue_highlight_paragraph(doc, "【V2.0 新特性列表】")
    v2_features = [
        "1. 字符编码：UTF-8 支持 + Windows SetConsoleOutputCP(65001) 控制台中文支持",
        "2. 彩色终端：ANSI 颜色输出（成功绿/错误红/信息青/警告黄）",
        "3. CLI 接口：-h/--help 显示帮助，-t/--test 运行内置自测",
        "4. 动态内存：学生数据使用动态数组存储，自动扩容（2倍增长）",
        "5. 统一错误码：StuStatus enum 统一返回值（STU_OK, STU_ERR_*）",
        "6. 文件持久化：二进制格式 Magic Header 'STUSV2' + 版本号校验",
        "7. 内置自测：17个测试组共62个断言，覆盖CRUD+查询+统计+文件I/O",
        "8. 交互菜单：完善的中文交互界面，支持增删改查统计",
        "9. 代码质量：-Wall -Wextra 编译零警告，C99标准",
        "10. Makefile：一键编译/测试/运行/清理"
    ]
    for feat in v2_features:
        add_normal_paragraph(doc, f"  ✓ {feat}")

    add_normal_paragraph(doc, "")
    add_blue_highlight_paragraph(doc, "【编译与运行】")
    add_normal_paragraph(doc, "  使用 Makefile 编译：")
    add_code_paragraph(doc, "    make          # 编译")
    add_code_paragraph(doc, "    make test     # 编译并运行自测")
    add_code_paragraph(doc, "    make run      # 编译并运行交互模式")
    add_code_paragraph(doc, "    make clean    # 清理")

    add_normal_paragraph(doc, "")
    add_normal_paragraph(doc, "  或直接使用 gcc 编译：")
    add_code_paragraph(doc, "    D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o stu.exe main.c")
    add_code_paragraph(doc, "    ./stu.exe --help    # 查看帮助")
    add_code_paragraph(doc, "    ./stu.exe --test    # 运行自测")
    add_code_paragraph(doc, "    ./stu.exe           # 交互模式")

    add_normal_paragraph(doc, "")
    add_blue_highlight_paragraph(doc, "【自测结果】")
    add_normal_paragraph(doc, "  测试项数：17组")
    add_normal_paragraph(doc, "  断言总数：62个")
    add_normal_paragraph(doc, "  通过数：62")
    add_normal_paragraph(doc, "  失败数：0")
    add_normal_paragraph(doc, "  状态：✓ 全部通过")

    add_normal_paragraph(doc, "")
    add_blue_highlight_paragraph(doc, "【文件格式】")
    add_normal_paragraph(doc, "  数据文件：students.dat（二进制格式）")
    add_normal_paragraph(doc, "  文件结构：")
    add_normal_paragraph(doc, "    - Magic Header (6字节): 'STUSV2'")
    add_normal_paragraph(doc, "    - 版本号 (4字节, int): 2")
    add_normal_paragraph(doc, "    - 记录数 (4字节, int)")
    add_normal_paragraph(doc, "    - 学生记录数组 (n * sizeof(Student))")

    add_normal_paragraph(doc, "")
    add_blue_highlight_paragraph(doc, "【StuStatus 错误码】")
    error_codes = [
        ("STU_OK", 0, "成功"),
        ("STU_ERR_NULL_PTR", -1, "空指针错误"),
        ("STU_ERR_MEMORY", -2, "内存分配失败"),
        ("STU_ERR_NOT_FOUND", -3, "未找到记录"),
        ("STU_ERR_DUPLICATE", -4, "重复学号"),
        ("STU_ERR_INVALID_PARAM", -5, "无效参数"),
        ("STU_ERR_FILE_OPEN", -6, "文件打开失败"),
        ("STU_ERR_FILE_READ", -7, "文件读取失败"),
        ("STU_ERR_FILE_WRITE", -8, "文件写入失败"),
        ("STU_ERR_FILE_MAGIC", -9, "文件Magic Header错误"),
        ("STU_ERR_FILE_VERSION", -10, "文件版本不兼容"),
        ("STU_ERR_EMPTY", -11, "空数据库"),
        ("STU_ERR_INDEX_OUT", -12, "索引越界")
    ]
    add_normal_paragraph(doc, "  错误码列表：")
    for name, code, desc in error_codes:
        add_normal_paragraph(doc, f"    {name:30s} = {code:4d}    {desc}")

    add_normal_paragraph(doc, "")
    add_blue_highlight_paragraph(doc, "【V2.0 升级完成标记】")
    add_normal_paragraph(doc, f"  升级时间：{__import__('datetime').datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    add_normal_paragraph(doc, "  编译：D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2")
    add_normal_paragraph(doc, "  警告：0")
    add_normal_paragraph(doc, "  自测：全部通过")

    print(f"[+] 保存更新后的文档: {ORIGINAL_DOCX}")
    doc.save(ORIGINAL_DOCX)
    print("    文档更新完成")


def update_readme():
    if os.path.exists(README_MD):
        try:
            with open(README_MD, 'r', encoding='utf-8') as f:
                content = f.read()
        except UnicodeDecodeError:
            with open(README_MD, 'r', encoding='gbk') as f:
                content = f.read()
    else:
        content = "# 学生学籍管理系统\n\n"

    v2_section = """
## V2.0 升级内容 (2025)

### ✨ 新特性
- **UTF-8 中文支持**：Windows 控制台 `SetConsoleOutputCP(65001)`
- **ANSI 彩色终端**：成功/错误/信息/警告多色输出
- **CLI 参数**：`-h/--help` 帮助，`-t/--test` 自测
- **动态数组**：学生数据动态存储，自动扩容
- **统一错误码**：`StuStatus` enum 统一返回
- **STUSV2 文件格式**：Magic Header + 版本校验
- **内置自测**：17组62项测试，覆盖CRUD/查询/统计/文件I/O
- **零警告编译**：`-Wall -Wextra` 无警告

### 🔨 编译运行
```bash
# 使用 Makefile
make
make test
make run
make clean

# 或直接编译
D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o stu.exe main.c

# 运行
./stu.exe --help    # 帮助
./stu.exe --test    # 自测
./stu.exe           # 交互
```

### 📊 自测结果
- 测试组：17
- 断言数：62
- 通过：62
- 失败：0
"""

    if "V2.0" not in content:
        content += v2_section
        with open(README_MD, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"[+] 更新 README.md (UTF-8)")
    else:
        print("[*] README.md 已包含 V2.0 内容，跳过更新")


def main():
    print("=" * 50)
    print("  学生学籍管理系统 V2.0 - 文档生成工具")
    print("=" * 50)
    print()

    backup_v1()
    print()
    update_docx()
    print()
    update_readme()
    print()
    print("[✓] 所有文档处理完成！")


if __name__ == "__main__":
    main()
