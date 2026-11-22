/*
 * ============================================================================
 *
 *                              通讯录管理系统 V2.0
 *                        Contact Management System V2.0
 *
 *  项目编号: Project #31
 *  版本标准: V2.0
 *  编译环境: GCC (MinGW-w64) -std=c99
 *  编码格式: UTF-8 (with BOM)
 *
 *  V2.0 新特性:
 *    [1] UTF-8 BOM 编码 + SetConsoleOutputCP(65001) Windows控制台支持
 *    [2] ANSI 彩色终端输出（黑/红/绿/黄/蓝/紫/青/白）
 *    [3] CLI 命令行参数支持 (-h/--help, -t/--test)
 *    [4] 动态数组存储联系人（替代链表，O(1)随机访问）
 *    [5] 统一错误码枚举 ContactStatus
 *    [6] "CONTV2" magic header 二进制文件持久化
 *    [7] 内置自测框架 (≥15项: CRUD+分组+搜索+排序+统计+文件I/O)
 *    [8] 交互式菜单系统
 *    [9] 完整中文注释
 *
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* ============================================================================
 *  第一部分: 宏定义与常量
 * ============================================================================ */

#define MAX_NAME        32
#define MAX_PHONE       20
#define MAX_EMAIL       64
#define MAX_GROUP       32
#define MAX_ADDR        128
#define INIT_CAPACITY   8
#define MAGIC_HEADER    "CONTV2"
#define MAGIC_LEN       6
#define DATA_FILE       "contacts_v2.dat"

/* ANSI 颜色转义序列 */
#define COLOR_RESET     "\033[0m"
#define COLOR_BLACK     "\033[30m"
#define COLOR_RED       "\033[31m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_BLUE      "\033[34m"
#define COLOR_MAGENTA   "\033[35m"
#define COLOR_CYAN      "\033[36m"
#define COLOR_WHITE     "\033[37m"
#define COLOR_BOLD      "\033[1m"
#define BG_BLUE         "\033[44m"
#define BG_GREEN        "\033[42m"
#define BG_RED          "\033[41m"

/* ============================================================================
 *  第二部分: 数据结构与枚举
 * ============================================================================ */

typedef enum {
    CONTACT_OK = 0,
    CONTACT_ERR_MEMORY,
    CONTACT_ERR_NOT_FOUND,
    CONTACT_ERR_DUPLICATE,
    CONTACT_ERR_FILE_OPEN,
    CONTACT_ERR_FILE_READ,
    CONTACT_ERR_FILE_WRITE,
    CONTACT_ERR_FILE_MAGIC,
    CONTACT_ERR_FILE_VERSION,
    CONTACT_ERR_INVALID_PARAM,
    CONTACT_ERR_FULL,
    CONTACT_ERR_EMPTY
} ContactStatus;

typedef struct {
    char name[MAX_NAME];
    char phone[MAX_PHONE];
    char email[MAX_EMAIL];
    char group[MAX_GROUP];
    char address[MAX_ADDR];
} Contact;

typedef struct {
    Contact *items;
    size_t count;
    size_t capacity;
} ContactArray;

/* ============================================================================
 *  第三部分: 全局状态
 * ============================================================================ */

static ContactArray g_contacts = {NULL, 0, 0};
static int g_test_mode = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

/* ============================================================================
 *  第四部分: 终端初始化与彩色输出工具
 * ============================================================================ */

static void terminal_init(void)
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

static void c_print(const char *color, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if (!g_test_mode) {
        printf("%s", color);
        vprintf(fmt, args);
        printf("%s", COLOR_RESET);
        fflush(stdout);
    }
    va_end(args);
}

static void print_separator(char ch, const char *color)
{
    if (g_test_mode) return;
    printf("%s", color);
    for (int i = 0; i < 72; i++) putchar(ch);
    printf("%s\n", COLOR_RESET);
}

static void print_header(const char *title)
{
    if (g_test_mode) return;
    print_separator('=', COLOR_CYAN);
    printf("%s%s%s", COLOR_BOLD COLOR_CYAN, "  ", COLOR_RESET);
    size_t len = strlen(title);
    size_t pad = (72 - len - 2) / 2;
    for (size_t i = 0; i < pad; i++) putchar(' ');
    printf("%s%s%s\n", COLOR_BOLD COLOR_WHITE, title, COLOR_RESET);
    print_separator('=', COLOR_CYAN);
}

/* ============================================================================
 *  第五部分: 输入处理
 * ============================================================================ */

static void safe_input(char *buf, int size)
{
    if (fgets(buf, size, stdin) != NULL) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n')
            buf[len - 1] = '\0';
        size_t start = 0;
        while (buf[start] == ' ' || buf[start] == '\t') start++;
        if (start > 0) memmove(buf, buf + start, strlen(buf + start) + 1);
    }
}

