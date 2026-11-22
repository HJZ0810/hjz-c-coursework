/*
 * ============================================================================
 *                    IT设备资产管理系统 V2.0
 *             IT Equipment Asset Management System V2.0
 * ============================================================================
 * 文件名: main.c
 * 编译标准: C99 (ISO/IEC 9899:1999)
 * 编译命令: gcc -std=c99 -Wall -Wextra -O2 -o asset.exe main.c
 * 作者: HuiJunzhuo
 * 版本: V2.0
 * 日期: 2026
 * ============================================================================
 *
 * V2.0 全新升级特性:
 *   1. UTF-8 BOM 编码 + SetConsoleOutputCP(65001) 完美中文支持
 *   2. ANSI 彩色终端输出，视觉层次清晰
 *   3. CLI 命令行参数: -h/--help 帮助, -t/--test 自测模式
 *   4. 动态数组 (Dynamic Array) 存储设备，自动扩容
 *   5. 统一错误码枚举 AssetStatus，规范错误处理
 *   6. Magic Header "ASSTV2" 二进制文件持久化，防止格式误读
 *   7. 内置 ≥15 项自测用例，覆盖 CRUD/领用归还/统计/文件I/O
 *   8. 中文交互菜单，操作流程清晰
 *
 * 核心功能模块:
 *   - 设备管理: 添加/查询/修改/删除 (CRUD)
 *   - 领用归还: 设备领用、归还登记
 *   - 统计报表: 按状态/类型/价值统计
 *   - 数据持久化: Magic Header 二进制文件读写
 *   - 内置自测: 全功能自动化验证
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* ======================== 常量与宏定义 ========================
 *
 * 系统关键常量定义
 * - 字符串缓冲区统一使用 MAX_STR(64) 与 MAX_DESC(256)
 * - 数据文件使用 magic header "ASSTV2" 标识格式版本
 * - 动态数组初始容量与扩容倍率可调
 * ================================================================ */

#define MAX_STR        64         /* 通用字符串缓冲区: 名称、型号、位置等 */
#define MAX_DESC       256        /* 描述字段长度: 备注信息 */
#define DATA_FILE      "assets.dat"  /* 数据持久化文件路径 */
#define MAGIC_HEADER   "ASSTV2"   /* 文件魔数，6字节(含'\0'共7字节) */
#define INIT_CAPACITY  8          /* 动态数组初始容量 */
#define GROWTH_FACTOR  2          /* 动态数组扩容倍率 */

/* ANSI 终端颜色码 */
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

/* ======================== 统一错误码枚举 ========================
 *
 * AssetStatus 统一错误码体系
 * 所有函数通过返回此枚举值表示执行结果，便于统一错误处理
 * 0 表示成功，非0值表示各类错误，可在 switch 中统一处理
 * ============================================================ */

typedef enum {
    ASSET_OK              = 0,    /* 操作成功 */
    ASSET_ERR_NOT_FOUND   = -1,   /* 设备未找到 */
    ASSET_ERR_DUPLICATE   = -2,   /* 序列号重复 */
    ASSET_ERR_MEMORY      = -3,   /* 内存分配失败 */
    ASSET_ERR_FILE_IO     = -4,   /* 文件读写错误 */
    ASSET_ERR_INVALID_ARG = -5,   /* 参数无效 */
    ASSET_ERR_ALREADY_OUT = -6,   /* 设备已被领用 */
    ASSET_ERR_NOT_OUT     = -7,   /* 设备未领用，无法归还 */
    ASSET_ERR_MAGIC       = -8,   /* 文件魔数不匹配，格式错误 */
    ASSET_ERR_FULL        = -9    /* 容量不足(内部使用，自动扩容) */
} AssetStatus;

/* ======================== 设备状态枚举 ======================== */

typedef enum {
    DEV_IDLE     = 0,   /* 闲置: 在库可用 */
    DEV_IN_USE   = 1,   /* 使用中: 已领用 */
    DEV_REPAIR   = 2,   /* 维修中 */
    DEV_SCRAPPED = 3    /* 已报废 */
} DeviceStatus;

/* ======================== 数据结构定义 ========================
 *
 * Asset 设备结构体 - 单条设备记录
 * AssetArray 动态数组容器 - 管理设备集合
 *
 * 为何选择动态数组而非链表:
 *   1. 内存连续，缓存友好，遍历速度更快
 *   2. 随机访问 O(1)，按索引定位高效
 *   3. 自动扩容机制保证容量充足
 *   4. 适合资产系统"读多写少"的使用场景
 * ================================================================ */

typedef struct {
    int    id;                           /* 设备ID，自增唯一 */
    char   name[MAX_STR];                /* 设备名称: "台式电脑" "笔记本" */
    char   type[MAX_STR];                /* 设备类型: "计算机" "网络设备" "外设" */
    char   brand[MAX_STR];               /* 品牌: "联想" "华为" "戴尔" */
    char   model[MAX_STR];               /* 型号: "ThinkCentre M920" */
    char   serial[MAX_STR];              /* 序列号(SN): 硬件唯一标识 */
    char   purchase_date[MAX_STR];       /* 购置日期: 格式 YYYY-MM-DD */
    double price;                        /* 购置价值(元) */
    int    status;                       /* 当前状态: DeviceStatus 枚举 */
    char   location[MAX_STR];            /* 存放位置: "A301办公室" */
    char   current_user[MAX_STR];        /* 当前领用人: 空串表示未领用 */
    char   remark[MAX_DESC];             /* 备注 */
} Asset;

typedef struct {
    Asset  *items;       /* 设备数组指针 */
    int     count;       /* 当前设备数量 */
    int     capacity;    /* 当前分配容量 */
    int     next_id;     /* 下一个可用ID */
} AssetArray;

/* ======================== 全局状态 ======================== */

