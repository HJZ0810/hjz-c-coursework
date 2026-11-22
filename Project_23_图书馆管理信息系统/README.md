<div align="center">

#  图书馆管理信息系统 V2.0

###  2025-2026学年第二学期算法实习

![Language](https://img.shields.io/badge/Language-C99-blue?style=for-the-badge&logo=c&logoColor=white)
![Data Structure](https://img.shields.io/badge/Data_Structure-动态数组+LIBV2文件-green?style=for-the-badge)
![Status](https://img.shields.io/badge/Status-V2.0_Released-brightgreen?style=for-the-badge)
![Tests](https://img.shields.io/badge/Tests-25_PASS-success?style=for-the-badge)
![Project](https://img.shields.io/badge/Project-23-orange?style=for-the-badge)

---

**开发者: 数据库侠客** · **学号:424123888888**
**班级组别:计算机组A班**
**作者:HuiJunzhuo**

---

*动态数组+LIBV2持久化实现图书馆管理信息系统V2.0*

</div>

---

## V2.0 全新升级

| 特性 | V1 | V2.0 |
|:---|:---|:---|
| 数据结构 | 链表 | 动态数组(自动扩容) |
| 文件格式 | 4个独立.dat | 1个带LIBV2 magic的.dat |
| 错误处理 | 散乱printf | LibStatus统一错误码 |
| 中文支持 | GBK乱码 | UTF-8 BOM + CP65001 |
| 终端输出 | 黑白 | ANSI彩色 |
| CLI参数 | 无 | -h/--help, -t/--test |
| 测试覆盖 | 10项手动 | 25项自动自测 |
| 编译警告 | - | -Wall -Wextra 零警告 |
| 统计功能 | 无 | SystemStats完整统计 |

---

## 快速开始

```bash
# 编译
make
# 或
D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o lib.exe main.c

# 运行自测(25项)
lib.exe --test

# 查看帮助
lib.exe --help

# 交互菜单模式
lib.exe
```

---

## 功能模块

1. **图书管理**: 添加/修改/删除/列表/关键字查询
2. **读者管理**: 添加/修改/删除/列表
3. **借还管理**: 借书/还书/借阅记录(全部/在借)
4. **查询统计**: 图书种类/总册数/可借/借出/读者数/在借数统计
5. **数据持久化**: LIBV2二进制格式自动保存/加载

---

## 文件说明

| 文件 | 说明 |
|:---|:---|
| main.c | V2.0完整源代码(UTF-8 BOM) |
| Makefile | GCC编译配置 |
| lib.exe | 编译输出可执行文件 |
| library_v2.dat | LIBV2格式数据文件(运行后生成) |
| gen_doc.py | 文档生成脚本 |
| *_V1_backup.docx | V1版本备份 |

---

## 测试结果

```
=== 图书馆管理系统 V2.0 内置自测 ===
  [PASS] T01-T25 全部25项测试
  测试结果: 25 通过, 0 失败
```

---

> **开发者: 数据库侠客** · **学号:424123888888** · **班级组别:计算机组A班**
> **作者:HuiJunzhuo** · **2025-2026学年第二学期算法实习**
