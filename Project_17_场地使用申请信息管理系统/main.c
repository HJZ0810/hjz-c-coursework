/*
 * ╔════════════════════════════════════════════════════════════════════╗
 * ║                                                                    ║
 * ║               场地使用申请信息管理系统 V2.0                        ║
 * ║               Venue Usage Application Management System            ║
 * ║                                                                    ║
 * ╠════════════════════════════════════════════════════════════════════╣
 * ║                                                                    ║
 * ║  项目名称：场地使用申请信息管理系统                                ║
 * ║  版    本：v2.0                                                    ║
 * ║  编    写：数据库课程                                              ║
 * ║  学    号：424123888888                                            ║
 * ║  班    级：计算机科学与技术A班                                      ║
 * ║  作    者：HuiJunzhuo                                              ║
 * ║  学年学期：2025-2026学年第二学期算法课                              ║
 * ║                                                                    ║
 * ╠════════════════════════════════════════════════════════════════════╣
 * ║  V2.0 新特性：                                                     ║
 * ║      1. UTF-8 BOM编码 + SetConsoleOutputCP(65001)控制台支持        ║
 * ║      2. ANSI彩色终端输出                                           ║
 * ║      3. CLI参数支持 (-h帮助 / -t或--test自测)                      ║
 * ║      4. 动态数组实现（场地+申请）                                   ║
 * ║      5. 统一错误码枚举 VenueStatus                                 ║
 * ║      6. "VENUV2" Magic Header二进制文件持久化                      ║
 * ║      7. 内置自测框架（至少15项测试用例）                            ║
 * ║      8. 申请审批流程（待审批/已批准/已拒绝）                        ║
 * ║      9. 统计报表功能                                               ║
 * ║     10. 完整中文注释                                               ║
 * ╚════════════════════════════════════════════════════════════════════╝
 */

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

/* ======================== ANSI 彩色终端 ======================== */

#define COLOR_RESET     "\x1b[0m"
#define COLOR_RED       "\x1b[31m"
#define COLOR_GREEN     "\x1b[32m"
#define COLOR_YELLOW    "\x1b[33m"
#define COLOR_BLUE      "\x1b[34m"
#define COLOR_MAGENTA   "\x1b[35m"
#define COLOR_CYAN      "\x1b[36m"
#define COLOR_WHITE     "\x1b[37m"
#define COLOR_BOLD      "\x1b[1m"
#define COLOR_DIM       "\x1b[2m"

#define C_RED(s)     COLOR_RED s COLOR_RESET
#define C_GREEN(s)   COLOR_GREEN s COLOR_RESET
#define C_YELLOW(s)  COLOR_YELLOW s COLOR_RESET
#define C_BLUE(s)    COLOR_BLUE s COLOR_RESET
#define C_MAGENTA(s) COLOR_MAGENTA s COLOR_RESET
#define C_CYAN(s)    COLOR_CYAN s COLOR_RESET
#define C_BOLD(s)    COLOR_BOLD s COLOR_RESET
#define C_DIM(s)     COLOR_DIM s COLOR_RESET

/* ======================== 常量定义 ======================== */

#define MAX_STR         128
#define MAGIC_HEADER    "VENUV2"
#define MAGIC_LEN       6
#define DATA_FILE       "venue_v2.dat"
#define INIT_CAPACITY   8
#define VERSION         "2.0"

/* ======================== 统一错误码枚举 ======================== */

typedef enum {
    VENUE_OK = 0,
    VENUE_ERR_MEMORY,
    VENUE_ERR_NOT_FOUND,
    VENUE_ERR_DUPLICATE,
    VENUE_ERR_INVALID_PARAM,
    VENUE_ERR_FILE_OPEN,
    VENUE_ERR_FILE_READ,
    VENUE_ERR_FILE_WRITE,
    VENUE_ERR_MAGIC_MISMATCH,
    VENUE_ERR_VERSION_MISMATCH,
    VENUE_ERR_EMPTY,
    VENUE_ERR_STATUS_INVALID,
    VENUE_ERR_IO
} VenueStatus;

/* ======================== 申请状态枚举 ======================== */

typedef enum {
    APPLY_PENDING = 0,
    APPLY_APPROVED,
    APPLY_REJECTED
} ApplyStatus;

/* ======================== 数据结构定义 ======================== */

typedef struct {
    char code[MAX_STR];
    char name[MAX_STR];
    char level[MAX_STR];
    char leader[MAX_STR];
    char address[MAX_STR];
} Unit;

typedef struct {
    char code[MAX_STR];
    char address[MAX_STR];
    char category[MAX_STR];
    int  capacity;
    double price;
} Venue;

typedef struct {
    int  id;
    char unit_code[MAX_STR];
    char venue_code[MAX_STR];
    char time_str[MAX_STR];
    char reason[MAX_STR];
    char remark[MAX_STR];
    ApplyStatus status;
    char review_note[MAX_STR];
    time_t create_time;
} Apply;

/* ======================== 动态数组结构 ======================== */

typedef struct {
    Unit   *items;
    int     count;
    int     capacity;
} UnitArray;

typedef struct {
    Venue  *items;
    int     count;
    int     capacity;
} VenueArray;

typedef struct {
    Apply  *items;
    int     count;
    int     capacity;
    int     next_id;
} ApplyArray;

/* ======================== 全局数据 ======================== */

static UnitArray  g_units   = {NULL, 0, 0};
static VenueArray g_venues  = {NULL, 0, 0};
static ApplyArray g_applies = {NULL, 0, 0, 1};

/* ======================== 控制台初始化 ======================== */

static void console_init(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(hOut, &mode)) {
        SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | 0x0001);
    }
#endif
}

/* ======================== 辅助函数 ======================== */

static void read_line(char *buf, int size)
{
    if (fgets(buf, size, stdin) != NULL) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n')
            buf[len - 1] = '\0';
    } else {
        buf[0] = '\0';
    }
}

static void print_separator(void)
{
    printf(C_CYAN("------------------------------------------------------------\n"));
}