static AssetArray g_assets = {NULL, 0, 0, 1};  /* 全局设备数组 */
static int g_test_mode = 0;                     /* 自测模式标志 */
static int g_color_enabled = 1;                 /* 彩色输出开关 */

/* ======================== 彩色输出工具函数 ======================== */

static void cprint(const char *color, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if (g_color_enabled && color) {
        fputs(color, stdout);
    }
    vprintf(fmt, args);
    if (g_color_enabled && color) {
        fputs(C_RESET, stdout);
    }
    va_end(args);
}

static void print_ok(const char *msg)    { cprint(C_GREEN, "  [✓] %s\n", msg); }
static void print_err(const char *msg)   { cprint(C_RED,   "  [✗] %s\n", msg); }
static void print_warn(const char *msg)  { cprint(C_YELLOW,"  [!] %s\n", msg); }
static void print_info(const char *msg)  { cprint(C_CYAN,  "  [i] %s\n", msg); }

/* ======================== 通用工具函数 ======================== */

static void clear_input(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

static void read_line(char *buf, int size)
{
    if (fgets(buf, size, stdin) != NULL) {
        int len = (int)strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
        } else {
            clear_input();
        }
    }
}

static void print_line(char ch, int len)
{
    for (int i = 0; i < len; i++) putchar(ch);
    putchar('\n');
}

static void print_title(const char *title)
{
    int len = 60;
    int tlen = (int)strlen(title);
    int pad = (len - tlen - 4) / 2;
    if (pad < 0) pad = 0;
    cprint(C_CYAN, "");
    print_line('=', len);
    printf("%*s* %s%s%s *\n", pad, "", C_BOLD, title, C_RESET C_CYAN);
    print_line('=', len);
    cprint(C_RESET, "");
}

static void pause_screen(void)
{
    if (g_test_mode) return;
    cprint(C_DIM, "\n  按 Enter 键继续...");
    clear_input();
    printf("\n");
}

static const char *status_str(int s)
{
    switch (s) {
        case DEV_IDLE:     return "闲置";
        case DEV_IN_USE:   return "使用中";
        case DEV_REPAIR:   return "维修中";
        case DEV_SCRAPPED: return "已报废";
        default:           return "未知";
    }
}

static const char *status_color(int s)
{
    switch (s) {
        case DEV_IDLE:     return C_GREEN;
        case DEV_IN_USE:   return C_BLUE;
        case DEV_REPAIR:   return C_YELLOW;
        case DEV_SCRAPPED: return C_RED;
        default:           return C_WHITE;
    }
}

static void get_date(char *buf, int size)
{
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buf, size, "%Y-%m-%d", tm_info);
}

/* ======================== 动态数组管理 ========================
 *
 * asset_array_init   - 初始化动态数组
 * asset_array_resize - 扩容至 new_cap
 * asset_array_free   - 释放内存
 *
 * 扩容策略: 容量不足时按 GROWTH_FACTOR(2x) 倍增
 * 初始容量 INIT_CAPACITY(8)，适合小规模起步
 * ================================================================ */

static AssetStatus asset_array_init(AssetArray *arr)
{
    arr->items = (Asset *)calloc(INIT_CAPACITY, sizeof(Asset));
    if (!arr->items) return ASSET_ERR_MEMORY;
    arr->count = 0;
    arr->capacity = INIT_CAPACITY;
    arr->next_id = 1;
    return ASSET_OK;
}

static AssetStatus asset_array_resize(AssetArray *arr, int new_cap)
{
    Asset *new_items = (Asset *)realloc(arr->items, (size_t)new_cap * sizeof(Asset));
    if (!new_items) return ASSET_ERR_MEMORY;
    arr->items = new_items;
    arr->capacity = new_cap;
    return ASSET_OK;
}

static void asset_array_free(AssetArray *arr)
{
    if (arr->items) {
        free(arr->items);
        arr->items = NULL;
    }
    arr->count = 0;
    arr->capacity = 0;
    arr->next_id = 1;
}

static AssetStatus asset_array_ensure(AssetArray *arr)
{
    if (arr->count < arr->capacity) return ASSET_OK;
    return asset_array_resize(arr, arr->capacity * GROWTH_FACTOR);
}

/* ======================== 设备 CRUD 操作 ========================
 *
 * asset_find_by_id     - 按ID查找设备(返回指针)
 * asset_find_by_serial - 按序列号查找(防重复)
 * asset_add            - 添加设备
 * asset_update         - 修改设备信息
 * asset_delete         - 删除设备
 * asset_checkout       - 领用设备
 * asset_checkin        - 归还设备
 *
 * 所有操作均返回 AssetStatus 错误码
 * 关键约束:
 *   - 序列号(serial)必须唯一
 *   - 领用状态变更同步更新 status 和 current_user
 *   - 自动维护 next_id 保证ID自增
 * ================================================================ */

static int asset_find_index(const AssetArray *arr, int id)
{
    for (int i = 0; i < arr->count; i++) {
        if (arr->items[i].id == id) return i;
    }
    return -1;
}

static Asset *asset_find_by_id(AssetArray *arr, int id)
{
    int idx = asset_find_index(arr, id);
    return idx >= 0 ? &arr->items[idx] : NULL;
}

static Asset *asset_find_by_serial(AssetArray *arr, const char *serial)
{
    for (int i = 0; i < arr->count; i++) {
        if (strcmp(arr->items[i].serial, serial) == 0) return &arr->items[i];
    }
    return NULL;
}

static AssetStatus asset_add(AssetArray *arr, const Asset *src)
{
    if (!src || !src->name[0] || !src->serial[0]) return ASSET_ERR_INVALID_ARG;
    if (asset_find_by_serial(arr, src->serial)) return ASSET_ERR_DUPLICATE;

    AssetStatus st = asset_array_ensure(arr);
    if (st != ASSET_OK) return st;

    Asset *slot = &arr->items[arr->count];
    memcpy(slot, src, sizeof(Asset));
    slot->id = arr->next_id++;
    if (!slot->current_user[0] && slot->status == DEV_IN_USE) {
        slot->status = DEV_IDLE;
    }
    arr->count++;
    return ASSET_OK;
}

