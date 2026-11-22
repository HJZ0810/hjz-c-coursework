/*
 * 学生社团成员信息管理系统 V2.0
 * Student Club Member Management System V2.0
 *
 * 编译器：GCC (C99标准)
 * 编译命令：gcc -std=c99 -Wall -Wextra -O2 main.c -o clubmgr
 *
 * V2.0 新特性：
 *   - UTF-8 BOM编码 + SetConsoleOutputCP(65001) Windows控制台支持
 *   - ANSI彩色终端输出
 *   - CLI参数解析 (--help/-h/--test/-t)
 *   - 动态数组存储（替换链表），支持自动扩容
 *   - 统一错误码枚举 ClubStatus
 *   - 文件持久化带Magic Header "CLUBV2"
 *   - 内置自测框架（15+测试用例）
 *   - 完整中文注释
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>

/*====================================================================================================
 *
 *  ANSI 终端颜色码定义
 *
 *====================================================================================================*/

#define ANSI_RESET      "\033[0m"
#define ANSI_BOLD       "\033[1m"
#define ANSI_DIM        "\033[2m"
#define ANSI_RED        "\033[31m"
#define ANSI_GREEN      "\033[32m"
#define ANSI_YELLOW     "\033[33m"
#define ANSI_BLUE       "\033[34m"
#define ANSI_MAGENTA    "\033[35m"
#define ANSI_CYAN       "\033[36m"
#define ANSI_WHITE      "\033[37m"
#define ANSI_BG_RED     "\033[41m"
#define ANSI_BG_GREEN   "\033[42m"
#define ANSI_BG_BLUE    "\033[44m"

#define COLOR_SUCCESS   ANSI_GREEN
#define COLOR_ERROR     ANSI_RED
#define COLOR_WARN      ANSI_YELLOW
#define COLOR_INFO      ANSI_CYAN
#define COLOR_HEADER    ANSI_BOLD ANSI_BLUE
#define COLOR_PROMPT    ANSI_BOLD ANSI_MAGENTA

/*====================================================================================================
 *
 *  常量与宏定义
 *
 *====================================================================================================*/

#define MAX_NAME        50
#define MAX_ID          20
#define MAX_COLLEGE     50
#define MAX_MAJOR       50
#define MAX_GRADE       10
#define MAX_LOCATION    100
#define MAX_DATE        20
#define MAX_POSITION    30
#define MAX_NOTE        100
#define MAX_DESC        200

#define INITIAL_CAPACITY    8
#define GROWTH_FACTOR       2

#define DATA_FILE       "clubv2.dat"
#define MAGIC_HEADER    "CLUBV2"
#define MAGIC_LEN       6
#define DATA_VERSION    2

/*====================================================================================================
 *
 *  统一错误码枚举
 *
 *====================================================================================================*/

typedef enum {
    CLUB_OK = 0,
    CLUB_ERR_MEMORY,
    CLUB_ERR_NOT_FOUND,
    CLUB_ERR_DUPLICATE,
    CLUB_ERR_INVALID_INPUT,
    CLUB_ERR_FILE_OPEN,
    CLUB_ERR_FILE_READ,
    CLUB_ERR_FILE_WRITE,
    CLUB_ERR_MAGIC_MISMATCH,
    CLUB_ERR_VERSION_MISMATCH,
    CLUB_ERR_EMPTY,
    CLUB_ERR_IO
} ClubStatus;

/*====================================================================================================
 *
 *  数据结构定义
 *
 *====================================================================================================*/

typedef struct {
    char name[MAX_NAME];
    char id[MAX_ID];
    int  age;
    char gender[10];
    char college[MAX_COLLEGE];
    char major[MAX_MAJOR];
    char grade[MAX_GRADE];
} Student;

typedef struct {
    char clubId[MAX_ID];
    char name[MAX_NAME];
    char leader[MAX_NAME];
    char location[MAX_LOCATION];
    char foundDate[MAX_DATE];
    char description[MAX_DESC];
} Club;

typedef struct {
    char studentId[MAX_ID];
    char clubId[MAX_ID];
    char joinDate[MAX_DATE];
    char position[MAX_POSITION];
    char note[MAX_NOTE];
} Member;

typedef struct {
    Student *items;
    size_t count;
    size_t capacity;
} StudentArray;

typedef struct {
    Club *items;
    size_t count;
    size_t capacity;
} ClubArray;

typedef struct {
    Member *items;
    size_t count;
    size_t capacity;
} MemberArray;

/*====================================================================================================
 *
 *  全局数据
 *
 *====================================================================================================*/

static StudentArray g_students = {NULL, 0, 0};
static ClubArray    g_clubs    = {NULL, 0, 0};
static MemberArray  g_members  = {NULL, 0, 0};

/*====================================================================================================
 *
 *  工具函数
 *
 *====================================================================================================*/

static void clearInputBuffer(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

static void readLine(char *buf, int size)
{
    if (fgets(buf, size, stdin) != NULL) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
        } else {
            clearInputBuffer();
        }
    }
}

static int isEmpty(const char *str)
{
    if (str == NULL) return 1;
    while (*str) {
        if (!isspace((unsigned char)*str)) return 0;
        str++;
    }
    return 1;
}

static void printSeparator(void)
{
    printf(ANSI_DIM "------------------------------------------------------------" ANSI_RESET "\n");
}

static void printSuccess(const char *msg)
{
    printf(COLOR_SUCCESS "  [✓] %s" ANSI_RESET "\n", msg);
}

static void printError(const char *msg)
{
    printf(COLOR_ERROR "  [✗] %s" ANSI_RESET "\n", msg);
}

static void printWarning(const char *msg)
{
    printf(COLOR_WARN "  [!] %s" ANSI_RESET "\n", msg);
}

static void printInfo(const char *msg)
{
    printf(COLOR_INFO "  [i] %s" ANSI_RESET "\n", msg);
}

/*====================================================================================================
 *
 *  动态数组管理
 *
 *====================================================================================================*/