static const char *status_to_string(ApplyStatus s)
{
    switch (s) {
        case APPLY_PENDING:  return C_YELLOW("待审批");
        case APPLY_APPROVED: return C_GREEN("已批准");
        case APPLY_REJECTED: return C_RED("已拒绝");
        default:             return "未知";
    }
}

/* ======================== 动态数组操作 - Unit ======================== */

static VenueStatus unit_array_init(UnitArray *arr)
{
    arr->capacity = INIT_CAPACITY;
    arr->count = 0;
    arr->items = (Unit *)malloc(sizeof(Unit) * arr->capacity);
    if (!arr->items) return VENUE_ERR_MEMORY;
    return VENUE_OK;
}

static VenueStatus unit_array_resize(UnitArray *arr)
{
    int new_cap = arr->capacity * 2;
    Unit *new_items = (Unit *)realloc(arr->items, sizeof(Unit) * new_cap);
    if (!new_items) return VENUE_ERR_MEMORY;
    arr->items = new_items;
    arr->capacity = new_cap;
    return VENUE_OK;
}

static Unit *unit_array_find(UnitArray *arr, const char *code)
{
    int i;
    for (i = 0; i < arr->count; i++) {
        if (strcmp(arr->items[i].code, code) == 0)
            return &arr->items[i];
    }
    return NULL;
}

static VenueStatus unit_array_add(UnitArray *arr, const Unit *u)
{
    if (unit_array_find(arr, u->code))
        return VENUE_ERR_DUPLICATE;
    if (arr->count >= arr->capacity) {
        VenueStatus s = unit_array_resize(arr);
        if (s != VENUE_OK) return s;
    }
    arr->items[arr->count] = *u;
    arr->count++;
    return VENUE_OK;
}

static VenueStatus unit_array_delete(UnitArray *arr, const char *code)
{
    int i;
    for (i = 0; i < arr->count; i++) {
        if (strcmp(arr->items[i].code, code) == 0) {
            int j;
            for (j = i; j < arr->count - 1; j++)
                arr->items[j] = arr->items[j + 1];
            arr->count--;
            return VENUE_OK;
        }
    }
    return VENUE_ERR_NOT_FOUND;
}

static void unit_array_free(UnitArray *arr)
{
    if (arr->items) {
        free(arr->items);
        arr->items = NULL;
    }
    arr->count = 0;
    arr->capacity = 0;
}

/* ======================== 动态数组操作 - Venue ======================== */

static VenueStatus venue_array_init(VenueArray *arr)
{
    arr->capacity = INIT_CAPACITY;
    arr->count = 0;
    arr->items = (Venue *)malloc(sizeof(Venue) * arr->capacity);
    if (!arr->items) return VENUE_ERR_MEMORY;
    return VENUE_OK;
}

static VenueStatus venue_array_resize(VenueArray *arr)
{
    int new_cap = arr->capacity * 2;
    Venue *new_items = (Venue *)realloc(arr->items, sizeof(Venue) * new_cap);
    if (!new_items) return VENUE_ERR_MEMORY;
    arr->items = new_items;
    arr->capacity = new_cap;
    return VENUE_OK;
}

static Venue *venue_array_find(VenueArray *arr, const char *code)
{
    int i;
    for (i = 0; i < arr->count; i++) {
        if (strcmp(arr->items[i].code, code) == 0)
            return &arr->items[i];
    }
    return NULL;
}

static VenueStatus venue_array_add(VenueArray *arr, const Venue *v)
{
    if (venue_array_find(arr, v->code))
        return VENUE_ERR_DUPLICATE;
    if (arr->count >= arr->capacity) {
        VenueStatus s = venue_array_resize(arr);
        if (s != VENUE_OK) return s;
    }
    arr->items[arr->count] = *v;
    arr->count++;
    return VENUE_OK;
}

static VenueStatus venue_array_delete(VenueArray *arr, const char *code)
{
    int i;
    for (i = 0; i < arr->count; i++) {
        if (strcmp(arr->items[i].code, code) == 0) {
            int j;
            for (j = i; j < arr->count - 1; j++)
                arr->items[j] = arr->items[j + 1];
            arr->count--;
            return VENUE_OK;
        }
    }
    return VENUE_ERR_NOT_FOUND;
}

static void venue_array_free(VenueArray *arr)
{
    if (arr->items) {
        free(arr->items);
        arr->items = NULL;
    }
    arr->count = 0;
    arr->capacity = 0;
}

/* ======================== 动态数组操作 - Apply ======================== */

static VenueStatus apply_array_init(ApplyArray *arr)
{
    arr->capacity = INIT_CAPACITY;
    arr->count = 0;
    arr->next_id = 1;
    arr->items = (Apply *)malloc(sizeof(Apply) * arr->capacity);
    if (!arr->items) return VENUE_ERR_MEMORY;
    return VENUE_OK;
}

static VenueStatus apply_array_resize(ApplyArray *arr)
{
    int new_cap = arr->capacity * 2;
    Apply *new_items = (Apply *)realloc(arr->items, sizeof(Apply) * new_cap);
    if (!new_items) return VENUE_ERR_MEMORY;
    arr->items = new_items;
    arr->capacity = new_cap;
    return VENUE_OK;
}

static Apply *apply_array_find(ApplyArray *arr, int id)
{
    int i;
    for (i = 0; i < arr->count; i++) {
        if (arr->items[i].id == id)
            return &arr->items[i];
    }
    return NULL;
}

static VenueStatus apply_array_add(ApplyArray *arr, const Apply *a)
{
    if (arr->count >= arr->capacity) {
        VenueStatus s = apply_array_resize(arr);
        if (s != VENUE_OK) return s;
    }
    arr->items[arr->count] = *a;
    arr->items[arr->count].id = arr->next_id++;
    arr->items[arr->count].create_time = time(NULL);
    arr->count++;
    return VENUE_OK;
}

static void apply_array_free(ApplyArray *arr)
{
    if (arr->items) {
        free(arr->items);
        arr->items = NULL;
    }
    arr->count = 0;
    arr->capacity = 0;
    arr->next_id = 1;
}

/* ======================== 文件持久化（VENUV2二进制格式） ======================== */

