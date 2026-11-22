/*
 * ============================================================================
 *
 *                             仓库管理信息系统 V2.0
 *                         Warehouse Management System
 *
 * ============================================================================
 *
 *  项目名称  : 仓库管理信息系统升级到V2.0标准 - 数据结构课程设计
 *  主要功能  : 商品CRUD、入库管理、出库管理、库存统计、库存预警、文件持久化
 *  设计思路  : 动态数组存储 + 二进制文件持久化(magic header "WAREV2")
 *  数据结构  : 商品动态数组 / 入库记录动态数组 / 出库记录动态数组
 *  编译方式  : gcc -std=c99 -Wall -Wextra -O2 -o warehouse.exe main.c
 *  作    者  : 数据结构课程
 *  版    本  : V2.0 (2026-06)
 *
 * ============================================================================
 *
 * ======================== V2.0 新特性 ========================
 *
 *  1. UTF-8 BOM 编码，支持中文显示
 *  2. Windows 控制台 SetConsoleOutputCP(65001) 设置UTF-8输出
 *  3. ANSI 彩色终端输出（彩色菜单/成功/错误提示）
 *  4. CLI 命令行参数：-h 帮助，-t/--test 运行内置自测
 *  5. 动态内存：商品、入库记录、出库记录全部使用动态数组
 *  6. 统一错误码枚举 WareStatus，所有函数返回状态码
 *  7. 二进制文件持久化，magic header "WAREV2" 校验
 *  8. 内置15+项自测：CRUD + 出入库 + 库存统计 + 文件I/O
 *  9. 交互式菜单系统，操作直观
 * 10. 完整中文注释
 *
 * ============================================================================
 */

#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ======================== ANSI 颜色定义 ======================== */

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"
#define COLOR_BOLD    "\033[1m"

/* ======================== 常量定义 ======================== */

#define MAX_NAME     64
#define MAX_SPEC     64
#define MAX_UNIT     16
#define MAX_DATE     16

#define INIT_CAP     16
#define GROW_FACTOR  2

#define FILE_DATA    "warehouse.dat"
#define MAGIC_HEADER "WAREV2"
#define MAGIC_LEN    6
#define FILE_VERSION 2

#define NEXT_ID_INIT 1001

/* ======================== 错误码枚举 ======================== */

typedef enum {
    WARE_OK = 0,
    WARE_ERR_MEMORY,
    WARE_ERR_NOT_FOUND,
    WARE_ERR_DUPLICATE,
    WARE_ERR_INVALID_PARAM,
    WARE_ERR_STOCK_INSUFFICIENT,
    WARE_ERR_FILE_OPEN,
    WARE_ERR_FILE_READ,
    WARE_ERR_FILE_WRITE,
    WARE_ERR_FILE_MAGIC,
    WARE_ERR_FILE_VERSION,
    WARE_ERR_IO,
    WARE_ERR_INPUT,
    WARE_ERR_EMPTY
} WareStatus;

/* ======================== 数据结构定义 ======================== */

typedef struct {
    int  id;
    char name[MAX_NAME];
    char spec[MAX_SPEC];
    char unit[MAX_UNIT];
    int  stock;
} Material;

typedef struct {
    int  mat_id;
    char name[MAX_NAME];
    int  quantity;
    char date[MAX_DATE];
} InRecord;

typedef struct {
    int  mat_id;
    char name[MAX_NAME];
    int  quantity;
    char date[MAX_DATE];
} OutRecord;

typedef struct {
    void  *data;
    size_t count;
    size_t capacity;
    size_t elem_size;
} DynArray;

typedef struct {
    DynArray materials;
    DynArray inbound;
    DynArray outbound;
    int      next_id;
} Warehouse;

/* ======================== 全局实例 ======================== */

static Warehouse g_ware;

/* ======================== 工具函数：控制台 ======================== */

static void console_init(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif
}

static void clear_input(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

static int safe_read_int(int *value)
{
    if (scanf("%d", value) != 1) {
        clear_input();
        return -1;
    }
    clear_input();
    return 0;
}

static int safe_read_string(char *buf, size_t max_len)
{
    if (fgets(buf, (int)max_len, stdin) == NULL)
        return -1;
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n')
        buf[len - 1] = '\0';
    return 0;
}

static void get_current_date(char *buf, size_t buf_size)
{
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buf, buf_size, "%Y-%m-%d", tm_info);
}

static void print_line(char ch, int len, const char *color)
{
    printf("%s", color);
    for (int i = 0; i < len; i++) putchar(ch);
    printf("%s\n", COLOR_RESET);
}

static void print_title(const char *title, int width, const char *color)
{
    int len = (int)strlen(title);
    int pad = (width - len) / 2;
    if (pad < 0) pad = 0;
    printf("%s", color);
    for (int i = 0; i < pad; i++) putchar(' ');
    printf("%s%s\n", title, COLOR_RESET);
}

static void print_success(const char *msg)
{
    printf("  %s[√]%s %s\n", COLOR_GREEN, COLOR_RESET, msg);
}

static void print_error(const char *msg)
{
    printf("  %s[×]%s %s\n", COLOR_RED, COLOR_RESET, msg);
}

static void print_info(const char *msg)
{
    printf("  %s[i]%s %s\n", COLOR_CYAN, COLOR_RESET, msg);
}

static void print_warning(const char *msg)
{
    printf("  %s[!]%s %s\n", COLOR_YELLOW, COLOR_RESET, msg);
}

/* ======================== 动态数组实现 ======================== */

static WareStatus dynarray_init(DynArray *arr, size_t elem_size, size_t init_cap)
{
    arr->elem_size = elem_size;
    arr->count = 0;
    arr->capacity = init_cap;
    arr->data = malloc(elem_size * init_cap);
    if (!arr->data)
        return WARE_ERR_MEMORY;
    return WARE_OK;
}

static void dynarray_free(DynArray *arr)
{
    if (arr->data) {
        free(arr->data);
        arr->data = NULL;
    }
    arr->count = 0;
    arr->capacity = 0;
}

static WareStatus dynarray_ensure(DynArray *arr, size_t needed)
{
    if (arr->count + needed <= arr->capacity)
        return WARE_OK;

    size_t new_cap = arr->capacity;
    while (new_cap < arr->count + needed)
        new_cap *= GROW_FACTOR;

    void *new_data = realloc(arr->data, arr->elem_size * new_cap);
    if (!new_data)
        return WARE_ERR_MEMORY;

    arr->data = new_data;
    arr->capacity = new_cap;
    return WARE_OK;
}

static WareStatus dynarray_push(DynArray *arr, const void *elem)
{
    WareStatus st = dynarray_ensure(arr, 1);
    if (st != WARE_OK)
        return st;

    char *base = (char *)arr->data;
    memcpy(base + arr->count * arr->elem_size, elem, arr->elem_size);
    arr->count++;
    return WARE_OK;
}

static void *dynarray_get(DynArray *arr, size_t index)
{
    if (index >= arr->count)
        return NULL;
    char *base = (char *)arr->data;
    return base + index * arr->elem_size;
}

static WareStatus dynarray_remove(DynArray *arr, size_t index)
{
    if (index >= arr->count)
        return WARE_ERR_NOT_FOUND;

    char *base = (char *)arr->data;
    size_t move_count = arr->count - index - 1;
    if (move_count > 0) {
        memmove(base + index * arr->elem_size,
                base + (index + 1) * arr->elem_size,
                move_count * arr->elem_size);
    }
    arr->count--;
    return WARE_OK;
}