static ClubStatus studentArrayInit(StudentArray *arr)
{
    arr->items = (Student*)calloc(INITIAL_CAPACITY, sizeof(Student));
    if (arr->items == NULL) return CLUB_ERR_MEMORY;
    arr->count = 0;
    arr->capacity = INITIAL_CAPACITY;
    return CLUB_OK;
}

static ClubStatus studentArrayResize(StudentArray *arr)
{
    size_t newCap = arr->capacity * GROWTH_FACTOR;
    Student *newItems = (Student*)realloc(arr->items, newCap * sizeof(Student));
    if (newItems == NULL) return CLUB_ERR_MEMORY;
    arr->items = newItems;
    arr->capacity = newCap;
    return CLUB_OK;
}

static ClubStatus studentArrayAdd(StudentArray *arr, const Student *s)
{
    if (arr->count >= arr->capacity) {
        ClubStatus st = studentArrayResize(arr);
        if (st != CLUB_OK) return st;
    }
    arr->items[arr->count] = *s;
    arr->count++;
    return CLUB_OK;
}

static int studentArrayFindById(const StudentArray *arr, const char *id)
{
    for (size_t i = 0; i < arr->count; i++) {
        if (strcmp(arr->items[i].id, id) == 0)
            return (int)i;
    }
    return -1;
}

static ClubStatus studentArrayDelete(StudentArray *arr, size_t index)
{
    if (index >= arr->count) return CLUB_ERR_NOT_FOUND;
    for (size_t i = index; i < arr->count - 1; i++) {
        arr->items[i] = arr->items[i + 1];
    }
    arr->count--;
    memset(&arr->items[arr->count], 0, sizeof(Student));
    return CLUB_OK;
}

static void studentArrayFree(StudentArray *arr)
{
    free(arr->items);
    arr->items = NULL;
    arr->count = 0;
    arr->capacity = 0;
}

static ClubStatus clubArrayInit(ClubArray *arr)
{
    arr->items = (Club*)calloc(INITIAL_CAPACITY, sizeof(Club));
    if (arr->items == NULL) return CLUB_ERR_MEMORY;
    arr->count = 0;
    arr->capacity = INITIAL_CAPACITY;
    return CLUB_OK;
}

static ClubStatus clubArrayResize(ClubArray *arr)
{
    size_t newCap = arr->capacity * GROWTH_FACTOR;
    Club *newItems = (Club*)realloc(arr->items, newCap * sizeof(Club));
    if (newItems == NULL) return CLUB_ERR_MEMORY;
    arr->items = newItems;
    arr->capacity = newCap;
    return CLUB_OK;
}

static ClubStatus clubArrayAdd(ClubArray *arr, const Club *c)
{
    if (arr->count >= arr->capacity) {
        ClubStatus st = clubArrayResize(arr);
        if (st != CLUB_OK) return st;
    }
    arr->items[arr->count] = *c;
    arr->count++;
    return CLUB_OK;
}

static int clubArrayFindById(const ClubArray *arr, const char *id)
{
    for (size_t i = 0; i < arr->count; i++) {
        if (strcmp(arr->items[i].clubId, id) == 0)
            return (int)i;
    }
    return -1;
}

static ClubStatus clubArrayDelete(ClubArray *arr, size_t index)
{
    if (index >= arr->count) return CLUB_ERR_NOT_FOUND;
    for (size_t i = index; i < arr->count - 1; i++) {
        arr->items[i] = arr->items[i + 1];
    }
    arr->count--;
    memset(&arr->items[arr->count], 0, sizeof(Club));
    return CLUB_OK;
}

static void clubArrayFree(ClubArray *arr)
{
    free(arr->items);
    arr->items = NULL;
    arr->count = 0;
    arr->capacity = 0;
}

static ClubStatus memberArrayInit(MemberArray *arr)
{
    arr->items = (Member*)calloc(INITIAL_CAPACITY, sizeof(Member));
    if (arr->items == NULL) return CLUB_ERR_MEMORY;
    arr->count = 0;
    arr->capacity = INITIAL_CAPACITY;
    return CLUB_OK;
}

static ClubStatus memberArrayResize(MemberArray *arr)
{
    size_t newCap = arr->capacity * GROWTH_FACTOR;
    Member *newItems = (Member*)realloc(arr->items, newCap * sizeof(Member));
    if (newItems == NULL) return CLUB_ERR_MEMORY;
    arr->items = newItems;
    arr->capacity = newCap;
    return CLUB_OK;
}

static ClubStatus memberArrayAdd(MemberArray *arr, const Member *m)
{
    if (arr->count >= arr->capacity) {
        ClubStatus st = memberArrayResize(arr);
        if (st != CLUB_OK) return st;
    }
    arr->items[arr->count] = *m;
    arr->count++;
    return CLUB_OK;
}

static int memberArrayFindByBoth(const MemberArray *arr, const char *sid, const char *cid)
{
    for (size_t i = 0; i < arr->count; i++) {
        if (strcmp(arr->items[i].studentId, sid) == 0 &&
            strcmp(arr->items[i].clubId, cid) == 0)
            return (int)i;
    }
    return -1;
}

static ClubStatus memberArrayDelete(MemberArray *arr, size_t index)
{
    if (index >= arr->count) return CLUB_ERR_NOT_FOUND;
    for (size_t i = index; i < arr->count - 1; i++) {
        arr->items[i] = arr->items[i + 1];
    }
    arr->count--;
    memset(&arr->items[arr->count], 0, sizeof(Member));
    return CLUB_OK;
}

static void memberArrayFree(MemberArray *arr)
{
    free(arr->items);
    arr->items = NULL;
    arr->count = 0;
    arr->capacity = 0;
}

/*====================================================================================================
 *
 *  文件持久化（带Magic Header）
 *
 *====================================================================================================*/

