/*
* ============================================================================
*
*    幼儿园培训班学生信息管理系统 V2.0 (增强版)
*    Kindergarten Training Class Student Info Management System V2.0
*
* ============================================================================
*
*  功能概述:
*    基于单向链表实现的幼儿园培训班学生信息管理，支持：
*      [1] 学生信息录入、删除、查询（按编号/项目/班级）
*      [2] 缴费到期提醒（距到期30天自动预警）
*      [3] 退费计算（按剩余完整月数折算，月费=年费/12）
*      [4] CSV文件持久化（students.dat）
*      [5] 按年龄自动分班（幼儿班3-4/小班5/中班6-7/大班8-9岁）
*      [6] 日期合法性校验（yyyy-mm-dd格式，含闰年判断）
*      [7] 缴费年限1-5年，年费2400元
*      [8] 测试数据模式（标准/含边界数据）
*      [9] 按年龄/缴费日期排序
*     [10] 统计仪表盘（总人数/项目分布/班级分布/总收入）
*     [11] 命令行模式 --help / -f 指定数据文件
*     [12] 彩色终端界面 —— ANSI颜色支持（Windows 10+ / Linux）
*     [13] 文件覆盖确认，编号查找Move-to-Front优化
*
*  数据结构:
*    Student  —— 单向链表节点，含id/name/gender/age/project/class_level/
*               pay_date/pay_years/next
*
*  编译命令:
*    gcc -std=c99 -Wall -O2 -o kindergarten.exe main.c
*
*  命令行用法:
*    kindergarten.exe                     交互模式（菜单）
*    kindergarten.exe -f <数据文件>       指定数据文件启动
*    kindergarten.exe -t                  测试模式启动
*    kindergarten.exe -h                  显示帮助
*
*  交互模式:
*    直接运行 kindergarten.exe 进入菜单交互模式
*
*  编码: UTF-8
* ============================================================================
*/

/* ======================== 标准头文件 ======================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define F_OK 0
#define access _access
#else
#include <unistd.h>
#endif

/* ======================== 常量与宏 ======================== */
#define MAX_NAME_LEN        32
#define MAX_ID_LEN          16
#define MAX_DATE_LEN        12
#define PROJECT_COUNT       6
#define CLASS_COUNT         4
#define FEE_PER_YEAR        2400.0f
#define DEFAULT_DATA_FILE   "students.dat"
#define WARN_DAYS           30

/* 颜色宏（ANSI转义序列） */
#define C_RESET     "\033[0m"
#define C_BOLD      "\033[1m"
#define C_DIM       "\033[2m"
#define C_RED       "\033[31m"
#define C_GREEN     "\033[32m"
#define C_YELLOW    "\033[33m"
#define C_BLUE      "\033[34m"
#define C_MAGENTA   "\033[35m"
#define C_CYAN      "\033[36m"
#define C_WHITE     "\033[37m"
#define C_GRAY      "\033[90m"
#define BG_RED      "\033[41m"
#define BG_GREEN    "\033[42m"
#define BG_YELLOW   "\033[43m"
#define BG_BLUE     "\033[44m"
#define BG_ORANGE   "\033[48;5;208m"
#define FG_ORANGE   "\033[38;5;208m"

/* ======================== 错误码 ======================== */
typedef enum {
    KGT_OK = 0,
    KGT_ERR_OPEN_FILE,
    KGT_ERR_READ_FILE,
    KGT_ERR_WRITE_FILE,
    KGT_ERR_MEMORY,
    KGT_ERR_EMPTY,
    KGT_ERR_DUPLICATE_ID,
    KGT_ERR_INVALID_DATE,
    KGT_ERR_INVALID_INPUT,
    KGT_ERR_NOT_FOUND,
    KGT_ERR_CANCEL,
    KGT_ERR_FILE_EXISTS,
} KgtStatus;

/* ======================== 枚举类型 ======================== */
typedef enum {
    PROJECT_PAINTING    = 0,
    PROJECT_SINGING     = 1,
    PROJECT_ELECTRIC_PIANO = 2,
    PROJECT_PIANO       = 3,
    PROJECT_DANCE       = 4,
    PROJECT_LANGUAGE    = 5
} ProjectType;

typedef enum {
    CLASS_TODDLER   = 0,
    CLASS_SMALL     = 1,
    CLASS_MEDIUM    = 2,
    CLASS_LARGE     = 3
} ClassType;

typedef enum {
    GENDER_MALE     = 0,
    GENDER_FEMALE   = 1
} Gender;

typedef enum {
    SORT_NONE = 0,
    SORT_BY_AGE,
    SORT_BY_PAYDATE,
} SortMode;

/* ======================== 数据结构 ======================== */
typedef struct Student {
    char        id[MAX_ID_LEN];
    char        name[MAX_NAME_LEN];
    Gender      gender;
    int         age;
    ProjectType project;
    ClassType   class_level;
    char        pay_date[MAX_DATE_LEN];
    int         pay_years;
    struct Student *next;
} Student;

/* ======================== 全局状态 ======================== */
static Student *g_head = NULL;
static int colorEnabled = 1;
static char g_dataFile[256] = DEFAULT_DATA_FILE;
static SortMode g_sortMode = SORT_NONE;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

/* ======================== 名称查找表 ======================== */
static const char *PROJECT_NAMES[PROJECT_COUNT] = {
    "绘画", "唱歌", "电子琴", "钢琴", "舞蹈", "语言表演"
};

static const char *CLASS_NAMES[CLASS_COUNT] = {
    "幼儿班(3-4岁)", "小班(5岁)", "中班(6-7岁)", "大班(8-9岁)"
};

static const char *GENDER_NAMES[] = {
    "男", "女"
};

/* ======================== 前向声明 ======================== */
static Student *create_student(const char *id, const char *name, Gender gender,
                               int age, ProjectType project, ClassType cls,
                               const char *pay_date, int pay_years);
static void     insert_student(Student *stu);
static Student *find_student_by_id(const char *id);
static int      delete_student_by_id(const char *id);
static void     free_all_students(void);
static int      count_students(void);
static void     sort_students(SortMode mode);

static ClassType    age_to_class(int age);
static int          validate_date(const char *date);
static int          parse_date(const char *date, int *year, int *month, int *day);
static int          days_between(const char *date1, const char *date2);
static const char  *get_current_date(void);
static int          compare_dates(const char *d1, const char *d2);

static void add_student(void);
static void delete_student(void);
static void query_student(void);
static void query_by_project(void);
static void query_by_class(void);
static void show_all_students(void);
static void check_payment_due(void);
static void calculate_refund(void);
static KgtStatus   save_to_file(const char *filename);
static KgtStatus   load_from_file(const char *filename);
static void print_student(const Student *stu);
static void show_statistics(void);
static void sort_menu(void);

