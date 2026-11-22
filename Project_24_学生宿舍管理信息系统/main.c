/* ============================================================================
 *  学生宿舍管理信息系统 V2.0
 *  Student Dormitory Management System V2.0
 *
 *  版本特性：
 *    - UTF-8 BOM + Windows控制台UTF-8支持
 *    - ANSI彩色终端输出
 *    - 动态数组存储（宿舍+学生）
 *    - 统一错误码枚举 DormStatus
 *    - Magic header "DORMV2" 文件持久化
 *    - CLI参数 (-h/--help, -t/--test)
 *    - 内置自测（15+项）
 *    - 交互菜单模式
 *    - C99标准
 *
 *  编译命令：gcc -std=c99 -Wall -Wextra -O2 -o dorm.exe main.c
 * ========================================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* ============================================================================
 *  ANSI 颜色定义
 * ========================================================================= */
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"
#define COLOR_BOLD    "\033[1m"

/* ============================================================================
 *  常量定义
 * ========================================================================= */
#define MAX_STR       64
#define MAX_NAME      32
#define MAGIC_HEADER  "DORMV2"
#define MAGIC_LEN     6
#define DATA_FILE     "dormv2.dat"
#define INIT_CAP      8

/* ============================================================================
 *  统一错误码枚举
 * ========================================================================= */
typedef enum {
    DORM_OK = 0,
    DORM_ERR_MEMORY,
    DORM_ERR_NOT_FOUND,
    DORM_ERR_DUPLICATE,
    DORM_ERR_FULL,
    DORM_ERR_EMPTY,
    DORM_ERR_FILE_IO,
    DORM_ERR_INVALID_PARAM,
    DORM_ERR_ALREADY_CHECKED_IN,
    DORM_ERR_NOT_CHECKED_IN,
    DORM_ERR_ALREADY_PAID,
    DORM_ERR_MAGIC_MISMATCH,
    DORM_ERR_VERSION
} DormStatus;

/* ============================================================================
 *  数据结构定义
 * ========================================================================= */

/* 宿舍信息 */
typedef struct {
    char id[MAX_STR];
    char building[MAX_STR];
    int  floor;
    int  capacity;
    int  occupied;
    char type[MAX_STR];
} Room;

/* 学生信息 */
typedef struct {
    char stu_id[MAX_STR];
    char name[MAX_NAME];
    char room_id[MAX_STR];
    char phone[MAX_STR];
    char check_in[MAX_STR];
    char check_out[MAX_STR];
    double fee;
    int  fee_paid;
} Student;

/* 动态数组容器 */
typedef struct {
    Room *data;
    int   count;
    int   capacity;
} RoomArray;

typedef struct {
    Student *data;
    int      count;
    int      capacity;
} StudentArray;

/* ============================================================================
 *  全局数据
 * ========================================================================= */
static RoomArray    g_rooms    = {NULL, 0, 0};
static StudentArray g_students = {NULL, 0, 0};
static int          g_test_mode = 0;
static int          g_test_passed = 0;
static int          g_test_failed = 0;

/* ============================================================================
 *  控制台初始化
 * ========================================================================= */