/* ======================== 仓库核心功能 ======================== */

static WareStatus ware_init(Warehouse *w)
{
    memset(w, 0, sizeof(Warehouse));
    w->next_id = NEXT_ID_INIT;

    WareStatus st;
    st = dynarray_init(&w->materials, sizeof(Material), INIT_CAP);
    if (st != WARE_OK) return st;

    st = dynarray_init(&w->inbound, sizeof(InRecord), INIT_CAP);
    if (st != WARE_OK) {
        dynarray_free(&w->materials);
        return st;
    }

    st = dynarray_init(&w->outbound, sizeof(OutRecord), INIT_CAP);
    if (st != WARE_OK) {
        dynarray_free(&w->materials);
        dynarray_free(&w->inbound);
        return st;
    }

    return WARE_OK;
}

static void ware_shutdown(Warehouse *w)
{
    dynarray_free(&w->materials);
    dynarray_free(&w->inbound);
    dynarray_free(&w->outbound);
    w->next_id = NEXT_ID_INIT;
}

static int ware_find_material_index(Warehouse *w, int id)
{
    for (size_t i = 0; i < w->materials.count; i++) {
        Material *m = (Material *)dynarray_get(&w->materials, i);
        if (m && m->id == id)
            return (int)i;
    }
    return -1;
}

static Material *ware_get_material(Warehouse *w, int id)
{
    int idx = ware_find_material_index(w, id);
    if (idx < 0) return NULL;
    return (Material *)dynarray_get(&w->materials, (size_t)idx);
}

static WareStatus ware_add_material(Warehouse *w, const char *name,
                                    const char *spec, const char *unit,
                                    int initial_stock, int *out_id)
{
    if (!name || !spec || !unit)
        return WARE_ERR_INVALID_PARAM;
    if (initial_stock < 0)
        return WARE_ERR_INVALID_PARAM;

    Material m;
    m.id = w->next_id++;
    strncpy(m.name, name, MAX_NAME - 1); m.name[MAX_NAME - 1] = '\0';
    strncpy(m.spec, spec, MAX_SPEC - 1); m.spec[MAX_SPEC - 1] = '\0';
    strncpy(m.unit, unit, MAX_UNIT - 1); m.unit[MAX_UNIT - 1] = '\0';
    m.stock = initial_stock;

    WareStatus st = dynarray_push(&w->materials, &m);
    if (st != WARE_OK) {
        w->next_id--;
        return st;
    }

    if (out_id)
        *out_id = m.id;
    return WARE_OK;
}

static WareStatus ware_delete_material(Warehouse *w, int id)
{
    int idx = ware_find_material_index(w, id);
    if (idx < 0)
        return WARE_ERR_NOT_FOUND;
    return dynarray_remove(&w->materials, (size_t)idx);
}

static WareStatus ware_modify_material(Warehouse *w, int id,
                                       const char *name, const char *spec,
                                       const char *unit)
{
    Material *m = ware_get_material(w, id);
    if (!m)
        return WARE_ERR_NOT_FOUND;

    if (name && name[0] != '\0' && strcmp(name, "0") != 0) {
        strncpy(m->name, name, MAX_NAME - 1);
        m->name[MAX_NAME - 1] = '\0';
    }
    if (spec && spec[0] != '\0' && strcmp(spec, "0") != 0) {
        strncpy(m->spec, spec, MAX_SPEC - 1);
        m->spec[MAX_SPEC - 1] = '\0';
    }
    if (unit && unit[0] != '\0' && strcmp(unit, "0") != 0) {
        strncpy(m->unit, unit, MAX_UNIT - 1);
        m->unit[MAX_UNIT - 1] = '\0';
    }
    return WARE_OK;
}

static WareStatus ware_inbound(Warehouse *w, int mat_id, int quantity,
                                const char *date)
{
    if (quantity <= 0)
        return WARE_ERR_INVALID_PARAM;

    Material *m = ware_get_material(w, mat_id);
    if (!m)
        return WARE_ERR_NOT_FOUND;

    m->stock += quantity;

    InRecord rec;
    rec.mat_id = mat_id;
    strncpy(rec.name, m->name, MAX_NAME - 1); rec.name[MAX_NAME - 1] = '\0';
    rec.quantity = quantity;
    if (date && date[0] != '\0') {
        strncpy(rec.date, date, MAX_DATE - 1); rec.date[MAX_DATE - 1] = '\0';
    } else {
        get_current_date(rec.date, sizeof(rec.date));
    }

    return dynarray_push(&w->inbound, &rec);
}

static WareStatus ware_outbound(Warehouse *w, int mat_id, int quantity,
                                 const char *date)
{
    if (quantity <= 0)
        return WARE_ERR_INVALID_PARAM;

    Material *m = ware_get_material(w, mat_id);
    if (!m)
        return WARE_ERR_NOT_FOUND;
    if (m->stock < quantity)
        return WARE_ERR_STOCK_INSUFFICIENT;

    m->stock -= quantity;

    OutRecord rec;
    rec.mat_id = mat_id;
    strncpy(rec.name, m->name, MAX_NAME - 1); rec.name[MAX_NAME - 1] = '\0';
    rec.quantity = quantity;
    if (date && date[0] != '\0') {
        strncpy(rec.date, date, MAX_DATE - 1); rec.date[MAX_DATE - 1] = '\0';
    } else {
        get_current_date(rec.date, sizeof(rec.date));
    }

    return dynarray_push(&w->outbound, &rec);
}

static int ware_total_stock(Warehouse *w)
{
    int total = 0;
    for (size_t i = 0; i < w->materials.count; i++) {
        Material *m = (Material *)dynarray_get(&w->materials, i);
        if (m) total += m->stock;
    }
    return total;
}

static size_t ware_material_count(Warehouse *w)
{
    return w->materials.count;
}

/* ======================== 文件持久化 ======================== */

static WareStatus ware_save(Warehouse *w, const char *filename)
{
    FILE *fp = fopen(filename, "wb");
    if (!fp)
        return WARE_ERR_FILE_OPEN;

    if (fwrite(MAGIC_HEADER, 1, MAGIC_LEN, fp) != MAGIC_LEN) {
        fclose(fp); return WARE_ERR_FILE_WRITE;
    }

    int version = FILE_VERSION;
    if (fwrite(&version, sizeof(int), 1, fp) != 1) {
        fclose(fp); return WARE_ERR_FILE_WRITE;
    }

    if (fwrite(&w->next_id, sizeof(int), 1, fp) != 1) {
        fclose(fp); return WARE_ERR_FILE_WRITE;
    }

    size_t mat_count = w->materials.count;
    if (fwrite(&mat_count, sizeof(size_t), 1, fp) != 1) {
        fclose(fp); return WARE_ERR_FILE_WRITE;
    }
    for (size_t i = 0; i < mat_count; i++) {
        Material *m = (Material *)dynarray_get(&w->materials, i);
        if (fwrite(m, sizeof(Material), 1, fp) != 1) {
            fclose(fp); return WARE_ERR_FILE_WRITE;
        }
    }

    size_t in_count = w->inbound.count;
    if (fwrite(&in_count, sizeof(size_t), 1, fp) != 1) {
        fclose(fp); return WARE_ERR_FILE_WRITE;
    }
    for (size_t i = 0; i < in_count; i++) {
        InRecord *r = (InRecord *)dynarray_get(&w->inbound, i);
        if (fwrite(r, sizeof(InRecord), 1, fp) != 1) {
            fclose(fp); return WARE_ERR_FILE_WRITE;
        }
    }

    size_t out_count = w->outbound.count;
    if (fwrite(&out_count, sizeof(size_t), 1, fp) != 1) {
        fclose(fp); return WARE_ERR_FILE_WRITE;
    }
    for (size_t i = 0; i < out_count; i++) {
        OutRecord *r = (OutRecord *)dynarray_get(&w->outbound, i);
        if (fwrite(r, sizeof(OutRecord), 1, fp) != 1) {
            fclose(fp); return WARE_ERR_FILE_WRITE;
        }
    }

    fclose(fp);
    return WARE_OK;
}