static VenueStatus save_to_file(const char *filename)
{
    FILE *fp = fopen(filename, "wb");
    if (!fp) return VENUE_ERR_FILE_OPEN;

    if (fwrite(MAGIC_HEADER, 1, MAGIC_LEN, fp) != MAGIC_LEN) {
        fclose(fp);
        return VENUE_ERR_FILE_WRITE;
    }

    int ver = 2;
    if (fwrite(&ver, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return VENUE_ERR_FILE_WRITE;
    }

    if (fwrite(&g_units.count, sizeof(int), 1, fp) != 1) { fclose(fp); return VENUE_ERR_FILE_WRITE; }
    if (g_units.count > 0) {
        if (fwrite(g_units.items, sizeof(Unit), g_units.count, fp) != (size_t)g_units.count) {
            fclose(fp); return VENUE_ERR_FILE_WRITE;
        }
    }

    if (fwrite(&g_venues.count, sizeof(int), 1, fp) != 1) { fclose(fp); return VENUE_ERR_FILE_WRITE; }
    if (g_venues.count > 0) {
        if (fwrite(g_venues.items, sizeof(Venue), g_venues.count, fp) != (size_t)g_venues.count) {
            fclose(fp); return VENUE_ERR_FILE_WRITE;
        }
    }

    if (fwrite(&g_applies.count, sizeof(int), 1, fp) != 1) { fclose(fp); return VENUE_ERR_FILE_WRITE; }
    if (fwrite(&g_applies.next_id, sizeof(int), 1, fp) != 1) { fclose(fp); return VENUE_ERR_FILE_WRITE; }
    if (g_applies.count > 0) {
        if (fwrite(g_applies.items, sizeof(Apply), g_applies.count, fp) != (size_t)g_applies.count) {
            fclose(fp); return VENUE_ERR_FILE_WRITE;
        }
    }

    fclose(fp);
    return VENUE_OK;
}

static VenueStatus load_from_file(const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp) return VENUE_ERR_FILE_OPEN;

    char magic[MAGIC_LEN + 1];
    if (fread(magic, 1, MAGIC_LEN, fp) != MAGIC_LEN) {
        fclose(fp);
        return VENUE_ERR_FILE_READ;
    }
    magic[MAGIC_LEN] = '\0';
    if (strcmp(magic, MAGIC_HEADER) != 0) {
        fclose(fp);
        return VENUE_ERR_MAGIC_MISMATCH;
    }

    int ver;
    if (fread(&ver, sizeof(int), 1, fp) != 1) { fclose(fp); return VENUE_ERR_FILE_READ; }
    if (ver != 2) { fclose(fp); return VENUE_ERR_VERSION_MISMATCH; }

    int count;

    if (fread(&count, sizeof(int), 1, fp) != 1) { fclose(fp); return VENUE_ERR_FILE_READ; }
    if (count > 0) {
        unit_array_free(&g_units);
        unit_array_init(&g_units);
        while (g_units.capacity < count) unit_array_resize(&g_units);
        if (fread(g_units.items, sizeof(Unit), count, fp) != (size_t)count) {
            fclose(fp); return VENUE_ERR_FILE_READ;
        }
        g_units.count = count;
    }

    if (fread(&count, sizeof(int), 1, fp) != 1) { fclose(fp); return VENUE_ERR_FILE_READ; }
    if (count > 0) {
        venue_array_free(&g_venues);
        venue_array_init(&g_venues);
        while (g_venues.capacity < count) venue_array_resize(&g_venues);
        if (fread(g_venues.items, sizeof(Venue), count, fp) != (size_t)count) {
            fclose(fp); return VENUE_ERR_FILE_READ;
        }
        g_venues.count = count;
    }

    if (fread(&count, sizeof(int), 1, fp) != 1) { fclose(fp); return VENUE_ERR_FILE_READ; }
    if (fread(&g_applies.next_id, sizeof(int), 1, fp) != 1) { fclose(fp); return VENUE_ERR_FILE_READ; }
    if (count > 0) {
        apply_array_free(&g_applies);
        apply_array_init(&g_applies);
        while (g_applies.capacity < count) apply_array_resize(&g_applies);
        if (fread(g_applies.items, sizeof(Apply), count, fp) != (size_t)count) {
            fclose(fp); return VENUE_ERR_FILE_READ;
        }
        g_applies.count = count;
    }

    fclose(fp);
    return VENUE_OK;
}

/* ======================== 数据显示函数 ======================== */

static void print_unit(const Unit *u)
{
    printf(C_CYAN("  单位编号: ") "%-10s" C_CYAN("  单位名称: ") "%s\n", u->code, u->name);
    printf(C_CYAN("  单位级别: ") "%-10s" C_CYAN("  负责人:   ") "%s\n", u->level, u->leader);
    printf(C_CYAN("  单位地址: ") "%s\n", u->address);
}

static void print_venue(const Venue *v)
{
    printf(C_CYAN("  场地编号: ") "%-10s" C_CYAN("  场地地址: ") "%s\n", v->code, v->address);
    printf(C_CYAN("  类    别: ") "%-10s" C_CYAN("  容纳人数: ") "%d\n", v->category, v->capacity);
    printf(C_CYAN("  计费价格: ") C_YELLOW("%.2f") " 元/小时\n", v->price);
}

static void print_apply(const Apply *a)
{
    Unit *u = unit_array_find(&g_units, a->unit_code);
    Venue *v = venue_array_find(&g_venues, a->venue_code);

    printf(C_CYAN("  申请编号: ") C_BOLD("%d") "\n", a->id);
    printf(C_CYAN("  申请单位: ") "%s (%s)\n", u ? u->name : C_RED("未知"), a->unit_code);
    printf(C_CYAN("  使用场地: ") "%s (%s)\n", v ? v->address : C_RED("未知"), a->venue_code);
    printf(C_CYAN("  使用时间: ") "%s\n", a->time_str);
    printf(C_CYAN("  使用事由: ") "%s\n", a->reason);
    if (strlen(a->remark) > 0)
        printf(C_CYAN("  备    注: ") "%s\n", a->remark);
    printf(C_CYAN("  审批状态: ") "%s\n", status_to_string(a->status));
    if (strlen(a->review_note) > 0)
        printf(C_CYAN("  审批备注: ") "%s\n", a->review_note);
}