static void load_test_data(int include_boundary);
static void run_menu(void);

/* ======================== 工具函数：颜色与终端 ======================== */

static void enableVTMode(void) {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) { colorEnabled = 0; return; }
    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) { colorEnabled = 0; return; }
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
    colorEnabled = 1;
#endif
}

static void cPrint(const char *color, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (colorEnabled && color) fputs(color, stdout);
    vprintf(fmt, args);
    if (colorEnabled && color) fputs(C_RESET, stdout);
    va_end(args);
}

static void printOK(const char *msg)    { cPrint(C_GREEN, "  [OK] "); printf("%s\n", msg); }
static void printErr(const char *msg)   { cPrint(C_RED,   "  [ERROR] "); printf("%s\n", msg); }
static void printInfo(const char *msg)  { cPrint(C_CYAN,  "  [INFO] "); printf("%s\n", msg); }

/* ======================== 工具函数：UI ======================== */

static void printBanner(void) {
    printf("\n");
    cPrint(C_BOLD C_BLUE,
        "  ============================================================================\n");
    cPrint(C_BOLD C_CYAN,
        "  |       Kindergarten Student Info Management System V2.0 (Enhanced)        |\n");
    cPrint(C_BOLD C_CYAN,
        "  |                     幼儿园培训班学生信息管理系统 增强版                   |\n");
    cPrint(C_BOLD C_BLUE,
        "  ============================================================================\n");
    printf("\n");
}

static void printDivider(void) {
    cPrint(C_GRAY, "  ----------------------------------------------------------------------------\n");
}

static void printThickDivider(void) {
    cPrint(C_BLUE, "  ============================================================================\n");
}

static void printSeparator(void) {
    cPrint(C_GRAY, "  ---------------------------------------------------------------"
           "--------------------\n");
}

static int readInt(const char *prompt, int min, int max) {
    int val;
    char buf[256];
    while (1) {
        printf("  %s", prompt);
        fflush(stdout);
        if (fgets(buf, sizeof(buf), stdin) == NULL) { clearerr(stdin); continue; }
        char *nl = strchr(buf, '\n');
        if (nl) *nl = '\0'; else { int c; while ((c=getchar())!='\n' && c!=EOF); }
        if (sscanf(buf, "%d", &val) == 1 && val >= min && val <= max)
            return val;
        cPrint(C_YELLOW, "  [!] 请输入 [%d, %d] 范围内的整数\n", min, max);
    }
}

static void readString(const char *prompt, char *buf, int size) {
    while (1) {
        printf("  %s", prompt);
        fflush(stdout);
        if (fgets(buf, size, stdin) == NULL) { clearerr(stdin); continue; }
        char *nl = strchr(buf, '\n');
        if (nl) *nl = '\0'; else { int c; while ((c=getchar())!='\n' && c!=EOF); }
        return;
    }
}

static int readYesNo(const char *prompt) {
    char buf[64];
    while (1) {
        printf("  %s (Y/N): ", prompt);
        fflush(stdout);
        if (fgets(buf, sizeof(buf), stdin) == NULL) return 0;
        char c = (char)toupper((unsigned char)buf[0]);
        if (c == 'Y') return 1;
        if (c == 'N') return 0;
        cPrint(C_YELLOW, "  [!] 请输入 Y 或 N\n");
    }
}

static int fileExists(const char *path) {
    return access(path, 0) == 0;
}

static void printStudentHeader(void) {
    printSeparator();
    cPrint(C_BOLD, "  %-8s %-10s %-4s %-4s %-10s %-16s %-12s %-4s\n",
           "编号", "姓名", "性别", "年龄", "项目", "班级", "缴费日期", "年限");
    printSeparator();
}

/* ======================== 链表基本操作 ======================== */

static Student *create_student(const char *id, const char *name, Gender gender,
                               int age, ProjectType project, ClassType cls,
                               const char *pay_date, int pay_years) {
    Student *stu = (Student *)malloc(sizeof(Student));
    if (!stu) {
        printErr("内存分配失败！");
        return NULL;
    }
    strncpy(stu->id, id, MAX_ID_LEN - 1);
    stu->id[MAX_ID_LEN - 1] = '\0';
    strncpy(stu->name, name, MAX_NAME_LEN - 1);
    stu->name[MAX_NAME_LEN - 1] = '\0';
    stu->gender      = gender;
    stu->age         = age;
    stu->project     = project;
    stu->class_level = cls;
    strncpy(stu->pay_date, pay_date, MAX_DATE_LEN - 1);
    stu->pay_date[MAX_DATE_LEN - 1] = '\0';
    stu->pay_years   = pay_years;
    stu->next        = NULL;
    return stu;
}

static void insert_student(Student *stu) {
    if (!stu) return;
    stu->next = NULL;
    if (!g_head) {
        g_head = stu;
    } else {
        Student *p = g_head;
        while (p->next) p = p->next;
        p->next = stu;
    }
    g_sortMode = SORT_NONE;
}

static Student *find_student_by_id(const char *id) {
    Student *p = g_head;
    Student *prev = NULL;
    while (p) {
        if (strcmp(p->id, id) == 0) {
            if (prev) {
                prev->next = p->next;
                p->next = g_head;
                g_head = p;
            }
            return p;
        }
        prev = p;
        p = p->next;
    }
    return NULL;
}

static int delete_student_by_id(const char *id) {
    if (!g_head) return 0;
    if (strcmp(g_head->id, id) == 0) {
        Student *target = g_head;
        g_head = g_head->next;
        free(target);
        return 1;
    }
    Student *p = g_head;
    while (p->next) {
        if (strcmp(p->next->id, id) == 0) {
            Student *target = p->next;
            p->next = target->next;
            free(target);
            return 1;
        }
        p = p->next;
    }
    return 0;
}

static void free_all_students(void) {
    Student *p = g_head;
    while (p) {
        Student *tmp = p;
        p = p->next;
        free(tmp);
    }
    g_head = NULL;
}

static int count_students(void) {
    int count = 0;
    Student *p = g_head;
    while (p) { count++; p = p->next; }
    return count;
}

/* ======================== 排序功能 ======================== */

static int compare_dates(const char *d1, const char *d2) {
    return strcmp(d1, d2);
}