static WareStatus ware_load(Warehouse *w, const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp)
        return WARE_ERR_FILE_OPEN;

    char magic[MAGIC_LEN + 1] = {0};
    if (fread(magic, 1, MAGIC_LEN, fp) != MAGIC_LEN) {
        fclose(fp); return WARE_ERR_FILE_READ;
    }
    if (memcmp(magic, MAGIC_HEADER, MAGIC_LEN) != 0) {
        fclose(fp); return WARE_ERR_FILE_MAGIC;
    }

    int version;
    if (fread(&version, sizeof(int), 1, fp) != 1) {
        fclose(fp); return WARE_ERR_FILE_READ;
    }
    if (version != FILE_VERSION) {
        fclose(fp); return WARE_ERR_FILE_VERSION;
    }

    int saved_next_id;
    if (fread(&saved_next_id, sizeof(int), 1, fp) != 1) {
        fclose(fp); return WARE_ERR_FILE_READ;
    }

    ware_shutdown(w);
    WareStatus st = ware_init(w);
    if (st != WARE_OK) { fclose(fp); return st; }
    w->next_id = saved_next_id;

    size_t mat_count;
    if (fread(&mat_count, sizeof(size_t), 1, fp) != 1) {
        fclose(fp); return WARE_ERR_FILE_READ;
    }
    for (size_t i = 0; i < mat_count; i++) {
        Material m;
        if (fread(&m, sizeof(Material), 1, fp) != 1) {
            fclose(fp); return WARE_ERR_FILE_READ;
        }
        dynarray_push(&w->materials, &m);
    }

    size_t in_count;
    if (fread(&in_count, sizeof(size_t), 1, fp) != 1) {
        fclose(fp); return WARE_ERR_FILE_READ;
    }
    for (size_t i = 0; i < in_count; i++) {
        InRecord r;
        if (fread(&r, sizeof(InRecord), 1, fp) != 1) {
            fclose(fp); return WARE_ERR_FILE_READ;
        }
        dynarray_push(&w->inbound, &r);
    }

    size_t out_count;
    if (fread(&out_count, sizeof(size_t), 1, fp) != 1) {
        fclose(fp); return WARE_ERR_FILE_READ;
    }
    for (size_t i = 0; i < out_count; i++) {
        OutRecord r;
        if (fread(&r, sizeof(OutRecord), 1, fp) != 1) {
            fclose(fp); return WARE_ERR_FILE_READ;
        }
        dynarray_push(&w->outbound, &r);
    }

    fclose(fp);
    return WARE_OK;
}

/* ======================== 显示函数 ======================== */

static void print_material_header(void)
{
    print_line('-', 64, COLOR_CYAN);
    printf("| %-6s | %-14s | %-14s | %-6s | %-8s |\n",
           "ID", "名称", "规格", "单位", "库存");
    print_line('-', 64, COLOR_CYAN);
}

static void print_material_row(const Material *m, int highlight)
{
    if (highlight) {
        printf("| %s%-6d%s | %s%-14s%s | %s%-14s%s | %s%-6s%s | %s%-8d%s |\n",
               COLOR_YELLOW, m->id, COLOR_RESET,
               COLOR_YELLOW, m->name, COLOR_RESET,
               COLOR_YELLOW, m->spec, COLOR_RESET,
               COLOR_YELLOW, m->unit, COLOR_RESET,
               COLOR_GREEN, m->stock, COLOR_RESET);
    } else {
        printf("| %-6d | %-14s | %-14s | %-6s | %-8d |\n",
               m->id, m->name, m->spec, m->unit, m->stock);
    }
}

static void list_materials(Warehouse *w)
{
    print_material_header();
    if (w->materials.count == 0) {
        printf("|%47s|\n", " 仓库空空如也~");
    } else {
        for (size_t i = 0; i < w->materials.count; i++) {
            Material *m = (Material *)dynarray_get(&w->materials, i);
            if (m) print_material_row(m, 0);
        }
    }
    print_line('-', 64, COLOR_CYAN);
    printf("  共 %s%zu%s 种商品，总库存 %s%d%s 件\n",
           COLOR_GREEN, w->materials.count, COLOR_RESET,
           COLOR_GREEN, ware_total_stock(w), COLOR_RESET);
}

static void list_inbound(Warehouse *w)
{
    print_line('-', 58, COLOR_CYAN);
    printf("| %-6s | %-14s | %-8s | %-12s |\n",
           "ID", "名称", "入库量", "日期");
    print_line('-', 58, COLOR_CYAN);
    if (w->inbound.count == 0) {
        printf("|%39s|\n", " 暂无入库记录");
    } else {
        for (size_t i = 0; i < w->inbound.count; i++) {
            InRecord *r = (InRecord *)dynarray_get(&w->inbound, i);
            if (r) {
                printf("| %-6d | %-14s | %-8d | %-12s |\n",
                       r->mat_id, r->name, r->quantity, r->date);
            }
        }
    }
    print_line('-', 58, COLOR_CYAN);
    printf("  共 %s%zu%s 条入库记录\n",
           COLOR_GREEN, w->inbound.count, COLOR_RESET);
}

static void list_outbound(Warehouse *w)
{
    print_line('-', 58, COLOR_CYAN);
    printf("| %-6s | %-14s | %-8s | %-12s |\n",
           "ID", "名称", "出库量", "日期");
    print_line('-', 58, COLOR_CYAN);
    if (w->outbound.count == 0) {
        printf("|%39s|\n", " 暂无出库记录");
    } else {
        for (size_t i = 0; i < w->outbound.count; i++) {
            OutRecord *r = (OutRecord *)dynarray_get(&w->outbound, i);
            if (r) {
                printf("| %-6d | %-14s | %-8d | %-12s |\n",
                       r->mat_id, r->name, r->quantity, r->date);
            }
        }
    }
    print_line('-', 58, COLOR_CYAN);
    printf("  共 %s%zu%s 条出库记录\n",
           COLOR_GREEN, w->outbound.count, COLOR_RESET);
}

static void stock_overview(Warehouse *w)
{
    printf("\n");
    print_title("★ 库 存 概 览 ★", 64, COLOR_BOLD COLOR_MAGENTA);
    list_materials(w);
    printf("\n");
}

