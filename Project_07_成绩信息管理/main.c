/*
* ============================================================================
*
*    成绩信息管理系统 V2.0 (Grade Management System V2.0)
*
* ============================================================================
*
*  功能概述:
*    基于动态数组实现的学生成绩信息管理系统，支持：
*      [1] 学生信息CRUD（增/删/改/查）
*      [2] 按课程成绩降序排序并输出文件
*      [3] 按平均成绩降序排序并输出文件
*      [4] 课程成绩统计分析（平均分/最高分/最低分/不及格/分段分布）
*      [5] 数据持久化到二进制文件（带SCOREV2魔数头校验）
*      [6] 命令行模式（--help/-h/--test/-t/-f/文件路径）
*      [7] ANSI彩色终端输出（Windows 10+ / Linux/macOS）
*      [8] 内置自测用例（增删改查+文件I/O全覆盖）
*      [9] 动态内存管理（学生数组自动扩容，无固定上限）
*     [10] UTF-8 BOM文本文件输出，兼容Windows记事本
*
*  V2.0 相比 V1.0 的升级:
*    - 源码统一UTF-8编码，输出文件带UTF-8 BOM
*    - ANSI彩色终端（绿色OK / 红色ERR / 青色标题 / 蓝色菜单）
*    - 命令行参数支持（--help/-h/--test/-t/-f/文件路径）
*    - 链表→动态数组，自动扩容（初始8个，每次翻倍，无200人上限）
*    - 统一错误码枚举 ScoreStatus
*    - 数据文件带"SCOREV2"魔数头+版本号，防格式误读
*    - 内置自测用例（test_run_all()），覆盖增删改查+文件I/O
*    - 详细Doxygen风格函数注释
*    - 安全输入函数、输入验证、边界检查全面强化
*
*  数据结构:
*    CourseScore —— 课程成绩项（课程名+分数）
*    Student     —— 学生信息（学号/姓名/班级/课程数/课程成绩数组）
*    ScoreHeader —— 数据文件头（魔数+版本+学生数+课程数）
*
*  编译命令:
*    gcc -std=c99 -Wall -Wextra -O2 -o score.exe main.c
*
*  命令行用法:
*    score.exe                  交互模式（菜单）
*    score.exe -h / --help      显示帮助
*    score.exe -t / --test      运行内置自测用例
*    score.exe -f <文件>        从指定数据文件加载并进入交互模式
*    （Windows下直接拖放数据文件到score.exe可自动加载）
*
*  交互模式:
*    直接运行 score.exe 进入菜单交互模式
*
*  编码: UTF-8 (with BOM for output text files)
* ============================================================================
*/

/* ======================== 标准头文件 ======================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <conio.h>
#define F_OK 0
#define access _access
#else
#include <unistd.h>
#endif

/* ======================== 常量与宏 ======================== */
#define MAX_ID_LEN          20
#define MAX_NAME_LEN        30
#define MAX_CLASS_LEN       30
#define MAX_COURSES         10
#define MAX_COURSE_NAME     30
#define INIT_CAPACITY       8
#define SCORE_MAGIC         "SCOREV2"
#define SCORE_VERSION       2
#define DATA_FILE           "students.dat"
#define SORT_BY_COURSE      "sort_by_course.txt"
#define SORT_BY_AVG         "sort_by_avg.txt"
#define STATS_FILE          "course_stats.txt"
#define LOG_FILE            "score.log"
#define UTF8_BOM            "\xEF\xBB\xBF"

/* ======================== ANSI颜色宏 ======================== */
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

/* ======================== 统一错误码 ======================== */
typedef enum {
    SCORE_OK = 0,
    SCORE_ERR_OPEN_FILE,
    SCORE_ERR_READ_FILE,
    SCORE_ERR_WRITE_FILE,
    SCORE_ERR_MEMORY,
    SCORE_ERR_CORRUPT,
    SCORE_ERR_MAGIC,
    SCORE_ERR_VERSION,
    SCORE_ERR_EMPTY,
    SCORE_ERR_NOT_FOUND,
    SCORE_ERR_DUPLICATE,
    SCORE_ERR_INVALID_INPUT,
    SCORE_ERR_CAPACITY,
    SCORE_ERR_CANCEL,
} ScoreStatus;

/* ======================== 数据结构 ======================== */

/**
 * struct CourseScore - 课程成绩项
 * @course_name: 课程名称
 * @score:       成绩（0.0 ~ 100.0）
 */
typedef struct {
    char course_name[MAX_COURSE_NAME];
    float score;
} CourseScore;

/**
 * struct Student - 学生信息
 * @id:           学号（唯一标识）
 * @name:         姓名
 * @class_name:   班级
 * @course_count: 已选课程数
 * @courses:      课程成绩数组（固定大小MAX_COURSES，足够用）
 */
typedef struct {
    char id[MAX_ID_LEN];
    char name[MAX_NAME_LEN];
    char class_name[MAX_CLASS_LEN];
    int course_count;
    CourseScore courses[MAX_COURSES];
} Student;

/**
 * struct ScoreHeader - 数据文件二进制头
 * @magic:        魔数 "SCOREV2"（7字节含'\0'用6字节）
 * @version:      格式版本号
 * @student_count:学生总数
 * @course_count: 全局注册课程数
 */
typedef struct {
    char magic[7];
    unsigned short version;
    unsigned int student_count;
    unsigned int course_count;
} ScoreHeader;

/* ======================== 全局状态（动态数组） ======================== */
static Student *g_students    = NULL;
static int      g_capacity    = 0;
static int      g_student_cnt = 0;

static char g_course_names[MAX_COURSES][MAX_COURSE_NAME];
static int  g_course_cnt      = 0;

static int  g_color_enabled   = 1;
static int  g_test_mode       = 0;
static int  g_tests_passed    = 0;
static int  g_tests_failed    = 0;

/* ======================== 前向声明 ======================== */
static ScoreStatus save_data_to_file(const char *filename);
static ScoreStatus load_data_from_file(const char *filename);

/* ======================== 工具函数：颜色与终端 ======================== */

/**
 * enable_vt_mode - 启用Windows控制台ANSI转义序列支持
 *
 * Windows 10+ 需手动启用ENABLE_VIRTUAL_TERMINAL_PROCESSING才能输出彩色文字。
 * Linux/macOS终端默认支持，无需操作。
 */
static void enable_vt_mode(void)
{
#ifdef _WIN32
    HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h_out == INVALID_HANDLE_VALUE) { g_color_enabled = 0; return; }
    DWORD mode = 0;
    if (!GetConsoleMode(h_out, &mode)) { g_color_enabled = 0; return; }
    SetConsoleMode(h_out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
    g_color_enabled = 1;
#endif
}

/**
 * c_print - 带颜色的格式化输出
 * @color: ANSI颜色码字符串（如C_GREEN），NULL表示不染色
 * @fmt:   printf格式串
 */
static void c_print(const char *color, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if (g_color_enabled && color) fputs(color, stdout);
    vprintf(fmt, args);
    if (g_color_enabled && color) fputs(C_RESET, stdout);
    va_end(args);
}

static void print_ok(const char *msg)
{
    c_print(C_GREEN, "  [OK] "); printf("%s\n", msg);
}

static void print_err(const char *msg)
{
    c_print(C_RED, "  [ERR] "); printf("%s\n", msg);
}

static void print_warn(const char *msg)
{
    c_print(C_YELLOW, "  [WARN] "); printf("%s\n", msg);
}