static void console_init(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

/* ============================================================================
 *  动态数组操作
 * ========================================================================= */
static DormStatus room_array_init(RoomArray *arr)
{
    arr->data = (Room *)calloc(INIT_CAP, sizeof(Room));
    if (!arr->data) return DORM_ERR_MEMORY;
    arr->count = 0;
    arr->capacity = INIT_CAP;
    return DORM_OK;
}

static DormStatus room_array_grow(RoomArray *arr)
{
    int new_cap = arr->capacity * 2;
    Room *new_data = (Room *)realloc(arr->data, (size_t)new_cap * sizeof(Room));
    if (!new_data) return DORM_ERR_MEMORY;
    arr->data = new_data;
    arr->capacity = new_cap;
    return DORM_OK;
}

static DormStatus room_array_add(RoomArray *arr, const Room *room)
{
    if (arr->count >= arr->capacity) {
        DormStatus s = room_array_grow(arr);
        if (s != DORM_OK) return s;
    }
    arr->data[arr->count++] = *room;
    return DORM_OK;
}

static DormStatus student_array_init(StudentArray *arr)
{
    arr->data = (Student *)calloc(INIT_CAP, sizeof(Student));
    if (!arr->data) return DORM_ERR_MEMORY;
    arr->count = 0;
    arr->capacity = INIT_CAP;
    return DORM_OK;
}

static DormStatus student_array_grow(StudentArray *arr)
{
    int new_cap = arr->capacity * 2;
    Student *new_data = (Student *)realloc(arr->data, (size_t)new_cap * sizeof(Student));
    if (!new_data) return DORM_ERR_MEMORY;
    arr->data = new_data;
    arr->capacity = new_cap;
    return DORM_OK;
}

static DormStatus student_array_add(StudentArray *arr, const Student *stu)
{
    if (arr->count >= arr->capacity) {
        DormStatus s = student_array_grow(arr);
        if (s != DORM_OK) return s;
    }
    arr->data[arr->count++] = *stu;
    return DORM_OK;
}

static void room_array_free(RoomArray *arr)
{
    free(arr->data);
    arr->data = NULL;
    arr->count = 0;
    arr->capacity = 0;
}

static void student_array_free(StudentArray *arr)
{
    free(arr->data);
    arr->data = NULL;
    arr->count = 0;
    arr->capacity = 0;
}

/* ============================================================================
 *  工具函数
 * ========================================================================= */
static void get_today(char *buf, int size)
{
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buf, (size_t)size, "%Y-%m-%d", tm_info);
}

static int find_room_index(const char *id)
{
    for (int i = 0; i < g_rooms.count; i++) {
        if (strcmp(g_rooms.data[i].id, id) == 0) return i;
    }
    return -1;
}

static int find_student_index(const char *stu_id)
{
    for (int i = 0; i < g_students.count; i++) {
        if (strcmp(g_students.data[i].stu_id, stu_id) == 0) return i;
    }
    return -1;
}

static int is_student_checked_in(const char *stu_id)
{
    int idx = find_student_index(stu_id);
    if (idx < 0) return 0;
    return g_students.data[idx].check_out[0] == '\0';
}

/* ============================================================================
 *  核心业务逻辑
 * ========================================================================= */

/* 添加宿舍 */
static DormStatus dorm_add_room(const Room *room)
{
    if (!room || room->id[0] == '\0' || room->capacity <= 0) {
        return DORM_ERR_INVALID_PARAM;
    }
    if (find_room_index(room->id) >= 0) {
        return DORM_ERR_DUPLICATE;
    }
    Room r = *room;
    r.occupied = 0;
    return room_array_add(&g_rooms, &r);
}

/* 删除宿舍 */
static DormStatus dorm_delete_room(const char *id)
{
    int idx = find_room_index(id);
    if (idx < 0) return DORM_ERR_NOT_FOUND;
    if (g_rooms.data[idx].occupied > 0) return DORM_ERR_FULL;
    for (int i = idx; i < g_rooms.count - 1; i++) {
        g_rooms.data[i] = g_rooms.data[i + 1];
    }
    g_rooms.count--;
    return DORM_OK;
}

/* 学生入住 */
static DormStatus dorm_check_in(const Student *stu)
{
    if (!stu || stu->stu_id[0] == '\0') return DORM_ERR_INVALID_PARAM;
    int ridx = find_room_index(stu->room_id);
    if (ridx < 0) return DORM_ERR_NOT_FOUND;
    if (g_rooms.data[ridx].occupied >= g_rooms.data[ridx].capacity) {
        return DORM_ERR_FULL;
    }
    int sidx = find_student_index(stu->stu_id);
    if (sidx >= 0 && is_student_checked_in(stu->stu_id)) {
        return DORM_ERR_ALREADY_CHECKED_IN;
    }
    if (sidx >= 0) {
        Student *s = &g_students.data[sidx];
        snprintf(s->room_id, MAX_STR, "%s", stu->room_id);
        if (stu->name[0]) snprintf(s->name, MAX_NAME, "%s", stu->name);
        if (stu->phone[0]) snprintf(s->phone, MAX_STR, "%s", stu->phone);
        get_today(s->check_in, MAX_STR);
        s->check_out[0] = '\0';
        s->fee = stu->fee;
        s->fee_paid = 0;
    } else {
        Student s = *stu;
        get_today(s.check_in, MAX_STR);
        s.check_out[0] = '\0';
        s.fee_paid = 0;
        DormStatus st = student_array_add(&g_students, &s);
        if (st != DORM_OK) return st;
    }
    g_rooms.data[ridx].occupied++;
    return DORM_OK;
}