static void stock_alert(Warehouse *w)
{
    int threshold;
    printf("请输入预警阈值（库存低于此值将显示）: ");
    if (safe_read_int(&threshold) != 0 || threshold < 0) {
        print_error("输入无效，请输入非负整数");
        return;
    }

    print_line('-', 64, COLOR_YELLOW);
    printf("| %-6s | %-14s | %-14s | %-6s | %-8s |\n",
           "ID", "名称", "规格", "单位", "库存");
    print_line('-', 64, COLOR_YELLOW);

    int found = 0;
    for (size_t i = 0; i < w->materials.count; i++) {
        Material *m = (Material *)dynarray_get(&w->materials, i);
        if (m && m->stock <= threshold) {
            print_material_row(m, 1);
            found++;
        }
    }

    if (found == 0) {
        printf("|%47s|\n", " 所有商品库存充足");
    }
    print_line('-', 64, COLOR_YELLOW);
    printf("  发现 %s%d%s 种商品库存低于 %d\n",
           found > 0 ? COLOR_RED : COLOR_GREEN, found, COLOR_RESET, threshold);
}

static void search_material(Warehouse *w)
{
    char keyword[MAX_NAME];
    printf("请输入搜索关键字: ");
    if (safe_read_string(keyword, sizeof(keyword)) != 0) {
        print_error("输入失败");
        return;
    }

    print_material_header();
    int found = 0;
    for (size_t i = 0; i < w->materials.count; i++) {
        Material *m = (Material *)dynarray_get(&w->materials, i);
        if (m && strstr(m->name, keyword)) {
            print_material_row(m, 1);
            found++;
        }
    }
    if (found == 0) {
        printf("|%47s|\n", " 未找到匹配商品");
    }
    print_line('-', 64, COLOR_CYAN);
    printf("  找到 %s%d%s 个匹配项\n",
           COLOR_GREEN, found, COLOR_RESET);
}

/* ======================== 交互菜单操作 ======================== */

static void ui_add_material(Warehouse *w)
{
    char name[MAX_NAME], spec[MAX_SPEC], unit[MAX_UNIT];
    int stock;

    printf("\n");
    print_title("--- 添 加 商 品 ---", 40, COLOR_GREEN);

    printf("商品名称: ");
    if (safe_read_string(name, sizeof(name)) != 0 || name[0] == '\0') {
        print_error("名称不能为空"); return;
    }
    printf("商品规格: ");
    if (safe_read_string(spec, sizeof(spec)) != 0 || spec[0] == '\0') {
        print_error("规格不能为空"); return;
    }
    printf("计量单位: ");
    if (safe_read_string(unit, sizeof(unit)) != 0 || unit[0] == '\0') {
        print_error("单位不能为空"); return;
    }
    printf("初始库存: ");
    if (safe_read_int(&stock) != 0 || stock < 0) {
        print_error("库存必须是非负整数"); return;
    }

    int new_id;
    WareStatus st = ware_add_material(w, name, spec, unit, stock, &new_id);
    if (st == WARE_OK) {
        ware_save(w, FILE_DATA);
        char msg[128];
        snprintf(msg, sizeof(msg), "商品添加成功，ID: %d", new_id);
        print_success(msg);
    } else {
        print_error("添加失败：内存错误");
    }
}

static void ui_delete_material(Warehouse *w)
{
    int id;
    printf("\n");
    print_title("--- 删 除 商 品 ---", 40, COLOR_RED);
    printf("请输入要删除的商品ID: ");
    if (safe_read_int(&id) != 0) {
        print_error("输入无效"); return;
    }

    Material *m = ware_get_material(w, id);
    if (!m) {
        print_error("未找到该商品"); return;
    }

    printf("即将删除: [%d] %s (%s)，库存: %d\n",
           m->id, m->name, m->spec, m->stock);
    printf("确认删除？(y/N): ");
    char confirm[8];
    safe_read_string(confirm, sizeof(confirm));
    if (confirm[0] != 'y' && confirm[0] != 'Y') {
        print_info("已取消删除"); return;
    }

    WareStatus st = ware_delete_material(w, id);
    if (st == WARE_OK) {
        ware_save(w, FILE_DATA);
        print_success("商品已删除");
    } else {
        print_error("删除失败");
    }
}

static void ui_modify_material(Warehouse *w)
{
    int id;
    printf("\n");
    print_title("--- 修 改 商 品 ---", 40, COLOR_YELLOW);
    printf("请输入要修改的商品ID: ");
    if (safe_read_int(&id) != 0) {
        print_error("输入无效"); return;
    }

    Material *m = ware_get_material(w, id);
    if (!m) {
        print_error("未找到该商品"); return;
    }

    printf("当前信息: [%d] %s | %s | %s | 库存:%d\n",
           m->id, m->name, m->spec, m->unit, m->stock);
    printf("(输入0表示不修改该项)\n");

    char name[MAX_NAME], spec[MAX_SPEC], unit[MAX_UNIT];
    printf("新名称: ");
    safe_read_string(name, sizeof(name));
    printf("新规格: ");
    safe_read_string(spec, sizeof(spec));
    printf("新单位: ");
    safe_read_string(unit, sizeof(unit));

    WareStatus st = ware_modify_material(w, id, name, spec, unit);
    if (st == WARE_OK) {
        ware_save(w, FILE_DATA);
        print_success("商品信息已更新");
    } else {
        print_error("修改失败");
    }
}

static void ui_inbound(Warehouse *w)
{
    int id, qty;
    char date[MAX_DATE] = {0};

    printf("\n");
    print_title("--- 商 品 入 库 ---", 40, COLOR_GREEN);
    printf("商品ID: ");
    if (safe_read_int(&id) != 0) { print_error("输入无效"); return; }

    Material *m = ware_get_material(w, id);
    if (!m) { print_error("未找到该商品"); return; }

    printf("商品: %s，当前库存: %d\n", m->name, m->stock);
    printf("入库数量: ");
    if (safe_read_int(&qty) != 0 || qty <= 0) {
        print_error("数量必须是正整数"); return;
    }

    printf("入库日期(留空使用今天): ");
    safe_read_string(date, sizeof(date));

    WareStatus st = ware_inbound(w, id, qty, date[0] ? date : NULL);
    if (st == WARE_OK) {
        ware_save(w, FILE_DATA);
        char msg[128];
        snprintf(msg, sizeof(msg), "入库成功，%s 当前库存: %d", m->name, m->stock);
        print_success(msg);
    } else {
        print_error("入库失败");
    }
}

static void ui_outbound(Warehouse *w)
{
    int id, qty;
    char date[MAX_DATE] = {0};

    printf("\n");
    print_title("--- 商 品 出 库 ---", 40, COLOR_MAGENTA);
    printf("商品ID: ");
    if (safe_read_int(&id) != 0) { print_error("输入无效"); return; }

    Material *m = ware_get_material(w, id);
    if (!m) { print_error("未找到该商品"); return; }

    printf("商品: %s，当前库存: %d\n", m->name, m->stock);
    printf("出库数量: ");
    if (safe_read_int(&qty) != 0 || qty <= 0) {
        print_error("数量必须是正整数"); return;
    }

    printf("出库日期(留空使用今天): ");
    safe_read_string(date, sizeof(date));

    WareStatus st = ware_outbound(w, id, qty, date[0] ? date : NULL);
    if (st == WARE_OK) {
        ware_save(w, FILE_DATA);
        char msg[128];
        snprintf(msg, sizeof(msg), "出库成功，%s 当前库存: %d", m->name, m->stock);
        print_success(msg);
    } else if (st == WARE_ERR_STOCK_INSUFFICIENT) {
        char msg[128];
        snprintf(msg, sizeof(msg), "库存不足！当前库存: %d，需要: %d", m->stock, qty);
        print_error(msg);
    } else {
        print_error("出库失败");
    }
}