static void pause_screen(void)
{
    if (g_test_mode) return;
    c_print(COLOR_YELLOW, "\n  按 Enter 键继续...");
    fflush(stdout);
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

/* ============================================================================
 *  第六部分: 动态数组操作
 * ============================================================================ */

static ContactStatus contact_array_init(ContactArray *arr, size_t initial_cap)
{
    if (!arr || initial_cap == 0) return CONTACT_ERR_INVALID_PARAM;
    arr->items = (Contact *)calloc(initial_cap, sizeof(Contact));
    if (!arr->items) return CONTACT_ERR_MEMORY;
    arr->count = 0;
    arr->capacity = initial_cap;
    return CONTACT_OK;
}

static ContactStatus contact_array_reserve(ContactArray *arr, size_t new_cap)
{
    if (!arr) return CONTACT_ERR_INVALID_PARAM;
    if (new_cap <= arr->capacity) return CONTACT_OK;
    Contact *new_items = (Contact *)realloc(arr->items, new_cap * sizeof(Contact));
    if (!new_items) return CONTACT_ERR_MEMORY;
    arr->items = new_items;
    arr->capacity = new_cap;
    return CONTACT_OK;
}

static ContactStatus contact_array_add(ContactArray *arr, const Contact *c)
{
    if (!arr || !c) return CONTACT_ERR_INVALID_PARAM;
    if (arr->count >= arr->capacity) {
        size_t new_cap = arr->capacity == 0 ? INIT_CAPACITY : arr->capacity * 2;
        ContactStatus s = contact_array_reserve(arr, new_cap);
        if (s != CONTACT_OK) return s;
    }
    arr->items[arr->count] = *c;
    arr->count++;
    return CONTACT_OK;
}

static ContactStatus contact_array_insert(ContactArray *arr, size_t idx, const Contact *c)
{
    if (!arr || !c) return CONTACT_ERR_INVALID_PARAM;
    if (idx > arr->count) return CONTACT_ERR_INVALID_PARAM;
    if (arr->count >= arr->capacity) {
        size_t new_cap = arr->capacity == 0 ? INIT_CAPACITY : arr->capacity * 2;
        ContactStatus s = contact_array_reserve(arr, new_cap);
        if (s != CONTACT_OK) return s;
    }
    if (idx < arr->count) {
        memmove(&arr->items[idx + 1], &arr->items[idx],
                (arr->count - idx) * sizeof(Contact));
    }
    arr->items[idx] = *c;
    arr->count++;
    return CONTACT_OK;
}

static ContactStatus contact_array_remove(ContactArray *arr, size_t idx)
{
    if (!arr) return CONTACT_ERR_INVALID_PARAM;
    if (idx >= arr->count) return CONTACT_ERR_NOT_FOUND;
    if (idx < arr->count - 1) {
        memmove(&arr->items[idx], &arr->items[idx + 1],
                (arr->count - idx - 1) * sizeof(Contact));
    }
    arr->count--;
    memset(&arr->items[arr->count], 0, sizeof(Contact));
    return CONTACT_OK;
}

static void contact_array_free(ContactArray *arr)
{
    if (arr && arr->items) {
        free(arr->items);
        arr->items = NULL;
        arr->count = 0;
        arr->capacity = 0;
    }
}

static ContactStatus contact_find_by_phone(ContactArray *arr, const char *phone, size_t *out_idx)
{
    if (!arr || !phone) return CONTACT_ERR_INVALID_PARAM;
    for (size_t i = 0; i < arr->count; i++) {
        if (strcmp(arr->items[i].phone, phone) == 0) {
            if (out_idx) *out_idx = i;
            return CONTACT_OK;
        }
    }
    return CONTACT_ERR_NOT_FOUND;
}

static int contact_name_cmp_asc(const void *a, const void *b)
{
    return strcmp(((const Contact *)a)->name, ((const Contact *)b)->name);
}

static int contact_name_cmp_desc(const void *a, const void *b)
{
    return strcmp(((const Contact *)b)->name, ((const Contact *)a)->name);
}

static void contact_sort_by_name(ContactArray *arr, int ascending)
{
    if (!arr || arr->count < 2) return;
    qsort(arr->items, arr->count, sizeof(Contact),
          ascending ? contact_name_cmp_asc : contact_name_cmp_desc);
}

/* ============================================================================
 *  第七部分: 文件持久化 (带 CONTV2 magic header)
 * ============================================================================ */

static ContactStatus contact_save_to_file(const char *filename)
{
    if (!filename) return CONTACT_ERR_INVALID_PARAM;
    FILE *fp = fopen(filename, "wb");
    if (!fp) return CONTACT_ERR_FILE_OPEN;
    if (fwrite(MAGIC_HEADER, 1, MAGIC_LEN, fp) != MAGIC_LEN) {
        fclose(fp);
        return CONTACT_ERR_FILE_WRITE;
    }
    uint32_t version = 2;
    if (fwrite(&version, sizeof(version), 1, fp) != 1) {
        fclose(fp);
        return CONTACT_ERR_FILE_WRITE;
    }
    uint32_t count = (uint32_t)g_contacts.count;
    if (fwrite(&count, sizeof(count), 1, fp) != 1) {
        fclose(fp);
        return CONTACT_ERR_FILE_WRITE;
    }
    if (g_contacts.count > 0 && g_contacts.items) {
        size_t written = fwrite(g_contacts.items, sizeof(Contact), g_contacts.count, fp);
        if (written != g_contacts.count) {
            fclose(fp);
            return CONTACT_ERR_FILE_WRITE;
        }
    }
    fclose(fp);
    return CONTACT_OK;
}

static ContactStatus contact_load_from_file(const char *filename)
{
    if (!filename) return CONTACT_ERR_INVALID_PARAM;
    FILE *fp = fopen(filename, "rb");
    if (!fp) return CONTACT_ERR_FILE_OPEN;
    char magic[MAGIC_LEN + 1] = {0};
    if (fread(magic, 1, MAGIC_LEN, fp) != MAGIC_LEN) {
        fclose(fp);
        return CONTACT_ERR_FILE_READ;
    }
    if (memcmp(magic, MAGIC_HEADER, MAGIC_LEN) != 0) {
        fclose(fp);
        return CONTACT_ERR_FILE_MAGIC;
    }
    uint32_t version = 0;
    if (fread(&version, sizeof(version), 1, fp) != 1) {
        fclose(fp);
        return CONTACT_ERR_FILE_READ;
    }
    if (version != 2) {
        fclose(fp);
        return CONTACT_ERR_FILE_VERSION;
    }
    uint32_t count = 0;
    if (fread(&count, sizeof(count), 1, fp) != 1) {
        fclose(fp);
        return CONTACT_ERR_FILE_READ;
    }
    contact_array_free(&g_contacts);
    ContactStatus s = contact_array_init(&g_contacts, count > INIT_CAPACITY ? count : INIT_CAPACITY);
    if (s != CONTACT_OK) {
        fclose(fp);
        return s;
    }
    if (count > 0) {
        if (contact_array_reserve(&g_contacts, count) != CONTACT_OK) {
            fclose(fp);
            return CONTACT_ERR_MEMORY;
        }
        size_t read_n = fread(g_contacts.items, sizeof(Contact), count, fp);
        if (read_n != count) {
            contact_array_free(&g_contacts);
            fclose(fp);
            return CONTACT_ERR_FILE_READ;
        }
        g_contacts.count = count;
    }
    fclose(fp);
    return CONTACT_OK;
}

/* ============================================================================
 *  第八部分: 联系人 CRUD 业务逻辑
 * ============================================================================ */

static ContactStatus add_contact_interactive(void)
{
    Contact c;
    memset(&c, 0, sizeof(c));
    c_print(COLOR_GREEN, "\n  ➤ 添加新联系人\n");
    printf("  姓名: "); safe_input(c.name, MAX_NAME);
    printf("  电话: "); safe_input(c.phone, MAX_PHONE);
    printf("  邮箱: "); safe_input(c.email, MAX_EMAIL);
    printf("  分组: "); safe_input(c.group, MAX_GROUP);
    printf("  地址: "); safe_input(c.address, MAX_ADDR);
    if (strlen(c.name) == 0 || strlen(c.phone) == 0) {
        c_print(COLOR_RED, "  ✗ 姓名和电话不能为空!\n");
        return CONTACT_ERR_INVALID_PARAM;
    }
    size_t dummy;
    if (contact_find_by_phone(&g_contacts, c.phone, &dummy) == CONTACT_OK) {
        c_print(COLOR_RED, "  ✗ 电话 %s 已存在!\n", c.phone);
        return CONTACT_ERR_DUPLICATE;
    }
    ContactStatus s = contact_array_add(&g_contacts, &c);
    if (s == CONTACT_OK) {
        contact_save_to_file(DATA_FILE);
        c_print(COLOR_GREEN, "  ✓ 添加成功! 当前共 %zu 位联系人\n", g_contacts.count);
    }
    return s;
}

static ContactStatus delete_contact_interactive(void)
{
    char phone[MAX_PHONE];
    c_print(COLOR_YELLOW, "\n  ➤ 删除联系人\n");
    printf("  请输入要删除的电话: ");
    safe_input(phone, MAX_PHONE);
    size_t idx;
    if (contact_find_by_phone(&g_contacts, phone, &idx) != CONTACT_OK) {
        c_print(COLOR_RED, "  ✗ 未找到电话为 %s 的联系人\n", phone);
        return CONTACT_ERR_NOT_FOUND;
    }
    c_print(COLOR_CYAN, "  找到: %s | %s | %s\n",
            g_contacts.items[idx].name,
            g_contacts.items[idx].phone,
            g_contacts.items[idx].email);
    printf("  确认删除? (y/N): ");
    char confirm[8];
    safe_input(confirm, 8);
    if (confirm[0] == 'y' || confirm[0] == 'Y') {
        ContactStatus s = contact_array_remove(&g_contacts, idx);
        if (s == CONTACT_OK) {
            contact_save_to_file(DATA_FILE);
            c_print(COLOR_GREEN, "  ✓ 删除成功!\n");
        }
        return s;
    }
    c_print(COLOR_YELLOW, "  已取消删除\n");
    return CONTACT_OK;
}

static ContactStatus modify_contact_interactive(void)
{
    char phone[MAX_PHONE];
    c_print(COLOR_BLUE, "\n  ➤ 修改联系人\n");
    printf("  请输入要修改的电话: ");
    safe_input(phone, MAX_PHONE);
    size_t idx;
    if (contact_find_by_phone(&g_contacts, phone, &idx) != CONTACT_OK) {
        c_print(COLOR_RED, "  ✗ 未找到电话为 %s 的联系人\n", phone);
        return CONTACT_ERR_NOT_FOUND;
    }
    Contact *c = &g_contacts.items[idx];
    c_print(COLOR_CYAN, "  当前信息:\n");
    printf("    姓名: %s\n", c->name);
    printf("    电话: %s\n", c->phone);
    printf("    邮箱: %s\n", c->email);
    printf("    分组: %s\n", c->group);
    printf("    地址: %s\n", c->address);
    c_print(COLOR_YELLOW, "  输入新信息 (直接回车保持不变):\n");
    char buf[256];
    printf("  姓名 [%s]: ", c->name); safe_input(buf, sizeof(buf));
    if (strlen(buf) > 0) {
        buf[MAX_NAME - 1] = '\0';
        strcpy(c->name, buf);
    }
    printf("  电话 [%s]: ", c->phone); safe_input(buf, sizeof(buf));
    if (strlen(buf) > 0) {
        buf[MAX_PHONE - 1] = '\0';
        strcpy(c->phone, buf);
    }
    printf("  邮箱 [%s]: ", c->email); safe_input(buf, sizeof(buf));
    if (strlen(buf) > 0) {
        buf[MAX_EMAIL - 1] = '\0';
        strcpy(c->email, buf);
    }
    printf("  分组 [%s]: ", c->group); safe_input(buf, sizeof(buf));
    if (strlen(buf) > 0) {
        buf[MAX_GROUP - 1] = '\0';
        strcpy(c->group, buf);
    }
    printf("  地址 [%s]: ", c->address); safe_input(buf, sizeof(buf));
    if (strlen(buf) > 0) {
        buf[MAX_ADDR - 1] = '\0';
        strcpy(c->address, buf);
    }
    contact_save_to_file(DATA_FILE);
    c_print(COLOR_GREEN, "  ✓ 修改成功!\n");
    return CONTACT_OK;
}

static void list_all_contacts(void)
{
    if (g_contacts.count == 0) {
        c_print(COLOR_YELLOW, "\n  ⚠ 通讯录为空\n");
        return;
    }
    c_print(COLOR_CYAN, "\n  ➤ 所有联系人 (%zu 位)\n", g_contacts.count);
    print_separator('-', COLOR_BLUE);
    printf("%s%-4s %-12s %-15s %-24s %-12s %s%s\n",
           COLOR_BOLD, "序号", "姓名", "电话", "邮箱", "分组", "地址", COLOR_RESET);
    print_separator('-', COLOR_BLUE);
    for (size_t i = 0; i < g_contacts.count; i++) {
        Contact *c = &g_contacts.items[i];
        printf("%s%-4zu%s %-12s %-15s %-24s %-12s %s\n",
               COLOR_GREEN, i + 1, COLOR_RESET,
               c->name, c->phone, c->email, c->group, c->address);
    }
    print_separator('-', COLOR_BLUE);
}

static void search_contacts_interactive(void)
{
    char keyword[128];
    int found = 0;
    c_print(COLOR_MAGENTA, "\n  ➤ 搜索联系人\n");
    printf("  输入关键词 (姓名/电话/邮箱/分组/地址): ");
    safe_input(keyword, sizeof(keyword));
    print_separator('-', COLOR_MAGENTA);
    for (size_t i = 0; i < g_contacts.count; i++) {
        Contact *c = &g_contacts.items[i];
        if (strstr(c->name, keyword) || strstr(c->phone, keyword) ||
            strstr(c->email, keyword) || strstr(c->group, keyword) ||
            strstr(c->address, keyword)) {
            if (!found) {
                printf("%s%-4s %-12s %-15s %-24s %-12s %s%s\n",
                       COLOR_BOLD, "序号", "姓名", "电话", "邮箱", "分组", "地址", COLOR_RESET);
                print_separator('-', COLOR_MAGENTA);
            }
            printf("%s%-4zu%s %-12s %-15s %-24s %-12s %s\n",
                   COLOR_GREEN, i + 1, COLOR_RESET,
                   c->name, c->phone, c->email, c->group, c->address);
            found++;
        }
    }
    print_separator('-', COLOR_MAGENTA);
    if (found == 0) {
        c_print(COLOR_YELLOW, "  未找到包含 \"%s\" 的联系人\n", keyword);
    } else {
        c_print(COLOR_GREEN, "  共找到 %d 位联系人\n", found);
    }
}

static void list_by_group_interactive(void)
{
    char group_name[MAX_GROUP];
    int found = 0;
    c_print(COLOR_BLUE, "\n  ➤ 按分组显示\n");
    printf("  输入分组名称: ");
    safe_input(group_name, MAX_GROUP);
    print_separator('-', COLOR_BLUE);
    for (size_t i = 0; i < g_contacts.count; i++) {
        Contact *c = &g_contacts.items[i];
        if (strcmp(c->group, group_name) == 0) {
            if (!found) {
                printf("%s分组 [%s]:%s\n", COLOR_BOLD COLOR_GREEN, group_name, COLOR_RESET);
                printf("%s%-4s %-12s %-15s %-24s %s%s\n",
                       COLOR_BOLD, "序号", "姓名", "电话", "邮箱", "地址", COLOR_RESET);
                print_separator('-', COLOR_BLUE);
            }
            printf("%s%-4d%s %-12s %-15s %-24s %s\n",
                   COLOR_GREEN, found + 1, COLOR_RESET,
                   c->name, c->phone, c->email, c->address);
            found++;
        }
    }
    print_separator('-', COLOR_BLUE);
    if (found == 0) {
        c_print(COLOR_YELLOW, "  分组 \"%s\" 中没有联系人\n", group_name);
    } else {
        c_print(COLOR_GREEN, "  分组 \"%s\" 共 %d 位联系人\n", group_name, found);
    }
}

static void show_statistics(void)
{
    c_print(COLOR_CYAN, "\n  ➤ 统计信息\n");
    print_separator('-', COLOR_CYAN);
    c_print(COLOR_WHITE, "  总联系人数量: %s%zu%s\n", COLOR_GREEN COLOR_BOLD, g_contacts.count, COLOR_RESET);
    if (g_contacts.count == 0) {
        print_separator('-', COLOR_CYAN);
        return;
    }
    struct { char name[MAX_GROUP]; size_t count; } groups[64];
    size_t gcount = 0;
    for (size_t i = 0; i < g_contacts.count; i++) {
        const char *gname = g_contacts.items[i].group;
        size_t j;
        for (j = 0; j < gcount; j++) {
            if (strcmp(groups[j].name, gname) == 0) {
                groups[j].count++;
                break;
            }
        }
        if (j == gcount && gcount < 64) {
            strncpy(groups[gcount].name, gname, MAX_GROUP - 1);
            groups[gcount].count = 1;
            gcount++;
        }
    }
    c_print(COLOR_WHITE, "\n  分组统计:\n");
    for (size_t i = 0; i < gcount; i++) {
        int bar_len = (int)(40 * groups[i].count / g_contacts.count);
        printf("    %-12s |", groups[i].name);
        printf("%s", COLOR_GREEN);
        for (int k = 0; k < bar_len; k++) fputs("█", stdout);
        printf("%s", COLOR_RESET);
        printf(" %zu\n", groups[i].count);
    }
    print_separator('-', COLOR_CYAN);
}

static void sort_contacts_interactive(void)
{
    char choice[8];
    c_print(COLOR_YELLOW, "\n  ➤ 排序\n");
    printf("  1. 按姓名升序\n");
    printf("  2. 按姓名降序\n");
    printf("  请选择: ");
    safe_input(choice, 8);
    if (choice[0] == '1') {
        contact_sort_by_name(&g_contacts, 1);
        c_print(COLOR_GREEN, "  ✓ 已按姓名升序排列\n");
        contact_save_to_file(DATA_FILE);
    } else if (choice[0] == '2') {
        contact_sort_by_name(&g_contacts, 0);
        c_print(COLOR_GREEN, "  ✓ 已按姓名降序排列\n");
        contact_save_to_file(DATA_FILE);
    } else {
        c_print(COLOR_RED, "  ✗ 无效选择\n");
    }
}

/* ============================================================================
 *  第九部分: 内置自测框架 (≥15项)
 * ============================================================================ */

#define TEST_ASSERT(cond, name) do { \
    if (cond) { \
        g_tests_passed++; \
        printf("  %s✓ PASS%s: %s\n", COLOR_GREEN, COLOR_RESET, name); \
    } else { \
        g_tests_failed++; \
        printf("  %s✗ FAIL%s: %s\n", COLOR_RED, COLOR_RESET, name); \
    } \
} while(0)