static void sort_students(SortMode mode) {
    if (!g_head || !g_head->next || mode == SORT_NONE) {
        g_sortMode = mode;
        return;
    }
    Student *sorted = NULL;
    Student *current = g_head;
    while (current) {
        Student *next = current->next;
        Student **pp = &sorted;
        while (*pp) {
            int cmp = 0;
            if (mode == SORT_BY_AGE) {
                cmp = (*pp)->age - current->age;
            } else if (mode == SORT_BY_PAYDATE) {
                cmp = compare_dates((*pp)->pay_date, current->pay_date);
            }
            if (cmp > 0) break;
            pp = &(*pp)->next;
        }
        current->next = *pp;
        *pp = current;
        current = next;
    }
    g_head = sorted;
    g_sortMode = mode;
}

static void sort_menu(void) {
    printf("\n");
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  排序功能\n");
    printDivider();
    printf("  1 - 按年龄排序\n");
    printf("  2 - 按缴费日期排序\n");
    printf("  0 - 取消排序（恢复录入顺序）\n");
    printDivider();
    int ch = readInt("请选择 (0-2): ", 0, 2);
    switch (ch) {
    case 1:
        sort_students(SORT_BY_AGE);
        printOK("已按年龄从小到大排序");
        break;
    case 2:
        sort_students(SORT_BY_PAYDATE);
        printOK("已按缴费日期从早到晚排序");
        break;
    case 0:
        g_sortMode = SORT_NONE;
        printInfo("排序已取消");
        break;
    }
}

/* ======================== 工具函数：日期与输入 ======================== */

static ClassType age_to_class(int age) {
    if (age >= 3 && age <= 4) return CLASS_TODDLER;
    if (age == 5)             return CLASS_SMALL;
    if (age >= 6 && age <= 7) return CLASS_MEDIUM;
    if (age >= 8 && age <= 9) return CLASS_LARGE;
    if (age < 3) return CLASS_TODDLER;
    return CLASS_LARGE;
}

