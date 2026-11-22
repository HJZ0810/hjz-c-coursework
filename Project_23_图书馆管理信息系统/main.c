/*
 * +==============================================================================+
 * |                                                                            |
 * |         *  图书馆管理信息系统 V2.0 (Library Management System)  *          |
 * |                                                                            |
 * +==============================================================================+
 * |  项目名称: 图书馆管理信息系统 V2.0                                          |
 * |  功能描述: 实现图书、读者、借阅记录的CRUD、查询统计、数据持久化、内置自测    |
 * |  数据结构: 动态数组 (图书+读者+借阅记录统一动态内存管理)                     |
 * |  文件格式: 二进制持久化, magic header "LIBV2"                               |
 * |  编译方式: gcc -std=c99 -Wall -Wextra -O2 -o lib.exe main.c                 |
 * |  学年学期: 2025-2026学年第二学期算法实习                                    |
 * |  开发者:   数据库侠客  学号:424123888888                                    |
 * |  班级组别: 计算机组A班  作者:HuiJunzhuo                                     |
 * |                                                                            |
 * +==============================================================================+
 * |  V2.0 升级要点:                                                            |
 * |    1. UTF-8 BOM + SetConsoleOutputCP(65001) 完美支持中文显示               |
 * |    2. ANSI 彩色终端输出 (错误红/成功绿/提示黄/标题青)                       |
 * |    3. CLI 参数: -h/--help 显示帮助, -t/--test 运行内置自测                  |
 * |    4. 数据结构从链表升级为动态数组, 更高效的随机访问和内存管理              |
 * |    5. 统一错误码枚举 LibStatus, 替代散乱的 printf 错误提示                  |
 * |    6. 二进制文件持久化带 magic header "LIBV2", 防止文件损坏误读            |
 * |    7. 内置自测框架: 15+项测试覆盖 CRUD/借还/统计/文件I/O                    |
 * |    8. 完整中文注释, 符合C99标准, -Wall -Wextra 零警告编译                   |
 * |                                                                            |
 * +==============================================================================+
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

/* ======================== 常量定义 ======================== */

#define MAX_STR         64
#define INIT_CAP        16
#define GROW_FACTOR     2
#define MAX_BORROW_PER_READER 5
#define MAGIC_HEADER    "LIBV2"
#define MAGIC_LEN       5
#define DATA_FILE       "library_v2.dat"

/* ======================== ANSI 颜色定义 ======================== */

#define COLOR_RESET     "\033[0m"
#define COLOR_RED       "\033[31m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_BLUE      "\033[34m"
#define COLOR_MAGENTA   "\033[35m"
#define COLOR_CYAN      "\033[36m"
#define COLOR_WHITE     "\033[37m"
#define COLOR_BOLD      "\033[1m"

/* ======================== 枚举类型定义 ======================== */

typedef enum {
    CAT_LITERATURE = 1,
    CAT_SCIENCE,
    CAT_TECHNOLOGY,
    CAT_HISTORY,
    CAT_PHILOSOPHY,
    CAT_ART,
    CAT_OTHER
} BookCategory;

typedef enum {
    RTYPE_STUDENT = 1,
    RTYPE_TEACHER,
    RTYPE_OTHER
} ReaderType;

typedef enum {
    LIB_OK = 0,
    LIB_ERR_MEMORY,
    LIB_ERR_FILE_OPEN,
    LIB_ERR_FILE_READ,
    LIB_ERR_FILE_WRITE,
    LIB_ERR_FILE_MAGIC,
    LIB_ERR_FILE_CORRUPT,
    LIB_ERR_BOOK_EXIST,
    LIB_ERR_BOOK_NOT_FOUND,
    LIB_ERR_READER_EXIST,
    LIB_ERR_READER_NOT_FOUND,
    LIB_ERR_BOOK_NOT_AVAILABLE,
    LIB_ERR_BORROW_LIMIT,
    LIB_ERR_BORROW_NOT_FOUND,
    LIB_ERR_BORROW_ALREADY_RETURNED,
    LIB_ERR_INVALID_PARAM,
    LIB_ERR_EMPTY_INPUT
} LibStatus;

/* ======================== 数据结构定义 ======================== */

typedef struct {
    char isbn[MAX_STR];
    char title[MAX_STR];
    char author[MAX_STR];
    BookCategory category;
    int total;
    int available;
} Book;

typedef struct {
    char id[MAX_STR];
    char name[MAX_STR];
    char phone[MAX_STR];
    ReaderType type;
    int borrow_count;
} Reader;

typedef struct {
    char reader_id[MAX_STR];
    char isbn[MAX_STR];
    char borrow_date[MAX_STR];
    char return_date[MAX_STR];
    int returned;
} Borrow;

typedef struct {
    Book *items;
    int count;
    int capacity;
} BookArray;

typedef struct {
    Reader *items;
    int count;
    int capacity;
} ReaderArray;

typedef struct {
    Borrow *items;
    int count;
    int capacity;
} BorrowArray;

/* ======================== 全局状态 ======================== */

static BookArray g_books = {NULL, 0, 0};
static ReaderArray g_readers = {NULL, 0, 0};
static BorrowArray g_borrows = {NULL, 0, 0};

/* ======================== 颜色输出工具函数 ======================== */

static void cprint(const char *color, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    printf("%s", color);
    vprintf(fmt, args);
    printf("%s", COLOR_RESET);
    va_end(args);
}

static void print_error(const char *msg)
{
    cprint(COLOR_RED, "  [错误] %s\n", msg);
}

static void print_success(const char *msg)
{
    cprint(COLOR_GREEN, "  [成功] %s\n", msg);
}

static void print_info(const char *msg)
{
    cprint(COLOR_YELLOW, "  [提示] %s\n", msg);
}

