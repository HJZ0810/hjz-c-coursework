<div align="center">

# ? 期刊管理系统

### ? 2025-2026学年第二学期算法复习

![Language](https://img.shields.io/badge/Language-C99-blue?style=for-the-badge&logo=c&logoColor=white)
![Data Structure](https://img.shields.io/badge/Data_Structure-Singly_Linked_List_and_Queue-green?style=for-the-badge)
![Status](https://img.shields.io/badge/Status-Completed-brightgreen?style=for-the-badge)
![Project](https://img.shields.io/badge/Project-10-orange?style=for-the-badge)

---

**??? 作者：惠俊卓** · **学号：424123888888**
**? 班级：华信竞赛队A部**
**? 审核：HuiJunzhuo**

---

*基于Singly_Linked_List_and_Queue实现的期刊管理系统*

</div>

---

## ? 目录

- [1. 需求分析](#1-需求分析)
- [2. 概要设计](#2-概要设计)
- [3. 详细设计](#3-详细设计)
- [4. 测试分析](#4-测试分析)
- [5. 总结](#5-总结)
- [? 快速??始](#-快速开始)
- [? 联系作者](#-联系作者)

---

## 1. 需求分析

### 1.1 项目背景

在高校图书馆和科研单位中，期刊种类繁多、读者流动性大、借阅周期固定、超期管理繁琐，传统的人工登记方式不仅效率低下、容易出错，而且在借阅状态跟踪、预约排队管理和超期提醒等环节需要反复人工核对，耗时耗力。随着期刊数量的增加和读者规模的扩大，手工管理方式已无法满足现代期刊借阅管理对快速、准确、规范操作的需求。特别是在需要预约排队和超期检查的场景下，人工操作极易产生遗漏和错误。

本项目旨在运用C语言和数据结构知识，设计并实现一个功能完善的期刊管理系统。该系统基于单链表数据结构管理期刊和读者信息，采用链式队列实现预约排队（FIFO，公平调度），支持期刊与读者的增删查改、借阅归还、预约管理、超期检查、统计信息以及「|」分隔符文本文件持久化存储等功能。系统提供两种查询模式（管理员查看全部/读者按名称模糊搜索），覆盖男子和女子两类期刊分类，能够满足中小规模图书馆期刊借阅管理的完整需求。

### 1.2 功能需求

| 序号 | 功能模块 | 说明 |
|:---:|:---|:---|
| 1 | 录入期刊 | 交互式录入，支持含空格字段 |
| 2 | 录入读者 | 交互式录入，编号唯一性检查 |
| 3 | 查询期刊 | 全部查看+名称模糊搜索 |
| 4 | 查询读者 | 全部查看+按编号查询+借阅记录反查 |
| 5 | 借阅期刊 | 验证+借阅+日期计算（自动跨月跨年） |
| 6 | 归还期刊 | 超期检查+预约联动自动出队 |
| 7 | 预定期刊 | FIFO队列，容量上限+防重复 |
| 8 | 统计信息 | 借阅率+最活跃读者+预约总数 |
| 9 | 超期检查 | 遍历筛选+字典序日期比较 |

### 1.3 数据需求

**JournalNode（期刊信息）结构体：**

| 字段 | 类型 | 说明 |
|:---|:---|:---|
| `id` | int | 期刊唯一编号 |
| `name[64]` | char[] | 期刊名称 |
| `author[32]` | char[] | 作者/编者 |
| `publisher[64]` | char[] | 出版社 |
| `isbn[20]` | char[] | ISBN号 |
| `status` | enum | STATUS_AVAILABLE / STATUS_BORROWED |
| `borrower_id` | int | 借阅人ID（0=无人） |
| `borrow_date[11]` | char[] | 借出日期 |
| `return_date[11]` | char[] | 应还日期 |
| `reservation` | struct | 预约队列（嵌入） |

**ReaderNode（读者信息）/ SystemContext（全局上下文）结构体：**

| 字段 | 类型 | 说明 |
|:---|:---|:---|
| `id` | int | 读者唯一编号 |
| `name[64]` | char[] | 读者姓名 |
| `dept[32]` | char[] | 部门/院系 |
| `phone[16]` | char[] | 联系电话 |
| `borrow_count` | int | 当前借阅数量 |
| `next` | 指针 | 下一个读者节点 |
|  |  |  |
| `journal_list` | 指针 | 期刊链表头指针 |
| `reader_list` | 指针 | 读者链表头指针 |
| `journal_count` | int | 期刊总数 |
| `reader_count` | int | 读者总数 |

---

## 2. 概要设计

### 2.1 数据结构设计

系统选择单链表作为期刊和读者的核心存储结构，基于以下设计考量：期刊和读者的数量在运行时动态变化，链表天然支持动态增删，无需预分配固定大小的数组。采用无头节点的单链表，头指针为ctx->journal_list / ctx->reader_list，实现简洁。插入操作采用尾插法（而非头插法），保持节点在链表中的顺序与录入顺序一致，方便用户按编号顺序浏览。

每本期刊内部嵌入一个ReservationQueue结构体（聚合设计），实现「每本期刊独立的预约队列」，避免了额外的关联表，简化了数据管理。预约队列采用链式队列实现（而非循环数组），因为预约人数上限虽为10，但实际场景中可能调整，链式结构更灵活；入队/出队只需修改指针，无需移动元素；内存按需分配，不会浪费固定数组的空间。

预约队列采用三元组(front, rear, count)管理，使用rear指针使入队操作从O(n)优化到O(1)（若只有front，每次入队需遍历找末尾）。count字段支持O(1)判断队满/队空，避免每次遍历计数。入队前执行双重约束检查：队列容量上限（count >= MAX_RESERVATIONS）和重复预约检查（遍历查找相同reader_id），确保业务逻辑正确性。

日期计算模块利用mktime()自动处理溢出：字符串解析→struct tm→mktime()转epoch秒→加days×86400秒→localtime()转回→sprintf格式化。mktime()会自动处理溢出（如1月32日→2月1日），跨月/跨年/闰年全部自动正确，无需手动处理。YYYY-MM-DD格式的另一个巧妙之处是字典序比较等价于日期大小比较，使得strcmp可以直接用于日期比较。

### 2.2 系统架构

```
┌──────────────────────???──────────────┐
│       表示层（终端菜单界面）         │
├─────────────────────────────────────┤
│    业务逻辑层（CRUD/查询/统计）      │
├─────────────────────────────────────┤
│   数据层（单链表 + 文件I/O）         │
└─────────────────────────────────────┘
```

### 2.3 模块划分

| 模块 | 功能 |
|:---|:---|
| 菜单界面 | print_menu(), clear_input_buffer(), pause_screen() |
| 工具函数 | safe_gets(), add_days(), date_compare() |
| 预约队列 | queue_enqueue(), queue_dequeue() |
| 期刊链表 | journal_create(), journal_add(), journal_print() |
| 读者链表 | reader_create(), reader_add(), reader_print() |
| 文件持久化 | save_data(), load_data() |
| 业务功能 | 九大功能函数 |
| 测试数据 | load_test_data() 预置数据 |

---

## 3. 详细设计

### 3.1 核心数据结构

```c
/*--- 常量定义 ---*/
#define MAX_NAME_LEN      64  /* 期刊/读者名称最大长度（含\0） */
#define MAX_AUTHOR_LEN    32  /* 作者/编者字段最大长度         */
#define MAX_PUB_LEN       64  /* 出版社字段最大长度             */
#define MAX_ISBN_LEN      20  /* ISBN号最大长度（含\0）         */
#define MAX_PHONE_LEN     16  /* 电话号码最大长度（含\0）       */
#define MAX_DEPT_LEN      32  /* 部门/院系名称最大长度          */
#define MAX_DATE_LEN      11  /* 日期字符串长度 YYYY-MM-DD\0  */
#define DATA_FILE         "journal_data.dat"
#define BORROW_DAYS       30  /* 默认借阅天数                   */
#define MAX_RESERVATIONS  10  /* 每本期刊最大预约排队人数       */

/*--- 枚举：期刊借阅状态 ---*/
typedef enum {
    STATUS_AVAILABLE = 0,   /* 可借阅：期刊在架上 */
    STATUS_BORROWED  = 1    /* 已借出：期刊被借走 */
} JournalStatus;

/*--- 枚举：操作结果码 ---*/
typedef enum {
    RESULT_SUCCESS = 0,             /* 操作成功 */
    RESULT_NOT_FOUND,               /* 未找到目标元素 */
    RESULT_ALREADY_BORROWED,        /* 期刊已被他人借出 */
    RESULT_NOT_BORROWED,            /* 期刊当前未被借出 */
    RESULT_RESERVATION_FULL,        /* 预约队列已满 */
    RESULT_DUPLICATE_ID,            /* 编号重复 */
    RESULT_FILE_ERROR               /* 文件读写操作失败 */
} OpResult;
/* 预约队列节点 */
typedef struct ReservationNode {
    int                     reader_id;  /* 预约读者唯一编号 */
    struct ReservationNode *next;       /* 下一个预约节点   */
} ReservationNode;

/* 预约队列管理结构（三元组） */
typedef struct {
    ReservationNode *front;     /* 队头指针（最早预约） */
    ReservationNode *rear;      /* 队尾指针（O(1)入队） */
    int              count;     /* 当前预约人数（O(1)判满） */
} ReservationQueue;

/* 期刊信息节点（单链表节点） */
typedef struct JournalNode {
    int                 id;                         /* 期刊唯一编号 */
    char                name[MAX_NAME_LEN];         /* 期刊名称     */
    char                author[MAX_AUTHOR_LEN];     /* 作者/编者   */
    char                publisher[MAX_PUB_LEN];     /* 出版社      */
    char                isbn[MAX_ISBN_LEN];         /* ISBN号      */
    JournalStatus       status;                     /* 借阅状态     */
    int                 borrower_id;                /* 借阅人ID    */
    char                borrow_date[MAX_DATE_LEN];  /* 借出日期    */
    char                return_date[MAX_DATE_LEN];  /* 应还日期    */
    ReservationQueue    reservation;                /* 预约队列（聚合嵌入） */
    struct JournalNode *next;                       /* 链表指针     */
} JournalNode;
```

### 3.2 核心函数

核心函数包括 safe_input(), add_contact(), search_contact(), login(), save/load_contacts() 等。

---

## 4. 测试分析

### 4.1 测试环境

| 项目 | 描述 |
|:---|:---|
| 操作系统 | Windows 10/11 64位 / Linux |
| 编译器 | GCC C99标准 |
| 编译命令 | gcc -std=c99 main.c -o main |

### 4.2 测试用例

| 编号 | 测试项 | 输入 | 结果 |
|:---:|:---|:---|:---:|
| T01 | 录入期刊 | ID=1001 | 成功 |
| T02 | 重复拒绝 | 重复ID=1001 | 正确拒绝 |
| T03 | 录入读者 | ID=2001 | 成功 |
| T04 | 查询全部 | 模式1 | 显示完整 |
| T05 | 模糊搜索 | 关键词「计算机」 | 正确匹配 |
| T06 | 借阅 | 1001→2001 | 成功+日期正确 |
| T07 | 重复借阅 | 已借出 | 提示预约 |
| T08 | 预约 | 2003预约1001 | 成功 |
| T09 | 重复预约 | 2003再次预约 | 正确拒绝 |
| T10 | 队列满 | 第11人预约 | 正确拒绝 |
| T11 | 归还联动 | 归还+有预约 | 自动提醒 |
| T12 | 超期检查 | 日期晚于应还 | 正确筛选 |

### 4.3 测试结果

所有测试用例通过。系统功能完整，运行稳定。

---

## 5. 总结

本次课程设计实现了一个功能完整的期刊借阅管理系统，完成了期刊与读者的增删查改、借阅归还、预约管理、超期检查和统计信息等全部功能。系统代码组织清晰、注释详尽（总计约1205行，含大量设计说明注释），采用模块化设计思想，划分为菜单界面、工具函数、预约队列操作、期刊链表操作、读者链表操作、文件持久化、核心业务功能和内置测试八个模块，各模块职责单一、耦合度低。

在技术实现方面，深入理解了单链表和链式队列在实际业务中的应用，掌握了枚举类型（JournalStatus/OpResult）的设计和使用方法，巩固了文本文件读写（fprintf/fscanf）和结构体序列化的相关知识。预约队列采用三元组(front/rear/count)管理，展示了指针操作的经典技巧（rear指针优化入队、出队时rear置空防悬空指针）。日期计算模块利用mktime()自动处理溢出，展示了C语言标准库时间函数的正确用法。通过本次实践，对C语言结构体设计、指针操作、内存管理、文件I/O和防御性编程有了更深层次的理解。

### 改进方向

- **查找效率O(n)**: 改用哈希表或平衡树，提升大规模数据查找效率
- **文本格式不标准**: 改用JSON或SQLite，提升兼容性和数据完整性
- **未实现删除操作**: 添加软删除标志或按ID删除节点
- **单机无网络支持**: 引入Socket或多线程，实现C/S架构多用户访问
- **纯文本终端界面**: 增加ncurses彩色界面和交互引导

---

## ? 快速开始

1. 打开 **Code::Blocks**
2. 创建新项目，将 `main.c` 中的代码完整复制粘贴到源文件中
3. 点击编译运行即可


---

## ? 联系作者

| 平台 | 账号/链接 |
|:---:|:---|
| ? 抖音 | `65178267913` |
| ? CSDN | [惠俊卓的博客](https://blog.csdn.net/hjz0810?type=blog) |

> **作者：惠俊卓** · **学号：424123888888** · **班级：华信竞赛队A部**
> **审核：HuiJunzhuo** · **2025-2026学年第二学期算法复习**
