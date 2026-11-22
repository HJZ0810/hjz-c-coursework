#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
update_docx.py - 哈希表设计 V2.0 文档更新工具

功能：
  1. 将原 Project_38_哈希表设计.docx 备份为 Project_38_哈希表设计_V1_backup.docx
  2. 在原文档首部添加 V2.0 升级说明段落，蓝色字体高亮标记
"""

import os
import shutil
from docx import Document
from docx.shared import Pt, RGBColor
from docx.enum.text import WD_COLOR_INDEX

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ORIGINAL = os.path.join(SCRIPT_DIR, "Project_38_哈希表设计.docx")
BACKUP = os.path.join(SCRIPT_DIR, "Project_38_哈希表设计_V1_backup.docx")

BLUE = RGBColor(0x00, 0x5A, 0xCC)


def backup_original():
    if os.path.exists(BACKUP):
        print(f"[SKIP] 备份已存在: {os.path.basename(BACKUP)}")
        return
    if not os.path.exists(ORIGINAL):
        print(f"[ERROR] 找不到原文件: {ORIGINAL}")
        return
    shutil.copy2(ORIGINAL, BACKUP)
    print(f"[OK] 已创建备份: {os.path.basename(BACKUP)}")


def add_v2_update_notice():
    if not os.path.exists(ORIGINAL):
        print(f"[ERROR] 找不到 {ORIGINAL}")
        return

    doc = Document(ORIGINAL)

    body = doc.element.body
    from docx.oxml.ns import qn
    from copy import deepcopy

    title_para = None
    first_p = doc.paragraphs[0] if doc.paragraphs else None

    new_paragraphs_xml = []

    def make_paragraph(text, bold=False, size=11, color=None, highlight=None, italic=False):
        from docx.oxml import OxmlElement
        from docx.oxml.ns import nsdecls
        p = OxmlElement("w:p")
        pPr = OxmlElement("w:pPr")
        spacing = OxmlElement("w:spacing")
        spacing.set(qn("w:before"), "200")
        spacing.set(qn("w:after"), "200")
        pPr.append(spacing)
        p.append(pPr)
        r = OxmlElement("w:r")
        rPr = OxmlElement("w:rPr")
        if bold:
            b = OxmlElement("w:b")
            rPr.append(b)
        if italic:
            i = OxmlElement("w:i")
            rPr.append(i)
        sz = OxmlElement("w:sz")
        sz.set(qn("w:val"), str(size * 2))
        rPr.append(sz)
        if color is not None:
            c = OxmlElement("w:color")
            c.set(qn("w:val"), f"{color[0]:02X}{color[1]:02X}{color[2]:02X}")
            rPr.append(c)
        if highlight is not None:
            hl = OxmlElement("w:highlight")
            hl.set(qn("w:val"), highlight)
            rPr.append(hl)
        r.append(rPr)
        t = OxmlElement("w:t")
        t.text = text
        t.set(qn("xml:space"), "preserve")
        r.append(t)
        p.append(r)
        return p

    insert_pos = 0
    if first_p is not None:
        insert_pos = body.index(first_p._element)

    notice_paras = [
        make_paragraph("【V2.0 升级说明 - 本次更新内容】", bold=True, size=12, color=BLUE),
        make_paragraph(
            "升级时间: 2025年 | 编译器: D:/bin/gcc.exe (MinGW GCC 14.2.0) | 标准: C99 -Wall -Wextra -O2 零警告",
            size=10, color=BLUE, italic=True
        ),
        make_paragraph(
            "V2.0 新增功能：", bold=True, size=11, color=BLUE
        ),
        make_paragraph("  - 冲突解决策略：由开放定址法升级为链地址法（Separate Chaining），性能更稳定", color=BLUE),
        make_paragraph("  - 哈希函数：BKDR字符串哈希 + 除留余数法（对素数取模），分布更均匀", color=BLUE),
        make_paragraph("  - 动态内存：全动态malloc/free分配，自动rehash扩容（负载因子超0.8触发）", color=BLUE),
        make_paragraph("  - 统一错误码：HashStatus枚举，取代V1的int返回值，语义清晰", color=BLUE),
        make_paragraph("  - 文件持久化：二进制格式，Magic Header=\"HASHV2\"，版本号校验，防篡改检测", color=BLUE),
        make_paragraph("  - UTF-8终端：SetConsoleOutputCP(65001) + ANSI彩色输出，跨控制台中文不乱码", color=BLUE),
        make_paragraph("  - CLI参数：-h/--help帮助、-t/--test内置自测（20项测试全通过）", color=BLUE),
        make_paragraph("  - 内置自测：20项测试覆盖插入/查找/删除/冲突处理/负载因子/文件I/O/扩容/Magic校验", color=BLUE),
        make_paragraph("  - 交互菜单：彩色可视化显示哈希表结构、统计信息、批量测试数据、保存/加载", color=BLUE),
        make_paragraph("  - 配套工具：gen_doc.py自动生成技术文档，Makefile一键编译/测试/清理", color=BLUE),
        make_paragraph(
            "=================================================================================",
            color=BLUE
        ),
    ]

    for i, p in enumerate(reversed(notice_paras)):
        body.insert(insert_pos, p)

    doc.save(ORIGINAL)
    print(f"[OK] 已在原文档首部插入 V2.0 蓝色高亮更新说明")


def main():
    print("=" * 60)
    print("  Project_38 哈希表设计 - docx 备份与 V2.0 更新")
    print("=" * 60)
    backup_original()
    add_v2_update_notice()
    print("=" * 60)
    print("[DONE] 完成")


if __name__ == "__main__":
    main()