static int validate_date(const char *date) {
    int y, m, d;
    if (!date || strlen(date) != 10) return 0;
    if (date[4] != '-' || date[7] != '-') return 0;
    if (sscanf(date, "%d-%d-%d", &y, &m, &d) != 3) return 0;
    if (y < 2000 || y > 2100) return 0;
    if (m < 1 || m > 12) return 0;
    if (d < 1 || d > 31) return 0;
    if ((m == 4 || m == 6 || m == 9 || m == 11) && d > 30) return 0;
    if (m == 2) {
        int leap = (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
        if (d > (leap ? 29 : 28)) return 0;
    }
    return 1;
}

static int parse_date(const char *date, int *year, int *month, int *day) {
    if (!validate_date(date)) return 0;
    return (sscanf(date, "%d-%d-%d", year, month, day) == 3) ? 1 : 0;
}

static int days_between(const char *date1, const char *date2) {
    int y1, m1, d1, y2, m2, d2;
    if (!parse_date(date1, &y1, &m1, &d1) || !parse_date(date2, &y2, &m2, &d2)) {
        return -1;
    }
    struct tm tm1 = {0}, tm2 = {0};
    tm1.tm_year = y1 - 1900; tm1.tm_mon = m1 - 1; tm1.tm_mday = d1;
    tm2.tm_year = y2 - 1900; tm2.tm_mon = m2 - 1; tm2.tm_mday = d2;
    time_t t1 = mktime(&tm1);
    time_t t2 = mktime(&tm2);
    if (t1 == (time_t)-1 || t2 == (time_t)-1) return -1;
    return (int)difftime(t2, t1) / 86400;
}

static const char *get_current_date(void) {
    static char buf[32];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
    return buf;
}

/* ======================== 显示与打印函数 ======================== */

static void print_student(const Student *stu) {
    if (!stu) return;
    const char *classColor = C_CYAN;
    switch (stu->class_level) {
        case CLASS_TODDLER: classColor = FG_ORANGE; break;
        case CLASS_SMALL:   classColor = C_GREEN;   break;
        case CLASS_MEDIUM:  classColor = C_YELLOW;  break;
        case CLASS_LARGE:   classColor = C_MAGENTA; break;
    }
    printf("  %-8s %-10s ", stu->id, stu->name);
    cPrint(stu->gender == GENDER_FEMALE ? C_MAGENTA : C_BLUE, "%-4s", GENDER_NAMES[stu->gender]);
    printf(" %-4d ", stu->age);
    cPrint(C_CYAN, "%-10s", PROJECT_NAMES[stu->project]);
    printf(" ");
    cPrint(classColor, "%-20s", CLASS_NAMES[stu->class_level]);
    printf(" %-12s %-4d\n", stu->pay_date, stu->pay_years);
}

/* ======================== 核心业务功能 ======================== */

static void add_student(void) {
    char id[MAX_ID_LEN];
    char name[MAX_NAME_LEN];
    char pay_date[MAX_DATE_LEN];
    int gender, age, project, pay_years;

    printf("\n");
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  录入学生信息\n");
    printDivider();

    readString("请输入编号: ", id, MAX_ID_LEN);
    if (strlen(id) == 0) { printErr("编号不能为空！"); return; }
    if (find_student_by_id(id)) {
        cPrint(C_RED, "  [ERROR] 编号 ");
        cPrint(C_BOLD C_RED, "%s", id);
        cPrint(C_RED, " 已存在！\n");
        return;
    }

    readString("请输入姓名: ", name, MAX_NAME_LEN);
    if (strlen(name) == 0) { printErr("姓名不能为空！"); return; }

    gender = readInt("请输入性别 (0=男, 1=女): ", 0, 1);
    age = readInt("请输入年龄 (3-9): ", 3, 9);

    cPrint(C_CYAN, "  可选项目:\n");
    for (int i = 0; i < PROJECT_COUNT; i++) {
        printf("    %d. %s\n", i, PROJECT_NAMES[i]);
    }
    project = readInt("请选择项目编号 (0-5): ", 0, PROJECT_COUNT - 1);

    while (1) {
        readString("请输入缴费日期 (yyyy-mm-dd): ", pay_date, MAX_DATE_LEN);
        if (validate_date(pay_date)) break;
        printErr("日期格式不正确，请重新输入（格式: yyyy-mm-dd）");
    }

    pay_years = readInt("请输入缴费年限 (1-5): ", 1, 5);

    ClassType cls = age_to_class(age);
    Student *stu = create_student(id, name, (Gender)gender, age,
                                  (ProjectType)project, cls, pay_date, pay_years);
    if (stu) {
        insert_student(stu);
        cPrint(C_GREEN, "  [OK] 已录入 ");
        cPrint(C_BOLD C_WHITE, "%s", name);
        cPrint(C_GREEN, "（%s，", PROJECT_NAMES[project]);
        cPrint(C_CYAN, "%s", CLASS_NAMES[cls]);
        cPrint(C_GREEN, "）\n");
    }
}

static void delete_student(void) {
    char id[MAX_ID_LEN];

    printf("\n");
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  删除学生信息\n");
    printDivider();

    readString("请输入要删除的学生编号: ", id, MAX_ID_LEN);

    Student *stu = find_student_by_id(id);
    if (!stu) {
        cPrint(C_YELLOW, "  [WARN] 未找到编号为 ");
        cPrint(C_BOLD, "%s", id);
        cPrint(C_YELLOW, " 的学生\n");
        return;
    }

    cPrint(C_CYAN, "  找到以下学生:\n");
    printStudentHeader();
    print_student(stu);
    printSeparator();

    if (readYesNo("确认删除")) {
        delete_student_by_id(id);
        cPrint(C_GREEN, "  [OK] 已删除编号 ");
        cPrint(C_BOLD, "%s", id);
        cPrint(C_GREEN, " 的学生\n");
    } else {
        cPrint(C_GRAY, "  [取消] 删除操作已取消\n");
    }
}

static void query_student(void) {
    char id[MAX_ID_LEN];

    printf("\n");
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  按编号查询\n");
    printDivider();

    readString("请输入学生编号: ", id, MAX_ID_LEN);

    Student *stu = find_student_by_id(id);
    if (!stu) {
        cPrint(C_YELLOW, "  [WARN] 未找到编号为 %s 的学生\n", id);
        return;
    }

    printStudentHeader();
    print_student(stu);
    printSeparator();
}

static void show_all_students(void) {
    printf("\n");
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  所有学生信息");
    if (g_sortMode == SORT_BY_AGE) cPrint(C_GREEN, " (按年龄排序)");
    else if (g_sortMode == SORT_BY_PAYDATE) cPrint(C_GREEN, " (按缴费日期排序)");
    printf("\n");
    printDivider();

    if (!g_head) {
        cPrint(C_YELLOW, "  [WARN] 当前没有学生信息\n");
        return;
    }

    int count = 0;
    printStudentHeader();
    Student *p = g_head;
    while (p) {
        print_student(p);
        count++;
        p = p->next;
    }
    printSeparator();
    cPrint(C_CYAN, "  共 %d 条记录\n", count);
}

static void query_by_project(void) {
    printf("\n");
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  按项目分组统计\n");
    printDivider();

    for (int proj = 0; proj < PROJECT_COUNT; proj++) {
        int total = 0;
        int cls_count[CLASS_COUNT] = {0};
        Student *p = g_head;
        while (p) {
            if ((int)p->project == proj) {
                total++;
                cls_count[p->class_level]++;
            }
            p = p->next;
        }

        printf("\n  ");
        cPrint(C_BOLD C_CYAN, "【%s】", PROJECT_NAMES[proj]);
        printf(" 共 ");
        cPrint(C_BOLD C_WHITE, "%d", total);
        printf(" 人\n");
        printSeparator();
        cPrint(C_BOLD, "  %-20s %-6s\n", "班级", "人数");
        printSeparator();
        for (int c = 0; c < CLASS_COUNT; c++) {
            const char *cc = C_CYAN;
            switch (c) {
                case CLASS_TODDLER: cc = FG_ORANGE; break;
                case CLASS_SMALL:   cc = C_GREEN; break;
                case CLASS_MEDIUM:  cc = C_YELLOW; break;
                case CLASS_LARGE:   cc = C_MAGENTA; break;
            }
            printf("  ");
            cPrint(cc, "%-20s", CLASS_NAMES[c]);
            printf(" %-6d\n", cls_count[c]);
        }

        if (total > 0) {
            printf("\n  ");
            cPrint(C_CYAN, "学生列表:\n");
            printStudentHeader();
            p = g_head;
            while (p) {
                if ((int)p->project == proj) print_student(p);
                p = p->next;
            }
            printSeparator();
        }
    }
}

static void query_by_class(void) {
    printf("\n");
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  按班级分组统计\n");
    printDivider();

    for (int cls = 0; cls < CLASS_COUNT; cls++) {
        int total = 0;
        int proj_count[PROJECT_COUNT] = {0};
        Student *p = g_head;
        while (p) {
            if ((int)p->class_level == cls) {
                total++;
                proj_count[p->project]++;
            }
            p = p->next;
        }

        const char *cc = C_CYAN;
        switch (cls) {
            case CLASS_TODDLER: cc = FG_ORANGE; break;
            case CLASS_SMALL:   cc = C_GREEN; break;
            case CLASS_MEDIUM:  cc = C_YELLOW; break;
            case CLASS_LARGE:   cc = C_MAGENTA; break;
        }
        printf("\n  ");
        cPrint(C_BOLD, "【");
        cPrint(cc, "%s", CLASS_NAMES[cls]);
        cPrint(C_BOLD, "】");
        printf(" 共 ");
        cPrint(C_BOLD C_WHITE, "%d", total);
        printf(" 人\n");
        printSeparator();
        cPrint(C_BOLD, "  %-12s %-6s\n", "项目", "人数");
        printSeparator();
        for (int pr = 0; pr < PROJECT_COUNT; pr++) {
            printf("  %-12s %-6d\n", PROJECT_NAMES[pr], proj_count[pr]);
        }

        if (total > 0) {
            printf("\n  ");
            cPrint(C_CYAN, "学生列表:\n");
            printStudentHeader();
            p = g_head;
            while (p) {
                if ((int)p->class_level == cls) print_student(p);
                p = p->next;
            }
            printSeparator();
        }
    }
}

static void check_payment_due(void) {
    printf("\n");
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  缴费到期提醒\n");
    printDivider();

    const char *today = get_current_date();
    printf("  当前日期: ");
    cPrint(C_BOLD C_WHITE, "%s\n\n", today);

    int found = 0;
    int expired = 0;
    int dueSoon = 0;
    Student *p = g_head;
    while (p) {
        int y, m, d;
        if (parse_date(p->pay_date, &y, &m, &d)) {
            y += p->pay_years;
            char expire_date[MAX_DATE_LEN];
            snprintf(expire_date, MAX_DATE_LEN, "%04d-%02d-%02d", y, m, d);
            int days = days_between(today, expire_date);
            if (days < 0) {
                cPrint(C_RED, "  [!] ");
                printf("[%s] ", p->id);
                cPrint(C_BOLD, "%s", p->name);
                cPrint(C_RED, " 已过期 %d 天！请尽快续费。\n", -days);
                found++; expired++;
            } else if (days <= WARN_DAYS) {
                cPrint(C_YELLOW, "  [*] ");
                printf("[%s] ", p->id);
                cPrint(C_BOLD, "%s", p->name);
                cPrint(C_YELLOW, " 将在 %d 天后到期，请及时续费。\n", days);
                found++; dueSoon++;
            }
        }
        p = p->next;
    }

    if (!found) {
        cPrint(C_GREEN, "  [OK] 目前没有即将到期的学生，所有缴费正常\n");
    } else {
        printDivider();
        cPrint(C_RED, "  已过期: %d 人  ", expired);
        cPrint(C_YELLOW, "即将到期: %d 人  ", dueSoon);
        cPrint(C_CYAN, "共需关注: %d 人\n", found);
    }
}

static void calculate_refund(void) {
    printf("\n");
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  退费计算\n");
    printDivider();

    const char *today = get_current_date();
    printf("  当前日期: ");
    cPrint(C_BOLD C_WHITE, "%s\n\n", today);

    int found = 0;
    float total_refund = 0.0f;
    float total_paid = 0.0f;

    Student *p = g_head;
    while (p) {
        int days = days_between(p->pay_date, today);
        float total_fee = FEE_PER_YEAR * p->pay_years;
        total_paid += total_fee;

        if (days < 0) {
            cPrint(C_GRAY, "  [--] [%s] %s 缴费日期在未来，不计算退费。\n", p->id, p->name);
            p = p->next;
            continue;
        }

        int total_days = p->pay_years * 365;
        if (days >= total_days) {
            cPrint(C_RED, "  [!] [%s] %s 已超出缴费期，无退费。\n", p->id, p->name);
        } else {
            int remain_days = total_days - days;
            int remain_months = (remain_days + 29) / 30;
            float monthly_fee = FEE_PER_YEAR / 12.0f;
            float refund = remain_months * monthly_fee;
            if (refund > total_fee) refund = total_fee;
            float used_fee = total_fee - refund;

            printf("  [%s] ", p->id);
            cPrint(C_BOLD, "%-10s", p->name);
            cPrint(C_CYAN, " 总费用: %.0f元", total_fee);
            cPrint(C_YELLOW, " | 已用: %.0f元", used_fee);
            cPrint(C_GREEN, " | 退费: %.0f元\n", refund);
            total_refund += refund;
            found++;
        }
        p = p->next;
    }

    printDivider();
    if (found > 0) {
        cPrint(C_CYAN, "  已缴总额: %.2f 元\n", total_paid);
        cPrint(C_GREEN C_BOLD, "  退费总额: %.2f 元\n", total_refund);
    } else {
        cPrint(C_YELLOW, "  [WARN] 没有可退费的学生\n");
    }
}

static KgtStatus save_to_file(const char *filename) {
    if (!filename) filename = g_dataFile;

    if (fileExists(filename)) {
        printf("\n  文件 ");
        cPrint(C_YELLOW, "%s", filename);
        printf(" 已存在。\n");
        if (!readYesNo("是否覆盖")) {
            cPrint(C_GRAY, "  [取消] 保存操作已取消\n");
            return KGT_ERR_CANCEL;
        }
    }

    FILE *fp = fopen(filename, "w");
    if (!fp) {
        printErr("无法打开文件进行写入！");
        return KGT_ERR_OPEN_FILE;
    }

    int count = 0;
    Student *p = g_head;
    while (p) {
        fprintf(fp, "%s,%s,%d,%d,%d,%d,%s,%d\n",
                p->id, p->name, (int)p->gender, p->age,
                (int)p->project, (int)p->class_level,
                p->pay_date, p->pay_years);
        count++;
        p = p->next;
    }

    fclose(fp);
    cPrint(C_GREEN, "  [OK] 已保存 %d 条记录到文件 ", count);
    cPrint(C_BOLD C_WHITE, "%s\n", filename);
    return KGT_OK;
}

static KgtStatus load_from_file(const char *filename) {
    if (!filename) filename = g_dataFile;

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        cPrint(C_YELLOW, "  [WARN] 文件 %s 不存在，将从空数据开始\n", filename);
        return KGT_ERR_OPEN_FILE;
    }

    free_all_students();

    char line[256];
    int count = 0;
    int line_num = 0;
    int skipped = 0;

    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
        if (len > 1 && line[len - 2] == '\r') line[len - 2] = '\0';

        char id[MAX_ID_LEN], name[MAX_NAME_LEN], pay_date[MAX_DATE_LEN];
        int gender, age, project, cls, pay_years;

        int parsed = sscanf(line, "%15[^,],%31[^,],%d,%d,%d,%d,%11[^,],%d",
                            id, name, &gender, &age, &project, &cls,
                            pay_date, &pay_years);

        if (parsed != 8) { skipped++; continue; }
        if (strlen(id) == 0 || strlen(name) == 0) { skipped++; continue; }
        if (gender < 0 || gender > 1) { skipped++; continue; }
        if (age < 3 || age > 9) { skipped++; continue; }
        if (project < 0 || project >= PROJECT_COUNT) { skipped++; continue; }
        if (cls < 0 || cls >= CLASS_COUNT) { skipped++; continue; }
        if (!validate_date(pay_date)) { skipped++; continue; }
        if (pay_years < 1 || pay_years > 5) { skipped++; continue; }

        Student *stu = create_student(id, name, (Gender)gender, age,
                                      (ProjectType)project, (ClassType)cls,
                                      pay_date, pay_years);
        if (stu) { insert_student(stu); count++; }
    }

    fclose(fp);
    cPrint(C_GREEN, "  [OK] 从文件 %s 加载了 %d 条记录", filename, count);
    if (skipped > 0) cPrint(C_YELLOW, "（跳过 %d 条无效记录）", skipped);
    printf("\n");
    return KGT_OK;
}