static void print_line(void)
{
    cprint(COLOR_CYAN, "==============================================================\n");
}

static void print_title(const char *title)
{
    printf("\n");
    print_line();
    cprint(COLOR_BOLD COLOR_CYAN, "  * %s\n", title);
    print_line();
}

/* ======================== 枚举值转字符串 ======================== */

static const char *category_name(BookCategory cat)
{
    switch (cat) {
        case CAT_LITERATURE: return "文学";
        case CAT_SCIENCE:    return "科学";
        case CAT_TECHNOLOGY: return "技术";
        case CAT_HISTORY:    return "历史";
        case CAT_PHILOSOPHY: return "哲学";
        case CAT_ART:        return "艺术";
        default:             return "其他";
    }
}

static const char *reader_type_name(ReaderType type)
{
    switch (type) {
        case RTYPE_STUDENT: return "学生";
        case RTYPE_TEACHER: return "教师";
        default:            return "其他";
    }
}

static const char *status_message(LibStatus s)
{
    switch (s) {
        case LIB_OK:                      return "操作成功";
        case LIB_ERR_MEMORY:              return "内存分配失败";
        case LIB_ERR_FILE_OPEN:           return "无法打开文件";
        case LIB_ERR_FILE_READ:           return "文件读取失败";
        case LIB_ERR_FILE_WRITE:          return "文件写入失败";
        case LIB_ERR_FILE_MAGIC:          return "文件格式错误(无效magic)";
        case LIB_ERR_FILE_CORRUPT:        return "文件数据损坏";
        case LIB_ERR_BOOK_EXIST:          return "ISBN已存在";
        case LIB_ERR_BOOK_NOT_FOUND:      return "图书不存在";
        case LIB_ERR_READER_EXIST:        return "读者ID已存在";
        case LIB_ERR_READER_NOT_FOUND:    return "读者不存在";
        case LIB_ERR_BOOK_NOT_AVAILABLE:  return "图书已全部借出";
        case LIB_ERR_BORROW_LIMIT:        return "已达借阅上限(5本)";
        case LIB_ERR_BORROW_NOT_FOUND:    return "未找到借阅记录";
        case LIB_ERR_BORROW_ALREADY_RETURNED: return "该记录已归还";
        case LIB_ERR_INVALID_PARAM:       return "无效参数";
        case LIB_ERR_EMPTY_INPUT:         return "输入不能为空";
        default:                          return "未知错误";
    }
}

/* ======================== 动态数组管理 ======================== */

static LibStatus book_array_reserve(int new_cap)
{
    if (new_cap <= g_books.capacity) return LIB_OK;
    Book *tmp = (Book *)realloc(g_books.items, (size_t)new_cap * sizeof(Book));
    if (!tmp) return LIB_ERR_MEMORY;
    g_books.items = tmp;
    g_books.capacity = new_cap;
    return LIB_OK;
}

static LibStatus reader_array_reserve(int new_cap)
{
    if (new_cap <= g_readers.capacity) return LIB_OK;
    Reader *tmp = (Reader *)realloc(g_readers.items, (size_t)new_cap * sizeof(Reader));
    if (!tmp) return LIB_ERR_MEMORY;
    g_readers.items = tmp;
    g_readers.capacity = new_cap;
    return LIB_OK;
}

static LibStatus borrow_array_reserve(int new_cap)
{
    if (new_cap <= g_borrows.capacity) return LIB_OK;
    Borrow *tmp = (Borrow *)realloc(g_borrows.items, (size_t)new_cap * sizeof(Borrow));
    if (!tmp) return LIB_ERR_MEMORY;
    g_borrows.items = tmp;
    g_borrows.capacity = new_cap;
    return LIB_OK;
}

static void free_all(void)
{
    free(g_books.items);
    free(g_readers.items);
    free(g_borrows.items);
    g_books.items = NULL; g_books.count = 0; g_books.capacity = 0;
    g_readers.items = NULL; g_readers.count = 0; g_readers.capacity = 0;
    g_borrows.items = NULL; g_borrows.count = 0; g_borrows.capacity = 0;
}

static LibStatus init_arrays(void)
{
    LibStatus s;
    s = book_array_reserve(INIT_CAP);
    if (s != LIB_OK) return s;
    s = reader_array_reserve(INIT_CAP);
    if (s != LIB_OK) return s;
    s = borrow_array_reserve(INIT_CAP);
    if (s != LIB_OK) return s;
    return LIB_OK;
}

/* ======================== 日期与输入工具 ======================== */

static void get_current_date(char *buf, int size)
{
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buf, (size_t)size, "%Y-%m-%d", tm_info);
}

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
        }
    } else {
        buf[0] = '\0';
    }
}

static int read_int(void)
{
    int val = 0;
    scanf("%d", &val);
    clear_input();
    return val;
}

/* ======================== 查找函数 ======================== */

static int find_book_index(const char *isbn)
{
    for (int i = 0; i < g_books.count; i++) {
        if (strcmp(g_books.items[i].isbn, isbn) == 0) return i;
    }
    return -1;
}

static int find_reader_index(const char *id)
{
    for (int i = 0; i < g_readers.count; i++) {
        if (strcmp(g_readers.items[i].id, id) == 0) return i;
    }
    return -1;
}

static int find_active_borrow_index(const char *reader_id, const char *isbn)
{
    for (int i = 0; i < g_borrows.count; i++) {
        if (!g_borrows.items[i].returned &&
            strcmp(g_borrows.items[i].reader_id, reader_id) == 0 &&
            strcmp(g_borrows.items[i].isbn, isbn) == 0) {
            return i;
        }
    }
    return -1;
}

/* ======================== 文件持久化 ======================== */