/* 学生退房 */
static DormStatus dorm_check_out(const char *stu_id)
{
    int sidx = find_student_index(stu_id);
    if (sidx < 0) return DORM_ERR_NOT_FOUND;
    Student *s = &g_students.data[sidx];
    if (s->check_out[0] != '\0') return DORM_ERR_NOT_CHECKED_IN;
    int ridx = find_room_index(s->room_id);
    get_today(s->check_out, MAX_STR);
    if (ridx >= 0 && g_rooms.data[ridx].occupied > 0) {
        g_rooms.data[ridx].occupied--;
    }
    return DORM_OK;
}

/* 学生缴费 */
static DormStatus dorm_pay_fee(const char *stu_id)
{
    int sidx = find_student_index(stu_id);
    if (sidx < 0) return DORM_ERR_NOT_FOUND;
    if (g_students.data[sidx].fee_paid) return DORM_ERR_ALREADY_PAID;
    g_students.data[sidx].fee_paid = 1;
    return DORM_OK;
}

/* 统计信息 */
typedef struct {
    int total_rooms;
    int total_students;
    int checked_in;
    int checked_out;
    int unpaid_fees;
    int available_beds;
} DormStats;

static DormStatus dorm_get_stats(DormStats *stats)
{
    if (!stats) return DORM_ERR_INVALID_PARAM;
    memset(stats, 0, sizeof(DormStats));
    stats->total_rooms = g_rooms.count;
    stats->total_students = g_students.count;
    for (int i = 0; i < g_students.count; i++) {
        if (g_students.data[i].check_out[0] == '\0') {
            stats->checked_in++;
        } else {
            stats->checked_out++;
        }
        if (!g_students.data[i].fee_paid && g_students.data[i].check_out[0] == '\0') {
            stats->unpaid_fees++;
        }
    }
    for (int i = 0; i < g_rooms.count; i++) {
        stats->available_beds += g_rooms.data[i].capacity - g_rooms.data[i].occupied;
    }
    return DORM_OK;
}

/* ============================================================================
 *  文件持久化 (带 Magic Header)
 * ========================================================================= */
static DormStatus dorm_save(const char *filename)
{
    FILE *fp = fopen(filename, "wb");
    if (!fp) return DORM_ERR_FILE_IO;
    fwrite(MAGIC_HEADER, 1, MAGIC_LEN, fp);
    int version = 2;
    fwrite(&version, sizeof(int), 1, fp);
    fwrite(&g_rooms.count, sizeof(int), 1, fp);
    fwrite(g_rooms.data, sizeof(Room), (size_t)g_rooms.count, fp);
    fwrite(&g_students.count, sizeof(int), 1, fp);
    fwrite(g_students.data, sizeof(Student), (size_t)g_students.count, fp);
    fclose(fp);
    return DORM_OK;
}

static DormStatus dorm_load(const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp) return DORM_ERR_FILE_IO;
    char magic[MAGIC_LEN + 1] = {0};
    if (fread(magic, 1, MAGIC_LEN, fp) != MAGIC_LEN) {
        fclose(fp); return DORM_ERR_FILE_IO;
    }
    if (strcmp(magic, MAGIC_HEADER) != 0) {
        fclose(fp); return DORM_ERR_MAGIC_MISMATCH;
    }
    int version;
    if (fread(&version, sizeof(int), 1, fp) != 1) {
        fclose(fp); return DORM_ERR_FILE_IO;
    }
    if (version != 2) {
        fclose(fp); return DORM_ERR_VERSION;
    }
    room_array_free(&g_rooms);
    student_array_free(&g_students);
    room_array_init(&g_rooms);
    student_array_init(&g_students);
    int count;
    fread(&count, sizeof(int), 1, fp);
    for (int i = 0; i < count; i++) {
        Room r;
        fread(&r, sizeof(Room), 1, fp);
        room_array_add(&g_rooms, &r);
    }
    fread(&count, sizeof(int), 1, fp);
    for (int i = 0; i < count; i++) {
        Student s;
        fread(&s, sizeof(Student), 1, fp);
        student_array_add(&g_students, &s);
    }
    fclose(fp);
    return DORM_OK;
}