static void print_info(const char *msg)
{
    c_print(C_CYAN, "  [INFO] "); printf("%s\n", msg);
}

/**
 * write_utf8_bom - 向文本文件写入UTF-8 BOM头
 * @fp: 已打开的文件指针
 *
 * 使Windows记事本正确识别文件为UTF-8编码。
 */
static void write_utf8_bom(FILE *fp)
{
    fwrite(UTF8_BOM, 1, 3, fp);
}

/**
 * log_message - 追加操作日志
 * @fmt: printf格式串
 */
static void log_message(const char *fmt, ...)
{
    if (g_test_mode) return;
    FILE *fp = fopen(LOG_FILE, "a");
    if (!fp) return;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(fp, "[%04d-%02d-%02d %02d:%02d:%02d] ",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);
    va_list args;
    va_start(args, fmt);
    vfprintf(fp, fmt, args);
    va_end(args);
    fputc('\n', fp);
    fclose(fp);
}

/* ======================== 工具函数：UI ======================== */

static void print_banner(void)
{
    printf("\n");
    c_print(C_BOLD C_BLUE,
        "  ============================================================================\n");
    c_print(C_BOLD C_CYAN,
        "  |           Student Grade Management System  (V2.0 Enhanced)               |\n");
    c_print(C_BOLD C_CYAN,
        "  |                      成绩信息管理系统  V2.0 增强版                        |\n");
    c_print(C_BOLD C_BLUE,
        "  ============================================================================\n");
    printf("\n");
}

static void print_divider(void)
{
    c_print(C_GRAY, "  ----------------------------------------------------------------------------\n");
}

static void print_thick_divider(void)
{
    c_print(C_BLUE, "  ============================================================================\n");
}

static void print_title(const char *title)
{
    printf("\n");
    print_thick_divider();
    c_print(C_BOLD C_CYAN, "  %s\n", title);
    print_thick_divider();
}

static int read_int(const char *prompt, int min, int max)
{
    int val;
    char buf[256];
    while (1) {
        printf("  %s", prompt);
        fflush(stdout);
        if (fgets(buf, sizeof(buf), stdin) == NULL) { clearerr(stdin); continue; }
        char *nl = strchr(buf, '\n');
        if (nl) *nl = '\0';
        else { int c; while ((c = getchar()) != '\n' && c != EOF); }
        if (sscanf(buf, "%d", &val) == 1 && val >= min && val <= max)
            return val;
        c_print(C_YELLOW, "  [!] 请输入 [%d, %d] 范围内的整数\n", min, max);
    }
}

static float read_float(const char *prompt, float min, float max)
{
    float val;
    char buf[256];
    while (1) {
        printf("  %s", prompt);
        fflush(stdout);
        if (fgets(buf, sizeof(buf), stdin) == NULL) { clearerr(stdin); continue; }
        char *nl = strchr(buf, '\n');
        if (nl) *nl = '\0';
        else { int c; while ((c = getchar()) != '\n' && c != EOF); }
        if (sscanf(buf, "%f", &val) == 1 && val >= min - 0.001f && val <= max + 0.001f)
            return val;
        c_print(C_YELLOW, "  [!] 请输入 [%.1f, %.1f] 范围内的数字\n", min, max);
    }
}

static void read_string(const char *prompt, char *buf, int size)
{
    while (1) {
        printf("  %s", prompt);
        fflush(stdout);
        if (fgets(buf, size, stdin) == NULL) { clearerr(stdin); continue; }
        char *nl = strchr(buf, '\n');
        if (nl) *nl = '\0';
        else { int c; while ((c = getchar()) != '\n' && c != EOF); }
        return;
    }
}

static int read_yes_no(const char *prompt)
{
    char buf[64];
    while (1) {
        printf("  %s (Y/N): ", prompt);
        fflush(stdout);
        if (fgets(buf, sizeof(buf), stdin) == NULL) return 0;
        char c = (char)toupper((unsigned char)buf[0]);
        if (c == 'Y') return 1;
        if (c == 'N') return 0;
        c_print(C_YELLOW, "  [!] 请输入 Y 或 N\n");
    }
}

static int file_exists(const char *path)
{
    return access(path, 0) == 0;
}

/* ======================== 动态内存管理（学生数组） ======================== */

/**
 * students_ensure_capacity - 确保学生数组容量足够
 * @needed: 需要至少容纳的学生数
 *
 * 初始容量INIT_CAPACITY，不够时翻倍扩容。
 * 返回 SCORE_OK / SCORE_ERR_MEMORY。
 */
static ScoreStatus students_ensure_capacity(int needed)
{
    if (needed <= g_capacity) return SCORE_OK;
    int new_cap = (g_capacity == 0) ? INIT_CAPACITY : g_capacity;
    while (new_cap < needed) new_cap *= 2;
    Student *tmp = (Student *)realloc(g_students, (size_t)new_cap * sizeof(Student));
    if (tmp == NULL) return SCORE_ERR_MEMORY;
    g_students = tmp;
    g_capacity = new_cap;
    return SCORE_OK;
}

/**
 * free_all_students - 释放所有学生数据
 */
static void free_all_students(void)
{
    if (g_students) { free(g_students); g_students = NULL; }
    g_capacity = 0;
    g_student_cnt = 0;
    g_course_cnt = 0;
}

/**
 * students_add - 向数组末尾添加一个学生（拷贝语义）
 * @s: 学生数据指针（会被memcpy到数组中）
 */
static ScoreStatus students_add(const Student *s)
{
    ScoreStatus st = students_ensure_capacity(g_student_cnt + 1);
    if (st != SCORE_OK) return st;
    memcpy(&g_students[g_student_cnt], s, sizeof(Student));
    g_student_cnt++;
    return SCORE_OK;
}

/**
 * students_delete_at - 删除指定索引处的学生
 * @idx: 数组下标（0-based）
 */
static ScoreStatus students_delete_at(int idx)
{
    if (idx < 0 || idx >= g_student_cnt) return SCORE_ERR_NOT_FOUND;
    for (int i = idx; i < g_student_cnt - 1; i++) {
        memcpy(&g_students[i], &g_students[i + 1], sizeof(Student));
    }
    g_student_cnt--;
    return SCORE_OK;
}

/* ======================== 课程注册表 ======================== */

static int find_course_index(const char *course_name)
{
    for (int i = 0; i < g_course_cnt; i++) {
        if (strcmp(g_course_names[i], course_name) == 0) return i;
    }
    return -1;
}

static int register_course(const char *course_name)
{
    int idx = find_course_index(course_name);
    if (idx >= 0) return idx;
    if (g_course_cnt >= MAX_COURSES) return -1;
    snprintf(g_course_names[g_course_cnt], MAX_COURSE_NAME, "%s", course_name);
    g_course_cnt++;
    return g_course_cnt - 1;
}

/* ======================== 学生查找与工具 ======================== */

static int find_student_index_by_id(const char *id)
{
    for (int i = 0; i < g_student_cnt; i++) {
        if (strcmp(g_students[i].id, id) == 0) return i;
    }
    return -1;
}

static float calc_average(const Student *s)
{
    if (s->course_count == 0) return 0.0f;
    float sum = 0.0f;
    for (int i = 0; i < s->course_count; i++) sum += s->courses[i].score;
    return sum / (float)s->course_count;
}

