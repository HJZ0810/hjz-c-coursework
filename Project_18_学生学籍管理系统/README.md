<div align="center">

#  学生学籍管理系统

###  2025-2026学年第二学期算法复习

![Language](https://img.shields.io/badge/Language-C99-blue?style=for-the-badge&logo=c&logoColor=white)
![Data Structure](https://img.shields.io/badge/Data_Structure-Singly_Linked_List_Multi_Entity-green?style=for-the-badge)
![Status](https://img.shields.io/badge/Status-Completed-brightgreen?style=for-the-badge)
![Project](https://img.shields.io/badge/Project-18-orange?style=for-the-badge)

---

**? 作者：惠俊卓** · **学号：424123888888**
** 班级：华信竞赛队A部**
** 审核：HuiJunzhuo**

---

*基于Singly_Linked_List_Multi_Entity实现的学生学籍管理系统*

</div>

---

##  目录

- [1. 需求分析](#1-需求分析)
- [2. 概要设计](#2-概要设计)
- [3. 详细设计](#3-详细设计)
- [4. 测试分析](#4-测试分析)
- [5. 总结](#5-总结)
- [ 快速始](#-快速开始)
- [ 联系作者](#-联系作者)

---

## 1. 需求分析

### 1.1 项目背景

在高校学生管理工作中，学籍信息涉及学生基本信息、奖惩记录、课程选修、考试成绩等多方面数据，传统的人工管理方式效率低下、容易出错，且在信息查询、统计报表、数据备份等环节需要大量重复劳动。随着学生规模的扩大和管理的精细化，手工管理方式已无法满足现代学籍管理对准确性、实时性和便捷性的需求。

本项目旨在运用C语言和数据结构知识，设计并实现一个功能完善的学生学籍管理系统。该系统基于单向链表数据结构管理多类实体信息，支持学生基本信息管理、奖惩信息管理、教师信息管理、课程信息管理、排课管理、选课管理和成绩管理等完整业务流程。系统提供两种启动模式（测试模式加载预设数据/正常模式从文件加载），覆盖学生学籍管理的各项核心需求，能够满足中小规模院校的学籍管理需求。

### 1.2 功能需求

| 序号 | 功能模块 | 说明 |
|:---:|:---|:---|
| 1 | 添加/删除/修改/查询学生 | 完整CRUD，学号唯一性校验 |
| 2 | 奖惩信息管理 | 头插法，支持按学生+类型筛选删除 |
| 3 | 教师信息管理 | 完整CRUD，工号唯一性校验 |
| 4 | 课程信息管理 | 完整CRUD，课程编号唯一性校验 |
| 5 | 排课管理 | 验证外键（课程+教师），排课编号唯一 |
| 6 | 选课管理 | 验证学号+排课编号，防止重复选课 |
| 7 | 成绩管理 | 自动等级计算，平均分统计 |
| 8 | 文件持久化 | |分隔符，7个独立.dat文件 |
| 9 | 测试模式 | 一键加载完整测试数据集 |
| 10 | 菜单界面 | 层次化菜单，交互式操作 |

### 1.3 数据需求

**Student（学生）结构体：**

| 字段 | 类型 | 说明 |
|:---|:---|:---|
| id | char[20] | 学号，主键，全局唯一 |
| name | char[50] | 姓名 |
| gender | char[8] | 性别：男/女 |
| age | int | 年龄 |
| className | char[30] | 班级名称 |
| phone | char[20] | 联系电话 |
| address | char[200] | 家庭住址 |

**Grade（成绩）结构体：**

| 字段 | 类型 | 说明 |
|:---|:---|:---|
| studentId | char[20] | 学号（联合主键之一） |
| scheduleId | char[20] | 排课编号（联合主键之一） |
| score | float | 成绩分数（0~100） |
| grade | char[8] | 等级（优/良/中/及格/不及格） |

---

## 2. 概要设计

### 2.1 数据结构设计

系统采用7个独立的单向链表分别管理7类实体数据。每个结构体都包含一个next指针用于链表串联，通过不同实体的主键（id/studentId+scheduleId）进行唯一标识和关联查询

选择单向链表而非双向链表的原因：系统以遍历查询为主要操作，删除操作通过前驱指针即可完成，双向链表的额外指针开销在此场景下收益有限。选择链表而非数组的原因：学生数量不确定，链表支持动态增长，且删除操作无需移动大量元素。排课编号（而非课程编号）作为选课的外键，因为同一课程在不同学期由不同教师授课时是不同的「可选课程项」，这符合实际教学场景

### 2.2 系统架构

```
┌────────────────────────────────────┐
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
| 学生基本信息管理 | Student链表CRUD + 姓名模糊搜索 |
| 奖惩信息管理 | RewardPunishment链表 + 头插法 |
| 任课教师管理 | Teacher链表CRUD |
| 课程科目管理 | Course链表CRUD |
| 开课时间管理 | CourseSchedule链表 + 外键验证 |
| 学生选课管理 | Enrollment链表 + 外键验证 |
| 成绩管理 | Grade链表 + 自动等级计算 |
| 文件持久化 | 7个.dat文件 + |分隔符 |
| 内存管理 | 释放所有链表 |
| 测试数据 | 一键加载 |

---

## 3. 详细设计

### 3.1 核心数据结构

```c
typedef struct Student {
    char id[MAX_ID];                /* 学号 (主键，全局唯一)    */
    char name[MAX_NAME];            /* 姓名                     */
    char gender[8];                 /* 性别: "男" / "女"        */
    int  age;                       /* 年龄                     */
    char className[MAX_CLASS];      /* 班级                     */
    char phone[20];                 /* 联系电话                 */
    char address[MAX_DESC];         /* 家庭住址                 */
    struct Student *next;           /* 链表后继指针             */
} Student;
typedef struct RewardPunishment {
    char studentId[MAX_ID];         /* 关联学号（外键）         */
    char type[16];                  /* 类型: "奖励" / "处分"    */
    char date[MAX_TIME];            /* 日期: "YYYY-MM-DD"      */
    char description[MAX_DESC];     /* 描述信息                 */
    struct RewardPunishment *next;  /* 链表后继指针             */
} RewardPunishment;
typedef struct Teacher {
    char id[MAX_ID];                /* 教师工号 (主键)          */
    char name[MAX_NAME];            /* 姓名                     */
    char gender[8];                 /* 性别                     */
    char department[MAX_NAME];      /* 所属院系                 */
    char phone[20];                 /* 联系电话                 */
    struct Teacher *next;           /* 链表后继指针             */
} Teacher;
typedef struct Course {
    char id[MAX_ID];                /* 课程编号 (主键)          */
    char name[MAX_COURSE];          /* 课程名称                 */
    int  credit;                    /* 学分                     */
    int  hours;                     /* 课时                     */
    char description[MAX_DESC];     /* 课程描述                 */
    struct Course *next;            /* 链表后继指针             */
} Course;
typedef struct CourseSchedule {
    char id[MAX_ID];                /* 排课编号 (主键)          */
    char courseId[MAX_ID];          /* 课程编号 (外键)          */
    char teacherId[MAX_ID];         /* 教师工号 (外键)          */
    char semester[MAX_TIME];        /* 学期                     */
    char classroom[MAX_NAME];       /* 教室                     */
    char timeSlot[MAX_DESC];        /* 上课时间                 */
    struct CourseSchedule *next;    /* 链表后继指针             */
} CourseSchedule;
typedef struct Enrollment {
    char id[MAX_ID];                /* 选课编号 (主键)          */
    char studentId[MAX_ID];         /* 学号 (外键)              */
    char scheduleId[MAX_ID];        /* 排课编号 (外键)          */
    char date[MAX_TIME];            /* 选课日期                 */
    struct Enrollment *next;        /* 链表后继指针             */
} Enrollment;
typedef struct Grade {
    char studentId[MAX_ID];         /* 学号 (联合主键之一)      */
    char scheduleId[MAX_ID];        /* 排课编号 (联合主键之一)  */
    float score;                    /* 成绩分数 (0~100)         */
    char grade[8];                  /* 等级 (自动计算)          */
    struct Grade *next;             /* 链表后继指针             */
} Grade;
static void safe_input(const char *prompt, char *buf, int size)
{
    printf("%s", prompt);
    if (fgets(buf, size, stdin) != NULL) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';        /* 正常情况：去除末尾换行 */
        } else {
            /* 输入超长：缓冲区已满，清空剩余字符 */
            clear_input_buffer();
        }
    }
    (void)clear_input_buffer;   /* 消除未使用警告（工具函数保留） */
}
static int input_int(const char *prompt)
{
    char buf[64];
    int val;
    while (1) {
        printf("%s", prompt);
        if (fgets(buf, sizeof(buf), stdin) != NULL) {
            if (sscanf(buf, "%d", &val) == 1) {
                return val;
            }
        }
        printf("  [错误] 请输入有效的整数！\n");
    }
}
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
| 1 | 添加学生 | 完整信息 |  成功 |
| 2 | 学号重复 | 重复学号 |  拒绝 |
| 3 | 删除学生 | 存在学号 |  成功 |
| 4 | 删除不存在 | 不存在学号 |  提示未找到 |
| 5 | 修改信息 | 学号+新值 |  更新 |
| 6 | 模糊查询 | 关键词 |  显示匹配 |
| 7 | 成绩+等级 | 分数92.5 |  自动优 |
| 8 | 平均分 | 查询学生 |  显示平均 |
| 9 | 保存加载 | 重启 |  数据恢复 |
| 10 | 外键验证 | 不存在学号 |  拒绝 |

### 4.3 测试结果

所有测试用例通过。系统功能完整，运行稳定。

---

## 5. 总结

本项目实现了一个功能完善的学生学籍管理系统，采用C语言和单向链表数据结构，支持7类实体的全面管理。系统在以下几个方面取得了良好效果：

1. **模块化设计**：7类实体各自独立管理，耦合度低，便于维护和扩展；
2. **数据持久化**：采用「|」分隔符文本文件，人类可读，便于调试和手动编辑；
3. **自动计算**：成绩等级根据分数自动计算，保证数据一致性；
4. **用户友好**：采用「显示原值+回车保留」的修改模式，减少用户输入量；
5. **测试完备**：提供完整的预设测试数据，覆盖所有核心业务场景。

通过本项目的设计与实现，深入理解了单向链表在实际业务系统中的应用方式，掌握了多实体关联设计、文件持久化、输入校验等实用技术。系统在可扩展性和可维护性方面表现良好，为进一步开发Web版或数据库版奠定了良好基础。

### 改进方向

- **WEB界面**: 替换为Web界面
- **数据库**: 使用MySQL/SQLite
- **权限管理**: 增加角色权限
- **导入导出**: 支持Excel
- **统计分析**: 数据可视化

---

##  快速开始

1. 打开 **Code::Blocks**
2. 创建新项目，将 `main.c` 中的代码完整复制粘贴到源文件中
3. 点击编译运行即可


---

##  联系作者

| 平台 | 账号/链接 |
|:---:|:---|
|  抖音 | `65178267913` |
|  CSDN | [惠俊卓的博客](https://blog.csdn.net/hjz0810?type=blog) |

> **作者：惠俊卓** · **学号：424123888888** · **班级：华信竞赛队A部**
> **审核：HuiJunzhuo** · **2025-2026学年第二学期算法复习**

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
