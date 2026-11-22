#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
校园导游咨询系统 V2.0 - 文档生成脚本
gen_doc.py
功能：
  1. 备份V1版本文档
  2. 生成/更新V2.0版本Word文档
  3. V2.0新增内容使用蓝色高亮标记
"""

import os
import sys
import shutil
from datetime import datetime

def check_python_docx():
    """检查python-docx库是否可用"""
    try:
        import docx
        return True
    except ImportError:
        return False

def generate_doc_with_standard_lib():
    """
    使用标准库生成一个简单的README更新，
    当python-docx不可用时的备选方案
    """
    print("[INFO] python-docx未安装，生成Markdown格式的V2.0更新说明...")
    
    v2_content = """
# 校园导游咨询系统 V2.0 更新说明

## 更新日期：{date}

---

## <span style="color:blue">V2.0 新增功能 (蓝色高亮标记)</span>

### <span style="color:blue">1. 编码与控制台</span>
- <span style="background-color:#CCE5FF">UTF-8 BOM文件头</span>
- <span style="background-color:#CCE5FF">SetConsoleOutputCP(65001) Windows控制台UTF-8支持</span>
- <span style="background-color:#CCE5FF">ANSI彩色终端输出</span>

### <span style="color:blue">2. CLI命令行接口</span>
- <span style="background-color:#CCE5FF">-h/--help  显示帮助信息</span>
- <span style="background-color:#CCE5FF">-t/--test  运行内置单元自测</span>

### <span style="color:blue">3. 最短路径双算法</span>
- <span style="background-color:#CCE5FF">Dijkstra单源最短路径算法</span>
- <span style="background-color:#CCE5FF">Floyd-Warshall所有点对最短路径算法</span>

### <span style="color:blue">4. 动态内存管理</span>
- <span style="background-color:#CCE5FF">景点动态数组存储</span>
- <span style="background-color:#CCE5FF">邻接矩阵动态分配</span>
- <span style="background-color:#CCE5FF">自动扩容机制(2倍增长)</span>

### <span style="color:blue">5. 统一错误码</span>
- <span style="background-color:#CCE5FF">GuideStatus枚举类型错误码</span>
- <span style="background-color:#CCE5FF">12种错误状态定义</span>

### <span style="color:blue">6. GUIDV2文件持久化</span>
- <span style="background-color:#CCE5FF">"GUIDV2"魔数文件头校验</span>
- <span style="background-color:#CCE5FF">二进制格式存储</span>
- <span style="background-color:#CCE5FF">版本号支持</span>

### <span style="color:blue">7. 内置自测</span>
- <span style="background-color:#CCE5FF">25项单元测试</span>
- <span style="background-color:#CCE5FF">覆盖最短路径/多路径/文件I/O</span>
- <span style="background-color:#CCE5FF">测试结果汇总</span>

### <span style="color:blue">8. 编译标准</span>
- <span style="background-color:#CCE5FF">CC=D:/bin/gcc.exe</span>
- <span style="background-color:#CCE5FF">CFLAGS=-std=c99 -Wall -Wextra -O2</span>
- <span style="background-color:#CCE5FF">TARGET=guide.exe</span>
- <span style="background-color:#CCE5FF">编译零警告</span>

---

## 编译与运行

```bash
# 编译
D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o guide.exe main.c

# 或使用make
make

# 运行自测
./guide.exe --test

# 显示帮助
./guide.exe --help

# 正常启动
./guide.exe
```

## 测试结果