/* ======================== 菜单：单位管理 ======================== */

static void menu_add_unit(void)
{
    Unit u;
    memset(&u, 0, sizeof(u));
    printf("\n=== " C_GREEN("添加单位") " ===\n");
    printf("请输入单位编号: ");   read_line(u.code, MAX_STR);
    printf("请输入单位名称: ");   read_line(u.name, MAX_STR);
    printf("请输入单位级别: ");   read_line(u.level, MAX_STR);
    printf("请输入负责人姓名: "); read_line(u.leader, MAX_STR);
    printf("请输入单位地址: ");   read_line(u.address, MAX_STR);

    VenueStatus s = unit_array_add(&g_units, &u);
    if (s == VENUE_OK)
        printf(C_GREEN("[成功]") " 单位 '%s' 添加成功\n", u.name);
    else if (s == VENUE_ERR_DUPLICATE)
        printf(C_RED("[错误]") " 单位编号 '%s' 已存在\n", u.code);
    else
        printf(C_RED("[错误]") " 添加失败\n");
}

static void menu_list_units(void)
{
    printf("\n=== " C_CYAN("单位列表") " ===\n");
    if (g_units.count == 0) {
        printf(C_YELLOW("[提示]") " 暂无单位信息\n");
        return;
    }
    int i;
    for (i = 0; i < g_units.count; i++) {
        print_separator();
        print_unit(&g_units.items[i]);
    }
    print_separator();
    printf(C_GREEN("  共 %d 个单位\n"), g_units.count);
}

static void menu_delete_unit(void)
{
    char code[MAX_STR];
    printf("\n=== " C_RED("删除单位") " ===\n");
    printf("请输入要删除的单位编号: ");
    read_line(code, MAX_STR);
    VenueStatus s = unit_array_delete(&g_units, code);
    if (s == VENUE_OK)
        printf(C_GREEN("[成功]") " 单位 '%s' 已删除\n", code);
    else
        printf(C_RED("[错误]") " 未找到单位 '%s'\n", code);
}

/* ======================== 菜单：场地管理 ======================== */

static void menu_add_venue(void)
{
    Venue v;
    char buf[MAX_STR];
    memset(&v, 0, sizeof(v));
    printf("\n=== " C_GREEN("添加场地") " ===\n");
    printf("请输入场地编号: ");   read_line(v.code, MAX_STR);
    printf("请输入场地地址: ");   read_line(v.address, MAX_STR);
    printf("请输入场地类别(教室/礼堂/会议室等): "); read_line(v.category, MAX_STR);
    printf("请输入容纳人数: ");   read_line(buf, MAX_STR); v.capacity = atoi(buf);
    printf("请输入场地价格(元/小时): "); read_line(buf, MAX_STR); v.price = atof(buf);

    VenueStatus s = venue_array_add(&g_venues, &v);
    if (s == VENUE_OK)
        printf(C_GREEN("[成功]") " 场地 '%s' 添加成功\n", v.code);
    else if (s == VENUE_ERR_DUPLICATE)
        printf(C_RED("[错误]") " 场地编号 '%s' 已存在\n", v.code);
    else
        printf(C_RED("[错误]") " 添加失败\n");
}

static void menu_list_venues(void)
{
    printf("\n=== " C_CYAN("场地列表") " ===\n");
    if (g_venues.count == 0) {
        printf(C_YELLOW("[提示]") " 暂无场地信息\n");
        return;
    }
    int i;
    for (i = 0; i < g_venues.count; i++) {
        print_separator();
        print_venue(&g_venues.items[i]);
    }
    print_separator();
    printf(C_GREEN("  共 %d 个场地\n"), g_venues.count);
}

static void menu_delete_venue(void)
{
    char code[MAX_STR];
    printf("\n=== " C_RED("删除场地") " ===\n");
    printf("请输入要删除的场地编号: ");
    read_line(code, MAX_STR);
    VenueStatus s = venue_array_delete(&g_venues, code);
    if (s == VENUE_OK)
        printf(C_GREEN("[成功]") " 场地 '%s' 已删除\n", code);
    else
        printf(C_RED("[错误]") " 未找到场地 '%s'\n", code);
}

/* ======================== 菜单：申请管理 ======================== */

static void menu_add_apply(void)
{
    Apply a;
    memset(&a, 0, sizeof(a));
    a.status = APPLY_PENDING;

    printf("\n=== " C_GREEN("提交场地使用申请") " ===\n");
    printf("请输入申请单位编号: "); read_line(a.unit_code, MAX_STR);
    printf("请输入使用场地编号: "); read_line(a.venue_code, MAX_STR);
    printf("请输入使用时间(如 2026-06-28 14:00): "); read_line(a.time_str, MAX_STR);
    printf("请输入使用事由: ");     read_line(a.reason, MAX_STR);
    printf("请输入备注(可为空): "); read_line(a.remark, MAX_STR);

    if (!unit_array_find(&g_units, a.unit_code)) {
        printf(C_RED("[错误]") " 未找到单位 '%s'，请先添加单位\n", a.unit_code);
        return;
    }
    if (!venue_array_find(&g_venues, a.venue_code)) {
        printf(C_RED("[错误]") " 未找到场地 '%s'，请先添加场地\n", a.venue_code);
        return;
    }

    VenueStatus s = apply_array_add(&g_applies, &a);
    if (s == VENUE_OK)
        printf(C_GREEN("[成功]") " 申请已提交，申请编号: %d\n", g_applies.next_id - 1);
    else
        printf(C_RED("[错误]") " 申请提交失败\n");
}