/* ============================================================================
 *  显示/UI 函数
 * ========================================================================= */
static void print_line(char ch, int len)
{
    for (int i = 0; i < len; i++) putchar(ch);
    putchar('\n');
}

static void print_title(const char *title)
{
    printf(COLOR_CYAN);
    print_line('=', 60);
    printf("%s%s%s\n", COLOR_BOLD, title, COLOR_RESET);
    printf(COLOR_CYAN);
    print_line('=', 60);
    printf(COLOR_RESET);
}

static void print_error(const char *msg)
{
    printf("%s[错误]%s %s\n", COLOR_RED, COLOR_RESET, msg);
}

static void print_success(const char *msg)
{
    printf("%s[成功]%s %s\n", COLOR_GREEN, COLOR_RESET, msg);
}

static void print_info(const char *msg)
{
    printf("%s[信息]%s %s\n", COLOR_YELLOW, COLOR_RESET, msg);
}

static void read_line(const char *prompt, char *buf, int size)
{
    printf("%s%s%s", COLOR_CYAN, prompt, COLOR_RESET);
    if (fgets(buf, size, stdin)) {
        int len = (int)strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    }
}

static void pause_screen(void)
{
    printf("\n%s按 Enter 继续...%s", COLOR_YELLOW, COLOR_RESET);
    getchar();
}

static void show_rooms(void)
{
    print_title("宿舍信息列表");
    if (g_rooms.count == 0) {
        print_info("暂无宿舍信息");
        return;
    }
    printf("  %-8s %-10s %-6s %-8s %-8s %-10s\n",
           "房号", "楼栋", "楼层", "总床位", "已入住", "类型");
    print_line('-', 60);
    for (int i = 0; i < g_rooms.count; i++) {
        Room *r = &g_rooms.data[i];
        printf("  %-8s %-10s %-6d %-8d %-8d %-10s\n",
               r->id, r->building, r->floor, r->capacity, r->occupied, r->type);
    }
    print_line('-', 60);
    printf("  共 %d 间宿舍\n", g_rooms.count);
}

static void show_students(void)
{
    print_title("学生住宿信息");
    if (g_students.count == 0) {
        print_info("暂无学生信息");
        return;
    }
    printf("  %-12s %-10s %-8s %-12s %-12s %-8s %-6s\n",
           "学号", "姓名", "房号", "入住日期", "退房日期", "费用", "状态");
    print_line('-', 72);
    for (int i = 0; i < g_students.count; i++) {
        Student *s = &g_students.data[i];
        printf("  %-12s %-10s %-8s %-12s %-12s %-8.0f %-6s\n",
               s->stu_id, s->name, s->room_id,
               s->check_in,
               s->check_out[0] ? s->check_out : "在住",
               s->fee,
               s->fee_paid ? "已缴" : "未缴");
    }
    print_line('-', 72);
    printf("  共 %d 条记录\n", g_students.count);
}

static void show_stats(void)
{
    DormStats stats;
    dorm_get_stats(&stats);
    print_title("系统统计信息");
    printf("  %-20s: %d\n", "宿舍总数", stats.total_rooms);
    printf("  %-20s: %d\n", "学生记录总数", stats.total_students);
    printf("  %-20s: %d\n", "当前在住人数", stats.checked_in);
    printf("  %-20s: %d\n", "已退房人数", stats.checked_out);
    printf("  %-20s: %d\n", "未缴费人数", stats.unpaid_fees);
    printf("  %-20s: %d\n", "剩余床位", stats.available_beds);
}

/* ============================================================================
 *  交互菜单
 * ========================================================================= */
static void menu_add_room(void)
{
    Room r;
    memset(&r, 0, sizeof(r));
    print_title("添加宿舍");
    read_line("  宿舍号: ", r.id, MAX_STR);
    read_line("  楼栋: ", r.building, MAX_STR);
    printf("%s  楼层: %s", COLOR_CYAN, COLOR_RESET);
    scanf("%d", &r.floor);
    printf("%s  床位数: %s", COLOR_CYAN, COLOR_RESET);
    scanf("%d", &r.capacity);
    getchar();
    read_line("  类型(男生/女生): ", r.type, MAX_STR);
    DormStatus s = dorm_add_room(&r);
    switch (s) {
        case DORM_OK: print_success("宿舍添加成功"); break;
        case DORM_ERR_DUPLICATE: print_error("宿舍号已存在"); break;
        case DORM_ERR_MEMORY: print_error("内存分配失败"); break;
        default: print_error("参数错误"); break;
    }
    dorm_save(DATA_FILE);
}