static LibStatus save_data(void)
{
    FILE *fp = fopen(DATA_FILE, "wb");
    if (!fp) return LIB_ERR_FILE_OPEN;

    if (fwrite(MAGIC_HEADER, 1, MAGIC_LEN, fp) != MAGIC_LEN) {
        fclose(fp); return LIB_ERR_FILE_WRITE;
    }

    if (fwrite(&g_books.count, sizeof(int), 1, fp) != 1) {
        fclose(fp); return LIB_ERR_FILE_WRITE;
    }
    if (g_books.count > 0) {
        if (fwrite(g_books.items, sizeof(Book), (size_t)g_books.count, fp) != (size_t)g_books.count) {
            fclose(fp); return LIB_ERR_FILE_WRITE;
        }
    }

    if (fwrite(&g_readers.count, sizeof(int), 1, fp) != 1) {
        fclose(fp); return LIB_ERR_FILE_WRITE;
    }
    if (g_readers.count > 0) {
        if (fwrite(g_readers.items, sizeof(Reader), (size_t)g_readers.count, fp) != (size_t)g_readers.count) {
            fclose(fp); return LIB_ERR_FILE_WRITE;
        }
    }

    if (fwrite(&g_borrows.count, sizeof(int), 1, fp) != 1) {
        fclose(fp); return LIB_ERR_FILE_WRITE;
    }
    if (g_borrows.count > 0) {
        if (fwrite(g_borrows.items, sizeof(Borrow), (size_t)g_borrows.count, fp) != (size_t)g_borrows.count) {
            fclose(fp); return LIB_ERR_FILE_WRITE;
        }
    }

    fclose(fp);
    return LIB_OK;
}

static LibStatus load_data(void)
{
    FILE *fp = fopen(DATA_FILE, "rb");
    if (!fp) return LIB_OK;

    char magic[MAGIC_LEN + 1] = {0};
    if (fread(magic, 1, MAGIC_LEN, fp) != MAGIC_LEN) {
        fclose(fp); return LIB_ERR_FILE_MAGIC;
    }
    if (strcmp(magic, MAGIC_HEADER) != 0) {
        fclose(fp); return LIB_ERR_FILE_MAGIC;
    }

    int count = 0;

    if (fread(&count, sizeof(int), 1, fp) != 1) {
        fclose(fp); return LIB_ERR_FILE_CORRUPT;
    }
    if (count > 0) {
        LibStatus s = book_array_reserve(count > INIT_CAP ? count : INIT_CAP);
        if (s != LIB_OK) { fclose(fp); return s; }
        if (fread(g_books.items, sizeof(Book), (size_t)count, fp) != (size_t)count) {
            fclose(fp); return LIB_ERR_FILE_CORRUPT;
        }
        g_books.count = count;
    }

    if (fread(&count, sizeof(int), 1, fp) != 1) {
        fclose(fp); return LIB_ERR_FILE_CORRUPT;
    }
    if (count > 0) {
        LibStatus s = reader_array_reserve(count > INIT_CAP ? count : INIT_CAP);
        if (s != LIB_OK) { fclose(fp); return s; }
        if (fread(g_readers.items, sizeof(Reader), (size_t)count, fp) != (size_t)count) {
            fclose(fp); return LIB_ERR_FILE_CORRUPT;
        }
        g_readers.count = count;
    }

    if (fread(&count, sizeof(int), 1, fp) != 1) {
        fclose(fp); return LIB_ERR_FILE_CORRUPT;
    }
    if (count > 0) {
        LibStatus s = borrow_array_reserve(count > INIT_CAP ? count : INIT_CAP);
        if (s != LIB_OK) { fclose(fp); return s; }
        if (fread(g_borrows.items, sizeof(Borrow), (size_t)count, fp) != (size_t)count) {
            fclose(fp); return LIB_ERR_FILE_CORRUPT;
        }
        g_borrows.count = count;
    }

    fclose(fp);
    return LIB_OK;
}

/* ======================== CRUD: 图书 ======================== */

static LibStatus add_book(const char *isbn, const char *title, const char *author,
                          BookCategory cat, int total)
{
    if (!isbn || !title || !author || total < 0) return LIB_ERR_INVALID_PARAM;
    if (strlen(isbn) == 0 || strlen(title) == 0) return LIB_ERR_EMPTY_INPUT;
    if (find_book_index(isbn) >= 0) return LIB_ERR_BOOK_EXIST;

    if (g_books.count >= g_books.capacity) {
        int new_cap = g_books.capacity == 0 ? INIT_CAP : g_books.capacity * GROW_FACTOR;
        LibStatus s = book_array_reserve(new_cap);
        if (s != LIB_OK) return s;
    }

    Book *b = &g_books.items[g_books.count++];
    memset(b, 0, sizeof(Book));
    strncpy(b->isbn, isbn, MAX_STR - 1);
    strncpy(b->title, title, MAX_STR - 1);
    strncpy(b->author, author, MAX_STR - 1);
    b->category = cat;
    b->total = total;
    b->available = total;
    return LIB_OK;
}

static LibStatus modify_book(const char *isbn, const char *title, const char *author,
                             BookCategory cat)
{
    int idx = find_book_index(isbn);
    if (idx < 0) return LIB_ERR_BOOK_NOT_FOUND;
    Book *b = &g_books.items[idx];
    if (title && strlen(title) > 0) strncpy(b->title, title, MAX_STR - 1);
    if (author && strlen(author) > 0) strncpy(b->author, author, MAX_STR - 1);
    if (cat >= CAT_LITERATURE && cat <= CAT_OTHER) b->category = cat;
    return LIB_OK;
}