/* ======================== 主菜单 ======================== */

static void show_main_menu(void)
{
    printf("\n");
    print_line('=', 64, COLOR_BOLD COLOR_BLUE);
    print_title("★ 仓 库 管 理 信 息 系 统 V2.0 ★", 64, COLOR_BOLD COLOR_CYAN);
    print_line('=', 64, COLOR_BOLD COLOR_BLUE);
    printf("  %s1.%s 商品管理\n", COLOR_GREEN, COLOR_RESET);
    printf("  %s2.%s 入库管理\n", COLOR_GREEN, COLOR_RESET);
    printf("  %s3.%s 出库管理\n", COLOR_GREEN, COLOR_RESET);
    printf("  %s4.%s 库存统计\n", COLOR_GREEN, COLOR_RESET);
    printf("  %s5.%s 搜索商品\n", COLOR_GREEN, COLOR_RESET);
    printf("  %s0.%s 退出系统\n", COLOR_RED, COLOR_RESET);
    print_line('=', 64, COLOR_BOLD COLOR_BLUE);
    printf("请选择操作: ");
}

static void material_submenu(Warehouse *w)
{
    while (1) {
        printf("\n");
        print_line('-', 64, COLOR_GREEN);
        print_title("--- 商 品 管 理 ---", 64, COLOR_GREEN);
        print_line('-', 64, COLOR_GREEN);
        printf("  1. 查看所有商品\n");
        printf("  2. 添加商品\n");
        printf("  3. 删除商品\n");
        printf("  4. 修改商品\n");
        printf("  0. 返回主菜单\n");
        print_line('-', 64, COLOR_GREEN);
        printf("请选择: ");

        int choice;
        if (safe_read_int(&choice) != 0) {
            print_error("输入无效，请输入数字"); continue;
        }
        switch (choice) {
            case 1: list_materials(w); break;
            case 2: ui_add_material(w); break;
            case 3: ui_delete_material(w); break;
            case 4: ui_modify_material(w); break;
            case 0: return;
            default: print_error("无效选项");
        }
    }
}

static void inbound_submenu(Warehouse *w)
{
    while (1) {
        printf("\n");
        print_line('-', 58, COLOR_GREEN);
        print_title("--- 入 库 管 理 ---", 58, COLOR_GREEN);
        print_line('-', 58, COLOR_GREEN);
        printf("  1. 商品入库\n");
        printf("  2. 查看入库记录\n");
        printf("  0. 返回主菜单\n");
        print_line('-', 58, COLOR_GREEN);
        printf("请选择: ");

        int choice;
        if (safe_read_int(&choice) != 0) {
            print_error("输入无效，请输入数字"); continue;
        }
        switch (choice) {
            case 1: ui_inbound(w); break;
            case 2: list_inbound(w); break;
            case 0: return;
            default: print_error("无效选项");
        }
    }
}

static void outbound_submenu(Warehouse *w)
{
    while (1) {
        printf("\n");
        print_line('-', 58, COLOR_MAGENTA);
        print_title("--- 出 库 管 理 ---", 58, COLOR_MAGENTA);
        print_line('-', 58, COLOR_MAGENTA);
        printf("  1. 商品出库\n");
        printf("  2. 查看出库记录\n");
        printf("  0. 返回主菜单\n");
        print_line('-', 58, COLOR_MAGENTA);
        printf("请选择: ");

        int choice;
        if (safe_read_int(&choice) != 0) {
            print_error("输入无效，请输入数字"); continue;
        }
        switch (choice) {
            case 1: ui_outbound(w); break;
            case 2: list_outbound(w); break;
            case 0: return;
            default: print_error("无效选项");
        }
    }
}

static void stock_submenu(Warehouse *w)
{
    while (1) {
        printf("\n");
        print_line('-', 64, COLOR_CYAN);
        print_title("--- 库 存 统 计 ---", 64, COLOR_CYAN);
        print_line('-', 64, COLOR_CYAN);
        printf("  1. 库存概览\n");
        printf("  2. 库存预警\n");
        printf("  0. 返回主菜单\n");
        print_line('-', 64, COLOR_CYAN);
        printf("请选择: ");

        int choice;
        if (safe_read_int(&choice) != 0) {
            print_error("输入无效，请输入数字"); continue;
        }
        switch (choice) {
            case 1: stock_overview(w); break;
            case 2: stock_alert(w); break;
            case 0: return;
            default: print_error("无效选项");
        }
    }
}

static void main_interactive_loop(Warehouse *w)
{
    print_success("系统启动成功，数据文件: " FILE_DATA);
    print_info("提示: 所有修改会自动保存到文件");

    while (1) {
        show_main_menu();
        int choice;
        if (safe_read_int(&choice) != 0) {
            print_error("输入无效，请输入数字"); continue;
        }
        switch (choice) {
            case 1: material_submenu(w); break;
            case 2: inbound_submenu(w); break;
            case 3: outbound_submenu(w); break;
            case 4: stock_submenu(w); break;
            case 5: search_material(w); break;
            case 0:
                ware_save(w, FILE_DATA);
                printf("\n");
                print_success("数据已保存，感谢使用仓库管理系统 V2.0，再见！");
                return;
            default: print_error("无效选项，请重新选择");
        }
    }
}

/* ======================== 内置自测模块 ======================== */

static int g_test_passed = 0;
static int g_test_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (cond) { \
        printf("  %s[PASS]%s %s\n", COLOR_GREEN, COLOR_RESET, msg); \
        g_test_passed++; \
    } else { \
        printf("  %s[FAIL]%s %s (line %d)\n", COLOR_RED, COLOR_RESET, msg, __LINE__); \
        g_test_failed++; \
    } \
} while(0)

#define RUN_TEST(fn) do { \
    printf("\n%s--- %s ---%s\n", COLOR_CYAN, #fn, COLOR_RESET); \
    fn(); \
} while(0)

static void test_init(void)
{
    Warehouse tw;
    WareStatus st = ware_init(&tw);
    TEST_ASSERT(st == WARE_OK, "ware_init初始化成功");
    TEST_ASSERT(tw.materials.count == 0, "初始商品数为0");
    TEST_ASSERT(tw.inbound.count == 0, "初始入库记录为0");
    TEST_ASSERT(tw.outbound.count == 0, "初始出库记录为0");
    TEST_ASSERT(tw.next_id == NEXT_ID_INIT, "初始ID为1001");
    ware_shutdown(&tw);
}