static void menu_delete_room(void)
{
    char id[MAX_STR];
    print_title("删除宿舍");
    read_line("  请输入要删除的宿舍号: ", id, MAX_STR);
    DormStatus s = dorm_delete_room(id);
    switch (s) {
        case DORM_OK: print_success("宿舍删除成功"); break;
        case DORM_ERR_NOT_FOUND: print_error("未找到该宿舍"); break;
        case DORM_ERR_FULL: print_error("宿舍还有学生入住，无法删除"); break;
        default: print_error("删除失败"); break;
    }
    dorm_save(DATA_FILE);
}

static void menu_check_in(void)
{
    Student s;
    memset(&s, 0, sizeof(s));
    print_title("学生入住登记");
    read_line("  宿舍号: ", s.room_id, MAX_STR);
    read_line("  学号: ", s.stu_id, MAX_STR);
    read_line("  姓名: ", s.name, MAX_NAME);
    read_line("  电话: ", s.phone, MAX_STR);
    printf("%s  住宿费: %s", COLOR_CYAN, COLOR_RESET);
    scanf("%lf", &s.fee);
    getchar();
    DormStatus st = dorm_check_in(&s);
    switch (st) {
        case DORM_OK: print_success("入住登记成功"); break;
        case DORM_ERR_NOT_FOUND: print_error("宿舍不存在"); break;
        case DORM_ERR_FULL: print_error("宿舍床位已满"); break;
        case DORM_ERR_ALREADY_CHECKED_IN: print_error("该学生已入住"); break;
        case DORM_ERR_MEMORY: print_error("内存分配失败"); break;
        default: print_error("入住失败"); break;
    }
    dorm_save(DATA_FILE);
}

static void menu_check_out(void)
{
    char id[MAX_STR];
    print_title("学生退房");
    read_line("  请输入学号: ", id, MAX_STR);
    DormStatus s = dorm_check_out(id);
    switch (s) {
        case DORM_OK: print_success("退房成功"); break;
        case DORM_ERR_NOT_FOUND: print_error("未找到该学生"); break;
        case DORM_ERR_NOT_CHECKED_IN: print_error("该学生未在住或已退房"); break;
        default: print_error("退房失败"); break;
    }
    dorm_save(DATA_FILE);
}

static void menu_pay_fee(void)
{
    char id[MAX_STR];
    print_title("学生缴费");
    read_line("  请输入学号: ", id, MAX_STR);
    DormStatus s = dorm_pay_fee(id);
    switch (s) {
        case DORM_OK: print_success("缴费成功"); break;
        case DORM_ERR_NOT_FOUND: print_error("未找到该学生"); break;
        case DORM_ERR_ALREADY_PAID: print_error("该学生已缴费"); break;
        default: print_error("缴费失败"); break;
    }
    dorm_save(DATA_FILE);
}

static void show_help(void)
{
    printf("%s", COLOR_CYAN);
    print_line('=', 60);
    printf("%s学生宿舍管理信息系统 V2.0%s\n", COLOR_BOLD, COLOR_RESET);
    printf("%s", COLOR_CYAN);
    print_line('=', 60);
    printf("%s", COLOR_RESET);
    printf("用法: dorm.exe [选项]\n\n");
    printf("选项:\n");
    printf("  -h, --help     显示帮助信息\n");
    printf("  -t, --test     运行内置自测\n");
    printf("  (无参数)       启动交互菜单模式\n\n");
    printf("数据文件: %s\n", DATA_FILE);
}