static LibStatus delete_book(const char *isbn)
{
    int idx = find_book_index(isbn);
    if (idx < 0) return LIB_ERR_BOOK_NOT_FOUND;
    Book *b = &g_books.items[idx];
    if (b->available < b->total) return LIB_ERR_BORROW_NOT_FOUND;

    for (int i = idx; i < g_books.count - 1; i++) {
        g_books.items[i] = g_books.items[i + 1];
    }
    g_books.count--;
    memset(&g_books.items[g_books.count], 0, sizeof(Book));
    return LIB_OK;
}

/* ======================== CRUD: 读者 ======================== */

static LibStatus add_reader(const char *id, const char *name, const char *phone,
                            ReaderType type)
{
    if (!id || !name || strlen(id) == 0 || strlen(name) == 0) return LIB_ERR_EMPTY_INPUT;
    if (find_reader_index(id) >= 0) return LIB_ERR_READER_EXIST;

    if (g_readers.count >= g_readers.capacity) {
        int new_cap = g_readers.capacity == 0 ? INIT_CAP : g_readers.capacity * GROW_FACTOR;
        LibStatus s = reader_array_reserve(new_cap);
        if (s != LIB_OK) return s;
    }

    Reader *r = &g_readers.items[g_readers.count++];
    memset(r, 0, sizeof(Reader));
    strncpy(r->id, id, MAX_STR - 1);
    strncpy(r->name, name, MAX_STR - 1);
    strncpy(r->phone, phone ? phone : "", MAX_STR - 1);
    r->type = type;
    r->borrow_count = 0;
    return LIB_OK;
}

static LibStatus modify_reader(const char *id, const char *name, const char *phone,
                               ReaderType type)
{
    int idx = find_reader_index(id);
    if (idx < 0) return LIB_ERR_READER_NOT_FOUND;
    Reader *r = &g_readers.items[idx];
    if (name && strlen(name) > 0) strncpy(r->name, name, MAX_STR - 1);
    if (phone && strlen(phone) > 0) strncpy(r->phone, phone, MAX_STR - 1);
    if (type >= RTYPE_STUDENT && type <= RTYPE_OTHER) r->type = type;
    return LIB_OK;
}

static LibStatus delete_reader(const char *id)
{
    int idx = find_reader_index(id);
    if (idx < 0) return LIB_ERR_READER_NOT_FOUND;
    Reader *r = &g_readers.items[idx];
    if (r->borrow_count > 0) return LIB_ERR_BORROW_NOT_FOUND;

    for (int i = idx; i < g_readers.count - 1; i++) {
        g_readers.items[i] = g_readers.items[i + 1];
    }
    g_readers.count--;
    memset(&g_readers.items[g_readers.count], 0, sizeof(Reader));
    return LIB_OK;
}

/* ======================== 借还书业务逻辑 ======================== */

static LibStatus borrow_book(const char *reader_id, const char *isbn)
{
    int ri = find_reader_index(reader_id);
    if (ri < 0) return LIB_ERR_READER_NOT_FOUND;
    int bi = find_book_index(isbn);
    if (bi < 0) return LIB_ERR_BOOK_NOT_FOUND;

    Reader *r = &g_readers.items[ri];
    Book *b = &g_books.items[bi];

    if (b->available <= 0) return LIB_ERR_BOOK_NOT_AVAILABLE;
    if (r->borrow_count >= MAX_BORROW_PER_READER) return LIB_ERR_BORROW_LIMIT;
    if (find_active_borrow_index(reader_id, isbn) >= 0) return LIB_ERR_BORROW_ALREADY_RETURNED;

    if (g_borrows.count >= g_borrows.capacity) {
        int new_cap = g_borrows.capacity == 0 ? INIT_CAP : g_borrows.capacity * GROW_FACTOR;
        LibStatus s = borrow_array_reserve(new_cap);
        if (s != LIB_OK) return s;
    }

    Borrow *br = &g_borrows.items[g_borrows.count++];
    memset(br, 0, sizeof(Borrow));
    strncpy(br->reader_id, reader_id, MAX_STR - 1);
    strncpy(br->isbn, isbn, MAX_STR - 1);
    get_current_date(br->borrow_date, MAX_STR);
    br->returned = 0;

    b->available--;
    r->borrow_count++;
    return LIB_OK;
}

static LibStatus return_book(const char *reader_id, const char *isbn)
{
    int bri = find_active_borrow_index(reader_id, isbn);
    if (bri < 0) return LIB_ERR_BORROW_NOT_FOUND;
    Borrow *br = &g_borrows.items[bri];
    if (br->returned) return LIB_ERR_BORROW_ALREADY_RETURNED;

    int bi = find_book_index(isbn);
    int ri = find_reader_index(reader_id);
    if (bi >= 0) g_books.items[bi].available++;
    if (ri >= 0 && g_readers.items[ri].borrow_count > 0) {
        g_readers.items[ri].borrow_count--;
    }

    br->returned = 1;
    get_current_date(br->return_date, MAX_STR);
    return LIB_OK;
}

/* ======================== 统计功能 ======================== */

typedef struct {
    int total_books;
    int total_copies;
    int available_copies;
    int borrowed_copies;
    int total_readers;
    int total_borrow_records;
    int active_borrows;
} SystemStats;

static SystemStats get_stats(void)
{
    SystemStats s = {0};
    s.total_books = g_books.count;
    for (int i = 0; i < g_books.count; i++) {
        s.total_copies += g_books.items[i].total;
        s.available_copies += g_books.items[i].available;
    }
    s.borrowed_copies = s.total_copies - s.available_copies;
    s.total_readers = g_readers.count;
    s.total_borrow_records = g_borrows.count;
    for (int i = 0; i < g_borrows.count; i++) {
        if (!g_borrows.items[i].returned) s.active_borrows++;
    }
    return s;
}

/* ======================== 打印显示函数 ======================== */