static void test_add_material(void)
{
    Warehouse tw;
    ware_init(&tw);

    int id1, id2, id3;
    WareStatus st;

    st = ware_add_material(&tw, "螺丝钉", "M6x20", "个", 5000, &id1);
    TEST_ASSERT(st == WARE_OK, "添加商品1成功");
    TEST_ASSERT(id1 == NEXT_ID_INIT, "第一个商品ID=1001");
    TEST_ASSERT(tw.materials.count == 1, "商品数=1");

    st = ware_add_material(&tw, "钢管", "DN50", "米", 200, &id2);
    TEST_ASSERT(st == WARE_OK, "添加商品2成功");
    TEST_ASSERT(id2 == NEXT_ID_INIT + 1, "第二个商品ID=1002");
    TEST_ASSERT(tw.materials.count == 2, "商品数=2");

    st = ware_add_material(&tw, "电线", "BV2.5", "米", 3000, &id3);
    TEST_ASSERT(st == WARE_OK, "添加商品3成功");
    TEST_ASSERT(id3 == NEXT_ID_INIT + 2, "第三个商品ID=1003");
    TEST_ASSERT(tw.materials.count == 3, "商品数=3");

    Material *m = ware_get_material(&tw, id1);
    TEST_ASSERT(m != NULL, "能获取到商品1");
    TEST_ASSERT(strcmp(m->name, "螺丝钉") == 0, "商品1名称正确");
    TEST_ASSERT(m->stock == 5000, "商品1库存正确");

    ware_shutdown(&tw);
}

static void test_invalid_params(void)
{
    Warehouse tw;
    ware_init(&tw);

    int id;
    WareStatus st;

    st = ware_add_material(&tw, NULL, "spec", "unit", 10, &id);
    TEST_ASSERT(st == WARE_ERR_INVALID_PARAM, "name=NULL返回参数错误");

    st = ware_add_material(&tw, "name", "spec", "unit", -5, &id);
    TEST_ASSERT(st == WARE_ERR_INVALID_PARAM, "负库存返回参数错误");

    st = ware_inbound(&tw, 9999, 10, NULL);
    TEST_ASSERT(st == WARE_ERR_NOT_FOUND, "不存在商品入库返回未找到");

    st = ware_inbound(&tw, NEXT_ID_INIT, 0, NULL);
    TEST_ASSERT(st == WARE_ERR_INVALID_PARAM, "入库数量0返回参数错误");

    st = ware_inbound(&tw, NEXT_ID_INIT, -10, NULL);
    TEST_ASSERT(st == WARE_ERR_INVALID_PARAM, "入库数量负数返回参数错误");

    ware_shutdown(&tw);
}

static void test_find_material(void)
{
    Warehouse tw;
    ware_init(&tw);
    int id;
    ware_add_material(&tw, "水泥", "P.O425", "袋", 800, &id);

    Material *m = ware_get_material(&tw, id);
    TEST_ASSERT(m != NULL, "查找存在商品成功");
    TEST_ASSERT(m->id == id, "ID匹配");
    TEST_ASSERT(strcmp(m->name, "水泥") == 0, "名称匹配");

    Material *m2 = ware_get_material(&tw, 99999);
    TEST_ASSERT(m2 == NULL, "查找不存在商品返回NULL");

    ware_shutdown(&tw);
}

static void test_delete_material(void)
{
    Warehouse tw;
    ware_init(&tw);
    int id1, id2, id3;
    ware_add_material(&tw, "A", "a", "个", 10, &id1);
    ware_add_material(&tw, "B", "b", "个", 20, &id2);
    ware_add_material(&tw, "C", "c", "个", 30, &id3);

    TEST_ASSERT(tw.materials.count == 3, "删除前有3个商品");

    WareStatus st = ware_delete_material(&tw, id2);
    TEST_ASSERT(st == WARE_OK, "删除中间商品成功");
    TEST_ASSERT(tw.materials.count == 2, "删除后剩2个商品");
    TEST_ASSERT(ware_get_material(&tw, id2) == NULL, "已删除商品找不到");

    Material *m1 = ware_get_material(&tw, id1);
    Material *m3 = ware_get_material(&tw, id3);
    TEST_ASSERT(m1 != NULL && strcmp(m1->name, "A") == 0, "商品A还在");
    TEST_ASSERT(m3 != NULL && strcmp(m3->name, "C") == 0, "商品C还在");

    st = ware_delete_material(&tw, 99999);
    TEST_ASSERT(st == WARE_ERR_NOT_FOUND, "删除不存在商品失败");

    ware_shutdown(&tw);
}

static void test_modify_material(void)
{
    Warehouse tw;
    ware_init(&tw);
    int id;
    ware_add_material(&tw, "原名", "原规格", "原单位", 100, &id);

    WareStatus st = ware_modify_material(&tw, id, "新名", "新规格", "新单位");
    TEST_ASSERT(st == WARE_OK, "修改商品成功");

    Material *m = ware_get_material(&tw, id);
    TEST_ASSERT(strcmp(m->name, "新名") == 0, "名称已更新");
    TEST_ASSERT(strcmp(m->spec, "新规格") == 0, "规格已更新");
    TEST_ASSERT(strcmp(m->unit, "新单位") == 0, "单位已更新");
    TEST_ASSERT(m->stock == 100, "库存未被修改");

    st = ware_modify_material(&tw, 99999, "x", "x", "x");
    TEST_ASSERT(st == WARE_ERR_NOT_FOUND, "修改不存在商品失败");

    ware_shutdown(&tw);
}

static void test_inbound(void)
{
    Warehouse tw;
    ware_init(&tw);
    int id;
    ware_add_material(&tw, "测试品", "spec", "个", 100, &id);

    WareStatus st = ware_inbound(&tw, id, 50, "2026-01-01");
    TEST_ASSERT(st == WARE_OK, "入库50成功");
    TEST_ASSERT(ware_get_material(&tw, id)->stock == 150, "库存变为150");
    TEST_ASSERT(tw.inbound.count == 1, "入库记录数=1");

    st = ware_inbound(&tw, id, 30, NULL);
    TEST_ASSERT(st == WARE_OK, "入库30（自动日期）成功");
    TEST_ASSERT(ware_get_material(&tw, id)->stock == 180, "库存变为180");
    TEST_ASSERT(tw.inbound.count == 2, "入库记录数=2");

    InRecord *r = (InRecord *)dynarray_get(&tw.inbound, 0);
    TEST_ASSERT(r != NULL && r->quantity == 50, "第一条入库记录数量=50");
    TEST_ASSERT(strcmp(r->date, "2026-01-01") == 0, "日期正确");

    ware_shutdown(&tw);
}

static void test_outbound(void)
{
    Warehouse tw;
    ware_init(&tw);
    int id;
    ware_add_material(&tw, "测试品", "spec", "个", 100, &id);

    WareStatus st = ware_outbound(&tw, id, 30, "2026-02-01");
    TEST_ASSERT(st == WARE_OK, "出库30成功");
    TEST_ASSERT(ware_get_material(&tw, id)->stock == 70, "库存变为70");
    TEST_ASSERT(tw.outbound.count == 1, "出库记录数=1");

    st = ware_outbound(&tw, id, 100, NULL);
    TEST_ASSERT(st == WARE_ERR_STOCK_INSUFFICIENT, "超额出库返回库存不足");
    TEST_ASSERT(ware_get_material(&tw, id)->stock == 70, "库存未变");
    TEST_ASSERT(tw.outbound.count == 1, "出库记录数未增加");

    st = ware_outbound(&tw, id, 70, NULL);
    TEST_ASSERT(st == WARE_OK, "出库剩余全部70成功");
    TEST_ASSERT(ware_get_material(&tw, id)->stock == 0, "库存变为0");

    st = ware_outbound(&tw, id, 1, NULL);
    TEST_ASSERT(st == WARE_ERR_STOCK_INSUFFICIENT, "零库存出库失败");

    ware_shutdown(&tw);
}