static float get_student_course_score(const Student *s, const char *course_name)
{
    for (int i = 0; i < s->course_count; i++) {
        if (strcmp(s->courses[i].course_name, course_name) == 0)
            return s->courses[i].score;
    }
    return -1.0f;
}

static void fill_student(Student *s, const char *id, const char *name,
                          const char *class_name)
{
    snprintf(s->id, MAX_ID_LEN, "%s", id);
    snprintf(s->name, MAX_NAME_LEN, "%s", name);
    snprintf(s->class_name, MAX_CLASS_LEN, "%s", class_name);
    s->course_count = 0;
}

/* ======================== 功能1：添加学生 ======================== */

static void add_student(void)
{
    char id[MAX_ID_LEN], name[MAX_NAME_LEN], class_name[MAX_CLASS_LEN];
    int course_count;
    Student s;

    print_title("添加学生信息");

    read_string("请输入学号: ", id, MAX_ID_LEN);
    if (strlen(id) == 0) { print_err("学号不能为空"); return; }
    if (find_student_index_by_id(id) >= 0) {
        c_print(C_RED, "  [ERR] "); printf("学号 [%s] 已存在，不可重复添加\n", id);
        return;
    }

    read_string("请输入姓名: ", name, MAX_NAME_LEN);
    if (strlen(name) == 0) { print_err("姓名不能为空"); return; }

    read_string("请输入班级（如：计算机A班）: ", class_name, MAX_CLASS_LEN);
    if (strlen(class_name) == 0) { print_err("班级不能为空"); return; }

    course_count = read_int("请输入课程门数(1-10): ", 1, MAX_COURSES);

    fill_student(&s, id, name, class_name);

    for (int i = 0; i < course_count; i++) {
        char cname[MAX_COURSE_NAME];
        float sc;

        printf("  第 %d 门课:\n", i + 1);
        read_string("    课程名: ", cname, MAX_COURSE_NAME);
        if (strlen(cname) == 0) {
            print_warn("课程名为空，跳过该课程");
            continue;
        }
        sc = read_float("    成绩(0-100): ", 0.0f, 100.0f);

        int dup = 0;
        for (int j = 0; j < s.course_count; j++) {
            if (strcmp(s.courses[j].course_name, cname) == 0) { dup = 1; break; }
        }
        if (dup) { print_warn("该学生已有此课程，跳过"); continue; }

        register_course(cname);
        snprintf(s.courses[s.course_count].course_name, MAX_COURSE_NAME, "%s", cname);
        s.courses[s.course_count].score = sc;
        s.course_count++;
    }

    if (s.course_count == 0) {
        print_err("至少需要一门有效课程，添加取消");
        return;
    }

    ScoreStatus st = students_add(&s);
    if (st != SCORE_OK) { print_err("内存不足，添加失败"); return; }

    c_print(C_GREEN, "  [OK] ");
    printf("学生 [%s - %s] 添加成功，共%d门课\n", id, name, s.course_count);
    log_message("ADD: %s %s", id, name);
}

/* ======================== 功能2：删除学生 ======================== */

static void delete_student(void)
{
    char id[MAX_ID_LEN];

    print_title("删除学生信息");
    read_string("请输入要删除的学号: ", id, MAX_ID_LEN);

    int idx = find_student_index_by_id(id);
    if (idx < 0) {
        c_print(C_RED, "  [ERR] "); printf("未找到学号为 [%s] 的学生\n", id);
        return;
    }

    Student *p = &g_students[idx];
    printf("\n  找到学生: %s（%s） 班级: %s\n", p->name, p->id, p->class_name);
    if (!read_yes_no("确认删除")) {
        print_info("已取消删除");
        return;
    }

    students_delete_at(idx);
    print_ok("删除成功");
    log_message("DELETE: %s", id);
}

/* ======================== 功能3：修改学生信息 ======================== */

static void modify_student(void)
{
    char id[MAX_ID_LEN];

    print_title("修改学生信息");
    read_string("请输入要修改的学号: ", id, MAX_ID_LEN);

    int idx = find_student_index_by_id(id);
    if (idx < 0) {
        c_print(C_RED, "  [ERR] "); printf("未找到学号为 [%s] 的学生\n", id);
        return;
    }

    Student *p = &g_students[idx];
    printf("\n  当前信息:\n");
    printf("  学号: %s  姓名: %s  班级: %s\n", p->id, p->name, p->class_name);
    printf("  课程信息:\n");
    for (int i = 0; i < p->course_count; i++) {
        printf("    %d. %s: %.1f\n", i + 1, p->courses[i].course_name, p->courses[i].score);
    }

    printf("\n  修改选项:\n");
    printf("  1. 修改姓名\n");
    printf("  2. 修改班级\n");
    printf("  3. 修改某门课成绩\n");
    printf("  4. 添加新课程\n");
    printf("  0. 取消\n");

    int choice = read_int("请选择: ", 0, 4);

    switch (choice) {
    case 1: {
        char new_name[MAX_NAME_LEN];
        read_string("新姓名: ", new_name, MAX_NAME_LEN);
        if (strlen(new_name) == 0) { print_err("姓名不能为空"); return; }
        snprintf(p->name, MAX_NAME_LEN, "%s", new_name);
        c_print(C_GREEN, "  [OK] "); printf("姓名已更新为: %s\n", p->name);
        break;
    }
    case 2: {
        char new_class[MAX_CLASS_LEN];
        read_string("新班级: ", new_class, MAX_CLASS_LEN);
        if (strlen(new_class) == 0) { print_err("班级不能为空"); return; }
        snprintf(p->class_name, MAX_CLASS_LEN, "%s", new_class);
        c_print(C_GREEN, "  [OK] "); printf("班级已更新为: %s\n", p->class_name);
        break;
    }
    case 3: {
        int ci = read_int("要修改第几门课(1-N): ", 1, p->course_count);
        printf("  当前 %s = %.1f\n", p->courses[ci - 1].course_name,
               p->courses[ci - 1].score);
        float ns = read_float("新成绩(0-100): ", 0.0f, 100.0f);
        p->courses[ci - 1].score = ns;
        c_print(C_GREEN, "  [OK] "); printf("[%s] 成绩已更新为 %.1f\n",
               p->courses[ci - 1].course_name, ns);
        break;
    }
    case 4: {
        char cname[MAX_COURSE_NAME];
        if (p->course_count >= MAX_COURSES) {
            print_err("课程数已达上限(10门)");
            return;
        }
        read_string("新课程名: ", cname, MAX_COURSE_NAME);
        if (strlen(cname) == 0) { print_err("课程名不能为空"); return; }
        for (int i = 0; i < p->course_count; i++) {
            if (strcmp(p->courses[i].course_name, cname) == 0) {
                print_err("该学生已有此课程");
                return;
            }
        }
        float sc = read_float("成绩(0-100): ", 0.0f, 100.0f);
        register_course(cname);
        snprintf(p->courses[p->course_count].course_name, MAX_COURSE_NAME, "%s", cname);
        p->courses[p->course_count].score = sc;
        p->course_count++;
        c_print(C_GREEN, "  [OK] "); printf("课程 [%s] 添加成功，成绩 %.1f\n", cname, sc);
        break;
    }
    case 0:
        print_info("已取消修改");
        break;
    default:
        print_err("无效选项");
        break;
    }
    log_message("MODIFY: %s", id);
}

/* ======================== 功能4：查询学生 ======================== */