static void print_book_header(void)
{
    cprint(COLOR_BOLD, "  %-12s %-18s %-12s %-8s %-6s %-6s\n",
           "ISBN", "书名", "作者", "分类", "总数", "可借");
    printf("  ------------ ------------------ ------------ -------- ------ ------\n");
}

static void print_book(const Book *b)
{
    printf("  %-12s %-18s %-12s %-8s %-6d %-6d\n",
           b->isbn, b->title, b->author, category_name(b->category),
           b->total, b->available);
}

static void print_reader_header(void)
{
    cprint(COLOR_BOLD, "  %-10s %-10s %-15s %-8s %-8s\n",
           "编号", "姓名", "电话", "类型", "借阅中");
    printf("  ---------- ---------- --------------- -------- --------\n");
}

static void print_reader(const Reader *r)
{
    printf("  %-10s %-10s %-15s %-8s %-8d\n",
           r->id, r->name, r->phone, reader_type_name(r->type), r->borrow_count);
}

static void print_borrow_header(void)
{
    cprint(COLOR_BOLD, "  %-10s %-12s %-12s %-12s %-8s\n",
           "读者编号", "ISBN", "借出日期", "归还日期", "状态");
    printf("  ---------- ------------ ------------ ------------ --------\n");
}

static void print_borrow(const Borrow *br)
{
    printf("  %-10s %-12s %-12s %-12s %-8s\n",
           br->reader_id, br->isbn, br->borrow_date,
           br->returned ? br->return_date : "--",
           br->returned ? "已归还" : "借阅中");
}

/* ======================== 初始化示例数据 ======================== */

static void init_sample_data(void)
{
    if (g_books.count > 0 || g_readers.count > 0) return;

    add_book("ISBN-0001", "C程序设计语言", "K&R", CAT_TECHNOLOGY, 3);
    add_book("ISBN-0002", "数据结构与算法", "严蔚敏", CAT_TECHNOLOGY, 3);
    add_book("ISBN-0003", "操作系统概念", "Silberschatz", CAT_TECHNOLOGY, 2);
    add_book("ISBN-0004", "数据库系统概论", "王珊", CAT_TECHNOLOGY, 2);
    add_book("ISBN-0005", "时间简史", "霍金", CAT_SCIENCE, 3);
    add_book("ISBN-0006", "人类简史", "赫拉利", CAT_HISTORY, 2);
    add_book("ISBN-0007", "编译原理", "龙书", CAT_TECHNOLOGY, 2);
    add_book("ISBN-0008", "计算机网络", "谢希仁", CAT_TECHNOLOGY, 3);

    add_reader("R0001", "张三", "13800000001", RTYPE_STUDENT);
    add_reader("R0002", "李四", "13800000002", RTYPE_STUDENT);
    add_reader("R0003", "王老师", "13800000003", RTYPE_TEACHER);
    add_reader("R0004", "赵六", "13800000004", RTYPE_STUDENT);
    add_reader("R0005", "钱七", "13800000005", RTYPE_OTHER);

    borrow_book("R0001", "ISBN-0001");
    return_book("R0001", "ISBN-0001");
    borrow_book("R0002", "ISBN-0003");
}

/* ======================== 内置自测框架 ======================== */

static int g_test_passed = 0;
static int g_test_failed = 0;

static void test_report(const char *name, int passed)
{
    if (passed) {
        cprint(COLOR_GREEN, "  [PASS] %s\n", name);
        g_test_passed++;
    } else {
        cprint(COLOR_RED, "  [FAIL] %s\n", name);
        g_test_failed++;
    }
}