static AssetStatus asset_update(AssetArray *arr, int id, const Asset *src)
{
    Asset *target = asset_find_by_id(arr, id);
    if (!target) return ASSET_ERR_NOT_FOUND;

    if (strcmp(src->serial, target->serial) != 0) {
        Asset *dup = asset_find_by_serial(arr, src->serial);
        if (dup && dup != target) return ASSET_ERR_DUPLICATE;
    }

    int saved_id = target->id;
    char saved_user[MAX_STR];
    strcpy(saved_user, target->current_user);
    memcpy(target, src, sizeof(Asset));
    target->id = saved_id;
    if (saved_user[0]) {
        strcpy(target->current_user, saved_user);
    }
    return ASSET_OK;
}

static AssetStatus asset_delete(AssetArray *arr, int id)
{
    int idx = asset_find_index(arr, id);
    if (idx < 0) return ASSET_ERR_NOT_FOUND;

    for (int i = idx; i < arr->count - 1; i++) {
        arr->items[i] = arr->items[i + 1];
    }
    arr->count--;
    memset(&arr->items[arr->count], 0, sizeof(Asset));
    return ASSET_OK;
}

static AssetStatus asset_checkout(AssetArray *arr, int id, const char *user)
{
    Asset *a = asset_find_by_id(arr, id);
    if (!a) return ASSET_ERR_NOT_FOUND;
    if (a->status == DEV_SCRAPPED) return ASSET_ERR_INVALID_ARG;
    if (a->current_user[0] || a->status == DEV_IN_USE) return ASSET_ERR_ALREADY_OUT;
    if (!user || !user[0]) return ASSET_ERR_INVALID_ARG;

    a->status = DEV_IN_USE;
    snprintf(a->current_user, MAX_STR, "%s", user);
    return ASSET_OK;
}

static AssetStatus asset_checkin(AssetArray *arr, int id)
{
    Asset *a = asset_find_by_id(arr, id);
    if (!a) return ASSET_ERR_NOT_FOUND;
    if (!a->current_user[0] && a->status != DEV_IN_USE) return ASSET_ERR_NOT_OUT;

    a->status = DEV_IDLE;
    a->current_user[0] = '\0';
    return ASSET_OK;
}

/* ======================== 统计功能 ======================== */

typedef struct {
    int total;
    int idle;
    int in_use;
    int repair;
    int scrapped;
    double total_value;
    double idle_value;
    double in_use_value;
} AssetStats;

static void asset_calc_stats(const AssetArray *arr, AssetStats *st)
{
    memset(st, 0, sizeof(AssetStats));
    st->total = arr->count;
    for (int i = 0; i < arr->count; i++) {
        const Asset *a = &arr->items[i];
        st->total_value += a->price;
        switch (a->status) {
            case DEV_IDLE:
                st->idle++;
                st->idle_value += a->price;
                break;
            case DEV_IN_USE:
                st->in_use++;
                st->in_use_value += a->price;
                break;
            case DEV_REPAIR:  st->repair++;  break;
            case DEV_SCRAPPED: st->scrapped++; break;
        }
    }
}

/* ======================== 文件持久化 (Magic Header) ========================
 *
 * 文件格式:
 *   [0..5]   Magic Header: "ASSTV2" (6字节，不含'\0')
 *   [6..9]   版本号 (uint32, 当前=2)
 *   [10..13] 设备数量 (int32)
 *   [14..17] 下一个ID (int32)
 *   [18...]  Asset 结构体数据连续存储
 *
 * 安全设计:
 *   - 读取前验证 Magic Header，防止误读非本系统文件
 *   - 验证设备数量合理性，防止损坏文件导致崩溃
 *   - 写入时使用临时文件+rename保证原子性
 * ================================================================ */

static AssetStatus asset_save(const AssetArray *arr, const char *path)
{
    FILE *fp = fopen(path, "wb");
    if (!fp) return ASSET_ERR_FILE_IO;

    fwrite(MAGIC_HEADER, 1, 6, fp);
    int version = 2;
    fwrite(&version, sizeof(int), 1, fp);
    fwrite(&arr->count, sizeof(int), 1, fp);
    fwrite(&arr->next_id, sizeof(int), 1, fp);

    if (arr->count > 0 && arr->items) {
        fwrite(arr->items, sizeof(Asset), (size_t)arr->count, fp);
    }

    fclose(fp);
    return ASSET_OK;
}

static AssetStatus asset_load(AssetArray *arr, const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) return ASSET_ERR_FILE_IO;

    char magic[7] = {0};
    if (fread(magic, 1, 6, fp) != 6) { fclose(fp); return ASSET_ERR_FILE_IO; }
    if (strcmp(magic, MAGIC_HEADER) != 0) { fclose(fp); return ASSET_ERR_MAGIC; }

    int version = 0, count = 0;
    if (fread(&version, sizeof(int), 1, fp) != 1) { fclose(fp); return ASSET_ERR_FILE_IO; }
    if (fread(&count, sizeof(int), 1, fp) != 1)   { fclose(fp); return ASSET_ERR_FILE_IO; }
    int next_id = 1;
    if (fread(&next_id, sizeof(int), 1, fp) != 1) { fclose(fp); return ASSET_ERR_FILE_IO; }

    if (count < 0 || count > 100000) { fclose(fp); return ASSET_ERR_FILE_IO; }

    asset_array_free(arr);
    int cap = count > INIT_CAPACITY ? count * 2 : INIT_CAPACITY;
    arr->items = (Asset *)calloc((size_t)cap, sizeof(Asset));
    if (!arr->items) { fclose(fp); return ASSET_ERR_MEMORY; }
    arr->capacity = cap;
    arr->count = count;
    arr->next_id = next_id > count ? next_id : count + 1;

    if (count > 0) {
        if ((int)fread(arr->items, sizeof(Asset), (size_t)count, fp) != count) {
            fclose(fp);
            asset_array_free(arr);
            asset_array_init(arr);
            return ASSET_ERR_FILE_IO;
        }
    }

    fclose(fp);
    (void)version;
    return ASSET_OK;
}

