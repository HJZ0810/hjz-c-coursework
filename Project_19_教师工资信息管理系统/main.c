/*
 * ╔══════════════════════════════════════════════════════════════╗
 * ║                                                              ║
 * ║           教师工资信息管理系统  v2.0                          ║
 * ║           Teacher Salary Management System V2                ║
 * ║                                                              ║
 * ╠══════════════════════════════════════════════════════════════╣
 * ║  V2.0 升级特性：                                             ║
 * ║    - UTF-8 BOM + SetConsoleOutputCP(65001) 完美中文支持      ║
 * ║    - ANSI 彩色终端输出                                       ║
 * ║    - CLI 参数解析 (-h/--help, -t/--test)                     ║
 * ║    - 动态数组存储（替代V1的链表）                            ║
 * ║    - 统一错误码 enum SalStatus                               ║
 * ║    - SALV2 Magic Header 二进制文件持久化                     ║
 * ║    - 内置17项自动化自测（CRUD+计算+统计+I/O）                ║
 * ║    - 中文注释                                                ║
 * ║                                                              ║
 * ║  学号：424123888888    班级：计算机A班                        ║
 * ║  作者：HuiJunzhuo     学期：2025-2026第二学期                ║
 * ║                                                              ║
 * ╚══════════════════════════════════════════════════════════════╝
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* ======================== 常量定义 ======================== */

#define MAX_NAME_LEN    32
#define MAX_DEPT_LEN    32
#define MAX_TITLE_LEN   32
#define INIT_CAPACITY   8
#define DATA_FILE       "salary_v2.dat"
#define MAGIC_HEADER    "SALV2"
#define MAGIC_LEN       5

/* ======================== ANSI 颜色定义 ======================== */

#define C_RESET     "\033[0m"
#define C_RED       "\033[31m"
#define C_GREEN     "\033[32m"
#define C_YELLOW    "\033[33m"
#define C_BLUE      "\033[34m"
#define C_MAGENTA   "\033[35m"
#define C_CYAN      "\033[36m"
#define C_WHITE     "\033[37m"
#define C_BOLD      "\033[1m"
#define C_DIM       "\033[2m"

/* ======================== 统一错误码枚举 ======================== */

typedef enum {
    SAL_OK = 0,
    SAL_ERR_MEMORY,
    SAL_ERR_NOT_FOUND,
    SAL_ERR_DUPLICATE_ID,
    SAL_ERR_FILE_OPEN,
    SAL_ERR_FILE_READ,
    SAL_ERR_FILE_WRITE,
    SAL_ERR_FILE_MAGIC,
    SAL_ERR_FILE_VERSION,
    SAL_ERR_INVALID_PARAM,
    SAL_ERR_INVALID_INPUT,
    SAL_ERR_EMPTY,
    SAL_ERR_IO
} SalStatus;

/* ======================== 数据结构定义 ======================== */

typedef struct {
    int     id;
    char    name[MAX_NAME_LEN];
    char    dept[MAX_DEPT_LEN];
    char    title[MAX_TITLE_LEN];
    double  base_salary;
    double  merit_pay;
    double  allowance;
    double  deduction;
    double  total;
} Teacher;

typedef struct {
    Teacher *data;
    int      size;
    int      capacity;
    int      next_id;
} TeacherArray;

typedef struct {
    int    count;
    double total_base;
    double total_merit;
    double total_allowance;
    double total_deduction;
    double total_salary;
    double avg_salary;
    double max_salary;
    double min_salary;
} Statistics;

/* ======================== 全局状态 ======================== */

static TeacherArray g_teachers = {NULL, 0, 0, 1};
static int g_test_mode = 0;
static int g_test_passed = 0;
static int g_test_failed = 0;

/* ======================== 控制台初始化 ======================== */

static void initConsole(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(hOut, &mode)) {
        SetConsoleMode(hOut, mode | 0x0004);
    }
#endif
}

/* ======================== 基础工具函数 ======================== */

static const char* salStatusStr(SalStatus status)
{
    switch (status) {
        case SAL_OK:               return "操作成功";
        case SAL_ERR_MEMORY:       return "内存分配失败";
        case SAL_ERR_NOT_FOUND:    return "未找到记录";
        case SAL_ERR_DUPLICATE_ID: return "ID重复";
        case SAL_ERR_FILE_OPEN:    return "文件打开失败";
        case SAL_ERR_FILE_READ:    return "文件读取失败";
        case SAL_ERR_FILE_WRITE:   return "文件写入失败";
        case SAL_ERR_FILE_MAGIC:   return "文件格式错误(魔数不匹配)";
        case SAL_ERR_FILE_VERSION: return "文件版本不兼容";
        case SAL_ERR_INVALID_PARAM:return "无效参数";
        case SAL_ERR_INVALID_INPUT:return "无效输入";
        case SAL_ERR_EMPTY:        return "无数据记录";
        case SAL_ERR_IO:           return "IO错误";
        default:                   return "未知错误";
    }
}