static int run_self_tests(void)
{
    g_test_passed = 0;
    g_test_failed = 0;

    cprint(COLOR_BOLD COLOR_CYAN, "\n=== 图书馆管理系统 V2.0 内置自测 ===\n\n");

    /* 测试1: 初始化 */
    test_report("T01 系统初始化(动态数组创建)", init_arrays() == LIB_OK);

    /* 测试2: 添加图书 */
    LibStatus s;
    s = add_book("T-001", "测试图书A", "测试作者A", CAT_TECHNOLOGY, 5);
    test_report("T02 添加图书(正常)", s == LIB_OK && g_books.count == 1);

    /* 测试3: 添加重复ISBN */
    s = add_book("T-001", "重复图书", "作者", CAT_LITERATURE, 1);
    test_report("T03 添加重复ISBN(应失败)", s == LIB_ERR_BOOK_EXIST);

    /* 测试4: 查询图书 */
    int idx = find_book_index("T-001");
    test_report("T04 查询图书(存在)", idx >= 0 && g_books.items[idx].total == 5);

    /* 测试5: 查询不存在图书 */
    idx = find_book_index("T-NOTEXIST");
    test_report("T05 查询不存在图书(应-1)", idx == -1);

    /* 测试6: 修改图书 */
    s = modify_book("T-001", "测试图书A(修订版)", NULL, CAT_SCIENCE);
    test_report("T06 修改图书信息", s == LIB_OK && strcmp(g_books.items[0].title, "测试图书A(修订版)") == 0);

    /* 测试7: 添加读者 */
    s = add_reader("TR01", "测试读者1", "13900000001", RTYPE_STUDENT);
    test_report("T07 添加读者(正常)", s == LIB_OK && g_readers.count == 1);

    /* 测试8: 添加重复读者 */
    s = add_reader("TR01", "重复读者", "13900000002", RTYPE_TEACHER);
    test_report("T08 添加重复读者ID(应失败)", s == LIB_ERR_READER_EXIST);

    /* 测试9: 查询读者 */
    int ri = find_reader_index("TR01");
    test_report("T09 查询读者(存在)", ri >= 0 && strcmp(g_readers.items[ri].name, "测试读者1") == 0);

    /* 测试10: 借书成功 */
    s = borrow_book("TR01", "T-001");
    int avail_after_borrow = g_books.items[0].available;
    int count_after_borrow = g_readers.items[0].borrow_count;
    test_report("T10 借书成功", s == LIB_OK && avail_after_borrow == 4 && count_after_borrow == 1);

    /* 测试11: 还书成功 */
    s = return_book("TR01", "T-001");
    test_report("T11 还书成功", s == LIB_OK && g_books.items[0].available == 5 && g_readers.items[0].borrow_count == 0);

    /* 测试12: 还不存在的借阅记录 */
    s = return_book("TR01", "T-001");
    test_report("T12 归还无记录(应失败)", s == LIB_ERR_BORROW_NOT_FOUND);

    /* 测试13: 借不存在的图书 */
    s = borrow_book("TR01", "T-NONEXIST");
    test_report("T13 借不存在图书(应失败)", s == LIB_ERR_BOOK_NOT_FOUND);

    /* 测试14: 借不存在读者 */
    s = borrow_book("TR-NONEXIST", "T-001");
    test_report("T14 不存在读者借书(应失败)", s == LIB_ERR_READER_NOT_FOUND);

    /* 测试15: 统计功能 */
    add_book("T-002", "统计测试书", "Stat", CAT_OTHER, 2);
    add_reader("TR02", "统计读者", "13900000002", RTYPE_OTHER);
    borrow_book("TR02", "T-002");
    SystemStats stats = get_stats();
    test_report("T15 统计功能验证", stats.total_books == 2 && stats.total_readers == 2 && stats.active_borrows == 1);

    /* 测试16: 文件保存 */
    s = save_data();
    test_report("T16 文件保存(save_data)", s == LIB_OK);

    /* 测试17: 清空并重新加载 */
    int saved_books = g_books.count;
    int saved_readers = g_readers.count;
    int saved_borrows = g_borrows.count;
    free_all();
    init_arrays();
    s = load_data();
    int load_ok = (s == LIB_OK && g_books.count == saved_books &&
                   g_readers.count == saved_readers && g_borrows.count == saved_borrows);
    test_report("T17 文件加载(load_data)", load_ok);

    /* 测试18: 数据一致性验证 */
    int bi2 = find_book_index("T-002");
    int ri2 = find_reader_index("TR02");
    int consistency = (bi2 >= 0 && ri2 >= 0 &&
                       g_books.items[bi2].available == 1 &&
                       g_readers.items[ri2].borrow_count == 1);
    test_report("T18 持久化数据一致性", consistency);

    /* 测试19: 借阅上限测试(借5本) */
    add_book("L-001", "限借书1", "L", CAT_LITERATURE, 1);
    add_book("L-002", "限借书2", "L", CAT_LITERATURE, 1);
    add_book("L-003", "限借书3", "L", CAT_LITERATURE, 1);
    add_book("L-004", "限借书4", "L", CAT_LITERATURE, 1);
    add_book("L-005", "限借书5", "L", CAT_LITERATURE, 1);
    add_book("L-006", "限借书6", "L", CAT_LITERATURE, 1);
    add_reader("L-R01", "上限测试读者", "139", RTYPE_STUDENT);
    borrow_book("L-R01", "L-001");
    borrow_book("L-R01", "L-002");
    borrow_book("L-R01", "L-003");
    borrow_book("L-R01", "L-004");
    borrow_book("L-R01", "L-005");
    s = borrow_book("L-R01", "L-006");
    test_report("T19 借阅上限(5本)限制", s == LIB_ERR_BORROW_LIMIT);

    /* 测试20: 删除有借出的图书(应失败) */
    add_reader("D-R01", "删除测试读者", "139", RTYPE_STUDENT);
    add_book("D-001", "待删图书", "D", CAT_OTHER, 1);
    borrow_book("D-R01", "D-001");
    s = delete_book("D-001");
    test_report("T20 删除借出中图书(应失败)", s == LIB_ERR_BORROW_NOT_FOUND);

    /* 测试21: 空参数验证 */
    s = add_book("", "空ISBN书", "A", CAT_OTHER, 1);
    test_report("T21 空输入参数校验", s == LIB_ERR_EMPTY_INPUT);

    /* 测试22: 修改不存在图书 */
    s = modify_book("NOTEXIST-ISBN", "X", "X", CAT_OTHER);
    test_report("T22 修改不存在图书(应失败)", s == LIB_ERR_BOOK_NOT_FOUND);

    /* 测试23: 删除不存在读者 */
    s = delete_reader("NO-SUCH-READER");
    test_report("T23 删除不存在读者(应失败)", s == LIB_ERR_READER_NOT_FOUND);

    /* 测试24: 修改读者 */
    add_reader("M-R01", "原姓名", "1390", RTYPE_STUDENT);
    s = modify_reader("M-R01", "新姓名", "13999999999", RTYPE_TEACHER);
    int mi = find_reader_index("M-R01");
    test_report("T24 修改读者信息", s == LIB_OK && mi >= 0 &&
               strcmp(g_readers.items[mi].name, "新姓名") == 0 &&
               g_readers.items[mi].type == RTYPE_TEACHER);

    /* 测试25: 删除图书(正常) */
    add_book("DEL-001", "待删正常书", "D", CAT_OTHER, 1);
    int bc_before = g_books.count;
    s = delete_book("DEL-001");
    test_report("T25 删除图书(正常)", s == LIB_OK && g_books.count == bc_before - 1);

    printf("\n");
    print_line();
    cprint(COLOR_BOLD, "  测试结果: ");
    cprint(COLOR_GREEN, "%d 通过", g_test_passed);
    printf(", ");
    cprint(COLOR_RED, "%d 失败", g_test_failed);
    printf("\n");
    print_line();

    remove(DATA_FILE);
    return g_test_failed == 0 ? 0 : 1;
}