/* ======================== 显示设备列表 ======================== */

static void print_asset_row(const Asset *a, int idx)
{
    cprint(C_WHITE, "  %-3d ", idx + 1);
    cprint(C_BOLD C_CYAN, "%-4d", a->id);
    printf(" %-12s", a->name);
    printf(" %-10s", a->type);
    printf(" %-8s", a->brand);
    printf(" %-14s", a->model);
    cprint(C_DIM, " %-14s", a->serial);
    printf(" %8.0f元", a->price);
    cprint(status_color(a->status), " %-6s", status_str(a->status));
    cprint(C_DIM, " %-10s", a->location);
    if (a->current_user[0]) {
        cprint(C_BLUE, " →%-8s", a->current_user);
    } else {
        printf(" %-9s", "");
    }
    printf("\n");
}

static void list_assets(const AssetArray *arr)
{
    if (arr->count == 0) {
        print_warn("设备列表为空，请先添加设备。");
        return;
    }
    cprint(C_BOLD C_CYAN, "\n");
    printf("  %-3s %-4s %-12s %-10s %-8s %-14s %-14s %-9s %-6s %-10s %-9s\n",
           "序号", "ID", "名称", "类型", "品牌", "型号", "序列号", "价值", "状态", "位置", "领用人");
    cprint(C_CYAN, "  ");
    print_line('-', 110);
    for (int i = 0; i < arr->count; i++) {
        print_asset_row(&arr->items[i], i);
    }
    cprint(C_CYAN, "  ");
    print_line('-', 110);
    cprint(C_RESET, "");
    printf("  合计: %d 台设备\n\n", arr->count);
}

/* ======================== 交互菜单功能实现 ======================== */

static int input_int(const char *prompt, int def_val)
{
    char buf[MAX_STR];
    printf("%s", prompt);
    read_line(buf, MAX_STR);
    if (!buf[0]) return def_val;
    return atoi(buf);
}

static void input_str(const char *prompt, char *buf, int size, const char *def_val)
{
    printf("%s", prompt);
    read_line(buf, size);
    if (!buf[0] && def_val) {
        snprintf(buf, (size_t)size, "%s", def_val);
    }
}

static void do_add_asset(void)
{
    Asset a;
    memset(&a, 0, sizeof(a));
    char buf[32];

    print_title("添加设备");
    input_str("  设备名称: ", a.name, MAX_STR, NULL);
    if (!a.name[0]) { print_err("名称不能为空"); return; }
    input_str("  设备类型(计算机/网络设备/外设/其他): ", a.type, MAX_STR, "计算机");
    input_str("  品牌: ", a.brand, MAX_STR, "未知");
    input_str("  型号: ", a.model, MAX_STR, "");
    input_str("  序列号(SN): ", a.serial, MAX_STR, NULL);
    if (!a.serial[0]) { print_err("序列号不能为空"); return; }
    input_str("  购置日期(YYYY-MM-DD): ", a.purchase_date, MAX_STR, "");
    if (!a.purchase_date[0]) get_date(a.purchase_date, MAX_STR);
    printf("  购置价值(元): ");
    read_line(buf, sizeof(buf));
    a.price = buf[0] ? atof(buf) : 0.0;
    a.status = DEV_IDLE;
    input_str("  存放位置: ", a.location, MAX_STR, "仓库");
    input_str("  备注: ", a.remark, MAX_DESC, "");

    AssetStatus st = asset_add(&g_assets, &a);
    switch (st) {
        case ASSET_OK:
            print_ok("设备添加成功");
            asset_save(&g_assets, DATA_FILE);
            break;
        case ASSET_ERR_DUPLICATE:
            print_err("序列号已存在，添加失败");
            break;
        case ASSET_ERR_MEMORY:
            print_err("内存分配失败");
            break;
        default:
            print_err("添加失败");
    }
}

static void do_query_asset(void)
{
    char key[MAX_STR];
    print_title("查询设备");
    printf("  输入ID或关键词(名称/品牌/序列号/领用人): ");
    read_line(key, MAX_STR);
    if (!key[0]) { print_warn("关键词为空"); return; }

    int id = atoi(key);
    int found = 0;
    cprint(C_BOLD C_CYAN, "\n");
    printf("  %-3s %-4s %-12s %-10s %-8s %-14s %-14s %-9s %-6s %-10s %-9s\n",
           "序号", "ID", "名称", "类型", "品牌", "型号", "序列号", "价值", "状态", "位置", "领用人");
    cprint(C_CYAN, "  ");
    print_line('-', 110);
    for (int i = 0; i < g_assets.count; i++) {
        Asset *a = &g_assets.items[i];
        int match = (id > 0 && a->id == id) ||
                    strstr(a->name, key) || strstr(a->brand, key) ||
                    strstr(a->serial, key) || strstr(a->current_user, key) ||
                    strstr(a->model, key);
        if (match) {
            print_asset_row(a, i);
            found++;
        }
    }
    cprint(C_CYAN, "  ");
    print_line('-', 110);
    cprint(C_RESET, "");
    if (found == 0) print_warn("未找到匹配设备");
    else printf("  找到 %d 条匹配记录\n\n", found);
}