static void main_menu(void)
{
    int ch;
    dorm_load(DATA_FILE);
    do {
        print_title("学生宿舍管理信息系统 V2.0");
        printf("  1. 查看宿舍列表\n");
        printf("  2. 添加宿舍\n");
        printf("  3. 删除宿舍\n");
        printf("  4. 查看学生列表\n");
        printf("  5. 学生入住\n");
        printf("  6. 学生退房\n");
        printf("  7. 学生缴费\n");
        printf("  8. 统计信息\n");
        printf("  0. 退出系统\n");
        print_line('-', 60);
        printf("%s  请选择: %s", COLOR_CYAN, COLOR_RESET);
        if (scanf("%d", &ch) != 1) {
            while (getchar() != '\n');
            ch = -1;
        }
        getchar();
        switch (ch) {
            case 1: show_rooms(); break;
            case 2: menu_add_room(); break;
            case 3: menu_delete_room(); break;
            case 4: show_students(); break;
            case 5: menu_check_in(); break;
            case 6: menu_check_out(); break;
            case 7: menu_pay_fee(); break;
            case 8: show_stats(); break;
            case 0: print_success("感谢使用，再见！"); dorm_save(DATA_FILE); break;
            default: print_error("无效选项"); break;
        }
        if (ch != 0) pause_screen();
    } while (ch != 0);
}

/* ============================================================================
 *  内置自测模块
 * ========================================================================= */
#define TEST_ASSERT(cond, msg) do { \
    if (cond) { \
        printf("%s[PASS]%s %s\n", COLOR_GREEN, COLOR_RESET, msg); \
        g_test_passed++; \
    } else { \
        printf("%s[FAIL]%s %s (line %d)\n", COLOR_RED, COLOR_RESET, msg, __LINE__); \
        g_test_failed++; \
    } \
} while(0)