/* ======================== 交互菜单 ======================== */

static void menu_book_add(void)
{
    print_title("添加图书");
    char isbn[MAX_STR], title[MAX_STR], author[MAX_STR];
    int cat, total;
    printf("  输入ISBN: "); read_line(isbn, MAX_STR);
    printf("  输入书名: "); read_line(title, MAX_STR);
    printf("  输入作者: "); read_line(author, MAX_STR);
    printf("  分类(1文学2科学3技术4历史5哲学6艺术7其他): "); cat = read_int();
    printf("  馆藏数量: "); total = read_int();
    LibStatus s = add_book(isbn, title, author, (BookCategory)cat, total);
    if (s == LIB_OK) { save_data(); print_success("图书添加成功"); }
    else print_error(status_message(s));
}

static void menu_book_modify(void)
{
    print_title("修改图书");
    char isbn[MAX_STR], title[MAX_STR], author[MAX_STR];
    int cat;
    printf("  输入要修改的ISBN: "); read_line(isbn, MAX_STR);
    int idx = find_book_index(isbn);
    if (idx < 0) { print_error("图书不存在"); return; }
    print_book_header(); print_book(&g_books.items[idx]);
    printf("  新书名(回车不修改): "); read_line(title, MAX_STR);
    printf("  新作者(回车不修改): "); read_line(author, MAX_STR);
    printf("  新分类(1-7, 0不修改): "); cat = read_int();
    LibStatus s = modify_book(isbn, title, author, (BookCategory)cat);
    if (s == LIB_OK) { save_data(); print_success("修改成功"); }
    else print_error(status_message(s));
}

static void menu_book_delete(void)
{
    print_title("删除图书");
    char isbn[MAX_STR];
    printf("  输入要删除的ISBN: "); read_line(isbn, MAX_STR);
    LibStatus s = delete_book(isbn);
    if (s == LIB_OK) { save_data(); print_success("删除成功"); }
    else print_error(status_message(s));
}

static void menu_book_list(void)
{
    print_title("图书列表");
    if (g_books.count == 0) { print_info("暂无图书"); return; }
    print_book_header();
    for (int i = 0; i < g_books.count; i++) print_book(&g_books.items[i]);
    printf("  共 %d 种图书\n", g_books.count);
}

static void menu_book_search(void)
{
    print_title("查询图书");
    char kw[MAX_STR];
    printf("  输入书名关键字: "); read_line(kw, MAX_STR);
    int cnt = 0;
    print_book_header();
    for (int i = 0; i < g_books.count; i++) {
        if (strstr(g_books.items[i].title, kw) || strstr(g_books.items[i].isbn, kw)) {
            print_book(&g_books.items[i]); cnt++;
        }
    }
    printf("  找到 %d 条记录\n", cnt);
}

static void menu_book(void)
{
    while (1) {
        print_title("图书管理");
        printf("  1. 添加图书\n  2. 修改图书\n  3. 删除图书\n  4. 图书列表\n  5. 查询图书\n  0. 返回\n");
        print_line(); printf("  选择: ");
        switch (read_int()) {
            case 1: menu_book_add(); break;
            case 2: menu_book_modify(); break;
            case 3: menu_book_delete(); break;
            case 4: menu_book_list(); break;
            case 5: menu_book_search(); break;
            case 0: return;
            default: print_error("无效选择");
        }
    }
}

static void menu_reader_add(void)
{
    print_title("添加读者");
    char id[MAX_STR], name[MAX_STR], phone[MAX_STR];
    int type;
    printf("  读者编号: "); read_line(id, MAX_STR);
    printf("  姓名: "); read_line(name, MAX_STR);
    printf("  电话: "); read_line(phone, MAX_STR);
    printf("  类型(1学生2教师3其他): "); type = read_int();
    LibStatus s = add_reader(id, name, phone, (ReaderType)type);
    if (s == LIB_OK) { save_data(); print_success("读者添加成功"); }
    else print_error(status_message(s));
}

static void menu_reader_modify(void)
{
    print_title("修改读者");
    char id[MAX_STR], name[MAX_STR], phone[MAX_STR];
    int type;
    printf("  读者编号: "); read_line(id, MAX_STR);
    int idx = find_reader_index(id);
    if (idx < 0) { print_error("读者不存在"); return; }
    print_reader_header(); print_reader(&g_readers.items[idx]);
    printf("  新姓名(回车不修改): "); read_line(name, MAX_STR);
    printf("  新电话(回车不修改): "); read_line(phone, MAX_STR);
    printf("  新类型(1-3, 0不修改): "); type = read_int();
    LibStatus s = modify_reader(id, name, phone, (ReaderType)type);
    if (s == LIB_OK) { save_data(); print_success("修改成功"); }
    else print_error(status_message(s));
}

static void menu_reader_delete(void)
{
    print_title("删除读者");
    char id[MAX_STR];
    printf("  读者编号: "); read_line(id, MAX_STR);
    LibStatus s = delete_reader(id);
    if (s == LIB_OK) { save_data(); print_success("删除成功"); }
    else print_error(status_message(s));
}

static void menu_reader_list(void)
{
    print_title("读者列表");
    if (g_readers.count == 0) { print_info("暂无读者"); return; }
    print_reader_header();
    for (int i = 0; i < g_readers.count; i++) print_reader(&g_readers.items[i]);
    printf("  共 %d 位读者\n", g_readers.count);
}

static void menu_reader(void)
{
    while (1) {
        print_title("读者管理");
        printf("  1. 添加读者\n  2. 修改读者\n  3. 删除读者\n  4. 读者列表\n  0. 返回\n");
        print_line(); printf("  选择: ");
        switch (read_int()) {
            case 1: menu_reader_add(); break;
            case 2: menu_reader_modify(); break;
            case 3: menu_reader_delete(); break;
            case 4: menu_reader_list(); break;
            case 0: return;
            default: print_error("无效选择");
        }
    }
}