static ClubStatus saveAllData(void)
{
    FILE *fp = fopen(DATA_FILE, "wb");
    if (fp == NULL) return CLUB_ERR_FILE_OPEN;

    if (fwrite(MAGIC_HEADER, 1, MAGIC_LEN, fp) != MAGIC_LEN) {
        fclose(fp);
        return CLUB_ERR_FILE_WRITE;
    }

    unsigned char ver = DATA_VERSION;
    if (fwrite(&ver, 1, 1, fp) != 1) {
        fclose(fp);
        return CLUB_ERR_FILE_WRITE;
    }

    size_t sc = g_students.count;
    size_t cc = g_clubs.count;
    size_t mc = g_members.count;

    if (fwrite(&sc, sizeof(size_t), 1, fp) != 1 ||
        fwrite(&cc, sizeof(size_t), 1, fp) != 1 ||
        fwrite(&mc, sizeof(size_t), 1, fp) != 1) {
        fclose(fp);
        return CLUB_ERR_FILE_WRITE;
    }

    if (g_students.count > 0) {
        if (fwrite(g_students.items, sizeof(Student), g_students.count, fp) != g_students.count) {
            fclose(fp);
            return CLUB_ERR_FILE_WRITE;
        }
    }
    if (g_clubs.count > 0) {
        if (fwrite(g_clubs.items, sizeof(Club), g_clubs.count, fp) != g_clubs.count) {
            fclose(fp);
            return CLUB_ERR_FILE_WRITE;
        }
    }
    if (g_members.count > 0) {
        if (fwrite(g_members.items, sizeof(Member), g_members.count, fp) != g_members.count) {
            fclose(fp);
            return CLUB_ERR_FILE_WRITE;
        }
    }

    fclose(fp);
    return CLUB_OK;
}

static ClubStatus loadAllData(void)
{
    FILE *fp = fopen(DATA_FILE, "rb");
    if (fp == NULL) return CLUB_ERR_FILE_OPEN;

    char magic[MAGIC_LEN + 1] = {0};
    if (fread(magic, 1, MAGIC_LEN, fp) != MAGIC_LEN) {
        fclose(fp);
        return CLUB_ERR_FILE_READ;
    }
    if (strcmp(magic, MAGIC_HEADER) != 0) {
        fclose(fp);
        return CLUB_ERR_MAGIC_MISMATCH;
    }

    unsigned char ver;
    if (fread(&ver, 1, 1, fp) != 1) {
        fclose(fp);
        return CLUB_ERR_FILE_READ;
    }
    if (ver != DATA_VERSION) {
        fclose(fp);
        return CLUB_ERR_VERSION_MISMATCH;
    }

    size_t sc = 0, cc = 0, mc = 0;
    if (fread(&sc, sizeof(size_t), 1, fp) != 1 ||
        fread(&cc, sizeof(size_t), 1, fp) != 1 ||
        fread(&mc, sizeof(size_t), 1, fp) != 1) {
        fclose(fp);
        return CLUB_ERR_FILE_READ;
    }

    studentArrayFree(&g_students);
    clubArrayFree(&g_clubs);
    memberArrayFree(&g_members);

    studentArrayInit(&g_students);
    clubArrayInit(&g_clubs);
    memberArrayInit(&g_members);

    for (size_t i = 0; i < sc; i++) {
        Student s;
        memset(&s, 0, sizeof(s));
        if (fread(&s, sizeof(Student), 1, fp) != 1) {
            fclose(fp);
            return CLUB_ERR_FILE_READ;
        }
        studentArrayAdd(&g_students, &s);
    }

    for (size_t i = 0; i < cc; i++) {
        Club c;
        memset(&c, 0, sizeof(c));
        if (fread(&c, sizeof(Club), 1, fp) != 1) {
            fclose(fp);
            return CLUB_ERR_FILE_READ;
        }
        clubArrayAdd(&g_clubs, &c);
    }

    for (size_t i = 0; i < mc; i++) {
        Member m;
        memset(&m, 0, sizeof(m));
        if (fread(&m, sizeof(Member), 1, fp) != 1) {
            fclose(fp);
            return CLUB_ERR_FILE_READ;
        }
        memberArrayAdd(&g_members, &m);
    }

    fclose(fp);
    return CLUB_OK;
}

/*====================================================================================================
 *
 *  初始化与清理
 *
 *====================================================================================================*/

static ClubStatus initSystem(void)
{
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    ClubStatus s1 = studentArrayInit(&g_students);
    ClubStatus s2 = clubArrayInit(&g_clubs);
    ClubStatus s3 = memberArrayInit(&g_members);
    if (s1 != CLUB_OK || s2 != CLUB_OK || s3 != CLUB_OK) {
        return CLUB_ERR_MEMORY;
    }

    ClubStatus st = loadAllData();
    if (st == CLUB_OK) {
        return CLUB_OK;
    }
    return CLUB_OK;
}

static void cleanupSystem(void)
{
    saveAllData();
    studentArrayFree(&g_students);
    clubArrayFree(&g_clubs);
    memberArrayFree(&g_members);
}

/*====================================================================================================
 *
 *  业务逻辑函数
 *
 *====================================================================================================*/

static ClubStatus addStudent(const char *name, const char *id, int age,
                              const char *gender, const char *college,
                              const char *major, const char *grade)
{
    if (isEmpty(name) || isEmpty(id) || age <= 0 || age > 150 ||
        isEmpty(gender) || isEmpty(college) || isEmpty(major) || isEmpty(grade)) {
        return CLUB_ERR_INVALID_INPUT;
    }
    if (studentArrayFindById(&g_students, id) >= 0) {
        return CLUB_ERR_DUPLICATE;
    }
    Student s;
    memset(&s, 0, sizeof(s));
    strncpy(s.name, name, MAX_NAME - 1);
    strncpy(s.id, id, MAX_ID - 1);
    s.age = age;
    strncpy(s.gender, gender, 9);
    strncpy(s.college, college, MAX_COLLEGE - 1);
    strncpy(s.major, major, MAX_MAJOR - 1);
    strncpy(s.grade, grade, MAX_GRADE - 1);
    return studentArrayAdd(&g_students, &s);
}