static void run_self_test(void)
{
    g_test_mode = 1;
    g_test_passed = 0;
    g_test_failed = 0;

    printf("%s", COLOR_CYAN);
    print_line('=', 60);
    printf("%s学生宿舍管理系统 V2.0 - 内置自测%s\n", COLOR_BOLD, COLOR_RESET);
    printf("%s", COLOR_CYAN);
    print_line('=', 60);
    printf("%s", COLOR_RESET);

    room_array_free(&g_rooms);
    student_array_free(&g_students);
    room_array_init(&g_rooms);
    student_array_init(&g_students);

    /* T01: 初始状态为空 */
    TEST_ASSERT(g_rooms.count == 0, "T01: 初始宿舍数为0");
    TEST_ASSERT(g_students.count == 0, "T02: 初始学生数为0");

    /* T03: 添加宿舍 */
    Room r1 = {"A101", "东一栋", 1, 4, 0, "男生"};
    TEST_ASSERT(dorm_add_room(&r1) == DORM_OK, "T03: 添加宿舍A101成功");

    /* T04: 重复添加 */
    TEST_ASSERT(dorm_add_room(&r1) == DORM_ERR_DUPLICATE, "T04: 重复添加宿舍A101失败");

    /* T05: 添加第二个宿舍 */
    Room r2 = {"A102", "东一栋", 1, 4, 0, "男生"};
    TEST_ASSERT(dorm_add_room(&r2) == DORM_OK, "T05: 添加宿舍A102成功");

    /* T06: 查找宿舍 */
    TEST_ASSERT(find_room_index("A101") == 0, "T06: 查找宿舍A101成功");
    TEST_ASSERT(find_room_index("B999") == -1, "T07: 查找不存在宿舍返回-1");

    /* T08: 学生入住 */
    Student s1 = {"2024001", "张三", "A101", "138******01", "", "", 1200.0, 0};
    TEST_ASSERT(dorm_check_in(&s1) == DORM_OK, "T08: 学生张三入住成功");

    /* T09: 重复入住 */
    Student s1b = {"2024001", "张三", "A102", "138******01", "", "", 1200.0, 0};
    TEST_ASSERT(dorm_check_in(&s1b) == DORM_ERR_ALREADY_CHECKED_IN, "T09: 重复入住被拒绝");

    /* T10: 入住不存在的宿舍 */
    Student s2 = {"2024002", "李四", "B999", "138******02", "", "", 1200.0, 0};
    TEST_ASSERT(dorm_check_in(&s2) == DORM_ERR_NOT_FOUND, "T10: 入住不存在宿舍失败");

    /* T11: 宿舍入住人数正确 */
    TEST_ASSERT(g_rooms.data[find_room_index("A101")].occupied == 1, "T11: A101入住人数为1");

    /* T12: 入住满员验证 */
    for (int i = 0; i < 3; i++) {
        char sid[32], name[32], phone[32];
        sprintf(sid, "2024%03d", i + 3);
        sprintf(name, "学生%d", i + 3);
        sprintf(phone, "13800000%02d", i + 3);
        Student s = {"", "", "A101", "", "", "", 1200.0, 0};
        strcpy(s.stu_id, sid);
        strcpy(s.name, name);
        strcpy(s.phone, phone);
        dorm_check_in(&s);
    }
    Student s5 = {"2024006", "赵六", "A101", "138******06", "", "", 1200.0, 0};
    TEST_ASSERT(dorm_check_in(&s5) == DORM_ERR_FULL, "T12: 宿舍满员拒绝入住");

    /* T13: 学生缴费 */
    TEST_ASSERT(dorm_pay_fee("2024003") == DORM_OK, "T13: 学生缴费成功");
    TEST_ASSERT(dorm_pay_fee("2024003") == DORM_ERR_ALREADY_PAID, "T14: 重复缴费被拒绝");

    /* T15: 学生退房 */
    TEST_ASSERT(dorm_check_out("2024001") == DORM_OK, "T15: 张三退房成功");
    TEST_ASSERT(dorm_check_out("2024001") == DORM_ERR_NOT_CHECKED_IN, "T16: 重复退房被拒绝");
    TEST_ASSERT(dorm_check_out("9999999") == DORM_ERR_NOT_FOUND, "T17: 不存在学生退房失败");

    /* T18: 删除有人的宿舍失败 */
    TEST_ASSERT(dorm_delete_room("A101") == DORM_ERR_FULL, "T18: 删除有人宿舍失败");

    /* T19: 文件保存 */
    TEST_ASSERT(dorm_save("test_dormv2.dat") == DORM_OK, "T19: 文件保存成功");

    /* T20: 文件加载 */
    int old_rooms = g_rooms.count;
    int old_stus = g_students.count;
    TEST_ASSERT(dorm_load("test_dormv2.dat") == DORM_OK, "T20: Magic header文件加载成功");
    TEST_ASSERT(g_rooms.count == old_rooms, "T21: 加载后宿舍数一致");
    TEST_ASSERT(g_students.count == old_stus, "T22: 加载后学生数一致");

    /* T23: 统计信息 */
    DormStats stats;
    dorm_get_stats(&stats);
    TEST_ASSERT(stats.total_rooms == 2, "T23: 统计-宿舍总数正确");
    TEST_ASSERT(stats.checked_in >= 3, "T24: 统计-在住人数正确");

    /* T25: 删除空宿舍 */
    TEST_ASSERT(dorm_delete_room("A102") == DORM_OK, "T25: 删除空宿舍A102成功");

    /* T26: 错误magic验证 */
    FILE *fp = fopen("test_bad.dat", "wb");
    fprintf(fp, "BADMAG");
    fclose(fp);
    TEST_ASSERT(dorm_load("test_bad.dat") == DORM_ERR_MAGIC_MISMATCH, "T26: 错误Magic header被拒绝");

    /* T27: 参数验证 */
    Room bad_room = {"", "", 0, 0, 0, ""};
    TEST_ASSERT(dorm_add_room(&bad_room) == DORM_ERR_INVALID_PARAM, "T27: 无效参数被拒绝");

    /* T28: 删除测试文件 */
    remove("test_dormv2.dat");
    remove("test_bad.dat");

    /* 清理 */
    room_array_free(&g_rooms);
    student_array_free(&g_students);

    printf("\n");
    print_line('=', 60);
    printf("%s测试结果: %s%d 通过, %s%d 失败, %s共 %d 项%s\n",
           COLOR_BOLD,
           COLOR_GREEN, g_test_passed,
           COLOR_RED, g_test_failed,
           COLOR_CYAN, g_test_passed + g_test_failed, COLOR_RESET);
    print_line('=', 60);

    g_test_mode = 0;
    exit(g_test_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

/* ============================================================================
 *  程序入口
 * ========================================================================= */
int main(int argc, char *argv[])
{
    console_init();

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            show_help();
            return EXIT_SUCCESS;
        }
        if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0) {
            run_self_test();
            return EXIT_SUCCESS;
        }
    }

    room_array_init(&g_rooms);
    student_array_init(&g_students);
    main_menu();
    room_array_free(&g_rooms);
    student_array_free(&g_students);

    return EXIT_SUCCESS;
}