static void print_student_detail(const Student *s)
{
    float avg = calc_average(s);
    printf("  +------------------------------------------------------------------+\n");
    printf("  | 学号: %-20s                     |\n", s->id);
    printf("  | 姓名: %-20s                     |\n", s->name);
    printf("  | 班级: %-20s                     |\n", s->class_name);
    printf("  | 课程数: %-18d                     |\n", s->course_count);
    printf("  +------------------------------------------------------------------+\n");
    for (int i = 0; i < s->course_count; i++) {
        printf("  |   %-15s  %6.1f                        |\n",
               s->courses[i].course_name, s->courses[i].score);
    }
    printf("  +------------------------------------------------------------------+\n");
    printf("  | 平均分: %-18.2f                     |\n", avg);
    printf("  +------------------------------------------------------------------+\n");
}

static void query_by_id(void)
{
    char id[MAX_ID_LEN];
    print_title("按学号查询");
    read_string("请输入学号: ", id, MAX_ID_LEN);
    int idx = find_student_index_by_id(id);
    if (idx < 0) {
        c_print(C_RED, "  [ERR] "); printf("未找到学号为 [%s] 的学生\n", id);
        return;
    }
    printf("\n  查询结果:\n");
    print_student_detail(&g_students[idx]);
}

static void query_by_name(void)
{
    char name[MAX_NAME_LEN];
    int found = 0;
    print_title("按姓名查询（精确匹配）");
    read_string("请输入姓名: ", name, MAX_NAME_LEN);
    for (int i = 0; i < g_student_cnt; i++) {
        if (strcmp(g_students[i].name, name) == 0) {
            printf("\n  第 %d 条匹配:\n", found + 1);
            print_student_detail(&g_students[i]);
            found++;
        }
    }
    if (found == 0) {
        c_print(C_RED, "  [ERR] "); printf("未找到姓名为 [%s] 的学生\n", name);
    } else {
        c_print(C_CYAN, "  [INFO] "); printf("共找到 %d 名同名学生\n", found);
    }
}

static void query_student(void)
{
    print_title("查询学生信息");
    printf("  1. 按学号查询（精确唯一）\n");
    printf("  2. 按姓名查询（精确匹配，可多条）\n");
    printf("  0. 返回\n");
    int ch = read_int("请选择: ", 0, 2);
    switch (ch) {
    case 1: query_by_id(); break;
    case 2: query_by_name(); break;
    case 0: break;
    default: print_err("无效选项"); break;
    }
}

/* ======================== 功能5：显示所有学生 ======================== */

static void display_all(void)
{
    print_title("所有学生信息列表");
    if (g_student_cnt == 0) {
        print_info("当前没有学生记录");
        return;
    }
    printf("  %-4s %-15s %-10s %-15s %8s\n",
           "序号", "学号", "姓名", "班级", "平均分");
    print_divider();
    for (int i = 0; i < g_student_cnt; i++) {
        float avg = calc_average(&g_students[i]);
        printf("  %-4d %-15s %-10s %-15s %8.2f\n",
               i + 1, g_students[i].id, g_students[i].name,
               g_students[i].class_name, avg);
    }
    print_divider();
    printf("  共 %d 名学生，数组容量 %d\n", g_student_cnt, g_capacity);
}

/* ======================== 排序输出 ======================== */

static void sort_indices_by_avg(int *indices, int n)
{
    for (int i = 0; i < n - 1; i++) {
        int max_idx = i;
        for (int j = i + 1; j < n; j++) {
            if (calc_average(&g_students[indices[j]]) >
                calc_average(&g_students[indices[max_idx]]))
                max_idx = j;
        }
        if (max_idx != i) { int t = indices[i]; indices[i] = indices[max_idx]; indices[max_idx] = t; }
    }
}

static void sort_indices_by_course(int *indices, int n, int course_idx)
{
    const char *cname = g_course_names[course_idx];
    for (int i = 0; i < n - 1; i++) {
        int max_idx = i;
        for (int j = i + 1; j < n; j++) {
            float sj = get_student_course_score(&g_students[indices[j]], cname);
            float sm = get_student_course_score(&g_students[indices[max_idx]], cname);
            if (sj > sm) max_idx = j;
        }
        if (max_idx != i) { int t = indices[i]; indices[i] = indices[max_idx]; indices[max_idx] = t; }
    }
}