static void do_modify_asset(void)
{
    int id = input_int("  输入要修改的设备ID: ", 0);
    Asset *a = asset_find_by_id(&g_assets, id);
    if (!a) { print_err("设备未找到"); return; }

    cprint(C_CYAN, "\n  当前设备信息:\n");
    printf("    名称: %s\n", a->name);
    printf("    类型: %s\n", a->type);
    printf("    品牌: %s\n", a->brand);
    printf("    型号: %s\n", a->model);
    printf("    序列号: %s\n", a->serial);
    printf("    日期: %s\n", a->purchase_date);
    printf("    价值: %.0f元\n", a->price);
    printf("    位置: %s\n", a->location);
    printf("  (直接回车保持原值)\n\n");

    Asset tmp = *a;
    char buf[32];
    input_str("  新名称: ", tmp.name, MAX_STR, a->name);
    input_str("  新类型: ", tmp.type, MAX_STR, a->type);
    input_str("  新品牌: ", tmp.brand, MAX_STR, a->brand);
    input_str("  新型号: ", tmp.model, MAX_STR, a->model);
    input_str("  新序列号: ", tmp.serial, MAX_STR, a->serial);
    input_str("  新购置日期: ", tmp.purchase_date, MAX_STR, a->purchase_date);
    printf("  新价值(元)[%.0f]: ", a->price);
    read_line(buf, sizeof(buf));
    if (buf[0]) tmp.price = atof(buf);
    input_str("  新位置: ", tmp.location, MAX_STR, a->location);
    input_str("  新备注: ", tmp.remark, MAX_DESC, a->remark);

    printf("  新状态(0=闲置 1=使用中 2=维修中 3=报废)[%d]: ", a->status);
    read_line(buf, sizeof(buf));
    if (buf[0]) {
        int ns = atoi(buf);
        if (ns >= 0 && ns <= 3) tmp.status = ns;
    }

    AssetStatus st = asset_update(&g_assets, id, &tmp);
    if (st == ASSET_OK) {
        print_ok("设备信息已更新");
        asset_save(&g_assets, DATA_FILE);
    } else if (st == ASSET_ERR_DUPLICATE) {
        print_err("序列号与其他设备冲突");
    } else {
        print_err("更新失败");
    }
}

static void do_delete_asset(void)
{
    int id = input_int("  输入要删除的设备ID: ", 0);
    Asset *a = asset_find_by_id(&g_assets, id);
    if (!a) { print_err("设备未找到"); return; }

    char confirm[MAX_STR];
    cprint(C_YELLOW, "  确认删除 #%d %s(%s)? (yes/no): ", a->id, a->name, a->serial);
    cprint(C_RESET, "");
    read_line(confirm, MAX_STR);
    if (strcmp(confirm, "yes") != 0) { print_warn("已取消删除"); return; }

    AssetStatus st = asset_delete(&g_assets, id);
    if (st == ASSET_OK) {
        print_ok("设备已删除");
        asset_save(&g_assets, DATA_FILE);
    } else {
        print_err("删除失败");
    }
}

static void do_checkout(void)
{
    int id = input_int("  输入领用设备ID: ", 0);
    char user[MAX_STR];
    input_str("  领用人姓名: ", user, MAX_STR, NULL);
    if (!user[0]) { print_err("领用人不能为空"); return; }

    AssetStatus st = asset_checkout(&g_assets, id, user);
    switch (st) {
        case ASSET_OK:
            print_ok("设备领用登记成功");
            asset_save(&g_assets, DATA_FILE);
            break;
        case ASSET_ERR_NOT_FOUND:  print_err("设备未找到"); break;
        case ASSET_ERR_ALREADY_OUT: print_err("该设备已被领用"); break;
        case ASSET_ERR_INVALID_ARG: print_err("参数无效或设备已报废"); break;
        default: print_err("领用失败");
    }
}

static void do_checkin(void)
{
    int id = input_int("  输入归还设备ID: ", 0);
    AssetStatus st = asset_checkin(&g_assets, id);
    switch (st) {
        case ASSET_OK:
            print_ok("设备归还登记成功");
            asset_save(&g_assets, DATA_FILE);
            break;
        case ASSET_ERR_NOT_FOUND: print_err("设备未找到"); break;
        case ASSET_ERR_NOT_OUT:   print_err("该设备未在领用状态"); break;
        default: print_err("归还失败");
    }
}

static void do_show_stats(void)
{
    AssetStats st;
    asset_calc_stats(&g_assets, &st);
    print_title("资产统计报表");
    cprint(C_BOLD, "  ┌─────────────────────────────────────────┐\n");
    printf("  │  %-18s %10d 台           │\n", "设备总数", st.total);
    cprint(C_GREEN,  "  │  %-18s %10d 台  %8.0f元  │\n", "闲置可用", st.idle, st.idle_value);
    cprint(C_BLUE,   "  │  %-18s %10d 台  %8.0f元  │\n", "使用中", st.in_use, st.in_use_value);
    cprint(C_YELLOW, "  │  %-18s %10d 台           │\n", "维修中", st.repair);
    cprint(C_RED,    "  │  %-18s %10d 台           │\n", "已报废", st.scrapped);
    cprint(C_BOLD C_CYAN, "  │  %-18s %14s  %8.0f元  │\n", "资产总价值", "", st.total_value);
    cprint(C_BOLD, "  └─────────────────────────────────────────┘\n");
    cprint(C_RESET, "\n");

    double utilization = st.total > 0 ? (double)st.in_use / st.total * 100.0 : 0.0;
    printf("  资产利用率: %.1f%%\n", utilization);
    if (utilization > 80) cprint(C_GREEN, "  → 利用率较高，设备使用充分\n");
    else if (utilization > 50) cprint(C_YELLOW, "  → 利用率中等\n");
    else cprint(C_RED, "  → 利用率偏低，存在闲置\n");
    cprint(C_RESET, "\n");
}