static void menu_list_applies(int filter)
{
    const char *title = "所有申请";
    if (filter == APPLY_PENDING)  title = "待审批申请";
    if (filter == APPLY_APPROVED) title = "已批准申请";
    if (filter == APPLY_REJECTED) title = "已拒绝申请";

    printf("\n=== " C_CYAN("%s") " ===\n", title);
    if (g_applies.count == 0) {
        printf(C_YELLOW("[提示]") " 暂无申请记录\n");
        return;
    }
    int i, shown = 0;
    for (i = 0; i < g_applies.count; i++) {
        if (filter != -1 && g_applies.items[i].status != (ApplyStatus)filter)
            continue;
        print_separator();
        print_apply(&g_applies.items[i]);
        shown++;
    }
    print_separator();
    if (shown == 0)
        printf(C_YELLOW("[提示]") " 暂无符合条件的申请\n");
    else
        printf(C_GREEN("  共 %d 条记录\n"), shown);
}

/* ======================== 菜单：审批管理 ======================== */

static void menu_review(void)
{
    char buf[MAX_STR];
    int id, action;
    printf("\n=== " C_MAGENTA("审批管理") " ===\n");
    printf("请输入要审批的申请编号: ");
    read_line(buf, MAX_STR);
    id = atoi(buf);

    Apply *a = apply_array_find(&g_applies, id);
    if (!a) {
        printf(C_RED("[错误]") " 未找到申请 #%d\n", id);
        return;
    }

    printf("\n当前申请详情:\n");
    print_separator();
    print_apply(a);
    print_separator();

    if (a->status != APPLY_PENDING) {
        printf(C_YELLOW("[提示]") " 该申请已审批，当前状态: %s\n", status_to_string(a->status));
        return;
    }

    printf("请选择审批操作 (1=批准 2=拒绝): ");
    read_line(buf, MAX_STR);
    action = atoi(buf);

    if (action == 1) {
        a->status = APPLY_APPROVED;
        printf("请输入审批备注(可为空): ");
        read_line(a->review_note, MAX_STR);
        printf(C_GREEN("[成功]") " 申请 #%d 已批准\n", id);
    } else if (action == 2) {
        a->status = APPLY_REJECTED;
        printf("请输入拒绝原因: ");
        read_line(a->review_note, MAX_STR);
        printf(C_GREEN("[成功]") " 申请 #%d 已拒绝\n", id);
    } else {
        printf(C_RED("[错误]") " 无效选择\n");
    }
}

/* ======================== 菜单：查询功能 ======================== */

static void menu_query(void)
{
    char keyword[MAX_STR];
    int choice;
    printf("\n=== " C_BLUE("查询申请") " ===\n");
    printf("  1. 按单位编号查询\n");
    printf("  2. 按场地编号查询\n");
    printf("  3. 按申请编号查询\n");
    printf("请选择查询方式: ");
    char buf[MAX_STR];
    read_line(buf, MAX_STR);
    choice = atoi(buf);

    int i, found = 0;

    if (choice == 1) {
        printf("请输入单位编号: ");
        read_line(keyword, MAX_STR);
        for (i = 0; i < g_applies.count; i++) {
            if (strcmp(g_applies.items[i].unit_code, keyword) == 0) {
                if (found == 0) print_separator();
                print_apply(&g_applies.items[i]);
                print_separator();
                found++;
            }
        }
    } else if (choice == 2) {
        printf("请输入场地编号: ");
        read_line(keyword, MAX_STR);
        for (i = 0; i < g_applies.count; i++) {
            if (strcmp(g_applies.items[i].venue_code, keyword) == 0) {
                if (found == 0) print_separator();
                print_apply(&g_applies.items[i]);
                print_separator();
                found++;
            }
        }
    } else if (choice == 3) {
        printf("请输入申请编号: ");
        read_line(buf, MAX_STR);
        int id = atoi(buf);
        Apply *a = apply_array_find(&g_applies, id);
        if (a) {
            print_separator();
            print_apply(a);
            print_separator();
            found = 1;
        }
    } else {
        printf(C_RED("[错误]") " 无效选择\n");
        return;
    }

    if (found == 0)
        printf(C_YELLOW("[提示]") " 未找到匹配记录\n");
    else
        printf(C_GREEN("  找到 %d 条记录\n"), found);
}

/* ======================== 菜单：统计报表 ======================== */

static void menu_statistics(void)
{
    printf("\n=== " C_MAGENTA("统计报表") " ===\n");
    print_separator();

    int pending = 0, approved = 0, rejected = 0;
    int i;
    for (i = 0; i < g_applies.count; i++) {
        switch (g_applies.items[i].status) {
            case APPLY_PENDING:  pending++; break;
            case APPLY_APPROVED: approved++; break;
            case APPLY_REJECTED: rejected++; break;
        }
    }

    printf(C_BOLD("  基础数据统计:\n"));
    printf("  ├─ 单位总数:     %d\n", g_units.count);
    printf("  ├─ 场地总数:     %d\n", g_venues.count);
    printf("  ├─ 申请总数:     %d\n", g_applies.count);
    printf("  │\n");
    printf(C_BOLD("  审批状态统计:\n"));
    printf("  ├─ 待审批:       " C_YELLOW("%d") "\n", pending);
    printf("  ├─ 已批准:       " C_GREEN("%d") "\n", approved);
    printf("  └─ 已拒绝:       " C_RED("%d") "\n", rejected);

    if (g_venues.count > 0) {
        printf("  │\n");
        printf(C_BOLD("  场地容量统计:\n"));
        int total_cap = 0;
        double total_price = 0;
        Venue *max_v = &g_venues.items[0], *min_v = &g_venues.items[0];
        for (i = 0; i < g_venues.count; i++) {
            total_cap += g_venues.items[i].capacity;
            total_price += g_venues.items[i].price;
            if (g_venues.items[i].capacity > max_v->capacity) max_v = &g_venues.items[i];
            if (g_venues.items[i].capacity < min_v->capacity) min_v = &g_venues.items[i];
        }
        printf("  ├─ 总容纳人数:   %d\n", total_cap);
        printf("  ├─ 平均容纳人数: %.0f\n", (double)total_cap / g_venues.count);
        printf("  ├─ 平均价格:     %.2f 元/小时\n", total_price / g_venues.count);
        printf("  ├─ 最大场地:     %s (%d人)\n", max_v->code, max_v->capacity);
        printf("  └─ 最小场地:     %s (%d人)\n", min_v->code, min_v->capacity);
    }

    print_separator();
}