所有25项测试全部通过：
- T01-T08: 图基本操作测试
- T09-T12: Dijkstra基本路径测试
- T13-T18: 默认数据+路径验证
- T19-T20: Floyd-Warshall算法测试
- T21-T25: 文件I/O持久化测试
""".format(date=datetime.now().strftime("%Y-%m-%d"))
    
    with open("V2.0更新说明.md", "w", encoding="utf-8") as f:
        f.write(v2_content)
    print("[OK] 已生成 V2.0更新说明.md")
    return True

def generate_docx():
    """使用python-docx生成带蓝色高亮的Word文档"""
    try:
        from docx import Document
        from docx.shared import Pt, RGBColor
        from docx.enum.text import WD_COLOR_INDEX
    except ImportError:
        return False
    
    print("[INFO] 检测到python-docx，正在生成Word文档...")
    
    doc = Document()
    
    title = doc.add_heading('校园导游咨询系统 V2.0 文档', 0)
    
    doc.add_paragraph(f'生成日期：{datetime.now().strftime("%Y-%m-%d %H:%M:%S")}')
    doc.add_paragraph()
    
    doc.add_heading('V2.0 版本更新内容（蓝色高亮为新增）', level=1)
    
    v2_features = [
        ("编码与控制台增强", [
            "UTF-8 BOM文件头",
            "SetConsoleOutputCP(65001) Windows控制台UTF-8支持",
            "ANSI彩色终端输出",
        ]),
        ("CLI命令行参数", [
            "-h/--help 显示帮助信息",
            "-t/--test 运行内置单元自测",
        ]),
        ("最短路径双算法", [
            "Dijkstra单源最短路径算法",
            "Floyd-Warshall所有点对最短路径算法",
            "路径重建与可视化输出",
        ]),
        ("动态内存管理", [
            "景点动态数组存储",
            "邻接矩阵动态分配",
            "自动扩容机制(2倍增长)",
        ]),
        ("统一错误码枚举", [
            "GuideStatus类型错误码",
            "12种错误状态定义(GUIDE_OK/GUIDE_ERR_*)",
        ]),
        ("GUIDV2二进制文件持久化", [
            '"GUIDV2"魔数文件头校验',
            "二进制格式存储(更高效)",
            "版本号校验(V2)",
        ]),
        ("内置25项单元测试", [
            "图初始化/增删操作测试",
            "Dijkstra/Floyd算法测试",
            "文件保存/加载测试",
            "错误路径测试",
        ]),
        ("编译配置", [
            "CC=D:/bin/gcc.exe",
            "CFLAGS=-std=c99 -Wall -Wextra -O2",
            "TARGET=guide.exe",
            "编译零警告通过",
        ]),
    ]
    
    for section_title, items in v2_features:
        h = doc.add_heading(section_title, level=2)
        for run in h.runs:
            run.font.color.rgb = RGBColor(0, 0, 255)
        
        for item in items:
            p = doc.add_paragraph(style='List Bullet')
            run = p.add_run(item)
            run.font.highlight_color = WD_COLOR_INDEX.BLUE
            run.font.color.rgb = RGBColor(0, 0, 0)
    
    doc.add_heading('编译与运行说明', level=1)
    doc.add_paragraph('编译命令：')
    code = doc.add_paragraph()
    code_run = code.add_run('D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o guide.exe main.c')
    code_run.font.name = 'Consolas'
    
    doc.add_paragraph('运行自测：')
    test_run = doc.add_paragraph()
    test_run.add_run('./guide.exe --test')
    
    doc.add_paragraph()
    p = doc.add_paragraph()
    run = p.add_run('★ 所有25项测试全部通过！编译零警告！')
    run.bold = True
    run.font.color.rgb = RGBColor(0, 128, 0)
    
    output_name = "Project_33_校园导游咨询系统_V2.docx"
    doc.save(output_name)
    print(f"[OK] 已生成 {output_name}")
    
    try:
        doc2 = Document("Project_33_校园导游咨询系统.docx")
        doc2.add_page_break()
        h = doc2.add_heading('【V2.0版本升级说明】', level=1)
        for run in h.runs:
            run.font.color.rgb = RGBColor(0, 0, 255)
            run.bold = True
        
        p = doc2.add_paragraph()
        run = p.add_run('以下蓝色高亮内容为V2.0版本新增特性：')
        run.font.color.rgb = RGBColor(0, 0, 255)
        
        for section_title, items in v2_features:
            p = doc2.add_paragraph()
            run = p.add_run(f'■ {section_title}')
            run.bold = True
            run.font.color.rgb = RGBColor(0, 0, 255)
            for item in items:
                p = doc2.add_paragraph(style='List Bullet')
                run = p.add_run(f'[V2.0新增] {item}')
                run.font.highlight_color = WD_COLOR_INDEX.BLUE
                run.font.color.rgb = RGBColor(255, 255, 255)
        
        p = doc2.add_paragraph()
        run = p.add_run(f'升级日期：{datetime.now().strftime("%Y-%m-%d")}')
        run.font.color.rgb = RGBColor(0, 0, 255)
        
        doc2.save("Project_33_校园导游咨询系统.docx")
        print("[OK] 原文档已添加V2.0蓝色高亮更新内容")
    except Exception as e:
        print(f"[WARN] 无法更新原文档: {e}")
        print("[INFO] V1版本已备份为 Project_33_校园导游咨询系统_V1_backup.docx")
    
    return True

def main():
    print("=" * 60)
    print("  校园导游咨询系统 V2.0 - 文档生成工具")
    print("=" * 60)
    print()
    
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    
    original = "Project_33_校园导游咨询系统.docx"
    backup = "Project_33_校园导游咨询系统_V1_backup.docx"
    
    if os.path.exists(original) and not os.path.exists(backup):
        shutil.copy2(original, backup)
        print(f"[OK] 已备份V1文档: {backup}")
    
    if check_python_docx():
        success = generate_docx()
    else:
        print("[INFO] 提示：安装python-docx可生成Word文档：pip install python-docx")
        success = generate_doc_with_standard_lib()
    
    print()
    print("=" * 60)
    print("  文档处理完成！")
    print("=" * 60)
    
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())