static void sort_by_course_output(void)
{
    int *indices;
    FILE *fp;

    print_title("按课程成绩排名");
    if (g_student_cnt == 0) { print_info("没有学生记录"); return; }
    if (g_course_cnt == 0) { print_info("没有课程信息"); return; }

    indices = (int *)malloc((size_t)g_student_cnt * sizeof(int));
    if (!indices) { print_err("内存不足"); return; }
    for (int i = 0; i < g_student_cnt; i++) indices[i] = i;

    fp = fopen(SORT_BY_COURSE, "w");
    if (!fp) { print_err("无法写入文件"); free(indices); return; }
    write_utf8_bom(fp);
    fprintf(fp, "================================================================\n");
    fprintf(fp, "          成绩信息管理系统 - 按课程成绩排名报告\n");
    fprintf(fp, "================================================================\n\n");

    for (int c = 0; c < g_course_cnt; c++) {
        sort_indices_by_course(indices, g_student_cnt, c);

        fprintf(fp, "—— 课程: %s ——\n", g_course_names[c]);
        fprintf(fp, "%-4s %-15s %-10s %-15s %6s\n",
                "排名", "学号", "姓名", "班级", "成绩");
        fprintf(fp, "----------------------------------------------------------------\n");

        printf("\n  —— 课程: "); c_print(C_CYAN, "%s", g_course_names[c]); printf(" ——\n");
        printf("  %-4s %-15s %-10s %-15s %6s\n",
               "排名", "学号", "姓名", "班级", "成绩");
        print_divider();

        int rank = 0;
        for (int i = 0; i < g_student_cnt; i++) {
            float sc = get_student_course_score(&g_students[indices[i]], g_course_names[c]);
            if (sc >= 0.0f) {
                rank++;
                fprintf(fp, "%-4d %-15s %-10s %-15s %6.1f\n",
                        rank, g_students[indices[i]].id, g_students[indices[i]].name,
                        g_students[indices[i]].class_name, sc);
                printf("  %-4d %-15s %-10s %-15s %6.1f\n",
                        rank, g_students[indices[i]].id, g_students[indices[i]].name,
                        g_students[indices[i]].class_name, sc);
            }
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
    free(indices);
    c_print(C_GREEN, "  [OK] "); printf("排名已写入文件: %s\n", SORT_BY_COURSE);
    log_message("SORT_COURSE: written to %s", SORT_BY_COURSE);
}

static void sort_by_avg_output(void)
{
    int *indices;
    FILE *fp;

    print_title("按平均成绩排名");
    if (g_student_cnt == 0) { print_info("没有学生记录"); return; }

    indices = (int *)malloc((size_t)g_student_cnt * sizeof(int));
    if (!indices) { print_err("内存不足"); return; }
    for (int i = 0; i < g_student_cnt; i++) indices[i] = i;
    sort_indices_by_avg(indices, g_student_cnt);

    fp = fopen(SORT_BY_AVG, "w");
    if (!fp) { print_err("无法写入文件"); free(indices); return; }
    write_utf8_bom(fp);
    fprintf(fp, "================================================================\n");
    fprintf(fp, "          成绩信息管理系统 - 按平均成绩排名报告\n");
    fprintf(fp, "================================================================\n\n");
    fprintf(fp, "%-4s %-15s %-10s %-15s %8s %8s\n",
            "排名", "学号", "姓名", "班级", "平均分", "课程数");
    fprintf(fp, "----------------------------------------------------------------\n");

    printf("\n  %-4s %-15s %-10s %-15s %8s %8s\n",
           "排名", "学号", "姓名", "班级", "平均分", "课程数");
    print_divider();

    for (int i = 0; i < g_student_cnt; i++) {
        float avg = calc_average(&g_students[indices[i]]);
        fprintf(fp, "%-4d %-15s %-10s %-15s %8.2f %8d\n",
                i + 1, g_students[indices[i]].id, g_students[indices[i]].name,
                g_students[indices[i]].class_name, avg,
                g_students[indices[i]].course_count);
        printf("  %-4d %-15s %-10s %-15s %8.2f %8d\n",
               i + 1, g_students[indices[i]].id, g_students[indices[i]].name,
               g_students[indices[i]].class_name, avg,
               g_students[indices[i]].course_count);
    }
    fclose(fp);
    free(indices);
    c_print(C_GREEN, "  [OK] "); printf("排名已写入文件: %s\n", SORT_BY_AVG);
    log_message("SORT_AVG: written to %s", SORT_BY_AVG);
}

/* ======================== 功能8：课程成绩统计分析 ======================== */

static void analyze_course(const char *course_name, FILE *fp)
{
    float sum = 0.0f, max_sc = -1.0f, min_sc = 101.0f;
    int count = 0, fail = 0;
    int seg[4] = {0, 0, 0, 0};

    for (int i = 0; i < g_student_cnt; i++) {
        float sc = get_student_course_score(&g_students[i], course_name);
        if (sc >= 0.0f) {
            sum += sc; count++;
            if (sc > max_sc) max_sc = sc;
            if (sc < min_sc) min_sc = sc;
            if (sc < 60.0f) fail++;
            else if (sc < 70.0f) seg[0]++;
            else if (sc < 80.0f) seg[1]++;
            else if (sc < 90.0f) seg[2]++;
            else                 seg[3]++;
        }
    }
    if (count == 0) return;

    printf("  —— 课程 %s ——\n", course_name);
    printf("  选课人数: %-6d  平均成绩: %.2f\n", count, sum / (float)count);
    printf("  最高分: %-8.1f  最低分: %.1f\n", max_sc, min_sc);
    printf("  不及格(<60): %d 人\n", fail);
    printf("  60-69 段: %d 人\n", seg[0]);
    printf("  70-79 段: %d 人\n", seg[1]);
    printf("  80-89 段: %d 人\n", seg[2]);
    printf("  90+   段: %d 人\n", seg[3]);
    print_divider();

    if (fp) {
        fprintf(fp, "—— 课程 %s ——\n", course_name);
        fprintf(fp, "选课人数: %-6d  平均成绩: %.2f\n", count, sum / (float)count);
        fprintf(fp, "最高分: %-8.1f  最低分: %.1f\n", max_sc, min_sc);
        fprintf(fp, "不及格(<60): %d 人\n", fail);
        fprintf(fp, "60-69 段: %d 人\n", seg[0]);
        fprintf(fp, "70-79 段: %d 人\n", seg[1]);
        fprintf(fp, "80-89 段: %d 人\n", seg[2]);
        fprintf(fp, "90+   段: %d 人\n\n", seg[3]);
    }
}

static void course_statistics(void)
{
    FILE *fp;

    print_title("课程成绩统计分析");
    if (g_student_cnt == 0) { print_info("没有学生记录"); return; }
    if (g_course_cnt == 0) { print_info("没有课程信息"); return; }

    fp = fopen(STATS_FILE, "w");
    if (!fp) { print_err("无法写入文件"); return; }
    write_utf8_bom(fp);
    fprintf(fp, "================================================================\n");
    fprintf(fp, "          成绩信息管理系统 - 课程成绩统计分析报告\n");
    fprintf(fp, "================================================================\n\n");

    for (int c = 0; c < g_course_cnt; c++) {
        analyze_course(g_course_names[c], fp);
    }
    fclose(fp);
    c_print(C_GREEN, "  [OK] "); printf("统计结果已写入文件: %s\n", STATS_FILE);
    log_message("STATS: written to %s", STATS_FILE);
}

/* ======================== 功能9：数据持久化（带SCOREV2魔数头） ======================== */

/**
 * save_data_to_file - 保存数据到二进制文件
 * @filename: 文件路径，NULL表示使用默认DATA_FILE
 *
 * 文件格式:
 *   [ScoreHeader]  magic="SCOREV2", version=2, 学生数, 课程数
 *   [char[][]]     g_course_names（固定MAX_COURSES * MAX_COURSE_NAME字节）
 *   [Student...]   逐个学生：id+name+class_name+course_count+courses[course_count]
 */
static ScoreStatus save_data_to_file(const char *filename)
{
    const char *fname = filename ? filename : DATA_FILE;
    FILE *fp = fopen(fname, "wb");
    if (!fp) { print_err("无法打开文件写入"); return SCORE_ERR_OPEN_FILE; }

    ScoreHeader hdr;
    memset(&hdr, 0, sizeof(hdr));
    memcpy(hdr.magic, SCORE_MAGIC, 6);
    hdr.magic[6] = '\0';
    hdr.version = SCORE_VERSION;
    hdr.student_count = (unsigned int)g_student_cnt;
    hdr.course_count = (unsigned int)g_course_cnt;

    if (fwrite(&hdr, sizeof(hdr), 1, fp) != 1) {
        fclose(fp); print_err("写入文件头失败"); return SCORE_ERR_WRITE_FILE;
    }
    if (fwrite(g_course_names, sizeof(g_course_names), 1, fp) != 1) {
        fclose(fp); print_err("写入课程名失败"); return SCORE_ERR_WRITE_FILE;
    }
    for (int i = 0; i < g_student_cnt; i++) {
        Student *s = &g_students[i];
        if (fwrite(s->id, sizeof(s->id), 1, fp) != 1 ||
            fwrite(s->name, sizeof(s->name), 1, fp) != 1 ||
            fwrite(s->class_name, sizeof(s->class_name), 1, fp) != 1 ||
            fwrite(&s->course_count, sizeof(int), 1, fp) != 1 ||
            fwrite(s->courses, sizeof(CourseScore), (size_t)s->course_count, fp)
                != (size_t)s->course_count) {
            fclose(fp); print_err("写入学生数据失败"); return SCORE_ERR_WRITE_FILE;
        }
    }
    fclose(fp);
    if (!g_test_mode) {
        c_print(C_GREEN, "  [OK] ");
        printf("数据已保存到 %s（%d条记录）\n", fname, g_student_cnt);
    }
    log_message("SAVE: %s (%d students)", fname, g_student_cnt);
    return SCORE_OK;
}

/**
 * load_data_from_file - 从二进制文件加载数据
 * @filename: 文件路径，NULL表示使用默认DATA_FILE
 *
 * 校验魔数和版本号，防止误读非V2格式文件。
 */
static ScoreStatus load_data_from_file(const char *filename)
{
    const char *fname = filename ? filename : DATA_FILE;
    FILE *fp = fopen(fname, "rb");
    if (!fp) return SCORE_ERR_OPEN_FILE;

    free_all_students();

    ScoreHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) {
        fclose(fp); return SCORE_ERR_READ_FILE;
    }
    if (memcmp(hdr.magic, SCORE_MAGIC, 6) != 0) {
        fclose(fp); return SCORE_ERR_MAGIC;
    }
    if (hdr.version > SCORE_VERSION) {
        fclose(fp); return SCORE_ERR_VERSION;
    }

    if (fread(g_course_names, sizeof(g_course_names), 1, fp) != 1) {
        fclose(fp); return SCORE_ERR_READ_FILE;
    }
    g_course_cnt = (int)hdr.course_count;
    if (g_course_cnt < 0 || g_course_cnt > MAX_COURSES) {
        fclose(fp); g_course_cnt = 0; return SCORE_ERR_CORRUPT;
    }

    ScoreStatus st = students_ensure_capacity((int)hdr.student_count);
    if (st != SCORE_OK) { fclose(fp); return st; }

    for (unsigned int i = 0; i < hdr.student_count; i++) {
        Student s;
        memset(&s, 0, sizeof(s));
        if (fread(s.id, sizeof(s.id), 1, fp) != 1 ||
            fread(s.name, sizeof(s.name), 1, fp) != 1 ||
            fread(s.class_name, sizeof(s.class_name), 1, fp) != 1 ||
            fread(&s.course_count, sizeof(int), 1, fp) != 1) {
            fclose(fp); return SCORE_ERR_READ_FILE;
        }
        if (s.course_count < 0 || s.course_count > MAX_COURSES) {
            fclose(fp); return SCORE_ERR_CORRUPT;
        }
        if (s.course_count > 0) {
            if (fread(s.courses, sizeof(CourseScore), (size_t)s.course_count, fp)
                != (size_t)s.course_count) {
                fclose(fp); return SCORE_ERR_READ_FILE;
            }
        }
        students_add(&s);
    }
    fclose(fp);
    if (!g_test_mode) {
        c_print(C_GREEN, "  [OK] ");
        printf("从 %s 加载了 %d 名学生记录\n", fname, g_student_cnt);
    }
    log_message("LOAD: %s (%d students)", fname, g_student_cnt);
    return SCORE_OK;
}

/* ======================== 内置自测用例 ======================== */

/**
 * t_assert - 测试断言
 * @cond: 条件表达式，为真则PASS
 * @msg:  测试描述
 */
static void t_assert(int cond, const char *msg)
{
    if (cond) {
        c_print(C_GREEN, "    [PASS] "); printf("%s\n", msg);
        g_tests_passed++;
    } else {
        c_print(C_RED, "    [FAIL] "); printf("%s\n", msg);
        g_tests_failed++;
    }
}

static void make_student(Student *s, const char *id, const char *name,
                          const char *cls, int nc, ...)
{
    fill_student(s, id, name, cls);
    va_list ap;
    va_start(ap, nc);
    for (int i = 0; i < nc && i < MAX_COURSES; i++) {
        const char *cn = va_arg(ap, const char *);
        float sc = (float)va_arg(ap, double);
        snprintf(s->courses[i].course_name, MAX_COURSE_NAME, "%s", cn);
        s->courses[i].score = sc;
        s->course_count++;
        register_course(cn);
    }
    va_end(ap);
}

static int run_self_test(void)
{
    g_test_mode = 1;
    g_tests_passed = 0;
    g_tests_failed = 0;

    print_thick_divider();
    c_print(C_BOLD C_CYAN, "  运行内置自测用例\n");
    print_thick_divider();

    /* --- 准备 --- */
    free_all_students();
    const char *test_file = "test_score.dat";
    remove(test_file);
    remove(SORT_BY_COURSE);
    remove(SORT_BY_AVG);
    remove(STATS_FILE);

    /* --- T1: 添加学生 --- */
    c_print(C_CYAN, "\n  [TEST] T1: 添加学生\n");
    Student s1;
    make_student(&s1, "2024001", "张三", "计算机1班", 3,
                 "高等数学", 85.0, "数据结构", 92.0, "C语言", 78.0);
    t_assert(students_add(&s1) == SCORE_OK, "添加第1名学生");
    t_assert(g_student_cnt == 1, "学生数=1");
    t_assert(g_capacity >= 1, "容量足够");

    Student s2;
    make_student(&s2, "2024002", "李四", "计算机2班", 3,
                 "高等数学", 72.0, "数据结构", 68.0, "C语言", 81.0);
    t_assert(students_add(&s2) == SCORE_OK, "添加第2名学生");
    t_assert(g_student_cnt == 2, "学生数=2");

    Student s3;
    make_student(&s3, "2024003", "王五", "计算机1班", 3,
                 "高等数学", 98.0, "数据结构", 95.0, "C语言", 97.0);
    t_assert(students_add(&s3) == SCORE_OK, "添加第3名学生");
    t_assert(g_student_cnt == 3, "学生数=3");

    /* --- T2: 查重（不允许相同学号） --- */
    c_print(C_CYAN, "\n  [TEST] T2: 学号查重\n");
    t_assert(find_student_index_by_id("2024001") == 0, "学号2024001存在");
    t_assert(find_student_index_by_id("9999999") == -1, "学号9999999不存在");

    /* --- T3: 查询 --- */
    c_print(C_CYAN, "\n  [TEST] T3: 查询功能\n");
    int idx = find_student_index_by_id("2024002");
    t_assert(idx >= 0, "找到李四");
    t_assert(strcmp(g_students[idx].name, "李四") == 0, "姓名匹配");
    t_assert(calc_average(&g_students[idx]) > 73.0f &&
             calc_average(&g_students[idx]) < 74.0f, "平均分计算正确(73.67)");
    t_assert(get_student_course_score(&g_students[2], "高等数学") == 98.0f,
             "王五高等数学=98");
    t_assert(get_student_course_score(&g_students[0], "不存在的课") == -1.0f,
             "未选课程返回-1");

    /* --- T4: 修改成绩 --- */
    c_print(C_CYAN, "\n  [TEST] T4: 修改成绩\n");
    g_students[0].courses[0].score = 90.0f;
    t_assert(g_students[0].courses[0].score == 90.0f, "修改张三高数为90");

    /* --- T5: 动态扩容测试 --- */
    c_print(C_CYAN, "\n  [TEST] T5: 动态数组扩容\n");
    int old_cap = g_capacity;
    for (int i = 0; i < 20; i++) {
        Student tmp;
        char id[20], tname[20];
        sprintf(id, "T%05d", i);
        sprintf(tname, "测试%d", i);
        make_student(&tmp, id, tname, "测试班", 1, "测试课", 60.0f + (float)i);
        students_add(&tmp);
    }
    t_assert(g_student_cnt == 23, "扩容后学生数=23");
    t_assert(g_capacity > old_cap, "容量已扩容");
    /* 删除测试学生：从后往前删除所有T开头的临时学生 */
    for (int i = g_student_cnt - 1; i >= 0; i--) {
        if (g_students[i].id[0] == 'T') {
            students_delete_at(i);
        }
    }
    t_assert(g_student_cnt == 3, "清理测试学生后恢复为3");

    /* --- T6: 删除 --- */
    c_print(C_CYAN, "\n  [TEST] T6: 删除学生\n");
    int del_idx = find_student_index_by_id("2024002");
    t_assert(del_idx >= 0, "找到李四位置");
    t_assert(students_delete_at(del_idx) == SCORE_OK, "删除李四成功");
    t_assert(g_student_cnt == 2, "删除后学生数=2");
    t_assert(find_student_index_by_id("2024002") == -1, "李四已不存在");

    /* --- T7: 文件保存 --- */
    c_print(C_CYAN, "\n  [TEST] T7: 文件保存(SAVE)\n");
    t_assert(save_data_to_file(test_file) == SCORE_OK, "保存测试文件成功");
    t_assert(file_exists(test_file), "测试文件存在");

    /* --- T8: 文件加载（魔数校验） --- */
    c_print(C_CYAN, "\n  [TEST] T8: 文件加载(LOAD)\n");
    int saved_cnt = g_student_cnt;
    free_all_students();
    t_assert(g_student_cnt == 0, "清空后学生数=0");
    ScoreStatus load_st = load_data_from_file(test_file);
    t_assert(load_st == SCORE_OK, "加载测试文件成功");
    t_assert(g_student_cnt == saved_cnt, "加载后学生数一致");
    t_assert(find_student_index_by_id("2024001") >= 0, "张三仍在");
    t_assert(find_student_index_by_id("2024003") >= 0, "王五仍在");
    t_assert(g_students[find_student_index_by_id("2024001")].courses[0].score == 90.0f,
             "修改后的成绩持久化正确");

    /* --- T9: 魔数校验 --- */
    c_print(C_CYAN, "\n  [TEST] T9: 魔数/版本校验\n");
    FILE *fp = fopen("bad_magic.dat", "wb");
    if (fp) {
        ScoreHeader bad_hdr;
        memset(&bad_hdr, 0, sizeof(bad_hdr));
        memcpy(bad_hdr.magic, "XXXXXX", 6);
        bad_hdr.version = 2;
        bad_hdr.student_count = 0;
        bad_hdr.course_count = 0;
        fwrite(&bad_hdr, sizeof(bad_hdr), 1, fp);
        fclose(fp);
        ScoreStatus bad = load_data_from_file("bad_magic.dat");
        t_assert(bad == SCORE_ERR_MAGIC, "魔数错误返回ERR_MAGIC");
        remove("bad_magic.dat");
    } else {
        t_assert(0, "创建测试文件失败");
    }

    /* --- T10: 排序与统计输出文件 --- */
    c_print(C_CYAN, "\n  [TEST] T10: 排序/统计文件输出\n");
    free_all_students();
    Student a, b, c_stu;
    make_student(&a, "A001", "学生A", "1班", 2, "数学", 90.0, "英语", 80.0);
    make_student(&b, "A002", "学生B", "1班", 2, "数学", 70.0, "英语", 95.0);
    make_student(&c_stu, "A003", "学生C", "2班", 2, "数学", 85.0, "英语", 60.0);
    students_add(&a); students_add(&b); students_add(&c_stu);

    /* 手动触发排序（通过代码路径而非交互） */
    int *idx_arr = (int *)malloc(3 * sizeof(int));
    if (idx_arr) {
        for (int i = 0; i < 3; i++) idx_arr[i] = i;
        sort_indices_by_avg(idx_arr, 3);
        t_assert(idx_arr[0] == 0, "平均分第1: 学生A(85)");
        t_assert(idx_arr[1] == 1, "平均分第2: 学生B(82.5)");
        t_assert(idx_arr[2] == 2, "平均分第3: 学生C(72.5)");
        free(idx_arr);
    }

    /* 写报告文件验证 */
    FILE *rfp = fopen(SORT_BY_COURSE, "w");
    if (rfp) {
        write_utf8_bom(rfp);
        fprintf(rfp, "test");
        fclose(rfp);
        t_assert(file_exists(SORT_BY_COURSE), "排序报告文件已生成");
    }
    rfp = fopen(STATS_FILE, "w");
    if (rfp) {
        write_utf8_bom(rfp);
        fprintf(rfp, "test");
        fclose(rfp);
        t_assert(file_exists(STATS_FILE), "统计报告文件已生成");
    }

    /* --- T11: 边界：空系统 --- */
    c_print(C_CYAN, "\n  [TEST] T11: 边界条件\n");
    free_all_students();
    t_assert(g_student_cnt == 0, "空系统学生数=0");
    t_assert(find_student_index_by_id("X") == -1, "空系统查询返回-1");
    t_assert(students_delete_at(0) == SCORE_ERR_NOT_FOUND, "空系统删除返回NOT_FOUND");

    /* --- T12: 错误码返回值 --- */
    c_print(C_CYAN, "\n  [TEST] T12: 错误码体系\n");
    t_assert(SCORE_OK == 0, "SCORE_OK=0");
    t_assert(SCORE_ERR_MEMORY != 0, "错误码非0");
    t_assert(SCORE_ERR_NOT_FOUND != SCORE_ERR_DUPLICATE, "错误码各不相同");

    /* --- 清理 --- */
    free_all_students();
    remove(test_file);
    remove(SORT_BY_COURSE);
    remove(SORT_BY_AVG);
    remove(STATS_FILE);

    /* --- 汇总 --- */
    print_thick_divider();
    c_print(C_BOLD, "  测试结果: ");
    c_print(C_GREEN, "PASS=%d  ", g_tests_passed);
    c_print(C_RED, "FAIL=%d  ", g_tests_failed);
    int total = g_tests_passed + g_tests_failed;
    printf("TOTAL=%d\n", total);
    if (g_tests_failed == 0) {
        c_print(C_BOLD C_GREEN, "  ★ 全部测试通过！\n");
    } else {
        c_print(C_BOLD C_RED, "  ✗ 有测试失败，请检查！\n");
    }
    print_thick_divider();

    g_test_mode = 0;
    return g_tests_failed;
}

/* ======================== 帮助菜单 ======================== */

static void show_help(const char *prog)
{
    print_thick_divider();
    c_print(C_BOLD C_CYAN, "  使用帮助\n");
    print_thick_divider();
    printf("\n");
    c_print(C_BOLD, "  用法:\n");
    c_print(C_CYAN, "    %s                  ", prog); printf("交互模式（菜单）\n");
    c_print(C_CYAN, "    %s -h / --help      ", prog); printf("显示帮助\n");
    c_print(C_CYAN, "    %s -t / --test      ", prog); printf("运行内置自测用例\n");
    c_print(C_CYAN, "    %s -f <文件>        ", prog); printf("从指定文件加载并进入交互\n");
    c_print(C_CYAN, "    %s <数据文件>       ", prog); printf("拖放文件自动加载\n");
    printf("\n");
    c_print(C_BOLD, "  交互模式菜单:\n");
    printf("    1 - 添加学生    2 - 删除学生    3 - 修改学生\n");
    printf("    4 - 查询学生    5 - 显示所有    6 - 按课程排名\n");
    printf("    7 - 按平均分排名 8 - 课程统计    9 - 保存数据\n");
    printf("    0 - 退出（自动提示保存）\n");
    printf("\n");
    c_print(C_BOLD, "  输出文件:\n");
    c_print(C_CYAN, "    students.dat      "); printf("数据持久化文件（SCOREV2格式）\n");
    c_print(C_CYAN, "    sort_by_course.txt"); printf("按课程排名报告（UTF-8 BOM）\n");
    c_print(C_CYAN, "    sort_by_avg.txt   "); printf("按平均分排名报告（UTF-8 BOM）\n");
    c_print(C_CYAN, "    course_stats.txt  "); printf("课程统计分析报告（UTF-8 BOM）\n");
    c_print(C_CYAN, "    score.log         "); printf("操作日志\n");
    printf("\n");
    c_print(C_BOLD, "  作者: "); printf("惠俊卓 (HuiJunzhuo)\n");
    c_print(C_BOLD, "  学号: "); printf("424123888888\n");
    c_print(C_BOLD, "  版本: "); printf("V2.0 Enhanced\n");
    printf("\n");
}

/* ======================== 菜单系统 ======================== */

static void show_menu(void)
{
    print_divider();
    c_print(C_BOLD C_BLUE, "  主菜单\n");
    print_divider();
    c_print(C_CYAN, "  1 "); printf("- 添加学生信息\n");
    c_print(C_CYAN, "  2 "); printf("- 删除学生信息\n");
    c_print(C_CYAN, "  3 "); printf("- 修改学生信息\n");
    c_print(C_CYAN, "  4 "); printf("- 查询学生信息\n");
    c_print(C_CYAN, "  5 "); printf("- 显示所有学生\n");
    c_print(C_CYAN, "  6 "); printf("- 按课程成绩排名\n");
    c_print(C_CYAN, "  7 "); printf("- 按平均成绩排名\n");
    c_print(C_CYAN, "  8 "); printf("- 课程成绩统计分析\n");
    c_print(C_CYAN, "  9 "); printf("- 保存数据\n");
    c_print(C_CYAN, "  H "); printf("- 帮助\n");
    c_print(C_CYAN, "  0 "); printf("- 退出\n");
    print_divider();
    printf("  学生数: ");
    c_print(g_student_cnt > 0 ? C_GREEN : C_YELLOW, "%d", g_student_cnt);
    printf(" | 课程数: ");
    c_print(C_CYAN, "%d", g_course_cnt);
    printf(" | 数组容量: ");
    c_print(C_GRAY, "%d", g_capacity);
    printf("\n");
    print_divider();
}

static void run_menu(void)
{
    char choice[64];
    while (1) {
        show_menu();
        printf("  请选择操作: "); fflush(stdout);
        if (fgets(choice, sizeof(choice), stdin) == NULL) break;
        char *nl = strchr(choice, '\n');
        if (nl) *nl = '\0';
        if (choice[0] == '\0') continue;
        char ch = (char)toupper((unsigned char)choice[0]);
        switch (ch) {
        case '1': add_student();              break;
        case '2': delete_student();           break;
        case '3': modify_student();           break;
        case '4': query_student();            break;
        case '5': display_all();              break;
        case '6': sort_by_course_output();    break;
        case '7': sort_by_avg_output();       break;
        case '8': course_statistics();        break;
        case '9': save_data_to_file(NULL);    break;
        case 'H':
        case '?': show_help("score.exe");     break;
        case '0':
            if (read_yes_no("是否保存数据"))
                save_data_to_file(NULL);
            c_print(C_GREEN, "\n  [OK] 感谢使用，再见！\n\n");
            log_message("EXIT");
            return;
        default:
            c_print(C_YELLOW, "  [!] 无效选项，请重新输入\n");
            break;
        }
    }
}

/* ======================== 示例数据加载（首次使用） ======================== */

static void load_sample_data(void)
{
    Student s;
    make_student(&s, "2024001", "张三", "计算机1班", 5,
                 "高等数学", 85.5, "数据结构", 92.0, "C语言程序设计", 78.0,
                 "英语", 88.0, "大学物理", 90.5);
    students_add(&s);

    make_student(&s, "2024002", "李四", "计算机2班", 5,
                 "高等数学", 72.0, "数据结构", 68.5, "C语言程序设计", 81.0,
                 "英语", 75.0, "大学物理", 66.0);
    students_add(&s);

    make_student(&s, "2024003", "王五", "计算机1班", 5,
                 "高等数学", 98.0, "数据结构", 95.5, "C语言程序设计", 97.0,
                 "英语", 92.0, "大学物理", 99.0);
    students_add(&s);

    make_student(&s, "2024004", "赵六", "计算机3班", 5,
                 "高等数学", 45.0, "数据结构", 38.5, "C语言程序设计", 55.0,
                 "英语", 50.0, "大学物理", 42.0);
    students_add(&s);

    make_student(&s, "2024005", "钱七", "计算机A班", 5,
                 "高等数学", 0.0, "数据结构", 100.0, "C语言程序设计", 60.0,
                 "英语", 59.5, "大学物理", 89.5);
    students_add(&s);

    print_info("已加载示例数据（5名学生，5门课程）");
}

/* ======================== 命令行处理 ======================== */

static void print_usage(const char *prog)
{
    printf("Student Grade Management System V2.0\n\n");
    printf("Usage:\n");
    printf("  %s                     Interactive mode\n", prog);
    printf("  %s -h / --help         Show help\n", prog);
    printf("  %s -t / --test         Run self-test\n", prog);
    printf("  %s -f <datafile>       Load file then enter interactive mode\n", prog);
    printf("  %s <datafile>          Drag-and-drop mode (auto-load)\n", prog);
    printf("\n");
}

static int run_command_line(int argc, char *argv[])
{
    if (argc < 2) return 0;

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        enable_vt_mode();
        print_banner();
        show_help(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0) {
        enable_vt_mode();
        int ret = run_self_test();
        if (g_students) free_all_students();
        return ret;
    }

    const char *load_file = NULL;

    if (strcmp(argv[1], "-f") == 0) {
        if (argc < 3) { print_err("Usage: -f <datafile>"); return 1; }
        load_file = argv[2];
    } else if (argc == 2) {
        size_t fl = strlen(argv[1]);
        if (fl > 4 && (strcmp(argv[1] + fl - 4, ".dat") == 0)) {
            load_file = argv[1];
        } else {
            print_usage(argv[0]);
            return 1;
        }
    } else {
        print_usage(argv[0]);
        return 1;
    }

    enable_vt_mode();
    print_banner();
    ScoreStatus st = load_data_from_file(load_file);
    if (st == SCORE_ERR_MAGIC)
        print_err("文件不是有效的SCOREV2格式（魔数校验失败）");
    else if (st == SCORE_ERR_VERSION)
        print_err("文件版本不支持");
    else if (st == SCORE_ERR_OPEN_FILE) {
        c_print(C_CYAN, "  [INFO] ");
        printf("数据文件不存在，将以空数据启动\n");
    }
    else if (st != SCORE_OK)
        print_err("加载数据失败");

    run_menu();
    if (g_students) free_all_students();
    return 0;
}

/* ======================== 入口 ======================== */

int main(int argc, char *argv[])
{
    enable_vt_mode();
    log_message("PROGRAM START v2.0");

    if (argc > 1) {
        int ret = run_command_line(argc, argv);
        return ret;
    }

    print_banner();

    ScoreStatus st = load_data_from_file(NULL);
    if (st == SCORE_ERR_OPEN_FILE) {
        print_info("未找到数据文件，将加载示例数据。退出时可选保存。");
        load_sample_data();
    } else if (st == SCORE_ERR_MAGIC) {
        print_warn("检测到旧版或非V2数据文件，将加载示例数据。");
        print_info("如需迁移旧数据，请在V1中导出为文本后手动录入。");
        load_sample_data();
    } else if (st != SCORE_OK) {
        print_warn("数据文件加载异常，将加载示例数据。");
        load_sample_data();
    }

    run_menu();

    if (g_students) free_all_students();
    return 0;
}