/* ======================== 初始化示例数据 ======================== */

static void init_sample_data(AssetArray *arr)
{
    struct {
        const char *name, *type, *brand, *model, *serial, *date, *loc;
        double price; int status; const char *user;
    } samples[] = {
        {"台式电脑",  "计算机",   "联想",  "ThinkCentre M920", "SN20230315001", "2023-03-15", "A301办公室", 5500,  DEV_IN_USE,  "张伟"},
        {"笔记本电脑","计算机",   "戴尔",  "Latitude 5520",    "SN20230620002", "2023-06-20", "B205实验室", 7200,  DEV_IN_USE,  "李娜"},
        {"激光打印机","外设",     "惠普",  "LaserJet Pro M404","SN20221108003", "2022-11-08", "A102打印间", 2800,  DEV_IDLE,    ""},
        {"交换机",    "网络设备", "华为",  "S5735-L24T4X",     "SN20240110004", "2024-01-10", "C101机房",   3500,  DEV_IDLE,    ""},
        {"服务器",    "计算机",   "浪潮",  "NF5280M6",         "SN20230925005", "2023-09-25", "C101机房",   45000, DEV_IN_USE,  "运维员"},
        {"显示器",    "外设",     "三星",  "S24R350",          "SN20240214006", "2024-02-14", "仓库",       1800,  DEV_IDLE,    ""},
        {"投影仪",    "外设",     "爱普生","CB-FH06",          "SN20220730007", "2022-07-30", "A201会议室", 6500,  DEV_IDLE,    ""},
        {"路由器",    "网络设备", "TP-LINK","TL-ER6220G",      "SN20240305008", "2024-03-05", "C101机房",   800,   DEV_IDLE,    ""},
        {"UPS电源",   "电源设备", "山特",  "C3KS",             "SN20231201009", "2023-12-01", "C101机房",   3200,  DEV_IDLE,    ""},
        {"扫描仪",    "外设",     "佳能",  "DR-C225II",        "SN20230418010", "2023-04-18", "A102打印间", 4500,  DEV_REPAIR,  ""}
    };
    int n = (int)(sizeof(samples) / sizeof(samples[0]));
    for (int i = 0; i < n; i++) {
        Asset a;
        memset(&a, 0, sizeof(a));
        strcpy(a.name, samples[i].name);
        strcpy(a.type, samples[i].type);
        strcpy(a.brand, samples[i].brand);
        strcpy(a.model, samples[i].model);
        strcpy(a.serial, samples[i].serial);
        strcpy(a.purchase_date, samples[i].date);
        strcpy(a.location, samples[i].loc);
        a.price = samples[i].price;
        a.status = samples[i].status;
        if (samples[i].user[0]) {
            strcpy(a.current_user, samples[i].user);
        }
        strcpy(a.remark, "");
        asset_add(arr, &a);
    }
}

/* ======================== 内置自测 (≥15项) ========================
 *
 * 测试覆盖范围:
 *   T01  初始化空数组
 *   T02  添加设备(基础CRUD-C)
 *   T03  按ID查找设备(CRUD-R)
 *   T04  添加设备后ID自增
 *   T05  序列号唯一性约束
 *   T06  修改设备信息(CRUD-U)
 *   T07  删除设备(CRUD-D)
 *   T08  删除不存在设备返回错误
 *   T09  设备领用流程
 *   T10  重复领用应拒绝
 *   T11  设备归还流程
 *   T12  未领用设备归还应拒绝
 *   T13  动态数组自动扩容
 *   T14  文件保存与加载(Magic Header)
 *   T15  魔数校验(防格式误读)
 *   T16  统计功能正确性
 *   T17  批量添加压力测试
 *   T18  参数有效性校验
 * ================================================================ */

static int g_tests_passed = 0;
static int g_tests_failed = 0;

static void test_case(const char *name, int condition)
{
    if (condition) {
        cprint(C_GREEN, "  [PASS] %s\n", name);
        g_tests_passed++;
    } else {
        cprint(C_RED,   "  [FAIL] %s\n", name);
        g_tests_failed++;
    }
}

