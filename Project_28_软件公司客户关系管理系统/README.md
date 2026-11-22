<div align="center">

# 软件公司客户关系管理系统 V2.0

### Customer Relationship Management V2.0

![Language](https://img.shields.io/badge/Language-C99-blue?style=for-the-badge&logo=c&logoColor=white)
![Data Structure](https://img.shields.io/badge/Data_Structure-Dynamic_Array-green?style=for-the-badge)
![Status](https://img.shields.io/badge/Status-V2.0_Release-brightgreen?style=for-the-badge)
![Tests](https://img.shields.io/badge/Tests-18_PASS-success?style=for-the-badge)
![Warnings](https://img.shields.io/badge/Warnings-ZERO-orange?style=for-the-badge)

---

**基于C99标准的动态数组CRM系统 · UTF-8彩色终端 · Magic Header持久化 · 18项内置自测**

</div>

---

## ✨ V2.0 升级亮点

| 升级项 | V1 (链表版) | **V2 (动态数组版)** |
|:---|:---|:---|
| 编码 | GBK/ANSI乱码 | **UTF-8 BOM + SetConsoleOutputCP(65001)** |
| 终端 | 单色ASCII | **ANSI彩色（红/绿/黄/蓝/青/紫/灰）** |
| CLI参数 | 无 | **-h/--help, -t/--test** |
| 数据结构 | 单链表（10实体） | **动态数组（3核心实体）** |
| 错误处理 | void/零散int | **统一枚举 CrmStatus（13种）** |
| 文件格式 | \|分隔文本 | **CRMV2二进制+Magic Header+版本号** |
| 自测 | 无 | **18项自动化测试** |
| 全局变量 | 10个全局头指针 | **单一 CrmContext 上下文** |
| 级联删除 | 无 | **删除客户自动清理联系人+跟进** |
| 编译器警告 | - | **-Wall -Wextra 零警告** |

---

## 📁 项目结构

```
Project_28_软件公司客户关系管理系统/
├── main.c              # CRM V2.0 源码（UTF-8 BOM, ~1350行）
├── Makefile            # 构建配置（CC=D:/bin/gcc.exe）
├── crm.exe             # 编译产物（独立可执行）
├── gen_doc.py          # V2.0文档生成脚本（蓝色高亮标注更新）
├── README.md           # 本文件
├── Project_28_软件公司客户关系管理系统.docx          # V2.0文档
├── Project_28_软件公司客户关系管理系统_V1_backup.docx # V1文档备份
└── crmv2.dat           # 运行时数据文件（自动生成）
```

---

## 🚀 快速开始

### 编译

```bash
# 方式一：使用Makefile
make

# 方式二：直接编译
D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o crm.exe main.c
```

### 运行

```bash
crm.exe              # 启动彩色交互菜单
crm.exe -h           # 显示帮助
crm.exe --test       # 运行18项内置自测
```

---

## 🧪 内置自测（18项全通过）

```
  [01] test_init()           初始化上下文            [PASS]
  [02] test_add_customer()   添加客户               [PASS]
  [03] test_customer_count() 客户数量校验           [PASS]
  [04] test_id_increment()   ID自增验证             [PASS]
  [05] test_duplicate_phone()重复电话检测           [PASS]
  [06] test_invalid_param()  无效参数检测           [PASS]
  [07] test_update_customer()更新客户信息           [PASS]
  [08] test_add_contact()    添加联系人             [PASS]
  [09] test_contact_count()  联系人统计             [PASS]
  [10] test_add_followup()   添加跟进记录           [PASS]
  [11] test_followup_count() 跟进统计               [PASS]
  [12] test_cascade_delete() 级联删除验证           [PASS]
  [13] test_grade_stats()    等级分布统计           [PASS]
  [14] test_status_stats()   状态分布统计           [PASS]
  [15] test_file_save()      文件保存               [PASS]
  [16] test_file_load()      文件加载+完整性验证    [PASS]
  [17] test_magic_header()   Magic Header校验       [PASS]
  [18] test_not_found()      不存在记录检测         [PASS]

  ✓ 全部 18 项测试通过！
```

---

## 🏗️ 核心架构

### 数据实体

```c
Customer  客户        { id, name, contact_person, region, industry,
                        status, grade, phone, address, create_time }
Contact   联系人【V2】 { id, cust_id(FK), name, title, phone, email, remark }
FollowUp  跟进记录【V2】{ id, cust_id(FK), date, method, content, next_date }
```

### 动态数组

通过宏`DECLARE_DYNARRAY`/`IMPLEMENT_DYNARRAY`生成类型安全的动态数组：
- 初始容量：4
- 扩容倍数：2
- 操作：init / push / remove_at / free

### 文件格式（CRMV2）

```
偏移  长度  字段           说明
0     5     Magic Header   必须为 "CRMV2"
5     1     Version        =2 (uint8)
6     4     Reserved       保留
10    4     next_cust_id   ID计数器(int32)
14    4     next_cont_id
18    4     next_fol_id
22    4     cust_count     数组大小
26    4     cont_count
30    4     fol_count
34    ...   Customer[]     二进制数据
            Contact[]
            FollowUp[]
```

---

## 📋 错误码 CrmStatus

| 值 | 枚举 | 说明 |
|:--:|:---|:---|
| 0 | CRM_OK | 操作成功 |
| 1 | CRM_ERR_MEMORY | 内存分配失败 |
| 2 | CRM_ERR_NOT_FOUND | 未找到记录 |
| 3 | CRM_ERR_DUPLICATE | 重复记录 |
| 4 | CRM_ERR_INVALID_PARAM | 参数非法 |
| 5 | CRM_ERR_FILE_OPEN | 文件打开失败 |
| 6 | CRM_ERR_FILE_READ | 文件读取失败 |
| 7 | CRM_ERR_FILE_WRITE | 文件写入失败 |
| 8 | CRM_ERR_FILE_MAGIC | 魔数不匹配 |
| 9 | CRM_ERR_FILE_VERSION | 版本不兼容 |
| 10-12 | CRM_ERR_FULL/EMPTY/IO | 其他错误 |

---

## 🎮 交互菜单

```
  ╔═══════════════════════════════════════════════╗
  ║        软件公司 CRM V2.0  主菜单              ║
  ╠═══════════════════════════════════════════════╣
  ║  1. 客户列表              2. 查看客户详情   ║
  ║  3. 添加客户              4. 删除客户       ║
  ║  5. 添加联系人            6. 添加跟进记录   ║
  ║  7. 数据统计              8. 保存数据       ║
  ║  9. 删除联系人           10. 删除跟进记录   ║
  ║  0. 保存并退出                               ║
  ╚═══════════════════════════════════════════════╝
```

---

## 📄 文档生成

```bash
# 安装依赖（已安装可跳过）
pip install python-docx

# 生成V2.0文档（V2更新部分蓝色高亮）
python gen_doc.py
```

脚本会自动：
1. 备份原docx为`*_V1_backup.docx`
2. 生成全新V2.0文档，V2新增/修改内容以蓝色(#0070C0)高亮

---

## 🔧 Makefile 目标

| 目标 | 说明 |
|:---|:---|
| `make` / `make all` | 编译生成 crm.exe |
| `make test` | 编译并运行18项自测 |
| `make run` | 编译并启动交互菜单 |
| `make clean` | 清理 exe 和数据文件 |

---

## 📝 编译验证

```
$ D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o crm.exe main.c
（零警告输出，exit code 0）

$ ./crm.exe --test
✓ 全部 18 项测试通过！
（exit code 0）
```

---

<div align="center">

**─── CRM V2.0 | 软件公司客户关系管理系统 | 2026 ───**

*C99 · 动态数组 · UTF-8 · ANSI彩色 · CRMV2持久化 · 18项自测 · 零警告*

</div>