static void test_inbound_outbound_combo(void)
{
    Warehouse tw;
    ware_init(&tw);
    int id;
    ware_add_material(&tw, "商品X", "X1", "件", 0, &id);

    ware_inbound(&tw, id, 100, "2026-01-01");
    ware_outbound(&tw, id, 30, "2026-01-02");
    ware_inbound(&tw, id, 50, "2026-01-03");
    ware_outbound(&tw, id, 40, "2026-01-04");

    int expected = 0 + 100 - 30 + 50 - 40;
    TEST_ASSERT(ware_get_material(&tw, id)->stock == expected,
                "库存计算正确：0+100-30+50-40=80");
    TEST_ASSERT(tw.inbound.count == 2, "共2条入库记录");
    TEST_ASSERT(tw.outbound.count == 2, "共2条出库记录");

    ware_shutdown(&tw);
}

static void test_stock_stats(void)
{
    Warehouse tw;
    ware_init(&tw);
    int id1, id2, id3;
    ware_add_material(&tw, "A", "a", "个", 100, &id1);
    ware_add_material(&tw, "B", "b", "个", 200, &id2);
    ware_add_material(&tw, "C", "c", "个", 300, &id3);

    TEST_ASSERT(ware_material_count(&tw) == 3, "商品种类=3");
    TEST_ASSERT(ware_total_stock(&tw) == 600, "总库存=100+200+300=600");

    ware_inbound(&tw, id1, 50, NULL);
    ware_outbound(&tw, id3, 100, NULL);
    TEST_ASSERT(ware_total_stock(&tw) == 550, "出入库后总库存=600+50-100=550");

    ware_shutdown(&tw);
}

static void test_file_save(void)
{
    const char *test_file = "_test_ware.dat";
    Warehouse tw;
    ware_init(&tw);

    int id1, id2;
    ware_add_material(&tw, "螺丝", "M6", "个", 1000, &id1);
    ware_add_material(&tw, "螺母", "M6", "个", 2000, &id2);
    ware_inbound(&tw, id1, 500, "2026-03-01");
    ware_outbound(&tw, id2, 300, "2026-03-02");

    WareStatus st = ware_save(&tw, test_file);
    TEST_ASSERT(st == WARE_OK, "保存文件成功");

    FILE *fp = fopen(test_file, "rb");
    TEST_ASSERT(fp != NULL, "文件确实存在");
    if (fp) {
        char magic[MAGIC_LEN + 1] = {0};
        fread(magic, 1, MAGIC_LEN, fp);
        TEST_ASSERT(memcmp(magic, MAGIC_HEADER, MAGIC_LEN) == 0,
                    "文件魔数正确WAREV2");
        int ver;
        fread(&ver, sizeof(int), 1, fp);
        TEST_ASSERT(ver == FILE_VERSION, "文件版本=2");
        fclose(fp);
    }

    ware_shutdown(&tw);
    remove(test_file);
}

static void test_file_load(void)
{
    const char *test_file = "_test_ware2.dat";
    Warehouse tw1, tw2;
    ware_init(&tw1);

    int id1, id2;
    ware_add_material(&tw1, "商品A", "规格A", "单位A", 111, &id1);
    ware_add_material(&tw1, "商品B", "规格B", "单位B", 222, &id2);
    ware_inbound(&tw1, id1, 50, "2026-04-01");
    ware_inbound(&tw1, id2, 60, "2026-04-02");
    ware_outbound(&tw1, id1, 10, "2026-04-03");

    ware_save(&tw1, test_file);

    ware_init(&tw2);
    WareStatus st = ware_load(&tw2, test_file);
    TEST_ASSERT(st == WARE_OK, "加载文件成功");
    TEST_ASSERT(tw2.materials.count == 2, "加载后商品数=2");
    TEST_ASSERT(tw2.inbound.count == 2, "加载后入库记录=2");
    TEST_ASSERT(tw2.outbound.count == 1, "加载后出库记录=1");
    TEST_ASSERT(tw2.next_id == tw1.next_id, "next_id正确恢复");

    Material *m1 = ware_get_material(&tw2, id1);
    Material *m2 = ware_get_material(&tw2, id2);
    TEST_ASSERT(m1 != NULL && m1->stock == 111 + 50 - 10, "商品1库存正确(151)");
    TEST_ASSERT(m2 != NULL && m2->stock == 222 + 60, "商品2库存正确(282)");
    TEST_ASSERT(strcmp(m1->name, "商品A") == 0, "商品1名称正确");
    TEST_ASSERT(strcmp(m2->spec, "规格B") == 0, "商品2规格正确");

    ware_shutdown(&tw1);
    ware_shutdown(&tw2);
    remove(test_file);
}

static void test_file_version(void)
{
    const char *test_file = "_test_bad.dat";
    FILE *fp = fopen(test_file, "wb");
    if (fp) {
        fwrite(MAGIC_HEADER, 1, MAGIC_LEN, fp);
        int bad_ver = 999;
        fwrite(&bad_ver, sizeof(int), 1, fp);
        fclose(fp);
    }

    Warehouse tw;
    ware_init(&tw);
    WareStatus st = ware_load(&tw, test_file);
    TEST_ASSERT(st == WARE_ERR_FILE_VERSION, "错误版本号返回版本不兼容");

    ware_shutdown(&tw);
    remove(test_file);
}

static void test_file_magic(void)
{
    const char *test_file = "_test_bad2.dat";
    FILE *fp = fopen(test_file, "wb");
    if (fp) {
        fprintf(fp, "INVALIDHEADER");
        fclose(fp);
    }

    Warehouse tw;
    ware_init(&tw);
    WareStatus st = ware_load(&tw, test_file);
    TEST_ASSERT(st == WARE_ERR_FILE_MAGIC, "错误魔数返回魔数错误");

    ware_shutdown(&tw);
    remove(test_file);
}

static void test_dynamic_grow(void)
{
    Warehouse tw;
    ware_init(&tw);

    const int N = 100;
    int ids[N];
    char name[32], spec[32], unit[16];

    for (int i = 0; i < N; i++) {
        snprintf(name, sizeof(name), "商品%03d", i);
        snprintf(spec, sizeof(spec), "规格%03d", i);
        snprintf(unit, sizeof(unit), "个");
        WareStatus st = ware_add_material(&tw, name, spec, unit, i * 10, &ids[i]);
        TEST_ASSERT(st == WARE_OK, "批量添加商品成功");
    }

    TEST_ASSERT(tw.materials.count == (size_t)N, "100个商品全部添加");
    TEST_ASSERT(tw.materials.capacity >= (size_t)N, "动态数组已扩容");
    TEST_ASSERT(ware_total_stock(&tw) == (N - 1) * N * 5,
                "100件商品总库存正确（0+10+20+...+990=49500）");

    for (int i = 0; i < N; i++) {
        Material *m = ware_get_material(&tw, ids[i]);
        TEST_ASSERT(m != NULL, "批量查找商品成功");
        TEST_ASSERT(m->stock == i * 10, "批量商品库存正确");
    }

    ware_shutdown(&tw);
}