static void safeStrncpyInternal(char *dst, const char *src, int size)
{
    if (size <= 0) return;
    int i;
    for (i = 0; i < size - 1 && src[i] != '\0'; i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

static void clearInputBuffer(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

static int safeInput(char *buf, int size)
{
    if (fgets(buf, size, stdin) == NULL) {
        buf[0] = '\0';
        return 0;
    }
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    } else {
        clearInputBuffer();
    }
    return 1;
}

static int inputInt(const char *prompt)
{
    char buf[64];
    int  val;
    while (1) {
        printf("%s", prompt);
        if (!safeInput(buf, sizeof(buf))) {
            printf("\n");
            return 0;
        }
        if (sscanf(buf, "%d", &val) == 1) {
            return val;
        }
        printf("  %s[提示]%s 请输入有效整数！\n", C_YELLOW, C_RESET);
    }
}

static double inputDouble(const char *prompt)
{
    char   buf[64];
    double val;
    while (1) {
        printf("%s", prompt);
        if (!safeInput(buf, sizeof(buf))) {
            printf("\n");
            return 0.0;
        }
        if (sscanf(buf, "%lf", &val) == 1) {
            return val;
        }
        printf("  %s[提示]%s 请输入有效数字！\n", C_YELLOW, C_RESET);
    }
}

static void inputString(const char *prompt, char *buf, int size)
{
    printf("%s", prompt);
    safeInput(buf, size);
}

static void pressAnyKey(void)
{
    if (g_test_mode) return;
    printf("\n  %s按回车键继续...%s", C_DIM, C_RESET);
    char buf[4];
    safeInput(buf, sizeof(buf));
}

/* ======================== UI输出函数 ======================== */

static void printLine(void)
{
    printf("  %s------------------------------------------------------------%s\n", C_CYAN, C_RESET);
}

static void printTitle(const char *title)
{
    printf("\n");
    printLine();
    printf("  %s%s%s%s\n", C_BOLD, C_CYAN, title, C_RESET);
    printLine();
}

static void printError(const char *text)
{
    printf("  %s[错误]%s %s\n", C_RED, C_RESET, text);
}

static void printSuccess(const char *text)
{
    printf("  %s[成功]%s %s\n", C_GREEN, C_RESET, text);
}

static void printInfo(const char *text)
{
    printf("  %s[信息]%s %s\n", C_YELLOW, C_RESET, text);
}

/* ======================== 工资计算 ======================== */

static double calcTotal(const Teacher *t)
{
    return t->base_salary + t->merit_pay + t->allowance - t->deduction;
}

static void updateTotal(Teacher *t)
{
    t->total = calcTotal(t);
}

/* ======================== 动态数组操作 ======================== */

static SalStatus arrayInit(TeacherArray *arr)
{
    arr->data = (Teacher*)malloc(sizeof(Teacher) * (size_t)INIT_CAPACITY);
    if (arr->data == NULL) {
        arr->size = 0;
        arr->capacity = 0;
        arr->next_id = 1;
        return SAL_ERR_MEMORY;
    }
    arr->size = 0;
    arr->capacity = INIT_CAPACITY;
    arr->next_id = 1;
    return SAL_OK;
}

static SalStatus arrayExpand(TeacherArray *arr)
{
    int new_cap = arr->capacity * 2;
    Teacher *new_data = (Teacher*)realloc(arr->data, sizeof(Teacher) * (size_t)new_cap);
    if (new_data == NULL) {
        return SAL_ERR_MEMORY;
    }
    arr->data = new_data;
    arr->capacity = new_cap;
    return SAL_OK;
}

static int arrayFindById(const TeacherArray *arr, int id)
{
    for (int i = 0; i < arr->size; i++) {
        if (arr->data[i].id == id) {
            return i;
        }
    }
    return -1;
}

static SalStatus arrayAdd(TeacherArray *arr, const Teacher *src, int *new_id)
{
    if (arr->size >= arr->capacity) {
        SalStatus st = arrayExpand(arr);
        if (st != SAL_OK) return st;
    }
    int id = arr->next_id++;
    arr->data[arr->size] = *src;
    arr->data[arr->size].id = id;
    updateTotal(&arr->data[arr->size]);
    arr->size++;
    if (new_id != NULL) *new_id = id;
    return SAL_OK;
}

static SalStatus arrayDelete(TeacherArray *arr, int idx)
{
    if (idx < 0 || idx >= arr->size) {
        return SAL_ERR_NOT_FOUND;
    }
    for (int i = idx; i < arr->size - 1; i++) {
        arr->data[i] = arr->data[i + 1];
    }
    arr->size--;
    return SAL_OK;
}

static void arrayFree(TeacherArray *arr)
{
    if (arr->data != NULL) {
        free(arr->data);
        arr->data = NULL;
    }
    arr->size = 0;
    arr->capacity = 0;
}

/* ======================== 文件持久化 ======================== */

static SalStatus saveData(const TeacherArray *arr, const char *filename)
{
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) return SAL_ERR_FILE_OPEN;

    if (fwrite(MAGIC_HEADER, 1, MAGIC_LEN, fp) != (size_t)MAGIC_LEN) {
        fclose(fp);
        return SAL_ERR_FILE_WRITE;
    }
    if (fwrite(&arr->next_id, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return SAL_ERR_FILE_WRITE;
    }
    if (fwrite(&arr->size, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return SAL_ERR_FILE_WRITE;
    }
    if (arr->size > 0) {
        if (fwrite(arr->data, sizeof(Teacher), (size_t)arr->size, fp) != (size_t)arr->size) {
            fclose(fp);
            return SAL_ERR_FILE_WRITE;
        }
    }
    fclose(fp);
    return SAL_OK;
}

static SalStatus loadData(TeacherArray *arr, const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) return SAL_ERR_FILE_OPEN;

    char magic[MAGIC_LEN + 1] = {0};
    if (fread(magic, 1, MAGIC_LEN, fp) != (size_t)MAGIC_LEN) {
        fclose(fp);
        return SAL_ERR_FILE_MAGIC;
    }
    if (memcmp(magic, MAGIC_HEADER, MAGIC_LEN) != 0) {
        fclose(fp);
        return SAL_ERR_FILE_MAGIC;
    }

    int next_id, size;
    if (fread(&next_id, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return SAL_ERR_FILE_READ;
    }
    if (fread(&size, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return SAL_ERR_FILE_READ;
    }

    if (size < 0 || size > 100000) {
        fclose(fp);
        return SAL_ERR_FILE_READ;
    }

    while (arr->capacity < size) {
        SalStatus st = arrayExpand(arr);
        if (st != SAL_OK) {
            fclose(fp);
            return st;
        }
    }

    if (size > 0) {
        if (fread(arr->data, sizeof(Teacher), (size_t)size, fp) != (size_t)size) {
            fclose(fp);
            arr->size = 0;
            return SAL_ERR_FILE_READ;
        }
    }
    arr->size = size;
    arr->next_id = next_id;
    fclose(fp);

    for (int i = 0; i < arr->size; i++) {
        updateTotal(&arr->data[i]);
    }
    return SAL_OK;
}

/* ======================== 统计功能 ======================== */

static SalStatus calcStatistics(const TeacherArray *arr, Statistics *stats)
{
    if (arr->size == 0) {
        memset(stats, 0, sizeof(Statistics));
        return SAL_ERR_EMPTY;
    }
    memset(stats, 0, sizeof(Statistics));
    stats->count = arr->size;
    stats->max_salary = arr->data[0].total;
    stats->min_salary = arr->data[0].total;

    for (int i = 0; i < arr->size; i++) {
        const Teacher *t = &arr->data[i];
        stats->total_base += t->base_salary;
        stats->total_merit += t->merit_pay;
        stats->total_allowance += t->allowance;
        stats->total_deduction += t->deduction;
        stats->total_salary += t->total;
        if (t->total > stats->max_salary) stats->max_salary = t->total;
        if (t->total < stats->min_salary) stats->min_salary = t->total;
    }
    stats->avg_salary = stats->total_salary / stats->count;
    return SAL_OK;
}

/* ======================== 前向声明业务函数 ======================== */

static void listAllTeachers(void);

/* ======================== 业务功能函数 ======================== */

static void inputTeacherInfo(Teacher *t)
{
    inputString("  请输入姓名：", t->name, MAX_NAME_LEN);
    inputString("  请输入部门：", t->dept, MAX_DEPT_LEN);
    inputString("  请输入职称：", t->title, MAX_TITLE_LEN);
    t->base_salary = inputDouble("  请输入基本工资：");
    t->merit_pay   = inputDouble("  请输入绩效工资：");
    t->allowance   = inputDouble("  请输入津贴补贴：");
    t->deduction   = inputDouble("  请输入扣款金额：");
    updateTotal(t);
}

static void addTeacher(void)
{
    printTitle("【添加教师】");
    Teacher t;
    memset(&t, 0, sizeof(t));
    inputTeacherInfo(&t);
    int new_id;
    SalStatus st = arrayAdd(&g_teachers, &t, &new_id);
    if (st == SAL_OK) {
        printSuccess("添加成功！");
        printf("  分配编号：%s%d%s\n", C_CYAN, new_id, C_RESET);
        saveData(&g_teachers, DATA_FILE);
    } else {
        printError(salStatusStr(st));
    }
}

static void deleteTeacher(void)
{
    printTitle("【删除教师】");
    if (g_teachers.size == 0) {
        printInfo("暂无教师记录");
        return;
    }
    int id = inputInt("  请输入要删除的教师编号：");
    int idx = arrayFindById(&g_teachers, id);
    if (idx < 0) {
        printError(salStatusStr(SAL_ERR_NOT_FOUND));
        return;
    }
    printf("  即将删除：%s%s%s\n", C_CYAN, g_teachers.data[idx].name, C_RESET);
    printf("  确认删除？(1=是，0=否)：");
    int confirm = inputInt("");
    if (confirm == 1) {
        arrayDelete(&g_teachers, idx);
        printSuccess("删除成功！");
        saveData(&g_teachers, DATA_FILE);
    } else {
        printInfo("已取消删除");
    }
}

static void modifyTeacher(void)
{
    printTitle("【修改教师】");
    if (g_teachers.size == 0) {
        printInfo("暂无教师记录");
        return;
    }
    int id = inputInt("  请输入要修改的教师编号：");
    int idx = arrayFindById(&g_teachers, id);
    if (idx < 0) {
        printError(salStatusStr(SAL_ERR_NOT_FOUND));
        return;
    }
    Teacher *t = &g_teachers.data[idx];
    printf("  当前信息：\n");
    printf("    姓名：%s  部门：%s  职称：%s\n", t->name, t->dept, t->title);
    printf("    直接回车保留原值。\n\n");

    char buf[64];
    inputString("  新姓名：", buf, sizeof(buf));
    if (strlen(buf) > 0) safeStrncpyInternal(t->name, buf, MAX_NAME_LEN);
    inputString("  新部门：", buf, sizeof(buf));
    if (strlen(buf) > 0) safeStrncpyInternal(t->dept, buf, MAX_DEPT_LEN);
    inputString("  新职称：", buf, sizeof(buf));
    if (strlen(buf) > 0) safeStrncpyInternal(t->title, buf, MAX_TITLE_LEN);

    printf("  新基本工资(回车保留)：");
    safeInput(buf, sizeof(buf));
    if (strlen(buf) > 0) t->base_salary = atof(buf);

    printf("  新绩效工资(回车保留)：");
    safeInput(buf, sizeof(buf));
    if (strlen(buf) > 0) t->merit_pay = atof(buf);

    printf("  新津贴补贴(回车保留)：");
    safeInput(buf, sizeof(buf));
    if (strlen(buf) > 0) t->allowance = atof(buf);

    printf("  新扣款金额(回车保留)：");
    safeInput(buf, sizeof(buf));
    if (strlen(buf) > 0) t->deduction = atof(buf);

    updateTotal(t);
    printSuccess("修改成功！");
    saveData(&g_teachers, DATA_FILE);
}

static void listAllTeachers(void)
{
    printTitle("【教师列表】");
    if (g_teachers.size == 0) {
        printInfo("暂无教师记录");
        return;
    }
    printLine();
    printf("  %s%-6s %-10s %-12s %-10s %-10s %-10s %-10s %-10s%s\n",
           C_BOLD, "编号", "姓名", "部门", "职称", "基本工资", "绩效", "津贴", "实发工资", C_RESET);
    printLine();
    for (int i = 0; i < g_teachers.size; i++) {
        const Teacher *t = &g_teachers.data[i];
        printf("  %-6d %-10s %-12s %-10s %-10.2f %-10.2f %-10.2f %s%-10.2f%s\n",
               t->id, t->name, t->dept, t->title,
               t->base_salary, t->merit_pay, t->allowance,
               C_GREEN, t->total, C_RESET);
    }
    printLine();
    printf("  共 %s%d%s 位教师\n", C_CYAN, g_teachers.size, C_RESET);
}

static void queryTeacher(void)
{
    printTitle("【查询教师】");
    if (g_teachers.size == 0) {
        printInfo("暂无教师记录");
        return;
    }
    printf("  %s1%s. 按编号查询\n", C_CYAN, C_RESET);
    printf("  %s2%s. 按姓名查询\n", C_CYAN, C_RESET);
    printf("  %s3%s. 列出全部教师\n", C_CYAN, C_RESET);
    int choice = inputInt("  请选择查询方式：");

    if (choice == 1) {
        int id = inputInt("  请输入教师编号：");
        int idx = arrayFindById(&g_teachers, id);
        if (idx < 0) {
            printError(salStatusStr(SAL_ERR_NOT_FOUND));
            return;
        }
        printLine();
        const Teacher *t = &g_teachers.data[idx];
        printf("  %s%-6s %-10s %-12s %-10s %-10s %-10s %-10s %-10s%s\n",
               C_BOLD, "编号", "姓名", "部门", "职称", "基本工资", "绩效", "津贴", "实发工资", C_RESET);
        printLine();
        printf("  %-6d %-10s %-12s %-10s %-10.2f %-10.2f %-10.2f %s%-10.2f%s\n",
               t->id, t->name, t->dept, t->title,
               t->base_salary, t->merit_pay, t->allowance,
               C_GREEN, t->total, C_RESET);
        printLine();
    } else if (choice == 2) {
        char name[MAX_NAME_LEN];
        inputString("  请输入教师姓名：", name, MAX_NAME_LEN);
        int found = 0;
        printLine();
        printf("  %s%-6s %-10s %-12s %-10s %-10s %-10s %-10s %-10s%s\n",
               C_BOLD, "编号", "姓名", "部门", "职称", "基本工资", "绩效", "津贴", "实发工资", C_RESET);
        printLine();
        for (int i = 0; i < g_teachers.size; i++) {
            if (strstr(g_teachers.data[i].name, name) != NULL) {
                const Teacher *t = &g_teachers.data[i];
                printf("  %-6d %-10s %-12s %-10s %-10.2f %-10.2f %-10.2f %s%-10.2f%s\n",
                       t->id, t->name, t->dept, t->title,
                       t->base_salary, t->merit_pay, t->allowance,
                       C_GREEN, t->total, C_RESET);
                found++;
            }
        }
        printLine();
        if (found == 0) printInfo("未找到匹配记录");
        else printf("  共找到 %s%d%s 条记录\n", C_CYAN, found, C_RESET);
    } else if (choice == 3) {
        listAllTeachers();
    }
}

static void showStatistics(void)
{
    printTitle("【工资统计】");
    Statistics stats;
    SalStatus st = calcStatistics(&g_teachers, &stats);
    if (st != SAL_OK) {
        printInfo("暂无数据可统计");
        return;
    }
    printLine();
    printf("  %s总人数：%s    %d 人\n", C_CYAN, C_RESET, stats.count);
    printLine();
    printf("  基本工资总额：  %s%.2f%s 元\n", C_YELLOW, stats.total_base, C_RESET);
    printf("  绩效工资总额：  %s%.2f%s 元\n", C_YELLOW, stats.total_merit, C_RESET);
    printf("  津贴补贴总额：  %s%.2f%s 元\n", C_YELLOW, stats.total_allowance, C_RESET);
    printf("  扣款总额：      %s%.2f%s 元\n", C_RED, stats.total_deduction, C_RESET);
    printLine();
    printf("  %s实发工资总额：%s %.2f 元\n", C_BOLD C_GREEN, C_RESET, stats.total_salary);
    printf("  平均工资：      %.2f 元\n", stats.avg_salary);
    printf("  最高工资：      %s%.2f%s 元\n", C_GREEN, stats.max_salary, C_RESET);
    printf("  最低工资：      %s%.2f%s 元\n", C_RED, stats.min_salary, C_RESET);
    printLine();
}

/* ======================== 内置自测系统 ======================== */

#define TEST_ASSERT(cond, msg) do { \
    if (cond) { \
        printf("  %s[PASS]%s %s\n", C_GREEN, C_RESET, msg); \
        g_test_passed++; \
    } else { \
        printf("  %s[FAIL]%s %s (line %d)\n", C_RED, C_RESET, msg, __LINE__); \
        g_test_failed++; \
    } \
} while(0)

#define TEST_STATUS(st, expected, msg) TEST_ASSERT((st) == (expected), msg)

static int runSelfTests(void)
{
    g_test_mode = 1;
    g_test_passed = 0;
    g_test_failed = 0;

    printf("\n%s╔══════════════════════════════════════════════╗%s\n", C_CYAN C_BOLD, C_RESET);
    printf("%s║          教师工资系统 V2.0 自测开始          ║%s\n", C_CYAN C_BOLD, C_RESET);
    printf("%s╚══════════════════════════════════════════════╝%s\n\n", C_CYAN C_BOLD, C_RESET);

    TeacherArray test_arr;
    SalStatus st;

    /* TC01: 初始化 */
    st = arrayInit(&test_arr);
    TEST_STATUS(st, SAL_OK, "TC01: 动态数组初始化");

    /* TC02: 初始状态 */
    TEST_ASSERT(test_arr.size == 0, "TC02: 初始size=0");
    TEST_ASSERT(test_arr.capacity == INIT_CAPACITY, "TC02: 初始capacity正确");
    TEST_ASSERT(test_arr.next_id == 1, "TC02: 初始next_id=1");

    /* TC03: 添加记录 */
    Teacher t1;
    memset(&t1, 0, sizeof(t1));
    safeStrncpyInternal(t1.name, "张三", MAX_NAME_LEN);
    safeStrncpyInternal(t1.dept, "计算机学院", MAX_DEPT_LEN);
    safeStrncpyInternal(t1.title, "教授", MAX_TITLE_LEN);
    t1.base_salary = 8000;
    t1.merit_pay = 3000;
    t1.allowance = 1500;
    t1.deduction = 500;
    int id1;
    st = arrayAdd(&test_arr, &t1, &id1);
    TEST_STATUS(st, SAL_OK, "TC03: 添加第1位教师");
    TEST_ASSERT(id1 == 1, "TC03: ID自动分配为1");
    TEST_ASSERT(test_arr.size == 1, "TC03: size=1");

    /* TC04: 工资计算 */
    double expected_total = 8000.0 + 3000.0 + 1500.0 - 500.0;
    TEST_ASSERT(test_arr.data[0].total == expected_total, "TC04: 工资计算正确(12000)");

    /* TC05: 添加多条 */
    Teacher t2;
    memset(&t2, 0, sizeof(t2));
    safeStrncpyInternal(t2.name, "李四", MAX_NAME_LEN);
    safeStrncpyInternal(t2.dept, "数学学院", MAX_DEPT_LEN);
    safeStrncpyInternal(t2.title, "副教授", MAX_TITLE_LEN);
    t2.base_salary = 7000;
    t2.merit_pay = 2500;
    t2.allowance = 1200;
    t2.deduction = 400;
    Teacher t3;
    memset(&t3, 0, sizeof(t3));
    safeStrncpyInternal(t3.name, "王五", MAX_NAME_LEN);
    safeStrncpyInternal(t3.dept, "外语学院", MAX_DEPT_LEN);
    safeStrncpyInternal(t3.title, "讲师", MAX_TITLE_LEN);
    t3.base_salary = 6000;
    t3.merit_pay = 2000;
    t3.allowance = 1000;
    t3.deduction = 300;
    int id2, id3;
    st = arrayAdd(&test_arr, &t2, &id2);
    TEST_STATUS(st, SAL_OK, "TC05: 添加第2位教师");
    st = arrayAdd(&test_arr, &t3, &id3);
    TEST_STATUS(st, SAL_OK, "TC05: 添加第3位教师");
    TEST_ASSERT(test_arr.size == 3, "TC05: size=3");
    TEST_ASSERT(id2 == 2 && id3 == 3, "TC05: ID自增正确");

    /* TC06: ID查找 */
    int idx = arrayFindById(&test_arr, 2);
    TEST_ASSERT(idx == 1, "TC06: 按ID查找找到索引1");
    TEST_ASSERT(strcmp(test_arr.data[idx].name, "李四") == 0, "TC06: 查找姓名正确");

    /* TC07: 查找不存在ID */
    idx = arrayFindById(&test_arr, 999);
    TEST_ASSERT(idx == -1, "TC07: 查找不存在ID返回-1");

    /* TC08: 扩容测试 */
    int initial_cap = test_arr.capacity;
    for (int i = 0; i < 10; i++) {
        Teacher tmp;
        memset(&tmp, 0, sizeof(tmp));
        safeStrncpyInternal(tmp.name, "测试", MAX_NAME_LEN);
        safeStrncpyInternal(tmp.dept, "测试部门", MAX_DEPT_LEN);
        safeStrncpyInternal(tmp.title, "测试职称", MAX_TITLE_LEN);
        tmp.base_salary = 5000;
        tmp.merit_pay = 1000;
        tmp.allowance = 500;
        tmp.deduction = 200;
        arrayAdd(&test_arr, &tmp, NULL);
    }
    TEST_ASSERT(test_arr.capacity > initial_cap, "TC08: 自动扩容生效");
    TEST_ASSERT(test_arr.size == 13, "TC08: 扩容后size=13");

    /* TC09: 统计功能 */
    Statistics stats;
    st = calcStatistics(&test_arr, &stats);
    TEST_STATUS(st, SAL_OK, "TC09: 统计计算成功");
    TEST_ASSERT(stats.count == 13, "TC09: 统计人数=13");
    TEST_ASSERT(stats.total_salary > 0, "TC09: 工资总额>0");
    TEST_ASSERT(stats.avg_salary > 0, "TC09: 平均工资>0");
    TEST_ASSERT(stats.max_salary >= stats.min_salary, "TC09: 最高>=最低");

    /* TC10: 删除记录 */
    st = arrayDelete(&test_arr, 1);
    TEST_STATUS(st, SAL_OK, "TC10: 删除记录成功");
    TEST_ASSERT(test_arr.size == 12, "TC10: 删除后size=12");

    /* TC11: 删除越界 */
    st = arrayDelete(&test_arr, -1);
    TEST_STATUS(st, SAL_ERR_NOT_FOUND, "TC11: 删除越界返回NOT_FOUND");
    st = arrayDelete(&test_arr, 999);
    TEST_STATUS(st, SAL_ERR_NOT_FOUND, "TC11: 删除不存在索引返回NOT_FOUND");

    /* TC12: 文件保存 */
    const char *test_file = "test_salv2.dat";
    st = saveData(&test_arr, test_file);
    TEST_STATUS(st, SAL_OK, "TC12: 保存数据文件成功");

    /* TC13: 文件加载 */
    TeacherArray load_arr;
    arrayInit(&load_arr);
    st = loadData(&load_arr, test_file);
    TEST_STATUS(st, SAL_OK, "TC13: 加载数据文件成功");
    TEST_ASSERT(load_arr.size == test_arr.size, "TC13: 加载后size一致");
    TEST_ASSERT(load_arr.next_id == test_arr.next_id, "TC13: 加载后next_id一致");
    TEST_ASSERT(strcmp(load_arr.data[0].name, "张三") == 0, "TC13: 第一条数据姓名正确");
    TEST_ASSERT(load_arr.data[0].total == expected_total, "TC13: 工资计算一致");
    remove(test_file);
    arrayFree(&load_arr);

    /* TC14: 魔数验证 */
    FILE *fp = fopen(test_file, "wb");
    if (fp != NULL) {
        fputs("BAD!!", fp);
        fclose(fp);
    }
    arrayInit(&load_arr);
    st = loadData(&load_arr, test_file);
    TEST_STATUS(st, SAL_ERR_FILE_MAGIC, "TC14: 魔数错误检测正确");
    arrayFree(&load_arr);
    remove(test_file);

    /* TC15: 空文件处理 */
    fp = fopen(test_file, "wb");
    if (fp != NULL) fclose(fp);
    arrayInit(&load_arr);
    st = loadData(&load_arr, test_file);
    TEST_STATUS(st, SAL_ERR_FILE_MAGIC, "TC15: 空文件加载失败");
    arrayFree(&load_arr);
    remove(test_file);

    /* TC16: 错误码字符串 */
    TEST_ASSERT(strcmp(salStatusStr(SAL_OK), "操作成功") == 0, "TC16: SAL_OK消息正确");
    TEST_ASSERT(strcmp(salStatusStr(SAL_ERR_MEMORY), "内存分配失败") == 0, "TC16: MEMORY消息正确");
    TEST_ASSERT(strcmp(salStatusStr(SAL_ERR_NOT_FOUND), "未找到记录") == 0, "TC16: NOT_FOUND消息正确");

    /* TC17: 内存释放 */
    arrayFree(&test_arr);
    TEST_ASSERT(test_arr.data == NULL, "TC17: 内存释放成功");
    TEST_ASSERT(test_arr.size == 0, "TC17: 释放后size=0");

    remove(DATA_FILE);

    printf("\n");
    printLine();
    printf("  %s自测完成：%s %s%d%s 通过, %s%d%s 失败\n",
           C_BOLD, C_RESET,
           C_GREEN, g_test_passed, C_RESET,
           g_test_failed > 0 ? C_RED : C_GREEN, g_test_failed, C_RESET);
    printLine();

    g_test_mode = 0;
    return g_test_failed == 0 ? 0 : 1;
}

/* ======================== 帮助信息 ======================== */

static void printHelp(const char *prog)
{
    printf("%s╔══════════════════════════════════════════════════════════════╗%s\n", C_CYAN C_BOLD, C_RESET);
    printf("%s║           教师工资信息管理系统  v2.0  使用帮助               ║%s\n", C_CYAN C_BOLD, C_RESET);
    printf("%s╚══════════════════════════════════════════════════════════════╝%s\n\n", C_CYAN C_BOLD, C_RESET);
    printf("  %s用法:%s %s [选项]\n\n", C_BOLD, C_RESET, prog);
    printf("  %s选项:%s\n", C_BOLD, C_RESET);
    printf("    %s-h, --help%s     显示本帮助信息\n", C_GREEN, C_RESET);
    printf("    %s-t, --test%s     运行内置自测程序\n", C_GREEN, C_RESET);
    printf("    (无参数)         启动交互菜单模式\n\n");
    printf("  %s交互菜单功能:%s\n", C_BOLD, C_RESET);
    printf("    1. 添加教师        2. 删除教师\n");
    printf("    3. 修改教师        4. 查询教师\n");
    printf("    5. 教师列表        6. 工资统计\n");
    printf("    0. 退出系统\n\n");
    printf("  %s数据文件:%s %s (SALV2二进制格式)\n", C_BOLD, C_RESET, DATA_FILE);
    printf("  %s数据自动保存于当前目录。%s\n\n", C_DIM, C_RESET);
}

/* ======================== 主菜单 ======================== */

static void mainMenu(void)
{
    while (1) {
        printf("\n\n");
        printf("%s╔══════════════════════════════════════════════════════════════╗%s\n", C_CYAN C_BOLD, C_RESET);
        printf("%s║           教师工资信息管理系统  v2.0                        ║%s\n", C_CYAN C_BOLD, C_RESET);
        printf("%s╠══════════════════════════════════════════════════════════════╣%s\n", C_CYAN, C_RESET);
        printf("%s║%s  1. 添加教师                 2. 删除教师                   %s║%s\n", C_CYAN, C_WHITE, C_CYAN, C_RESET);
        printf("%s║%s  3. 修改教师                 4. 查询教师                   %s║%s\n", C_CYAN, C_WHITE, C_CYAN, C_RESET);
        printf("%s║%s  5. 教师列表                 6. 工资统计                   %s║%s\n", C_CYAN, C_WHITE, C_CYAN, C_RESET);
        printf("%s║%s  0. 退出系统                                               %s║%s\n", C_CYAN, C_RED, C_CYAN, C_RESET);
        printf("%s╚══════════════════════════════════════════════════════════════╝%s\n", C_CYAN C_BOLD, C_RESET);

        int choice = inputInt("  请输入您的选择：");

        switch (choice) {
            case 1: addTeacher(); break;
            case 2: deleteTeacher(); break;
            case 3: modifyTeacher(); break;
            case 4: queryTeacher(); break;
            case 5: listAllTeachers(); break;
            case 6: showStatistics(); break;
            case 0:
                printSuccess("感谢使用，再见！");
                return;
            default:
                printError("无效选择，请重新输入");
        }
        if (choice != 0) pressAnyKey();
    }
}

/* ======================== 主函数 ======================== */

int main(int argc, char *argv[])
{
    initConsole();

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printHelp(argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--test") == 0) {
            return runSelfTests();
        }
    }

    SalStatus st = arrayInit(&g_teachers);
    if (st != SAL_OK) {
        printf("%s[致命错误]%s %s\n", C_RED, C_RESET, salStatusStr(st));
        return 1;
    }

    st = loadData(&g_teachers, DATA_FILE);
    if (st == SAL_OK) {
        printf("  %s[信息]%s 已加载 %s%d%s 条教师记录\n",
               C_GREEN, C_RESET, C_CYAN, g_teachers.size, C_RESET);
    } else if (st == SAL_ERR_FILE_OPEN) {
        printf("  %s[信息]%s 未找到数据文件，将创建新文件\n", C_YELLOW, C_RESET);
    } else {
        printf("  %s[警告]%s 数据文件加载失败：%s\n", C_YELLOW, C_RESET, salStatusStr(st));
    }

    printf("\n  %s欢迎使用教师工资信息管理系统 V2.0！%s\n", C_BOLD C_CYAN, C_RESET);

    mainMenu();

    saveData(&g_teachers, DATA_FILE);
    arrayFree(&g_teachers);

    return 0;
}