/* ======================== 统计仪表盘 ======================== */

static void show_statistics(void) {
    printf("\n");
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  统计仪表盘\n");
    printThickDivider();

    int total = count_students();
    printf("  学生总数: ");
    cPrint(C_BOLD C_WHITE C_GREEN, " %d 人\n", total);
    printDivider();

    if (total == 0) {
        cPrint(C_YELLOW, "  [WARN] 当前没有学生数据\n");
        printThickDivider();
        return;
    }

    int male = 0, female = 0;
    int proj_count[PROJECT_COUNT] = {0};
    int cls_count[CLASS_COUNT] = {0};
    float total_revenue = 0.0f;

    Student *p = g_head;
    while (p) {
        if (p->gender == GENDER_MALE) male++; else female++;
        proj_count[p->project]++;
        cls_count[p->class_level]++;
        total_revenue += FEE_PER_YEAR * p->pay_years;
        p = p->next;
    }

    cPrint(C_BOLD C_CYAN, "  性别分布\n");
    printDivider();
    printf("  男生: "); cPrint(C_BLUE,   "%d 人 (%.1f%%)", male,   total>0?(float)male/total*100:0);
    printf("    女生: "); cPrint(C_MAGENTA,"%d 人 (%.1f%%)\n", female, total>0?(float)female/total*100:0);
    printDivider();

    cPrint(C_BOLD C_CYAN, "  项目分布\n");
    printDivider();
    for (int i = 0; i < PROJECT_COUNT; i++) {
        int barLen = proj_count[i] > 0 ? proj_count[i] : 0;
        int maxBar = 20;
        if (total > 0) barLen = (int)((float)proj_count[i] / total * maxBar) + (proj_count[i]>0?1:0);
        if (barLen > maxBar) barLen = maxBar;
        printf("  %-10s ", PROJECT_NAMES[i]);
        cPrint(C_GREEN, "%-*s", barLen, "████████████████████");
        printf(" %d 人 (%.1f%%)\n", proj_count[i], total>0?(float)proj_count[i]/total*100:0);
    }
    printDivider();

    cPrint(C_BOLD C_CYAN, "  班级分布\n");
    printDivider();
    const char *classColors[] = {FG_ORANGE, C_GREEN, C_YELLOW, C_MAGENTA};
    for (int i = 0; i < CLASS_COUNT; i++) {
        int barLen = 0;
        int maxBar = 20;
        if (total > 0) barLen = (int)((float)cls_count[i] / total * maxBar) + (cls_count[i]>0?1:0);
        if (barLen > maxBar) barLen = maxBar;
        printf("  ");
        cPrint(classColors[i], "%-20s", CLASS_NAMES[i]);
        cPrint(classColors[i], " %-*s", barLen, "████████████████████");
        printf(" %d 人 (%.1f%%)\n", cls_count[i], total>0?(float)cls_count[i]/total*100:0);
    }
    printDivider();

    cPrint(C_BOLD C_CYAN, "  收入统计\n");
    printDivider();
    printf("  年费标准: "); cPrint(C_YELLOW, "%.0f 元/年", FEE_PER_YEAR);
    printf("    缴费年限范围: 1-5 年\n");
    printf("  总收入: ");
    cPrint(C_BOLD C_GREEN, " %.2f 元\n", total_revenue);
    printf("  人均缴费: ");
    cPrint(C_CYAN, "%.2f 元/人\n", total>0?total_revenue/total:0);

    printThickDivider();
}