static ClubStatus addClub(const char *clubId, const char *name,
                           const char *leader, const char *location,
                           const char *foundDate, const char *description)
{
    if (isEmpty(clubId) || isEmpty(name)) {
        return CLUB_ERR_INVALID_INPUT;
    }
    if (clubArrayFindById(&g_clubs, clubId) >= 0) {
        return CLUB_ERR_DUPLICATE;
    }
    Club c;
    memset(&c, 0, sizeof(c));
    strncpy(c.clubId, clubId, MAX_ID - 1);
    strncpy(c.name, name, MAX_NAME - 1);
    strncpy(c.leader, leader ? leader : "", MAX_NAME - 1);
    strncpy(c.location, location ? location : "", MAX_LOCATION - 1);
    strncpy(c.foundDate, foundDate ? foundDate : "", MAX_DATE - 1);
    strncpy(c.description, description ? description : "", MAX_DESC - 1);
    return clubArrayAdd(&g_clubs, &c);
}

static ClubStatus addMember(const char *studentId, const char *clubId,
                             const char *joinDate, const char *position,
                             const char *note)
{
    if (isEmpty(studentId) || isEmpty(clubId)) {
        return CLUB_ERR_INVALID_INPUT;
    }
    if (studentArrayFindById(&g_students, studentId) < 0) {
        return CLUB_ERR_NOT_FOUND;
    }
    if (clubArrayFindById(&g_clubs, clubId) < 0) {
        return CLUB_ERR_NOT_FOUND;
    }
    if (memberArrayFindByBoth(&g_members, studentId, clubId) >= 0) {
        return CLUB_ERR_DUPLICATE;
    }
    Member m;
    memset(&m, 0, sizeof(m));
    strncpy(m.studentId, studentId, MAX_ID - 1);
    strncpy(m.clubId, clubId, MAX_ID - 1);
    strncpy(m.joinDate, joinDate ? joinDate : "", MAX_DATE - 1);
    strncpy(m.position, position ? position : "成员", MAX_POSITION - 1);
    strncpy(m.note, note ? note : "", MAX_NOTE - 1);
    return memberArrayAdd(&g_members, &m);
}

static ClubStatus deleteStudent(const char *id)
{
    int idx = studentArrayFindById(&g_students, id);
    if (idx < 0) return CLUB_ERR_NOT_FOUND;

    for (int i = (int)g_members.count - 1; i >= 0; i--) {
        if (strcmp(g_members.items[i].studentId, id) == 0) {
            memberArrayDelete(&g_members, (size_t)i);
        }
    }
    return studentArrayDelete(&g_students, (size_t)idx);
}

static ClubStatus deleteClub(const char *clubId)
{
    int idx = clubArrayFindById(&g_clubs, clubId);
    if (idx < 0) return CLUB_ERR_NOT_FOUND;

    for (int i = (int)g_members.count - 1; i >= 0; i--) {
        if (strcmp(g_members.items[i].clubId, clubId) == 0) {
            memberArrayDelete(&g_members, (size_t)i);
        }
    }
    return clubArrayDelete(&g_clubs, (size_t)idx);
}

static ClubStatus deleteMember(const char *studentId, const char *clubId)
{
    int idx = memberArrayFindByBoth(&g_members, studentId, clubId);
    if (idx < 0) return CLUB_ERR_NOT_FOUND;
    return memberArrayDelete(&g_members, (size_t)idx);
}

/*====================================================================================================
 *
 *  显示函数
 *
 *====================================================================================================*/

static void listStudents(void)
{
    printSeparator();
    printf(COLOR_HEADER "  学生信息列表 (%zu 人)" ANSI_RESET "\n", g_students.count);
    printSeparator();
    if (g_students.count == 0) {
        printWarning("暂无学生信息");
        return;
    }
    printf(COLOR_INFO "  %-12s %-10s %-4s %-4s %-14s %-14s %-8s" ANSI_RESET "\n",
           "学号", "姓名", "年龄", "性别", "学院", "专业", "年级");
    printSeparator();
    for (size_t i = 0; i < g_students.count; i++) {
        Student *s = &g_students.items[i];
        printf("  %-12s %-10s %-4d %-4s %-14s %-14s %-8s\n",
               s->id, s->name, s->age, s->gender, s->college, s->major, s->grade);
    }
    printSeparator();
}

static void listClubs(void)
{
    printSeparator();
    printf(COLOR_HEADER "  社团信息列表 (%zu 个)" ANSI_RESET "\n", g_clubs.count);
    printSeparator();
    if (g_clubs.count == 0) {
        printWarning("暂无社团信息");
        return;
    }
    printf(COLOR_INFO "  %-8s %-14s %-10s %-14s %-12s" ANSI_RESET "\n",
           "编号", "名称", "负责人", "活动地点", "成立时间");
    printSeparator();
    for (size_t i = 0; i < g_clubs.count; i++) {
        Club *c = &g_clubs.items[i];
        printf("  %-8s %-14s %-10s %-14s %-12s\n",
               c->clubId, c->name, c->leader, c->location, c->foundDate);
    }
    printSeparator();
}

static void listMembers(void)
{
    printSeparator();
    printf(COLOR_HEADER "  成员归属列表 (%zu 条)" ANSI_RESET "\n", g_members.count);
    printSeparator();
    if (g_members.count == 0) {
        printWarning("暂无成员归属信息");
        return;
    }
    printf(COLOR_INFO "  %-12s %-8s %-12s %-10s" ANSI_RESET "\n",
           "学号", "社团编号", "加入时间", "职位");
    printSeparator();
    for (size_t i = 0; i < g_members.count; i++) {
        Member *m = &g_members.items[i];
        const char *sname = "?";
        const char *cname = "?";
        int si = studentArrayFindById(&g_students, m->studentId);
        if (si >= 0) sname = g_students.items[si].name;
        int ci = clubArrayFindById(&g_clubs, m->clubId);
        if (ci >= 0) cname = g_clubs.items[ci].name;
        printf("  %-12s %-8s %-12s %-10s  (%s - %s)\n",
               m->studentId, m->clubId, m->joinDate, m->position, sname, cname);
    }
    printSeparator();
}

/*====================================================================================================
 *
 *  交互式菜单
 *
 *====================================================================================================*/

static void showHelp(void)
{
    printf(COLOR_HEADER "\n  学生社团成员信息管理系统 V2.0\n" ANSI_RESET);
    printf("  用法: clubmgr [选项]\n\n");
    printf(COLOR_INFO "  选项:\n" ANSI_RESET);
    printf("    -h, --help      显示帮助信息\n");
    printf("    -t, --test      运行内置自测\n");
    printf("    (无参数)        启动交互菜单\n\n");
}