static void test_empty_ops(void)
{
    Warehouse tw;
    ware_init(&tw);

    TEST_ASSERT(ware_material_count(&tw) == 0, "空系统商品数=0");
    TEST_ASSERT(ware_total_stock(&tw) == 0, "空系统总库存=0");
    TEST_ASSERT(ware_get_material(&tw, NEXT_ID_INIT) == NULL, "空系统查商品返回NULL");
    TEST_ASSERT(ware_delete_material(&tw, NEXT_ID_INIT) == WARE_ERR_NOT_FOUND,
                "空系统删除商品失败");

    ware_shutdown(&tw);
}

static void test_record_integrity(void)
{
    Warehouse tw;
    ware_init(&tw);
    int id1 = 0, id2 = 0;
    ware_add_material(&tw, "XX", "xx", "件", 50, &id1);
    ware_add_material(&tw, "YY", "yy", "件", 50, &id2);

    ware_inbound(&tw, id1, 10, "D1");
    ware_inbound(&tw, id2, 20, "D2");
    ware_outbound(&tw, id1, 5, "D3");
    ware_outbound(&tw, id2, 15, "D4");
    ware_inbound(&tw, id1, 25, "D5");

    TEST_ASSERT(tw.inbound.count == 3, "3条入库记录");
    TEST_ASSERT(tw.outbound.count == 2, "2条出库记录");

    InRecord *in0 = (InRecord *)dynarray_get(&tw.inbound, 0);
    InRecord *in2 = (InRecord *)dynarray_get(&tw.inbound, 2);
    OutRecord *out1 = (OutRecord *)dynarray_get(&tw.outbound, 1);

    (void)id2;
    TEST_ASSERT(in0 != NULL && in0->mat_id == id1 && in0->quantity == 10, "入库记录0正确");
    TEST_ASSERT(in2 != NULL && in2->mat_id == id1 && in2->quantity == 25, "入库记录2正确");
    TEST_ASSERT(out1 != NULL && out1->mat_id == id2 && out1->quantity == 15, "出库记录1正确");
    {
        Material *m1 = ware_get_material(&tw, id1);
        Material *m2 = ware_get_material(&tw, id2);
        TEST_ASSERT(m1 != NULL && m1->stock == 50 + 10 - 5 + 25, "商品1最终库存正确(80)");
        TEST_ASSERT(m2 != NULL && m2->stock == 50 + 20 - 15, "商品2最终库存正确(55)");
    }

    ware_shutdown(&tw);
}

static int run_all_tests(void)
{
    printf("\n");
    print_line('=', 64, COLOR_BOLD COLOR_BLUE);
    print_title("★ 仓库管理系统 V2.0 内置自测 ★", 64, COLOR_BOLD COLOR_CYAN);
    print_line('=', 64, COLOR_BOLD COLOR_BLUE);

    g_test_passed = 0;
    g_test_failed = 0;

    RUN_TEST(test_init);
    RUN_TEST(test_add_material);
    RUN_TEST(test_invalid_params);
    RUN_TEST(test_find_material);
    RUN_TEST(test_delete_material);
    RUN_TEST(test_modify_material);
    RUN_TEST(test_inbound);
    RUN_TEST(test_outbound);
    RUN_TEST(test_inbound_outbound_combo);
    RUN_TEST(test_stock_stats);
    RUN_TEST(test_file_save);
    RUN_TEST(test_file_load);
    RUN_TEST(test_file_version);
    RUN_TEST(test_file_magic);
    RUN_TEST(test_dynamic_grow);
    RUN_TEST(test_empty_ops);
    RUN_TEST(test_record_integrity);

    printf("\n");
    print_line('=', 64, COLOR_BOLD COLOR_BLUE);
    printf("  测试结果: %s通过 %d%s, %s失败 %d%s, 总计 %d\n",
           COLOR_GREEN, g_test_passed, COLOR_RESET,
           g_test_failed > 0 ? COLOR_RED : COLOR_GREEN, g_test_failed, COLOR_RESET,
           g_test_passed + g_test_failed);
    print_line('=', 64, g_test_failed == 0 ? COLOR_GREEN : COLOR_RED);

    return g_test_failed == 0 ? 0 : 1;
}

/* ======================== 帮助信息 ======================== */

static void print_help(const char *prog_name)
{
    printf("\n");
    print_line('=', 64, COLOR_BOLD COLOR_BLUE);
    print_title("仓库管理信息系统 V2.0 - 使用帮助", 64, COLOR_BOLD COLOR_CYAN);
    print_line('=', 64, COLOR_BOLD COLOR_BLUE);
    printf("\n");
    printf("  用法: %s [选项]\n\n", prog_name);
    printf("  选项:\n");
    printf("    %s-h, --help%s     显示此帮助信息\n", COLOR_GREEN, COLOR_RESET);
    printf("    %s-t, --test%s     运行内置自测套件\n", COLOR_GREEN, COLOR_RESET);
    printf("    (无参数)          启动交互式菜单系统\n\n");
    printf("  数据文件: %s%s%s (二进制格式，含魔数 WAREV2)\n\n",
           COLOR_YELLOW, FILE_DATA, COLOR_RESET);
    printf("  V2.0 新特性:\n");
    printf("    - UTF-8 编码，中文完美显示\n");
    printf("    - ANSI 彩色终端界面\n");
    printf("    - 动态数组存储，自动扩容\n");
    printf("    - 统一错误码枚举 WareStatus\n");
    printf("    - 二进制持久化 + 魔数校验\n");
    printf("    - 17项内置自测覆盖CRUD/出入库/文件IO\n");
    printf("\n");
}

/* ======================== 主函数 ======================== */

int main(int argc, char *argv[])
{
    (void)argc;
    console_init();

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        }
        if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0) {
            return run_all_tests();
        }
        printf("%s未知选项: %s%s\n", COLOR_RED, argv[1], COLOR_RESET);
        printf("使用 %s-h%s 查看帮助\n", COLOR_GREEN, COLOR_RESET);
        return 1;
    }

    ware_init(&g_ware);

    WareStatus st = ware_load(&g_ware, FILE_DATA);
    if (st == WARE_OK) {
        /* 数据加载成功 */
    } else if (st == WARE_ERR_FILE_OPEN) {
        print_info("首次运行，正在初始化示例数据...");
        int id = 0;
        ware_add_material(&g_ware, "螺丝钉", "M6x20", "个", 5000, &id);
        ware_add_material(&g_ware, "钢管", "DN50", "米", 200, &id);
        ware_add_material(&g_ware, "电线", "BV2.5", "米", 3000, &id);
        ware_add_material(&g_ware, "水泥", "P.O425", "袋", 800, &id);
        ware_add_material(&g_ware, "油漆", "红色", "桶", 150, &id);
        ware_save(&g_ware, FILE_DATA);
    } else {
        char msg[128];
        snprintf(msg, sizeof(msg), "数据文件损坏或不兼容 (错误码: %d)，将使用空库启动", (int)st);
        print_warning(msg);
        ware_shutdown(&g_ware);
        ware_init(&g_ware);
    }

    main_interactive_loop(&g_ware);

    ware_shutdown(&g_ware);
    return 0;
}