/* ======================== 测试数据 ======================== */

static void load_test_data(int include_boundary) {
    printf("\n");
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  加载测试数据\n");
    printDivider();

    typedef struct {
        const char *id;
        const char *name;
        int gender;
        int age;
        int project;
        const char *pay_date;
        int pay_years;
    } TestData;

    TestData valid_data[] = {
        {"S001", "小明",    0, 4, 0, "2025-03-01", 1},
        {"S002", "小红",    1, 5, 0, "2025-03-15", 2},
        {"S003", "小华",    0, 7, 1, "2024-09-01", 1},
        {"S004", "小丽",    1, 8, 1, "2025-01-10", 2},
        {"S005", "小强",    0, 3, 2, "2025-06-01", 1},
        {"S006", "小芳",    1, 6, 2, "2025-02-20", 3},
        {"S007", "小伟",    0, 9, 3, "2024-05-01", 2},
        {"S008", "小雪",    1, 4, 3, "2025-04-01", 1},
        {"S009", "小杰",    0, 6, 4, "2025-07-01", 2},
        {"S010", "小美",    1, 7, 4, "2025-01-01", 1},
        {"S011", "小龙",    0, 5, 5, "2025-03-01", 3},
        {"S012", "小琳",    1, 8, 5, "2024-08-01", 1},
    };
    int valid_count = sizeof(valid_data) / sizeof(valid_data[0]);

    for (int i = 0; i < valid_count; i++) {
        ClassType cls = age_to_class(valid_data[i].age);
        Student *stu = create_student(
            valid_data[i].id, valid_data[i].name,
            (Gender)valid_data[i].gender, valid_data[i].age,
            (ProjectType)valid_data[i].project, cls,
            valid_data[i].pay_date, valid_data[i].pay_years);
        if (stu) {
            insert_student(stu);
            printf("  ");
            cPrint(C_GREEN, "[+]");
            printf(" %s - %s\n", stu->id, stu->name);
        }
    }

    if (include_boundary) {
        printf("\n  ");
        cPrint(C_CYAN, "--- 边界测试数据 ---\n");

        Student *s_min = create_student("S013", "小小", 0, 3,
            PROJECT_PAINTING, CLASS_TODDLER, "2025-05-01", 1);
        if (s_min) { insert_student(s_min); cPrint(C_GREEN, "  [+] 最小年龄(3岁)录入成功\n"); }

        Student *s_max = create_student("S014", "大大", 1, 9,
            PROJECT_DANCE, CLASS_LARGE, "2025-05-01", 5);
        if (s_max) { insert_student(s_max); cPrint(C_GREEN, "  [+] 最大年龄(9岁)录入成功\n"); }

        Student *s_y1 = create_student("S015", "短期", 0, 5,
            PROJECT_SINGING, CLASS_SMALL, "2025-05-10", 1);
        if (s_y1) { insert_student(s_y1); cPrint(C_GREEN, "  [+] 最短缴费年限(1年)录入成功\n"); }

        Student *s_y5 = create_student("S016", "长期", 1, 6,
            PROJECT_PIANO, CLASS_MEDIUM, "2025-05-10", 5);
        if (s_y5) { insert_student(s_y5); cPrint(C_GREEN, "  [+] 最长缴费年限(5年)录入成功\n"); }
    }

    printDivider();
    cPrint(C_GREEN, "  [OK] 测试数据加载完毕，共 %d 条记录\n", count_students());
}

/* ======================== 帮助信息 ======================== */