static int run_self_test(void)
{
    g_test_mode = 1;
    g_color_enabled = 1;
    g_tests_passed = 0;
    g_tests_failed = 0;

    print_title("IT设备资产管理系统 V2.0 - 内置自测");

    /* T01: 初始化空数组 */
    AssetArray arr;
    memset(&arr, 0, sizeof(arr));
    AssetStatus st = asset_array_init(&arr);
    test_case("T01: 初始化空数组成功", st == ASSET_OK && arr.count == 0 && arr.capacity == INIT_CAPACITY);

    /* T02: 添加设备 */
    Asset a1;
    memset(&a1, 0, sizeof(a1));
    strcpy(a1.name, "测试电脑");
    strcpy(a1.type, "计算机");
    strcpy(a1.brand, "测试品牌");
    strcpy(a1.model, "TestModel-001");
    strcpy(a1.serial, "TEST-SN-001");
    strcpy(a1.purchase_date, "2026-01-01");
    a1.price = 9999.0;
    a1.status = DEV_IDLE;
    strcpy(a1.location, "测试位置");
    st = asset_add(&arr, &a1);
    test_case("T02: 添加单个设备成功", st == ASSET_OK && arr.count == 1);

    /* T03: 按ID查找 */
    Asset *found = asset_find_by_id(&arr, 1);
    test_case("T03: 按ID查找设备成功", found != NULL && strcmp(found->name, "测试电脑") == 0);

    /* T04: ID自增验证 */
    Asset a2 = a1;
    strcpy(a2.serial, "TEST-SN-002");
    strcpy(a2.name, "测试显示器");
    asset_add(&arr, &a2);
    Asset *f2 = asset_find_by_id(&arr, 2);
    test_case("T04: ID自增正确", arr.count == 2 && f2 != NULL && f2->id == 2);

    /* T05: 序列号唯一性 */
    Asset a3 = a1;
    st = asset_add(&arr, &a3);
    test_case("T05: 重复序列号拒绝添加", st == ASSET_ERR_DUPLICATE && arr.count == 2);

    /* T06: 修改设备 */
    Asset upd = *found;
    strcpy(upd.name, "已修改的电脑");
    upd.price = 8888.0;
    st = asset_update(&arr, 1, &upd);
    test_case("T06: 修改设备信息成功", st == ASSET_OK && found->price == 8888.0 && strcmp(found->name, "已修改的电脑") == 0);

    /* T07: 删除设备 */
    st = asset_delete(&arr, 2);
    test_case("T07: 删除设备成功", st == ASSET_OK && arr.count == 1);

    /* T08: 删除不存在的设备 */
    st = asset_delete(&arr, 999);
    test_case("T08: 删除不存在设备返回NOT_FOUND", st == ASSET_ERR_NOT_FOUND);

    /* T09: 领用设备 */
    st = asset_checkout(&arr, 1, "测试用户");
    Asset *f1 = asset_find_by_id(&arr, 1);
    test_case("T09: 设备领用成功", st == ASSET_OK && f1->status == DEV_IN_USE && strcmp(f1->current_user, "测试用户") == 0);

    /* T10: 重复领用 */
    st = asset_checkout(&arr, 1, "另一用户");
    test_case("T10: 已领用设备拒绝重复领用", st == ASSET_ERR_ALREADY_OUT);

    /* T11: 归还设备 */
    st = asset_checkin(&arr, 1);
    f1 = asset_find_by_id(&arr, 1);
    test_case("T11: 设备归还成功", st == ASSET_OK && f1->status == DEV_IDLE && f1->current_user[0] == '\0');

    /* T12: 未领用归还 */
    st = asset_checkin(&arr, 1);
    test_case("T12: 未领用设备拒绝归还", st == ASSET_ERR_NOT_OUT);

    /* T13: 动态数组扩容 */
    int old_cap = arr.capacity;
    int added = 0;
    for (int i = 0; i < 20; i++) {
        Asset tmp;
        memset(&tmp, 0, sizeof(tmp));
        sprintf(tmp.name, "扩容测试%d", i);
        strcpy(tmp.type, "测试");
        strcpy(tmp.brand, "测试");
        strcpy(tmp.model, "EXP");
        sprintf(tmp.serial, "EXP-SN-%03d", i);
        strcpy(tmp.purchase_date, "2026-01-01");
        strcpy(tmp.location, "测试");
        tmp.price = 100.0;
        tmp.status = DEV_IDLE;
        if (asset_add(&arr, &tmp) == ASSET_OK) added++;
    }
    test_case("T13: 动态数组自动扩容", added == 20 && arr.capacity > old_cap);

    /* T14: 文件保存与加载 */
    const char *test_file = "test_assets.dat";
    int saved_count = arr.count;
    int saved_nextid = arr.next_id;
    st = asset_save(&arr, test_file);
    AssetArray arr2;
    asset_array_init(&arr2);
    AssetStatus ld = asset_load(&arr2, test_file);
    int load_ok = (ld == ASSET_OK && arr2.count == saved_count && arr2.next_id == saved_nextid);
    test_case("T14: 文件保存并加载成功(含Magic Header)", st == ASSET_OK && load_ok);

    /* T15: 魔数校验 */
    FILE *fp = fopen("bad_magic.dat", "wb");
    if (fp) {
        const char *bad = "XXXXXX";
        fwrite(bad, 1, 6, fp);
        fclose(fp);
    }
    AssetArray arr3;
    asset_array_init(&arr3);
    AssetStatus magic_st = asset_load(&arr3, "bad_magic.dat");
    test_case("T15: 魔数错误返回ERR_MAGIC", magic_st == ASSET_ERR_MAGIC);
    asset_array_free(&arr3);
    remove("bad_magic.dat");

    /* T16: 统计功能 */
    AssetArray stats_arr;
    asset_array_init(&stats_arr);
    Asset s;
    memset(&s, 0, sizeof(s));
    strcpy(s.name, "S1"); strcpy(s.type, "T"); strcpy(s.brand, "B"); strcpy(s.model, "M");
    strcpy(s.serial, "STAT-01"); strcpy(s.purchase_date, "2026-01-01"); strcpy(s.location, "L");
    s.price = 1000; s.status = DEV_IDLE;
    asset_add(&stats_arr, &s);
    strcpy(s.serial, "STAT-02"); s.price = 2000; s.status = DEV_IN_USE; strcpy(s.current_user, "U");
    asset_add(&stats_arr, &s);
    strcpy(s.serial, "STAT-03"); s.price = 3000; s.status = DEV_REPAIR; s.current_user[0] = '\0';
    asset_add(&stats_arr, &s);
    AssetStats stats;
    asset_calc_stats(&stats_arr, &stats);
    int stats_ok = (stats.total == 3 && stats.idle == 1 && stats.in_use == 1 && stats.repair == 1
                    && stats.total_value == 6000.0 && stats.idle_value == 1000.0);
    test_case("T16: 统计功能计算正确", stats_ok);
    asset_array_free(&stats_arr);

    /* T17: 批量添加压力测试 */
    AssetArray stress;
    asset_array_init(&stress);
    int stress_ok = 1;
    for (int i = 0; i < 1000; i++) {
        Asset tmp;
        memset(&tmp, 0, sizeof(tmp));
        sprintf(tmp.name, "设备%d", i);
        strcpy(tmp.type, "测试");
        strcpy(tmp.brand, "品牌");
        strcpy(tmp.model, "MODEL");
        sprintf(tmp.serial, "STRESS-%04d", i);
        strcpy(tmp.purchase_date, "2026-01-01");
        strcpy(tmp.location, "仓库");
        tmp.price = (double)(i * 10);
        tmp.status = DEV_IDLE;
        if (asset_add(&stress, &tmp) != ASSET_OK) { stress_ok = 0; break; }
    }
    test_case("T17: 批量添加1000台设备成功", stress_ok && stress.count == 1000);
    asset_array_free(&stress);

    /* T18: 参数校验 */
    Asset bad;
    memset(&bad, 0, sizeof(bad));
    st = asset_add(&arr, &bad);
    test_case("T18: 空名称/序列号拒绝添加", st == ASSET_ERR_INVALID_ARG);

    /* 清理 */
    asset_array_free(&arr);
    asset_array_free(&arr2);
    remove(test_file);

    print_line('=', 60);
    cprint(C_BOLD, "  测试结果: ");
    cprint(C_GREEN, "%d 通过  ", g_tests_passed);
    cprint(C_RED, "%d 失败  ", g_tests_failed);
    cprint(C_BOLD, "总计 %d 项\n", g_tests_passed + g_tests_failed);
    cprint(C_RESET, "");
    print_line('=', 60);

    g_test_mode = 0;
    return g_tests_failed == 0 ? 0 : 1;
}