static void menu_borrow(void)
{
    print_title("借书");
    char rid[MAX_STR], isbn[MAX_STR];
    printf("  读者编号: "); read_line(rid, MAX_STR);
    printf("  图书ISBN: "); read_line(isbn, MAX_STR);
    LibStatus s = borrow_book(rid, isbn);
    if (s == LIB_OK) { save_data(); print_success("借书成功"); }
    else print_error(status_message(s));
}

static void menu_return(void)
{
    print_title("还书");
    char rid[MAX_STR], isbn[MAX_STR];
    printf("  读者编号: "); read_line(rid, MAX_STR);
    printf("  图书ISBN: "); read_line(isbn, MAX_STR);
    LibStatus s = return_book(rid, isbn);
    if (s == LIB_OK) { save_data(); print_success("还书成功"); }
    else print_error(status_message(s));
}

static void menu_borrow_list(void)
{
    print_title("借阅记录");
    printf("  1. 全部记录\n  2. 仅借阅中\n  选择: ");
    int opt = read_int();
    int cnt = 0;
    print_borrow_header();
    for (int i = 0; i < g_borrows.count; i++) {
        if (opt == 2 && g_borrows.items[i].returned) continue;
        print_borrow(&g_borrows.items[i]); cnt++;
    }
    printf("  共 %d 条记录\n", cnt);
}

static void menu_borrow_menu(void)
{
    while (1) {
        print_title("借还管理");
        printf("  1. 借书\n  2. 还书\n  3. 借阅记录\n  0. 返回\n");
        print_line(); printf("  选择: ");
        switch (read_int()) {
            case 1: menu_borrow(); break;
            case 2: menu_return(); break;
            case 3: menu_borrow_list(); break;
            case 0: return;
            default: print_error("无效选择");
        }
    }
}

static void menu_stats(void)
{
    print_title("系统统计");
    SystemStats s = get_stats();
    cprint(COLOR_CYAN, "  图书种类:     %d\n", s.total_books);
    cprint(COLOR_CYAN, "  馆藏总册数:   %d\n", s.total_copies);
    cprint(COLOR_GREEN, "  可借册数:     %d\n", s.available_copies);
    cprint(COLOR_MAGENTA, "  已借出册数:   %d\n", s.borrowed_copies);
    cprint(COLOR_CYAN, "  读者总数:     %d\n", s.total_readers);
    cprint(COLOR_CYAN, "  借阅记录总数: %d\n", s.total_borrow_records);
    cprint(COLOR_YELLOW, "  当前借阅中:   %d\n", s.active_borrows);
}

static void main_menu_loop(void)
{
    while (1) {
        print_title("图书馆管理信息系统 V2.0");
        SystemStats st = get_stats();
        cprint(COLOR_GREEN, "  图书:%d 读者:%d 在借:%d\n\n",
               st.total_books, st.total_readers, st.active_borrows);
        printf("  1. 图书管理\n  2. 读者管理\n  3. 借还管理\n  4. 查询统计\n  0. 退出系统\n");
        print_line(); printf("  选择: ");
        switch (read_int()) {
            case 1: menu_book(); break;
            case 2: menu_reader(); break;
            case 3: menu_borrow_menu(); break;
            case 4: menu_stats(); break;
            case 0:
                save_data();
                cprint(COLOR_GREEN COLOR_BOLD, "\n  感谢使用,数据已保存! 再见!\n\n");
                return;
            default: print_error("无效选择");
        }
    }
}

/* ======================== CLI帮助 ======================== */

static void print_help(void)
{
    cprint(COLOR_BOLD COLOR_CYAN, "\n图书馆管理信息系统 V2.0\n");
    print_line();
    printf("  用法: lib.exe [选项]\n\n");
    cprint(COLOR_BOLD, "  选项:\n");
    printf("    -h, --help     显示帮助信息\n");
    printf("    -t, --test     运行内置自测(25项测试)\n");
    printf("    (无参数)       启动交互菜单模式\n\n");
    cprint(COLOR_BOLD, "  功能:\n");
    printf("    * 图书管理: 添加/修改/删除/查询/列出\n");
    printf("    * 读者管理: 添加/修改/删除/列出\n");
    printf("    * 借还管理: 借书/还书/借阅记录\n");
    printf("    * 统计: 图书/读者/借阅数据统计\n");
    printf("    * 持久化: LIBV2 二进制格式存储\n\n");
    print_line();
}

/* ======================== 控制台初始化 ======================== */

static void setup_console(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(hOut, &mode)) {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, mode);
    }
#endif
}

/* ======================== 主函数 ======================== */

int main(int argc, char *argv[])
{
    setup_console();

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_help();
            return 0;
        }
        if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0) {
            if (init_arrays() != LIB_OK) {
                print_error("初始化失败");
                return 1;
            }
            int ret = run_self_tests();
            free_all();
            return ret;
        }
        print_error("未知参数, 使用 -h 查看帮助");
        return 1;
    }

    if (init_arrays() != LIB_OK) {
        print_error("内存初始化失败");
        return 1;
    }

    LibStatus load_s = load_data();
    if (load_s != LIB_OK && load_s != LIB_ERR_FILE_OPEN) {
        print_error(status_message(load_s));
    }
    init_sample_data();

    cprint(COLOR_BOLD COLOR_CYAN, "\n");
    cprint(COLOR_BOLD COLOR_CYAN, "  图书馆管理信息系统 V2.0\n");
    cprint(COLOR_CYAN, "  数据库侠客 | 424123888888 | HuiJunzhuo\n");
    printf("\n");

    main_menu_loop();

    free_all();
    return 0;
}