static void showHelp(void) {
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  使用帮助\n");
    printThickDivider();
    printf("\n");
    cPrint(C_BOLD, "  菜单功能:\n");
    printf("    1. 录入学生信息     —— 添加新学生，按年龄自动分班\n");
    printf("    2. 删除学生信息     —— 按编号删除，二次确认\n");
    printf("    3. 按编号查询       —— 精确查找单个学生\n");
    printf("    4. 查看所有学生     —— 表格形式列出全部学生\n");
    printf("    5. 按项目查询       —— 按培训项目分组统计\n");
    printf("    6. 按班级查询       —— 按班级分组统计\n");
    printf("    7. 缴费到期提醒     —— 30天内到期/已过期预警\n");
    printf("    8. 退费计算         —— 按剩余月数计算退费金额\n");
    printf("    9. 保存数据         —— 保存到CSV文件\n");
    printf("   10. 加载数据         —— 从CSV文件加载\n");
    printf("   11. 统计仪表盘       —— 总人数/分布/收入统计\n");
    printf("   12. 排序功能         —— 按年龄/缴费日期排序\n");
    printf("   13. 加载测试数据     —— 快速加载演示数据\n");
    printf("    0. 退出系统\n");
    printf("\n");
    cPrint(C_BOLD, "  命令行模式:\n");
    cPrint(C_CYAN, "    kindergarten.exe              "); printf("交互模式\n");
    cPrint(C_CYAN, "    kindergarten.exe -f <文件>    "); printf("指定数据文件启动\n");
    cPrint(C_CYAN, "    kindergarten.exe -t           "); printf("测试模式启动\n");
    cPrint(C_CYAN, "    kindergarten.exe -h           "); printf("显示帮助\n");
    printf("\n");
    cPrint(C_BOLD, "  班级划分规则:\n");
    cPrint(FG_ORANGE, "    幼儿班 "); printf("3-4岁  ");
    cPrint(C_GREEN, "小班 "); printf("5岁  ");
    cPrint(C_YELLOW, "中班 "); printf("6-7岁  ");
    cPrint(C_MAGENTA, "大班 "); printf("8-9岁\n");
    printf("\n");
    cPrint(C_BOLD, "  费用标准:\n");
    printf("    年费: %.0f 元/年    缴费年限: 1-5年    月费: 年费/12\n", FEE_PER_YEAR);
    printf("    退费规则: 剩余月数×月费（不足整月按整月算）\n");
    printf("\n");
    cPrint(C_BOLD, "  数据文件:\n");
    cPrint(C_CYAN, "    %s  ", g_dataFile); printf("CSV格式，每行一条记录\n");
    printf("    格式: id,name,gender,age,project,class_level,pay_date,pay_years\n");
    printf("\n");
}

/* ======================== 菜单系统 ======================== */

static void showMenu(void) {
    printDivider();
    cPrint(C_BOLD C_BLUE, "  主菜单\n");
    printDivider();
    cPrint(C_CYAN, "  1 "); printf("- 录入学生信息\n");
    cPrint(C_CYAN, "  2 "); printf("- 删除学生信息\n");
    cPrint(C_CYAN, "  3 "); printf("- 按编号查询\n");
    cPrint(C_CYAN, "  4 "); printf("- 查看所有学生\n");
    cPrint(C_CYAN, "  5 "); printf("- 按项目查询\n");
    cPrint(C_CYAN, "  6 "); printf("- 按班级查询\n");
    cPrint(C_CYAN, "  7 "); printf("- 缴费到期提醒\n");
    cPrint(C_CYAN, "  8 "); printf("- 退费计算\n");
    cPrint(C_CYAN, "  9 "); printf("- 保存数据到文件\n");
    cPrint(C_CYAN, " 10 "); printf("- 从文件加载数据\n");
    cPrint(C_CYAN, " 11 "); printf("- 统计仪表盘\n");
    cPrint(C_CYAN, " 12 "); printf("- 排序功能\n");
    cPrint(C_CYAN, " 13 "); printf("- 加载测试数据\n");
    cPrint(C_CYAN, "  H/?"); printf("- 帮助信息\n");
    cPrint(C_CYAN, "  0 "); printf("- 退出系统\n");
    printDivider();
    printf("  当前状态: ");
    cPrint(C_BOLD C_WHITE, "%d", count_students());
    printf(" 名学生 | 数据文件: ");
    cPrint(C_CYAN, "%s", g_dataFile);
    if (g_sortMode == SORT_BY_AGE) cPrint(C_GREEN, " | 按年龄排序");
    else if (g_sortMode == SORT_BY_PAYDATE) cPrint(C_GREEN, " | 按日期排序");
    printf("\n");
    printDivider();
}

static void run_menu(void) {
    char choice[64];
    while (1) {
        showMenu();
        printf("  请选择操作: "); fflush(stdout);
        if (fgets(choice, sizeof(choice), stdin) == NULL) break;
        char *nl = strchr(choice, '\n'); if (nl) *nl = '\0';
        if (choice[0] == '\0') continue;
        char c = (char)toupper((unsigned char)choice[0]);
        int num = -1;
        if (isdigit((unsigned char)choice[0])) num = atoi(choice);

        if (c == 'H' || c == '?') { showHelp(); continue; }

        switch (num) {
        case 1:  add_student();       break;
        case 2:  delete_student();    break;
        case 3:  query_student();     break;
        case 4:  show_all_students(); break;
        case 5:  query_by_project();  break;
        case 6:  query_by_class();    break;
        case 7:  check_payment_due(); break;
        case 8:  calculate_refund();  break;
        case 9:  save_to_file(NULL);  break;
        case 10: load_from_file(NULL); break;
        case 11: show_statistics();   break;
        case 12: sort_menu();         break;
        case 13:
            if (count_students() > 0) {
                if (!readYesNo("当前有数据，加载测试数据会清空现有数据，是否继续"))
                    break;
            }
            printf("  1 - 标准测试数据(12条)\n");
            printf("  2 - 含边界测试数据(16条)\n");
            int tm = readInt("请选择 (1-2): ", 1, 2);
            free_all_students();
            load_test_data(tm == 2);
            break;
        case 0: {
            printf("\n");
            printThickDivider();
            cPrint(C_BOLD C_CYAN, "  退出系统\n");
            printDivider();
            if (g_head) {
                if (readYesNo("是否保存当前数据")) {
                    save_to_file(NULL);
                }
                calculate_refund();
            }
            cPrint(C_GREEN C_BOLD, "\n  [OK] 感谢使用，再见！\n\n");
            return;
        }
        default:
            cPrint(C_YELLOW, "  [!] 无效选项，请重新输入\n");
            break;
        }
    }
}

/* ======================== 内置自测用例 ======================== */

static void t_assert(int cond, const char *msg) {
    if (cond) {
        cPrint(C_GREEN, "    [PASS] "); printf("%s\n", msg);
        g_tests_passed++;
    } else {
        cPrint(C_RED, "    [FAIL] "); printf("%s\n", msg);
        g_tests_failed++;
    }
}