/* ======================== 内置自测框架 ======================== */

static int g_test_passed = 0;
static int g_test_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (cond) { \
        printf(C_GREEN("  [PASS] ") msg "\n"); \
        g_test_passed++; \
    } else { \
        printf(C_RED("  [FAIL] ") msg " (%s:%d)\n", __FILE__, __LINE__); \
        g_test_failed++; \
    } \
} while (0)

#define TEST_SECTION(name) printf(C_CYAN("\n--- " name " ---\n"))

static void run_self_test(void)
{
    printf(C_BOLD(C_MAGENTA("╔════════════════════════════════════════════╗\n")));
    printf(C_BOLD(C_MAGENTA("║       场地使用申请管理系统 V2.0 自测       ║\n")));
    printf(C_BOLD(C_MAGENTA("╚════════════════════════════════════════════╝\n")));

    unit_array_init(&g_units);
    venue_array_init(&g_venues);
    apply_array_init(&g_applies);

    /* 测试1-4: Unit动态数组基础操作 */
    TEST_SECTION("Unit动态数组测试");
    {
        Unit u1; memset(&u1, 0, sizeof(u1));
        strcpy(u1.code, "U001"); strcpy(u1.name, "测试学院");
        strcpy(u1.level, "校级"); strcpy(u1.leader, "张三");
        strcpy(u1.address, "办公楼301");
        VenueStatus s = unit_array_add(&g_units, &u1);
        TEST_ASSERT(s == VENUE_OK, "测试1: 添加第一个单位成功");
        TEST_ASSERT(g_units.count == 1, "测试2: 单位数量为1");
    }
    {
        Unit u2; memset(&u2, 0, sizeof(u2));
        strcpy(u2.code, "U002"); strcpy(u2.name, "信息学院");
        strcpy(u2.level, "院级"); strcpy(u2.leader, "李四");
        strcpy(u2.address, "教学楼B201");
        unit_array_add(&g_units, &u2);
        TEST_ASSERT(g_units.count == 2, "测试3: 添加第二个单位后数量为2");
    }
    {
        Unit *found = unit_array_find(&g_units, "U001");
        TEST_ASSERT(found != NULL && strcmp(found->name, "测试学院") == 0, "测试4: 查找单位U001成功");
    }
    {
        Unit dup; memset(&dup, 0, sizeof(dup));
        strcpy(dup.code, "U001"); strcpy(dup.name, "重复");
        VenueStatus s = unit_array_add(&g_units, &dup);
        TEST_ASSERT(s == VENUE_ERR_DUPLICATE, "测试5: 添加重复编号单位返回DUPLICATE");
    }
    {
        VenueStatus s = unit_array_delete(&g_units, "U999");
        TEST_ASSERT(s == VENUE_ERR_NOT_FOUND, "测试6: 删除不存在单位返回NOT_FOUND");
    }

    /* 测试7-11: Venue动态数组操作 */
    TEST_SECTION("Venue动态数组测试");
    {
        Venue v1; memset(&v1, 0, sizeof(v1));
        strcpy(v1.code, "V001"); strcpy(v1.address, "教学楼A101");
        strcpy(v1.category, "教室"); v1.capacity = 120; v1.price = 200.0;
        VenueStatus s = venue_array_add(&g_venues, &v1);
        TEST_ASSERT(s == VENUE_OK, "测试7: 添加第一个场地成功");
    }
    {
        Venue v2; memset(&v2, 0, sizeof(v2));
        strcpy(v2.code, "V002"); strcpy(v2.address, "大礼堂");
        strcpy(v2.category, "礼堂"); v2.capacity = 1000; v2.price = 500.0;
        venue_array_add(&g_venues, &v2);
        Venue v3; memset(&v3, 0, sizeof(v3));
        strcpy(v3.code, "V003"); strcpy(v3.address, "会议室2号");
        strcpy(v3.category, "会议室"); v3.capacity = 30; v3.price = 100.0;
        venue_array_add(&g_venues, &v3);
        TEST_ASSERT(g_venues.count == 3, "测试8: 添加3个场地后数量为3");
    }
    {
        Venue *found = venue_array_find(&g_venues, "V002");
        TEST_ASSERT(found != NULL && found->capacity == 1000, "测试9: 查找场地V002容量正确");
    }
    {
        TEST_ASSERT(venue_array_find(&g_venues, "V999") == NULL, "测试10: 查找不存在场地返回NULL");
    }
    {
        venue_array_delete(&g_venues, "V003");
        TEST_ASSERT(g_venues.count == 2, "测试11: 删除场地后数量为2");
    }

    /* 测试12-18: Apply申请与审批流程 */
    TEST_SECTION("Apply申请与审批测试");
    {
        Apply a1; memset(&a1, 0, sizeof(a1));
        strcpy(a1.unit_code, "U001"); strcpy(a1.venue_code, "V001");
        strcpy(a1.time_str, "2026-06-28 14:00");
        strcpy(a1.reason, "学院毕业晚会");
        VenueStatus s = apply_array_add(&g_applies, &a1);
        TEST_ASSERT(s == VENUE_OK, "测试12: 提交第一个申请成功");
        TEST_ASSERT(g_applies.items[0].id == 1, "测试13: 第一个申请ID为1");
        TEST_ASSERT(g_applies.items[0].status == APPLY_PENDING, "测试14: 新申请状态为待审批");
    }
    {
        Apply a2; memset(&a2, 0, sizeof(a2));
        strcpy(a2.unit_code, "U002"); strcpy(a2.venue_code, "V002");
        strcpy(a2.time_str, "2026-07-01 09:00");
        strcpy(a2.reason, "信息技术讲座");
        apply_array_add(&g_applies, &a2);
        TEST_ASSERT(g_applies.count == 2, "测试15: 添加第二个申请后数量为2");
        TEST_ASSERT(g_applies.items[1].id == 2, "测试16: 第二个申请ID为2");
    }
    {
        Apply *a = apply_array_find(&g_applies, 1);
        TEST_ASSERT(a != NULL, "测试17: 按ID查找申请成功");
        a->status = APPLY_APPROVED;
        strcpy(a->review_note, "同意使用");
        TEST_ASSERT(a->status == APPLY_APPROVED, "测试18: 审批通过状态正确");
    }
    {
        Apply *a = apply_array_find(&g_applies, 2);
        a->status = APPLY_REJECTED;
        strcpy(a->review_note, "时间冲突");
        int pending = 0, approved = 0, rejected = 0;
        int i;
        for (i = 0; i < g_applies.count; i++) {
            if (g_applies.items[i].status == APPLY_PENDING) pending++;
            if (g_applies.items[i].status == APPLY_APPROVED) approved++;
            if (g_applies.items[i].status == APPLY_REJECTED) rejected++;
        }
        TEST_ASSERT(pending == 0 && approved == 1 && rejected == 1, "测试19: 审批状态统计正确");
    }

    /* 测试20+: 文件持久化 */
    TEST_SECTION("文件持久化测试");
    {
        VenueStatus s = save_to_file(DATA_FILE);
        TEST_ASSERT(s == VENUE_OK, "测试20: 保存数据到文件成功");
    }
    {
        unit_array_free(&g_units);
        venue_array_free(&g_venues);
        apply_array_free(&g_applies);
        unit_array_init(&g_units);
        venue_array_init(&g_venues);
        apply_array_init(&g_applies);
        VenueStatus s = load_from_file(DATA_FILE);
        TEST_ASSERT(s == VENUE_OK, "测试21: 从文件加载数据成功");
        TEST_ASSERT(g_units.count == 2, "测试22: 加载后单位数量正确");
        TEST_ASSERT(g_venues.count == 2, "测试23: 加载后场地数量正确");
        TEST_ASSERT(g_applies.count == 2, "测试24: 加载后申请数量正确");
    }
    {
        Apply *a = apply_array_find(&g_applies, 1);
        TEST_ASSERT(a != NULL && a->status == APPLY_APPROVED, "测试25: 加载后审批状态保持正确");
    }
    {
        VenueStatus s = load_from_file("nonexistent_file.dat");
        TEST_ASSERT(s == VENUE_ERR_FILE_OPEN, "测试26: 打开不存在文件返回FILE_OPEN错误");
    }

    /* 动态数组扩容测试 */
    TEST_SECTION("动态数组扩容测试");
    {
        int i;
        for (i = 0; i < 10; i++) {
            char code[32];
            sprintf(code, "T%03d", i);
            Venue v; memset(&v, 0, sizeof(v));
            strcpy(v.code, code);
            strcpy(v.address, "测试地址");
            strcpy(v.category, "测试");
            v.capacity = 100; v.price = 50.0;
            venue_array_add(&g_venues, &v);
        }
        TEST_ASSERT(g_venues.count == 12, "测试27: 扩容后添加10个场地总数12");
        TEST_ASSERT(g_venues.capacity >= 12, "测试28: 动态数组容量自动扩容");
    }

    /* 清理测试数据 */
    remove(DATA_FILE);
    unit_array_free(&g_units);
    venue_array_free(&g_venues);
    apply_array_free(&g_applies);

    printf(C_BOLD("\n========================================\n"));
    printf(C_BOLD("测试结果: " C_GREEN("%d 通过") "  " C_RED("%d 失败") "\n"),
           g_test_passed, g_test_failed);
    printf(C_BOLD("========================================\n"));

    if (g_test_failed == 0) {
        printf(C_GREEN("\n所有测试通过！✓\n"));
    } else {
        printf(C_RED("\n存在失败的测试用例！✗\n"));
    }
}