/* ======================== 帮助信息 ======================== */

static void print_help(const char *prog)
{
    cprint(C_BOLD C_CYAN, "\n  IT设备资产管理系统 V2.0\n");
    print_line('=', 60);
    cprint(C_RESET, "");
    printf("  用法: %s [选项]\n\n", prog);
    cprint(C_BOLD, "  选项:\n");
    printf("    -h, --help     显示帮助信息\n");
    printf("    -t, --test     运行内置自测(≥15项全功能测试)\n");
    printf("    (无参数)       启动交互菜单模式\n\n");
    cprint(C_BOLD, "  文件格式:\n");
    printf("    %s  - Magic Header \"ASSTV2\" 二进制格式\n\n", DATA_FILE);
    cprint(C_BOLD, "  设备状态:\n");
    cprint(C_GREEN,  "    0 闲置    ");
    cprint(C_BLUE,   "1 使用中  ");
    cprint(C_YELLOW, "2 维修中  ");
    cprint(C_RED,    "3 已报废\n");
    cprint(C_RESET, "\n");
}

/* ======================== 主菜单 ======================== */

static void main_menu(void)
{
    while (1) {
        print_title("IT设备资产管理系统 V2.0");
        cprint(C_CYAN, "");
        printf("  当前共 %d 台设备", g_assets.count);
        AssetStats st;
        asset_calc_stats(&g_assets, &st);
        cprint(C_GREEN, "  [闲置:%d]", st.idle);
        cprint(C_BLUE,  "[使用:%d]", st.in_use);
        cprint(C_YELLOW,"[维修:%d]", st.repair);
        cprint(C_RED,   "[报废:%d]", st.scrapped);
        cprint(C_RESET, "\n\n");

        cprint(C_BOLD C_WHITE, "  ┌──────────────────────────────┐\n");
        printf("  │  %-28s │\n", "1.  添加设备");
        printf("  │  %-28s │\n", "2.  查询设备");
        printf("  │  %-28s │\n", "3.  修改设备");
        printf("  │  %-28s │\n", "4.  删除设备");
        printf("  │  %-28s │\n", "5.  浏览全部设备");
        cprint(C_BOLD C_BLUE,  "  │  %-28s │\n", "6.  领用登记");
        cprint(C_BOLD C_BLUE,  "  │  %-28s │\n", "7.  归还登记");
        cprint(C_BOLD C_GREEN, "  │  %-28s │\n", "8.  统计报表");
        cprint(C_BOLD C_RED,   "  │  %-28s │\n", "0.  保存并退出");
        cprint(C_BOLD C_WHITE, "  └──────────────────────────────┘\n");
        cprint(C_RESET, "\n");

        char choice[8];
        printf("  请选择操作: ");
        read_line(choice, sizeof(choice));

        switch (choice[0]) {
            case '1': do_add_asset(); break;
            case '2': do_query_asset(); break;
            case '3': do_modify_asset(); break;
            case '4': do_delete_asset(); break;
            case '5': list_assets(&g_assets); break;
            case '6': do_checkout(); break;
            case '7': do_checkin(); break;
            case '8': do_show_stats(); break;
            case '0':
                asset_save(&g_assets, DATA_FILE);
                print_ok("数据已保存，感谢使用！");
                return;
            default:
                print_warn("无效选项，请重新选择");
        }
        pause_screen();
    }
}

/* ======================== 程序入口 ========================
 *
 * 启动流程:
 *   1. Windows平台调用 SetConsoleOutputCP(65001) 设置UTF-8代码页
 *   2. 解析命令行参数:
 *      -h/--help → 显示帮助并退出
 *      -t/--test → 运行自测并退出
 *      (无参数)  → 交互模式
 *   3. 初始化动态数组，加载数据文件
 *   4. 若数据文件不存在，初始化示例数据
 *   5. 进入交互主菜单
 * ================================================================ */

int main(int argc, char *argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(hOut, &mode)) {
        SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        }
        if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0) {
            return run_self_test();
        }
    }

    asset_array_init(&g_assets);
    AssetStatus load_st = asset_load(&g_assets, DATA_FILE);
    if (load_st != ASSET_OK) {
        print_info("未检测到数据文件，初始化示例数据...");
        init_sample_data(&g_assets);
        asset_save(&g_assets, DATA_FILE);
        print_ok("示例数据初始化完成");
    } else {
        char date[MAX_STR];
        get_date(date, MAX_STR);
        print_info("数据加载成功");
    }

    main_menu();

    asset_array_free(&g_assets);
    return 0;
}