static int run_self_test(void) {
    g_tests_passed = 0;
    g_tests_failed = 0;

    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  运行内置自测用例\n");
    printThickDivider();

    free_all_students();
    const char *test_file = "__test_kgt.dat";
    remove(test_file);

    cPrint(C_CYAN, "\n  [TEST] T1: 加载测试数据\n");
    load_test_data(0);
    t_assert(count_students() == 12, "加载后学生数=12");

    cPrint(C_CYAN, "\n  [TEST] T2: 按编号查询\n");
    Student *s = find_student_by_id("S001");
    t_assert(s != NULL, "查找S001存在");
    t_assert(s != NULL && strcmp(s->name, "小明") == 0, "S001姓名为小明");
    t_assert(find_student_by_id("S999") == NULL, "查找S999不存在");

    cPrint(C_CYAN, "\n  [TEST] T3: 删除学生\n");
    int del_ret = delete_student_by_id("S012");
    t_assert(del_ret == 1, "删除S012成功");
    t_assert(count_students() == 11, "删除后学生数=11");
    t_assert(find_student_by_id("S012") == NULL, "S012已不存在");
    t_assert(delete_student_by_id("S999") == 0, "删除不存在学生返回0");

    cPrint(C_CYAN, "\n  [TEST] T4: 添加学生\n");
    Student *s_new = create_student("S013", "测试新生", GENDER_MALE, 5,
                                     PROJECT_PAINTING, CLASS_SMALL, "2025-06-01", 1);
    t_assert(s_new != NULL, "创建新学生成功");
    if (s_new) {
        insert_student(s_new);
        t_assert(count_students() == 12, "添加后学生数=12");
        Student *found = find_student_by_id("S013");
        t_assert(found != NULL, "新学生S013可查到");
        t_assert(found != NULL && found->age == 5, "新学生年龄=5");
    } else {
        t_assert(0, "创建新学生成功");
        t_assert(0, "添加后学生数=12");
        t_assert(0, "新学生S013可查到");
        t_assert(0, "新学生年龄=5");
    }

    cPrint(C_CYAN, "\n  [TEST] T5: 保存到文件\n");
    remove(test_file);
    KgtStatus save_st = save_to_file(test_file);
    t_assert(save_st == KGT_OK, "保存测试文件成功");

    cPrint(C_CYAN, "\n  [TEST] T6: 从文件加载（持久化往返）\n");
    free_all_students();
    t_assert(count_students() == 0, "释放后学生数=0");
    KgtStatus load_st = load_from_file(test_file);
    t_assert(load_st == KGT_OK, "加载测试文件成功");
    t_assert(count_students() == 12, "加载后学生数=12");
    t_assert(find_student_by_id("S001") != NULL, "S001仍在数据中");
    t_assert(find_student_by_id("S013") != NULL, "S013仍在数据中");
    Student *s001 = find_student_by_id("S001");
    t_assert(s001 != NULL && strcmp(s001->name, "小明") == 0, "S001姓名正确");

    cPrint(C_CYAN, "\n  [TEST] T7: 查询函数\n");
    t_assert(find_student_by_id("S002") != NULL, "S002存在");
    t_assert(find_student_by_id("S011") != NULL, "S011存在");
    s = find_student_by_id("S002");
    t_assert(s != NULL && s->gender == GENDER_FEMALE, "S002性别为女");
    t_assert(count_students() == 12, "最终学生数=12");

    free_all_students();
    remove(test_file);

    printThickDivider();
    cPrint(C_BOLD, "  测试结果: ");
    cPrint(C_GREEN, "PASS=%d  ", g_tests_passed);
    cPrint(C_RED, "FAIL=%d  ", g_tests_failed);
    int total = g_tests_passed + g_tests_failed;
    printf("TOTAL=%d\n", total);
    if (g_tests_failed == 0) {
        cPrint(C_BOLD C_GREEN, "  ★ 全部测试通过！\n");
    } else {
        cPrint(C_BOLD C_RED, "  ✗ 有测试失败，请检查！\n");
    }
    printThickDivider();

    return g_tests_failed > 0 ? 1 : 0;
}

/* ======================== 命令行处理 ======================== */

static void printUsage(const char *prog) {
    printf("Kindergarten Student Info Management System V2.0\n\n");
    printf("用法:\n");
    printf("  %s                     交互模式（菜单）\n", prog);
    printf("  %s -f <数据文件>       指定数据文件启动\n", prog);
    printf("  %s -t                  测试模式启动（加载标准测试数据）\n", prog);
    printf("  %s -h                  显示帮助\n", prog);
}

static int runCommandLine(int argc, char *argv[]) {
    if (argc < 2) return 0;

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        printUsage(argv[0]);
        return 1;
    }

    colorEnabled = 1;
    enableVTMode();

    if (strcmp(argv[1], "-f") == 0) {
        if (argc < 3) { printErr("用法: -f <数据文件>"); return 1; }
        strncpy(g_dataFile, argv[2], sizeof(g_dataFile) - 1);
        g_dataFile[sizeof(g_dataFile) - 1] = '\0';
        load_from_file(g_dataFile);
        return 0;
    }

    if (strcmp(argv[1], "-t") == 0) {
        return run_self_test();
    }

    printUsage(argv[0]);
    return 1;
}

/* ======================== 入口 ======================== */

int main(int argc, char *argv[]) {
    enableVTMode();

    if (argc > 1) {
        int cmdMode = (argc >= 2 && (strcmp(argv[1], "-f") == 0 ||
                                     strcmp(argv[1], "-t") == 0 ||
                                     strcmp(argv[1], "-h") == 0 ||
                                     strcmp(argv[1], "--help") == 0));
        if (cmdMode) {
            int ret = runCommandLine(argc, argv);
            free_all_students();
            return ret;
        }
    }

    printBanner();

    printf("  请选择启动模式:\n");
    cPrint(C_CYAN, "    1. "); printf("正常模式（从文件加载数据）\n");
    cPrint(C_CYAN, "    2. "); printf("测试模式（加载12条标准测试数据）\n");
    cPrint(C_CYAN, "    3. "); printf("完整测试模式（含边界数据，共16条）\n");
    cPrint(C_CYAN, "    4. "); printf("空白启动（不加载任何数据）\n");
    printDivider();

    int mode = readInt("请选择 (1-4): ", 1, 4);

    switch (mode) {
        case 1: load_from_file(NULL); break;
        case 2: load_test_data(0);    break;
        case 3: load_test_data(1);    break;
        case 4: printInfo("空白模式启动，请手动录入或稍后加载数据"); break;
    }

    run_menu();
    free_all_students();
    return 0;
}