/* ======================== CLI帮助信息 ======================== */

static void print_help(const char *prog)
{
    printf(C_BOLD("场地使用申请信息管理系统 V%s\n\n"), VERSION);
    printf("用法: %s [选项]\n\n", prog);
    printf("选项:\n");
    printf("  -h, --help      显示帮助信息\n");
    printf("  -t, --test      运行内置自测\n");
    printf("  (无参数)        启动交互菜单模式\n\n");
    printf("交互菜单功能:\n");
    printf("  1. 申请管理 - 提交场地使用申请\n");
    printf("  2. 审批管理 - 审批待处理申请\n");
    printf("  3. 查询功能 - 多条件查询申请\n");
    printf("  4. 统计报表 - 数据统计分析\n");
    printf("  5. 单位管理 - 添加/删除/列表单位\n");
    printf("  6. 场地管理 - 添加/删除/列表场地\n");
    printf("  7. 全部数据 - 列出所有数据\n");
    printf("  8. 保存数据 - 手动保存到文件\n");
    printf("  0. 退出系统\n");
}

/* ======================== 加载测试示例数据 ======================== */

static void load_sample_data(void)
{
    Unit u;
    Venue v;
    Apply a;

    memset(&u, 0, sizeof(u));
    strcpy(u.code, "U001"); strcpy(u.name, "计算机学院");
    strcpy(u.level, "院级"); strcpy(u.leader, "王院长");
    strcpy(u.address, "信息楼A栋301");
    unit_array_add(&g_units, &u);

    memset(&u, 0, sizeof(u));
    strcpy(u.code, "U002"); strcpy(u.name, "学生处");
    strcpy(u.level, "校级"); strcpy(u.leader, "李主任");
    strcpy(u.address, "行政楼1栋201");
    unit_array_add(&g_units, &u);

    memset(&u, 0, sizeof(u));
    strcpy(u.code, "U003"); strcpy(u.name, "教务处");
    strcpy(u.level, "校级"); strcpy(u.leader, "张处长");
    strcpy(u.address, "行政楼2栋101");
    unit_array_add(&g_units, &u);

    memset(&v, 0, sizeof(v));
    strcpy(v.code, "V001"); strcpy(v.address, "教学楼A101");
    strcpy(v.category, "多媒体教室"); v.capacity = 120; v.price = 200.0;
    venue_array_add(&g_venues, &v);

    memset(&v, 0, sizeof(v));
    strcpy(v.code, "V002"); strcpy(v.address, "大礼堂");
    strcpy(v.category, "礼堂"); v.capacity = 1000; v.price = 800.0;
    venue_array_add(&g_venues, &v);

    memset(&v, 0, sizeof(v));
    strcpy(v.code, "V003"); strcpy(v.address, "会议室2号");
    strcpy(v.category, "会议室"); v.capacity = 30; v.price = 100.0;
    venue_array_add(&g_venues, &v);

    memset(&a, 0, sizeof(a));
    strcpy(a.unit_code, "U001"); strcpy(a.venue_code, "V001");
    strcpy(a.time_str, "2026-07-05 14:00");
    strcpy(a.reason, "程序设计竞赛");
    a.status = APPLY_PENDING;
    apply_array_add(&g_applies, &a);

    memset(&a, 0, sizeof(a));
    strcpy(a.unit_code, "U002"); strcpy(a.venue_code, "V002");
    strcpy(a.time_str, "2026-07-10 19:00");
    strcpy(a.reason, "新生迎新晚会");
    a.status = APPLY_PENDING;
    apply_array_add(&g_applies, &a);

    printf(C_GREEN("[成功]") " 示例数据已加载：3个单位，3个场地，2条申请\n");
}