static void menuAddStudent(void)
{
    char name[MAX_NAME], id[MAX_ID], gender[10];
    char college[MAX_COLLEGE], major[MAX_MAJOR], grade[MAX_GRADE];
    int age;

    printSeparator();
    printf(COLOR_HEADER "  添加学生信息" ANSI_RESET "\n");
    printSeparator();

    printf(COLOR_PROMPT "  请输入学号: " ANSI_RESET);
    readLine(id, MAX_ID);
    if (isEmpty(id)) { printError("学号不能为空"); return; }

    printf(COLOR_PROMPT "  请输入姓名: " ANSI_RESET);
    readLine(name, MAX_NAME);
    if (isEmpty(name)) { printError("姓名不能为空"); return; }

    printf(COLOR_PROMPT "  请输入年龄: " ANSI_RESET);
    if (scanf("%d", &age) != 1 || age <= 0 || age > 150) {
        printError("年龄无效");
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    printf(COLOR_PROMPT "  请输入性别(男/女): " ANSI_RESET);
    readLine(gender, 10);
    if (isEmpty(gender)) { printError("性别不能为空"); return; }

    printf(COLOR_PROMPT "  请输入学院: " ANSI_RESET);
    readLine(college, MAX_COLLEGE);
    if (isEmpty(college)) { printError("学院不能为空"); return; }

    printf(COLOR_PROMPT "  请输入专业: " ANSI_RESET);
    readLine(major, MAX_MAJOR);
    if (isEmpty(major)) { printError("专业不能为空"); return; }

    printf(COLOR_PROMPT "  请输入年级: " ANSI_RESET);
    readLine(grade, MAX_GRADE);
    if (isEmpty(grade)) { printError("年级不能为空"); return; }

    ClubStatus st = addStudent(name, id, age, gender, college, major, grade);
    if (st == CLUB_OK) {
        printSuccess("学生添加成功");
        saveAllData();
    } else if (st == CLUB_ERR_DUPLICATE) {
        printError("该学号已存在");
    } else {
        printError("添加失败");
    }
}

static void menuAddClub(void)
{
    char clubId[MAX_ID], name[MAX_NAME], leader[MAX_NAME];
    char location[MAX_LOCATION], foundDate[MAX_DATE], description[MAX_DESC];

    printSeparator();
    printf(COLOR_HEADER "  添加社团信息" ANSI_RESET "\n");
    printSeparator();

    printf(COLOR_PROMPT "  请输入社团编号: " ANSI_RESET);
    readLine(clubId, MAX_ID);
    if (isEmpty(clubId)) { printError("编号不能为空"); return; }

    printf(COLOR_PROMPT "  请输入社团名称: " ANSI_RESET);
    readLine(name, MAX_NAME);
    if (isEmpty(name)) { printError("名称不能为空"); return; }

    printf(COLOR_PROMPT "  请输入负责人(可留空): " ANSI_RESET);
    readLine(leader, MAX_NAME);

    printf(COLOR_PROMPT "  请输入活动地点(可留空): " ANSI_RESET);
    readLine(location, MAX_LOCATION);

    printf(COLOR_PROMPT "  请输入成立日期(YYYY-MM-DD，可留空): " ANSI_RESET);
    readLine(foundDate, MAX_DATE);

    printf(COLOR_PROMPT "  请输入描述(可留空): " ANSI_RESET);
    readLine(description, MAX_DESC);

    ClubStatus st = addClub(clubId, name, leader, location, foundDate, description);
    if (st == CLUB_OK) {
        printSuccess("社团添加成功");
        saveAllData();
    } else if (st == CLUB_ERR_DUPLICATE) {
        printError("该社团编号已存在");
    } else {
        printError("添加失败");
    }
}

static void menuAddMember(void)
{
    char sid[MAX_ID], cid[MAX_ID], jd[MAX_DATE], pos[MAX_POSITION], note[MAX_NOTE];

    printSeparator();
    printf(COLOR_HEADER "  加入社团（添加成员归属）" ANSI_RESET "\n");
    printSeparator();

    printf(COLOR_PROMPT "  请输入学生学号: " ANSI_RESET);
    readLine(sid, MAX_ID);
    if (isEmpty(sid)) { printError("学号不能为空"); return; }

    printf(COLOR_PROMPT "  请输入社团编号: " ANSI_RESET);
    readLine(cid, MAX_ID);
    if (isEmpty(cid)) { printError("社团编号不能为空"); return; }

    printf(COLOR_PROMPT "  请输入加入日期(YYYY-MM-DD，可留空): " ANSI_RESET);
    readLine(jd, MAX_DATE);

    printf(COLOR_PROMPT "  请输入职位(默认成员，可留空): " ANSI_RESET);
    readLine(pos, MAX_POSITION);

    printf(COLOR_PROMPT "  请输入备注(可留空): " ANSI_RESET);
    readLine(note, MAX_NOTE);

    ClubStatus st = addMember(sid, cid, jd, pos, note);
    if (st == CLUB_OK) {
        printSuccess("加入社团成功");
        saveAllData();
    } else if (st == CLUB_ERR_NOT_FOUND) {
        printError("学号或社团编号不存在");
    } else if (st == CLUB_ERR_DUPLICATE) {
        printError("该学生已在此社团中");
    } else {
        printError("加入失败");
    }
}

static void menuDeleteStudent(void)
{
    char id[MAX_ID], confirm[10];
    printSeparator();
    printf(COLOR_HEADER "  删除学生信息" ANSI_RESET "\n");
    printSeparator();
    printf(COLOR_PROMPT "  请输入要删除的学号: " ANSI_RESET);
    readLine(id, MAX_ID);
    if (isEmpty(id)) { printError("学号不能为空"); return; }

    int idx = studentArrayFindById(&g_students, id);
    if (idx < 0) { printError("未找到该学生"); return; }

    printf(COLOR_WARN "  确认删除学生 %s (%s)？(y/n): " ANSI_RESET,
           g_students.items[idx].name, id);
    readLine(confirm, 10);
    if (confirm[0] == 'y' || confirm[0] == 'Y') {
        deleteStudent(id);
        printSuccess("删除成功");
        saveAllData();
    } else {
        printInfo("已取消");
    }
}

static void menuDeleteClub(void)
{
    char id[MAX_ID], confirm[10];
    printSeparator();
    printf(COLOR_HEADER "  删除社团信息" ANSI_RESET "\n");
    printSeparator();
    printf(COLOR_PROMPT "  请输入要删除的社团编号: " ANSI_RESET);
    readLine(id, MAX_ID);
    if (isEmpty(id)) { printError("编号不能为空"); return; }

    int idx = clubArrayFindById(&g_clubs, id);
    if (idx < 0) { printError("未找到该社团"); return; }

    printf(COLOR_WARN "  确认删除社团 %s (%s)？(y/n): " ANSI_RESET,
           g_clubs.items[idx].name, id);
    readLine(confirm, 10);
    if (confirm[0] == 'y' || confirm[0] == 'Y') {
        deleteClub(id);
        printSuccess("删除成功");
        saveAllData();
    } else {
        printInfo("已取消");
    }
}

static void menuDeleteMember(void)
{
    char sid[MAX_ID], cid[MAX_ID], confirm[10];
    printSeparator();
    printf(COLOR_HEADER "  退出社团" ANSI_RESET "\n");
    printSeparator();
    printf(COLOR_PROMPT "  请输入学生学号: " ANSI_RESET);
    readLine(sid, MAX_ID);
    printf(COLOR_PROMPT "  请输入社团编号: " ANSI_RESET);
    readLine(cid, MAX_ID);

    int idx = memberArrayFindByBoth(&g_members, sid, cid);
    if (idx < 0) { printError("未找到该成员记录"); return; }

    printf(COLOR_WARN "  确认退出？(y/n): " ANSI_RESET);
    readLine(confirm, 10);
    if (confirm[0] == 'y' || confirm[0] == 'Y') {
        deleteMember(sid, cid);
        printSuccess("已退出社团");
        saveAllData();
    } else {
        printInfo("已取消");
    }
}

static void searchMenu(void)
{
    int choice;
    do {
        printSeparator();
        printf(COLOR_HEADER "  查询子菜单" ANSI_RESET "\n");
        printSeparator();
        printf("  1. 查询学生信息（学号/姓名）\n");
        printf("  2. 查询社团信息（编号/名称）\n");
        printf("  3. 查询学生参加的社团\n");
        printf("  4. 查询社团的成员\n");
        printf("  0. 返回主菜单\n");
        printSeparator();
        printf(COLOR_PROMPT "  请选择: " ANSI_RESET);

        if (scanf("%d", &choice) != 1) {
            clearInputBuffer();
            printError("无效输入");
            continue;
        }
        clearInputBuffer();

        switch (choice) {
            case 1: {
                char kw[MAX_NAME];
                printf(COLOR_PROMPT "  请输入学号或姓名: " ANSI_RESET);
                readLine(kw, MAX_NAME);
                int found = 0;
                for (size_t i = 0; i < g_students.count; i++) {
                    Student *s = &g_students.items[i];
                    if (strcmp(s->id, kw) == 0 || strstr(s->name, kw) != NULL) {
                        if (found == 0) printSeparator();
                        printf("  学号: %s\n  姓名: %s\n  年龄: %d\n  性别: %s\n"
                               "  学院: %s\n  专业: %s\n  年级: %s\n",
                               s->id, s->name, s->age, s->gender,
                               s->college, s->major, s->grade);
                        printSeparator();
                        found++;
                    }
                }
                if (found == 0) printWarning("未找到匹配学生");
                else printf(COLOR_INFO "  找到 %d 条匹配记录" ANSI_RESET "\n", found);
                break;
            }
            case 2: {
                char kw[MAX_NAME];
                printf(COLOR_PROMPT "  请输入编号或名称: " ANSI_RESET);
                readLine(kw, MAX_NAME);
                int found = 0;
                for (size_t i = 0; i < g_clubs.count; i++) {
                    Club *c = &g_clubs.items[i];
                    if (strcmp(c->clubId, kw) == 0 || strstr(c->name, kw) != NULL) {
                        if (found == 0) printSeparator();
                        printf("  编号: %s\n  名称: %s\n  负责人: %s\n"
                               "  地点: %s\n  成立: %s\n  描述: %s\n",
                               c->clubId, c->name, c->leader,
                               c->location, c->foundDate, c->description);
                        printSeparator();
                        found++;
                    }
                }
                if (found == 0) printWarning("未找到匹配社团");
                else printf(COLOR_INFO "  找到 %d 条匹配记录" ANSI_RESET "\n", found);
                break;
            }
            case 3: {
                char sid[MAX_ID];
                printf(COLOR_PROMPT "  请输入学号: " ANSI_RESET);
                readLine(sid, MAX_ID);
                int si = studentArrayFindById(&g_students, sid);
                if (si < 0) { printError("学生不存在"); break; }
                printSeparator();
                printf("  %s 参加的社团:\n", g_students.items[si].name);
                printSeparator();
                int found = 0;
                for (size_t i = 0; i < g_members.count; i++) {
                    if (strcmp(g_members.items[i].studentId, sid) == 0) {
                        int ci = clubArrayFindById(&g_clubs, g_members.items[i].clubId);
                        const char *cname = ci >= 0 ? g_clubs.items[ci].name : "?";
                        printf("  - %s (%s)  职位: %s  加入: %s\n",
                               cname, g_members.items[i].clubId,
                               g_members.items[i].position, g_members.items[i].joinDate);
                        found++;
                    }
                }
                if (found == 0) printWarning("该学生未参加任何社团");
                break;
            }
            case 4: {
                char cid[MAX_ID];
                printf(COLOR_PROMPT "  请输入社团编号: " ANSI_RESET);
                readLine(cid, MAX_ID);
                int ci = clubArrayFindById(&g_clubs, cid);
                if (ci < 0) { printError("社团不存在"); break; }
                printSeparator();
                printf("  %s 社团成员:\n", g_clubs.items[ci].name);
                printSeparator();
                int found = 0;
                for (size_t i = 0; i < g_members.count; i++) {
                    if (strcmp(g_members.items[i].clubId, cid) == 0) {
                        int si = studentArrayFindById(&g_students, g_members.items[i].studentId);
                        const char *sname = si >= 0 ? g_students.items[si].name : "?";
                        printf("  - %s (%s)  职位: %s  加入: %s\n",
                               sname, g_members.items[i].studentId,
                               g_members.items[i].position, g_members.items[i].joinDate);
                        found++;
                    }
                }
                if (found == 0) printWarning("该社团暂无成员");
                break;
            }
            case 0:
                break;
            default:
                printError("无效选项");
        }
    } while (choice != 0);
}

static void mainMenu(void)
{
    int choice;
    do {
        printSeparator();
        printf(COLOR_HEADER "\n");
        printf("      ╔══════════════════════════════════════════╗\n");
        printf("      ║     学生社团成员信息管理系统 V2.0         ║\n");
        printf("      ╚══════════════════════════════════════════╝\n" ANSI_RESET);
        printSeparator();
        printf("  当前数据: " COLOR_INFO "%zu 学生 / %zu 社团 / %zu 成员记录" ANSI_RESET "\n",
               g_students.count, g_clubs.count, g_members.count);
        printSeparator();
        printf("  【学生管理】\n");
        printf("    1. 添加学生        2. 查看所有学生      3. 删除学生\n");
        printf("  【社团管理】\n");
        printf("    4. 添加社团        5. 查看所有社团      6. 删除社团\n");
        printf("  【成员管理】\n");
        printf("    7. 加入社团        8. 查看所有成员      9. 退出社团\n");
        printf("  【其他】\n");
        printf("   10. 查询信息        0. 保存并退出\n");
        printSeparator();
        printf(COLOR_PROMPT "  请选择操作: " ANSI_RESET);

        if (scanf("%d", &choice) != 1) {
            clearInputBuffer();
            printError("无效输入，请输入数字");
            continue;
        }
        clearInputBuffer();

        switch (choice) {
            case 1: menuAddStudent(); break;
            case 2: listStudents(); break;
            case 3: menuDeleteStudent(); break;
            case 4: menuAddClub(); break;
            case 5: listClubs(); break;
            case 6: menuDeleteClub(); break;
            case 7: menuAddMember(); break;
            case 8: listMembers(); break;
            case 9: menuDeleteMember(); break;
            case 10: searchMenu(); break;
            case 0:
                saveAllData();
                printSuccess("数据已保存，再见！");
                break;
            default:
                printError("无效选项，请重新选择");
        }
    } while (choice != 0);
}

/*====================================================================================================
 *
 *  内置自测框架
 *
 *====================================================================================================*/

static int g_testPassed = 0;
static int g_testFailed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (cond) { \
        printf(COLOR_SUCCESS "  [PASS] " ANSI_RESET "%s\n", msg); \
        g_testPassed++; \
    } else { \
        printf(COLOR_ERROR "  [FAIL] " ANSI_RESET "%s (line %d)\n", msg, __LINE__); \
        g_testFailed++; \
    } \
} while(0)