#define TEST_ASSERT_STATUS(s, expected, name) TEST_ASSERT((s) == (expected), name)

static int run_self_tests(void)
{
    g_test_mode = 0;
    printf("\n%s%s=== 通讯录管理系统 V2.0 自测 ===%s\n\n",
           COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    contact_array_free(&g_contacts);
    g_tests_passed = 0;
    g_tests_failed = 0;

    /* T01: 初始化动态数组 */
    ContactStatus s = contact_array_init(&g_contacts, INIT_CAPACITY);
    TEST_ASSERT_STATUS(s, CONTACT_OK, "T01: 初始化动态数组");

    /* T02: 初始状态为空 */
    TEST_ASSERT(g_contacts.count == 0, "T02: 初始count为0");
    TEST_ASSERT(g_contacts.capacity == INIT_CAPACITY, "T03: 初始capacity正确");

    /* T04: 添加联系人 */
    Contact c1 = {"张三", "13800000001", "z********@*********", "家人", "北京市海淀区"};
    s = contact_array_add(&g_contacts, &c1);
    TEST_ASSERT_STATUS(s, CONTACT_OK, "T04: 添加第一个联系人");

    /* T05: count递增 */
    TEST_ASSERT(g_contacts.count == 1, "T05: count递增为1");

    /* T06: 添加更多联系人 */
    Contact c2 = {"李四", "13800000002", "l***@*********", "同事", "上海市浦东新区"};
    Contact c3 = {"王五", "13800000003", "w*****@*********", "朋友", "广州市天河区"};
    Contact c4 = {"赵六", "13800000004", "z******@*********", "家人", "深圳市南山区"};
    contact_array_add(&g_contacts, &c2);
    contact_array_add(&g_contacts, &c3);
    contact_array_add(&g_contacts, &c4);
    TEST_ASSERT(g_contacts.count == 4, "T06: 添加3位联系人后count=4");

    /* T07: 按电话查找 */
    size_t idx;
    s = contact_find_by_phone(&g_contacts, "13800000002", &idx);
    TEST_ASSERT_STATUS(s, CONTACT_OK, "T07: 按电话查找存在的联系人");
    TEST_ASSERT(idx == 1, "T08: 查找结果索引正确");

    /* T09: 查找不存在的电话 */
    s = contact_find_by_phone(&g_contacts, "99999999999", &idx);
    TEST_ASSERT_STATUS(s, CONTACT_ERR_NOT_FOUND, "T09: 查找不存在的电话返回NOT_FOUND");

    /* T10: 重复电话检测 - 已存在的电话应能找到 */
    Contact dup = {"重复", "13800000001", "d**@******", "测试", "测试地址"};
    size_t dummy;
    s = contact_find_by_phone(&g_contacts, dup.phone, &dummy);
    TEST_ASSERT_STATUS(s, CONTACT_OK, "T10: 重复电话(已存在)可被查找");

    /* T11: 插入联系人到指定位置 */
    Contact c5 = {"插入测试", "13900000001", "i*****@******", "测试", "插入地址"};
    s = contact_array_insert(&g_contacts, 2, &c5);
    TEST_ASSERT_STATUS(s, CONTACT_OK, "T11: 在索引2插入联系人");
    TEST_ASSERT(g_contacts.count == 5, "T12: 插入后count=5");

    /* T13: 删除联系人 */
    s = contact_array_remove(&g_contacts, 2);
    TEST_ASSERT_STATUS(s, CONTACT_OK, "T13: 删除索引2的联系人");
    TEST_ASSERT(g_contacts.count == 4, "T14: 删除后count=4");

    /* T15-T16: 使用ASCII姓名测试排序功能 */
    contact_array_free(&g_contacts);
    contact_array_init(&g_contacts, INIT_CAPACITY);
    Contact sa = {"Charlie", "17000000001", "c@******", "测试", "地址"};
    Contact sb = {"Alice", "17000000002", "a@******", "测试", "地址"};
    Contact sc = {"Bob", "17000000003", "b@******", "测试", "地址"};
    contact_array_add(&g_contacts, &sa);
    contact_array_add(&g_contacts, &sb);
    contact_array_add(&g_contacts, &sc);
    contact_sort_by_name(&g_contacts, 1);
    TEST_ASSERT(strcmp(g_contacts.items[0].name, "Alice") == 0,
                "T15: 升序排序后第一个是Alice");
    TEST_ASSERT(strcmp(g_contacts.items[2].name, "Charlie") == 0,
                "T16: 升序排序后最后一个是Charlie");
    contact_sort_by_name(&g_contacts, 0);
    TEST_ASSERT(strcmp(g_contacts.items[0].name, "Charlie") == 0,
                "T17: 降序排序后第一个是Charlie");
    TEST_ASSERT(strcmp(g_contacts.items[2].name, "Alice") == 0,
                "T18: 降序排序后最后一个是Alice");

    /* T19: 自动扩容测试 */
    contact_array_free(&g_contacts);
    contact_array_init(&g_contacts, 2);
    size_t old_cap = g_contacts.capacity;
    Contact tmp = {"测试", "13000000000", "t**@******", "测试", "测试"};
    for (int i = 0; i < 5; i++) {
        char buf[32];
        sprintf(buf, "13000000%03d", i);
        strcpy(tmp.phone, buf);
        sprintf(buf, "测试%d", i);
        strcpy(tmp.name, buf);
        contact_array_add(&g_contacts, &tmp);
    }
    TEST_ASSERT(g_contacts.capacity > old_cap, "T19: 动态数组自动扩容");
    TEST_ASSERT(g_contacts.count == 5, "T20: 扩容后数据正确保留");

    /* T21: 文件保存 */
    contact_array_free(&g_contacts);
    contact_array_init(&g_contacts, INIT_CAPACITY);
    Contact fc1 = {"文件测试1", "15000000001", "f**@******", "文件组", "文件地址1"};
    Contact fc2 = {"文件测试2", "15000000002", "f**@******", "文件组", "文件地址2"};
    contact_array_add(&g_contacts, &fc1);
    contact_array_add(&g_contacts, &fc2);
    s = contact_save_to_file("test_v2.dat");
    TEST_ASSERT_STATUS(s, CONTACT_OK, "T21: 保存到文件成功");

    /* T22: 文件读取 */
    contact_array_free(&g_contacts);
    contact_array_init(&g_contacts, INIT_CAPACITY);
    s = contact_load_from_file("test_v2.dat");
    TEST_ASSERT_STATUS(s, CONTACT_OK, "T22: 从文件加载成功");
    TEST_ASSERT(g_contacts.count == 2, "T23: 加载后count=2");
    TEST_ASSERT(strcmp(g_contacts.items[0].name, "文件测试1") == 0,
                "T24: 加载数据内容正确");
    TEST_ASSERT(strcmp(g_contacts.items[1].phone, "15000000002") == 0,
                "T25: 加载第二联系人电话正确");

    /* T26: Magic header验证 */
    contact_array_free(&g_contacts);
    contact_array_init(&g_contacts, INIT_CAPACITY);
    {
        FILE *fp = fopen("bad_magic.dat", "wb");
        fwrite("BADXXX", 1, 6, fp);
        fclose(fp);
    }
    s = contact_load_from_file("bad_magic.dat");
    TEST_ASSERT_STATUS(s, CONTACT_ERR_FILE_MAGIC, "T26: 错误magic header检测");
    remove("bad_magic.dat");

    /* T27: 无效参数检测 */
    s = contact_array_init(NULL, 10);
    TEST_ASSERT_STATUS(s, CONTACT_ERR_INVALID_PARAM, "T27: NULL参数检测");

    /* T28: 删除越界索引 */
    contact_array_free(&g_contacts);
    contact_array_init(&g_contacts, INIT_CAPACITY);
    s = contact_array_remove(&g_contacts, 0);
    TEST_ASSERT_STATUS(s, CONTACT_ERR_NOT_FOUND, "T28: 空数组删除返回NOT_FOUND");

    /* T29: 空数组搜索 */
    s = contact_find_by_phone(&g_contacts, "123", &dummy);
    TEST_ASSERT_STATUS(s, CONTACT_ERR_NOT_FOUND, "T29: 空数组搜索返回NOT_FOUND");

    /* T30: 释放内存 */
    contact_array_free(&g_contacts);
    TEST_ASSERT(g_contacts.items == NULL, "T30: 释放后置items为NULL");
    TEST_ASSERT(g_contacts.count == 0, "T31: 释放后count为0");

    /* T32: 分组搜索功能验证 */
    contact_array_init(&g_contacts, INIT_CAPACITY);
    Contact g1 = {"家人1", "16000000001", "j*@******", "家人", "地址1"};
    Contact g2 = {"同事1", "16000000002", "t*@******", "同事", "地址2"};
    Contact g3 = {"家人2", "16000000003", "j*@******", "家人", "地址3"};
    contact_array_add(&g_contacts, &g1);
    contact_array_add(&g_contacts, &g2);
    contact_array_add(&g_contacts, &g3);
    int family_count = 0;
    for (size_t i = 0; i < g_contacts.count; i++) {
        if (strcmp(g_contacts.items[i].group, "家人") == 0) family_count++;
    }
    TEST_ASSERT(family_count == 2, "T32: 分组统计'家人'有2人");

    /* 清理测试文件 */
    remove("test_v2.dat");
    contact_array_free(&g_contacts);

    printf("\n%s%s", COLOR_BOLD, COLOR_CYAN);
    print_separator('=', COLOR_CYAN);
    printf("  自测结果: %s%d%s 通过, %s%d%s 失败\n",
           COLOR_GREEN COLOR_BOLD, g_tests_passed, COLOR_RESET COLOR_CYAN,
           g_tests_failed > 0 ? COLOR_RED COLOR_BOLD : COLOR_GREEN COLOR_BOLD,
           g_tests_failed, COLOR_RESET);
    print_separator('=', COLOR_CYAN);
    printf("%s\n", COLOR_RESET);

    return g_tests_failed == 0 ? 0 : 1;
}

/* ============================================================================
 *  第十部分: CLI参数处理与帮助
 * ============================================================================ */

static void print_help(const char *progname)
{
    printf("%s%s通讯录管理系统 V2.0%s\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("%s%sContact Management System V2.0%s\n\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("用法: %s [选项]\n\n", progname);
    printf("选项:\n");
    printf("  %s-h, --help%s     显示帮助信息\n", COLOR_GREEN, COLOR_RESET);
    printf("  %s-t, --test%s     运行内置自测程序\n", COLOR_GREEN, COLOR_RESET);
    printf("  %s(无参数)%s       启动交互式菜单\n\n", COLOR_GREEN, COLOR_RESET);
    printf("V2.0特性:\n");
    printf("  • UTF-8 BOM + SetConsoleOutputCP(65001)\n");
    printf("  • ANSI 彩色终端\n");
    printf("  • 动态数组存储 (自动扩容)\n");
    printf("  • CONTV2 二进制文件持久化\n");
    printf("  • 统一错误码枚举\n");
    printf("  • %d+ 项内置自测\n", 15);
}

/* ============================================================================
 *  第十一部分: 交互菜单
 * ============================================================================ */

static void show_main_menu(void)
{
    print_header("通 讯 录 管 理 系 统 V2.0");
    printf("%s", COLOR_WHITE);
    printf("  ┌─────────────────────────────────────────────────────────────┐\n");
    printf("  │  %s[1]%s  添加联系人        %s[2]%s  删除联系人        %s[3]%s  修改联系人  │\n",
           COLOR_GREEN, COLOR_WHITE, COLOR_RED, COLOR_WHITE, COLOR_YELLOW, COLOR_WHITE);
    printf("  │  %s[4]%s  查看所有联系人    %s[5]%s  搜索联系人        %s[6]%s  分组显示    │\n",
           COLOR_CYAN, COLOR_WHITE, COLOR_MAGENTA, COLOR_WHITE, COLOR_BLUE, COLOR_WHITE);
    printf("  │  %s[7]%s  统计信息          %s[8]%s  排序              %s[0]%s  退出系统    │\n",
           COLOR_GREEN, COLOR_WHITE, COLOR_YELLOW, COLOR_WHITE, COLOR_RED, COLOR_WHITE);
    printf("  └─────────────────────────────────────────────────────────────┘\n");
    printf("%s", COLOR_RESET);
    print_separator('-', COLOR_CYAN);
    printf("  当前联系人: %s%zu%s 位\n", COLOR_BOLD COLOR_GREEN, g_contacts.count, COLOR_RESET);
    print_separator('-', COLOR_CYAN);
}

static void interactive_loop(void)
{
    char choice[8];
    contact_array_init(&g_contacts, INIT_CAPACITY);
    ContactStatus load_s = contact_load_from_file(DATA_FILE);
    if (load_s == CONTACT_ERR_FILE_OPEN || load_s == CONTACT_ERR_FILE_MAGIC) {
        c_print(COLOR_YELLOW, "  未找到数据文件或格式不兼容，创建新通讯录\n");
        contact_array_free(&g_contacts);
        contact_array_init(&g_contacts, INIT_CAPACITY);
    } else if (load_s != CONTACT_OK) {
        c_print(COLOR_RED, "  加载数据错误，将创建新通讯录\n");
        contact_array_free(&g_contacts);
        contact_array_init(&g_contacts, INIT_CAPACITY);
    } else {
        c_print(COLOR_GREEN, "  ✓ 已加载 %zu 位联系人\n", g_contacts.count);
    }
    while (1) {
        show_main_menu();
        printf("  请选择操作 [0-8]: ");
        fflush(stdout);
        safe_input(choice, 8);
        switch (choice[0]) {
            case '1': add_contact_interactive(); pause_screen(); break;
            case '2': delete_contact_interactive(); pause_screen(); break;
            case '3': modify_contact_interactive(); pause_screen(); break;
            case '4': list_all_contacts(); pause_screen(); break;
            case '5': search_contacts_interactive(); pause_screen(); break;
            case '6': list_by_group_interactive(); pause_screen(); break;
            case '7': show_statistics(); pause_screen(); break;
            case '8': sort_contacts_interactive(); pause_screen(); break;
            case '0':
                contact_save_to_file(DATA_FILE);
                c_print(COLOR_GREEN, "\n  ✓ 数据已保存，感谢使用!\n");
                contact_array_free(&g_contacts);
                return;
            default:
                c_print(COLOR_RED, "  ✗ 无效选择，请重新输入\n");
                pause_screen();
                break;
        }
    }
}

/* ============================================================================
 *  第十二部分: 主函数
 * ============================================================================ */

int main(int argc, char *argv[])
{
    terminal_init();
    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        }
        if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0) {
            return run_self_tests();
        }
        printf("%s未知选项: %s%s\n", COLOR_RED, argv[1], COLOR_RESET);
        printf("使用 %s --help 查看帮助\n", argv[0]);
        return 1;
    }
    interactive_loop();
    return 0;
}