/* ======================== 主菜单 ======================== */

static void menu_main_loop(void)
{
    int choice;
    char buf[MAX_STR];
    while (1) {
        printf("\n");
        print_separator();
        printf(C_BOLD("        场地使用申请信息管理系统 V%s\n"), VERSION);
        print_separator();
        printf("  1. " C_CYAN("申请管理") "   - 提交场地使用申请\n");
        printf("  2. " C_MAGENTA("审批管理") " - 审批待处理申请\n");
        printf("  3. " C_BLUE("查询功能") "   - 多条件查询申请\n");
        printf("  4. " C_MAGENTA("统计报表") " - 数据统计分析\n");
        printf("  5. " C_GREEN("单位管理") "   - 添加/删除/列表\n");
        printf("  6. " C_GREEN("场地管理") "   - 添加/删除/列表\n");
        printf("  7. " C_CYAN("全部数据") "   - 浏览所有记录\n");
        printf("  8. " C_YELLOW("保存数据") "   - 保存到文件\n");
        printf("  0. " C_RED("退出系统") "\n");
        print_separator();
        printf("当前数据: " C_CYAN("%d单位") " | " C_CYAN("%d场地") " | " C_CYAN("%d申请") "\n",
               g_units.count, g_venues.count, g_applies.count);
        printf("请选择操作: ");

        read_line(buf, MAX_STR);
        choice = atoi(buf);

        switch (choice) {
            case 1: menu_add_apply(); break;
            case 2:
                menu_list_applies(APPLY_PENDING);
                if (g_applies.count > 0) menu_review();
                break;
            case 3: menu_query(); break;
            case 4: menu_statistics(); break;
            case 5: {
                while (1) {
                    printf("\n--- " C_GREEN("单位管理") " ---\n");
                    printf("  1. 添加单位\n  2. 单位列表\n  3. 删除单位\n  0. 返回\n请选择: ");
                    read_line(buf, MAX_STR);
                    int c = atoi(buf);
                    if (c == 1) menu_add_unit();
                    else if (c == 2) menu_list_units();
                    else if (c == 3) menu_delete_unit();
                    else if (c == 0) break;
                    else printf(C_RED("[错误]") " 无效选择\n");
                }
                break;
            }
            case 6: {
                while (1) {
                    printf("\n--- " C_GREEN("场地管理") " ---\n");
                    printf("  1. 添加场地\n  2. 场地列表\n  3. 删除场地\n  0. 返回\n请选择: ");
                    read_line(buf, MAX_STR);
                    int c = atoi(buf);
                    if (c == 1) menu_add_venue();
                    else if (c == 2) menu_list_venues();
                    else if (c == 3) menu_delete_venue();
                    else if (c == 0) break;
                    else printf(C_RED("[错误]") " 无效选择\n");
                }
                break;
            }
            case 7:
                menu_list_units();
                menu_list_venues();
                menu_list_applies(-1);
                break;
            case 8: {
                VenueStatus s = save_to_file(DATA_FILE);
                if (s == VENUE_OK)
                    printf(C_GREEN("[成功]") " 数据已保存到 %s\n", DATA_FILE);
                else
                    printf(C_RED("[错误]") " 保存失败\n");
                break;
            }
            case 0:
                save_to_file(DATA_FILE);
                printf(C_GREEN("\n感谢使用，再见！\n"));
                return;
            default:
                printf(C_RED("[错误]") " 无效选择，请重新输入\n");
        }
        fflush(stdout);
    }
}

/* ======================== 主函数入口 ======================== */

int main(int argc, char *argv[])
{
    console_init();

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        }
        if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0) {
            run_self_test();
            return g_test_failed > 0 ? 1 : 0;
        }
        printf(C_RED("未知选项: %s\n使用 -h 查看帮助\n"), argv[1]);
        return 1;
    }

    printf(C_BOLD(C_CYAN("============================================================\n")));
    printf(C_BOLD(C_CYAN("  场地使用申请信息管理系统 V%s\n")), VERSION);
    printf(C_BOLD(C_CYAN("  编码：数据库课程  班级：计算机A班\n")));
    printf(C_BOLD(C_CYAN("============================================================\n")));

    unit_array_init(&g_units);
    venue_array_init(&g_venues);
    apply_array_init(&g_applies);

    VenueStatus load_status = load_from_file(DATA_FILE);
    if (load_status == VENUE_OK) {
        printf(C_GREEN("[成功]") " 已从 %s 加载数据\n", DATA_FILE);
        printf("  单位: %d  场地: %d  申请: %d\n",
               g_units.count, g_venues.count, g_applies.count);
    } else {
        printf(C_YELLOW("[提示]") " 未找到数据文件，是否加载示例数据？(1=是 0=否): ");
        char buf[MAX_STR];
        read_line(buf, MAX_STR);
        if (atoi(buf) == 1) {
            load_sample_data();
        } else {
            printf("  已进入空数据状态，请先添加单位和场地\n");
        }
    }

    menu_main_loop();

    unit_array_free(&g_units);
    venue_array_free(&g_venues);
    apply_array_free(&g_applies);

    return 0;
}