#define TEST_STATUS(st, expected, msg) TEST_ASSERT((st) == (expected), msg)

static void runSelfTest(void)
{
    printf(COLOR_HEADER "\n╔══════════════════════════════════════════╗\n");
    printf("║          运行内置自测 V2.0                ║\n");
    printf("╚══════════════════════════════════════════╝\n\n" ANSI_RESET);

    studentArrayFree(&g_students);
    clubArrayFree(&g_clubs);
    memberArrayFree(&g_members);
    studentArrayInit(&g_students);
    clubArrayInit(&g_clubs);
    memberArrayInit(&g_members);

    remove(DATA_FILE);

    ClubStatus st;

    /* TC01: 添加学生 - 正常 */
    st = addStudent("张三", "2024001", 20, "男", "计算机学院", "软件工程", "2024级");
    TEST_STATUS(st, CLUB_OK, "TC01: 添加学生成功");

    /* TC02: 添加学生 - 重复学号 */
    st = addStudent("张三", "2024001", 20, "男", "计算机学院", "软件工程", "2024级");
    TEST_STATUS(st, CLUB_ERR_DUPLICATE, "TC02: 重复学号返回错误");

    /* TC03: 添加学生 - 无效输入 */
    st = addStudent("", "2024002", 20, "男", "计算机学院", "软件工程", "2024级");
    TEST_STATUS(st, CLUB_ERR_INVALID_INPUT, "TC03: 空姓名返回错误");

    /* TC04: 添加学生 - 年龄无效 */
    st = addStudent("李四", "2024002", 200, "男", "计算机学院", "软件工程", "2024级");
    TEST_STATUS(st, CLUB_ERR_INVALID_INPUT, "TC04: 无效年龄返回错误");

    /* TC05: 查询学生 */
    int idx = studentArrayFindById(&g_students, "2024001");
    TEST_ASSERT(idx >= 0, "TC05: 按学号查询学生成功");

    /* TC06: 添加社团 */
    st = addClub("C001", "动漫社", "王五", "教学楼301", "2020-09-01", "ACG爱好者社团");
    TEST_STATUS(st, CLUB_OK, "TC06: 添加社团成功");

    /* TC07: 重复社团 */
    st = addClub("C001", "动漫社", "王五", "教学楼301", "2020-09-01", "ACG爱好者社团");
    TEST_STATUS(st, CLUB_ERR_DUPLICATE, "TC07: 重复社团编号返回错误");

    /* TC08: 查询社团 */
    int cidx = clubArrayFindById(&g_clubs, "C001");
    TEST_ASSERT(cidx >= 0, "TC08: 按编号查询社团成功");

    /* TC09: 添加成员 */
    st = addMember("2024001", "C001", "2024-09-01", "社员", "新成员");
    TEST_STATUS(st, CLUB_OK, "TC09: 添加成员成功");

    /* TC10: 重复加入社团 */
    st = addMember("2024001", "C001", "2024-09-01", "社员", "新成员");
    TEST_STATUS(st, CLUB_ERR_DUPLICATE, "TC10: 重复加入社团返回错误");

    /* TC11: 添加成员 - 学生不存在 */
    st = addMember("9999999", "C001", "2024-09-01", "社员", "");
    TEST_STATUS(st, CLUB_ERR_NOT_FOUND, "TC11: 不存在的学生返回错误");

    /* TC12: 添加成员 - 社团不存在 */
    st = addMember("2024001", "X999", "2024-09-01", "社员", "");
    TEST_STATUS(st, CLUB_ERR_NOT_FOUND, "TC12: 不存在的社团返回错误");

    /* TC13: 添加更多数据用于I/O测试 */
    addStudent("李四", "2024002", 19, "女", "文学院", "汉语言", "2024级");
    addStudent("王五", "2024003", 21, "男", "计算机学院", "计算机科学", "2023级");
    addClub("C002", "篮球社", "赵六", "体育馆", "2019-03-15", "篮球运动");
    addClub("C003", "音乐社", "钱七", "艺术楼", "2021-06-20", "音乐交流");
    addMember("2024002", "C001", "2024-09-02", "成员", "");
    addMember("2024002", "C002", "2024-09-03", "成员", "");
    addMember("2024003", "C003", "2024-09-04", "社长", " founding member");
    TEST_ASSERT(g_students.count == 3 && g_clubs.count == 3 && g_members.count == 4,
                "TC13: 批量添加数据后计数正确");

    /* TC14: 保存文件 */
    st = saveAllData();
    TEST_STATUS(st, CLUB_OK, "TC14: 保存数据到文件成功");

    /* TC15: 清空并重新加载 */
    studentArrayFree(&g_students);
    clubArrayFree(&g_clubs);
    memberArrayFree(&g_members);
    studentArrayInit(&g_students);
    clubArrayInit(&g_clubs);
    memberArrayInit(&g_members);
    st = loadAllData();
    TEST_STATUS(st, CLUB_OK, "TC15: 从文件加载数据成功");

    /* TC16: 验证加载后数据完整性 */
    TEST_ASSERT(g_students.count == 3, "TC16a: 加载后学生数量正确(3)");
    TEST_ASSERT(g_clubs.count == 3, "TC16b: 加载后社团数量正确(3)");
    TEST_ASSERT(g_members.count == 4, "TC16c: 加载后成员数量正确(4)");

    /* TC17: 验证加载的数据内容 */
    idx = studentArrayFindById(&g_students, "2024001");
    TEST_ASSERT(idx >= 0 && strcmp(g_students.items[idx].name, "张三") == 0,
                "TC17: 加载后学生姓名正确");

    /* TC18: 删除学生（连带删除成员记录） */
    size_t memberBefore = g_members.count;
    st = deleteStudent("2024001");
    TEST_STATUS(st, CLUB_OK, "TC18a: 删除学生成功");
    TEST_ASSERT(g_members.count < memberBefore, "TC18b: 删除学生时级联删除成员记录");

    /* TC19: 删除社团（连带删除成员记录） */
    memberBefore = g_members.count;
    st = deleteClub("C001");
    TEST_STATUS(st, CLUB_OK, "TC19a: 删除社团成功");
    TEST_ASSERT(g_members.count < memberBefore, "TC19b: 删除社团时级联删除成员记录");

    /* TC20: 删除不存在的记录 */
    st = deleteStudent("9999999");
    TEST_STATUS(st, CLUB_ERR_NOT_FOUND, "TC20: 删除不存在的学生返回错误");

    /* TC21: 动态数组扩容测试 */
    for (int i = 0; i < 20; i++) {
        char id[32], name[32];
        sprintf(id, "TEST%d", i);
        sprintf(name, "测试学生%d", i);
        addStudent(name, id, 18 + i % 5, "男", "测试学院", "测试专业", "2024级");
    }
    TEST_ASSERT(g_students.count == 2 + 20, "TC21: 动态数组扩容添加20个学生成功");

    /* TC22: Magic Header验证 */
    FILE *fp = fopen(DATA_FILE, "rb");
    TEST_ASSERT(fp != NULL, "TC22a: 数据文件存在");
    if (fp) {
        char mg[MAGIC_LEN + 1] = {0};
        fread(mg, 1, MAGIC_LEN, fp);
        fclose(fp);
        TEST_ASSERT(strcmp(mg, MAGIC_HEADER) == 0, "TC22b: Magic Header验证正确");
    }

    /* 清理测试数据 */
    studentArrayFree(&g_students);
    clubArrayFree(&g_clubs);
    memberArrayFree(&g_members);
    studentArrayInit(&g_students);
    clubArrayInit(&g_clubs);
    memberArrayInit(&g_members);
    remove(DATA_FILE);

    printSeparator();
    printf(COLOR_INFO "\n  测试完成: " ANSI_RESET);
    printf(COLOR_SUCCESS "%d 通过" ANSI_RESET ", " COLOR_ERROR "%d 失败" ANSI_RESET "\n\n",
           g_testPassed, g_testFailed);

    if (g_testFailed == 0) {
        printf(COLOR_SUCCESS "  所有测试通过！✓\n\n" ANSI_RESET);
    } else {
        printf(COLOR_ERROR "  有 %d 个测试失败！\n\n" ANSI_RESET, g_testFailed);
    }
}

/*====================================================================================================
 *
 *  主函数 - CLI参数解析
 *
 *====================================================================================================*/

int main(int argc, char *argv[])
{
    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            SetConsoleOutputCP(65001);
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD mode = 0;
            GetConsoleMode(hOut, &mode);
            SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
            showHelp();
            return 0;
        }
        if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0) {
            SetConsoleOutputCP(65001);
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD mode = 0;
            GetConsoleMode(hOut, &mode);
            SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
            runSelfTest();
            return g_testFailed > 0 ? 1 : 0;
        }
    }

    if (initSystem() != CLUB_OK) {
        fprintf(stderr, "系统初始化失败\n");
        return 1;
    }

    mainMenu();
    cleanupSystem();
    return 0;
}
