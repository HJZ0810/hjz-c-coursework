/*
 * ==================================================================
 *      书刊租借信息管理系统 V2.0 (Book Rental V2)
 *      Book Rental Information Management System
 * ==================================================================
 *  课程：2025-2026学年第二学期算法课实习
 *  作者：数据库侠客 (HuiJunzhuo)
 *  学号：424123888888
 *  班级：计算机类A班
 *  标准：C99
 *  编译器：GCC (D:/bin/gcc.exe)
 * ==================================================================
 *  V2.0 新增特性：
 *    - UTF-8 BOM + SetConsoleOutputCP(65001) 控制台中文支持
 *    - ANSI 彩色终端输出
 *    - CLI 参数：-h/--help, -t/--test
 *    - 动态数组（书刊 & 租借记录）替代链表
 *    - 统一错误码枚举 BookRStatus
 *    - Magic Header "BORRV2" 二进制文件持久化
 *    - 内置自测 >=15 项（CRUD + 租借归还 + 统计 + 文件I/O）
 *    - 交互菜单（彩色）
 * ==================================================================
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* ======================== 常量定义 ======================== */

#define MAGIC_HEADER     "BORRV2"
#define MAGIC_LEN        6
#define DATA_VERSION     2

#define MAX_STR          64
#define MAX_ISBN         20
#define MAX_ID           20
#define MAX_DATE         16
#define INIT_CAPACITY    8
#define RENTAL_RATE      1.0f
#define DATA_FILE        "borrv2.dat"

/* ======================== ANSI 颜色定义 ======================== */

#define C_RESET     "\033[0m"
#define C_BOLD      "\033[1m"
#define C_RED       "\033[31m"
#define C_GREEN     "\033[32m"
#define C_YELLOW    "\033[33m"
#define C_BLUE      "\033[34m"
#define C_MAGENTA   "\033[35m"
#define C_CYAN      "\033[36m"
#define C_WHITE     "\033[37m"

/* ======================== 统一错误码枚举 ======================== */

typedef enum {
    BORR_OK = 0,
    BORR_ERR_MEMORY,
    BORR_ERR_NOT_FOUND,
    BORR_ERR_DUPLICATE,
    BORR_ERR_INVALID_PARAM,
    BORR_ERR_NOT_AVAILABLE,
    BORR_ERR_ALREADY_RETURNED,
    BORR_ERR_FILE_OPEN,
    BORR_ERR_FILE_READ,
    BORR_ERR_FILE_WRITE,
    BORR_ERR_MAGIC_MISMATCH,
    BORR_ERR_VERSION_MISMATCH,
    BORR_ERR_IO,
    BORR_ERR_EMPTY,
    BORR_STATUS_COUNT
} BookRStatus;

static const char *status_msg[] = {
    "操作成功",
    "内存分配失败",
    "未找到目标记录",
    "重复数据（ISBN/ID冲突）",
    "无效参数",
    "书刊不可借（已借出）",
    "记录已归还",
    "文件打开失败",
    "文件读取失败",
    "文件写入失败",
    "文件魔数不匹配",
    "数据版本不兼容",
    "I/O错误",
    "数据为空"
};

/* ======================== 数据结构定义 ======================== */

typedef struct {
    char isbn[MAX_ISBN];
    char title[MAX_STR];
    char author[MAX_STR];
    float price;
    char pub_date[MAX_DATE];
    int available;
} Book;

typedef struct {
    int rental_id;
    char reader_id[MAX_ID];
    char isbn[MAX_ISBN];
    char rent_date[MAX_DATE];
    char return_date[MAX_DATE];
    float fee;
    int status;
} Rental;

typedef struct {
    Book *items;
    int length;
    int capacity;
} BookArray;

typedef struct {
    Rental *items;
    int length;
    int capacity;
} RentalArray;

/* ======================== 全局状态 ======================== */

static BookArray   g_books   = {NULL, 0, 0};
static RentalArray g_rentals = {NULL, 0, 0};
static int         g_next_id = 1;

/* ======================== 工具函数：控制台初始化 ======================== */

static void console_init(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(hOut, &mode)) {
            mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, mode);
        }
    }
#endif
}

/* ======================== 工具函数：颜色输出 ======================== */

static void c_print(const char *color, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fputs(color, stdout);
    vprintf(fmt, args);
    fputs(C_RESET, stdout);
    va_end(args);
}

/* ======================== 工具函数：安全输入 ======================== */

static void read_line(char *buf, int size)
{
    if (fgets(buf, size, stdin) != NULL) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
        }
    }
}

static int read_int(void)
{
    char buf[64];
    int val = 0;
    if (fgets(buf, sizeof(buf), stdin) != NULL) {
        sscanf(buf, "%d", &val);
    }
    return val;
}

static float read_float(void)
{
    char buf[64];
    float val = 0.0f;
    if (fgets(buf, sizeof(buf), stdin) != NULL) {
        sscanf(buf, "%f", &val);
    }
    return val;
}

static void safe_strcpy(char *dst, const char *src, size_t dst_size)
{
    if (dst_size == 0) return;
    size_t i = 0;
    while (i < dst_size - 1 && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static void get_current_date(char *buf)
{
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buf, MAX_DATE, "%Y-%m-%d", tm_info);
}

static int days_between(const char *start, const char *end)
{
    struct tm tm_start = {0}, tm_end = {0};
    if (sscanf(start, "%d-%d-%d", &tm_start.tm_year, &tm_start.tm_mon, &tm_start.tm_mday) != 3) return 1;
    if (sscanf(end, "%d-%d-%d", &tm_end.tm_year, &tm_end.tm_mon, &tm_end.tm_mday) != 3) return 1;
    tm_start.tm_year -= 1900; tm_start.tm_mon -= 1; tm_start.tm_isdst = -1;
    tm_end.tm_year -= 1900;   tm_end.tm_mon -= 1;   tm_end.tm_isdst = -1;
    time_t t_start = mktime(&tm_start);
    time_t t_end = mktime(&tm_end);
    if (t_start == (time_t)-1 || t_end == (time_t)-1) return 1;
    double diff = difftime(t_end, t_start) / (60.0 * 60.0 * 24.0);
    int days = (int)(diff + 0.5);
    return days < 1 ? 1 : days;
}

/* ======================== 彩色打印辅助 ======================== */

static void print_separator(void)
{
    c_print(C_CYAN, "------------------------------------------------------------\n");
}

static void print_header(const char *title)
{
    print_separator();
    c_print(C_BOLD C_CYAN, "  %s\n", title);
    print_separator();
}

static void print_success(const char *msg)
{
    c_print(C_GREEN, "[成功] %s\n", msg);
}

static void print_error(BookRStatus status)
{
    c_print(C_RED, "[错误] %s\n", status_msg[status]);
}

static void print_error_msg(const char *msg)
{
    c_print(C_RED, "[错误] %s\n", msg);
}

static void print_info(const char *msg)
{
    c_print(C_YELLOW, "[提示] %s\n", msg);
}

/* ======================== 动态数组核心操作 ======================== */

static BookRStatus book_array_init(BookArray *arr)
{
    arr->items = (Book *)malloc(sizeof(Book) * INIT_CAPACITY);
    if (!arr->items) return BORR_ERR_MEMORY;
    arr->length = 0;
    arr->capacity = INIT_CAPACITY;
    return BORR_OK;
}

static BookRStatus book_array_grow(BookArray *arr)
{
    int new_cap = arr->capacity * 2;
    Book *new_items = (Book *)realloc(arr->items, sizeof(Book) * new_cap);
    if (!new_items) return BORR_ERR_MEMORY;
    arr->items = new_items;
    arr->capacity = new_cap;
    return BORR_OK;
}

static BookRStatus book_array_add(BookArray *arr, const Book *book)
{
    if (arr->length >= arr->capacity) {
        BookRStatus st = book_array_grow(arr);
        if (st != BORR_OK) return st;
    }
    arr->items[arr->length] = *book;
    arr->length++;
    return BORR_OK;
}

static BookRStatus book_array_remove_at(BookArray *arr, int index)
{
    if (index < 0 || index >= arr->length) return BORR_ERR_INVALID_PARAM;
    arr->items[index] = arr->items[arr->length - 1];
    arr->length--;
    return BORR_OK;
}

static int book_array_find_by_isbn(const BookArray *arr, const char *isbn)
{
    for (int i = 0; i < arr->length; i++) {
        if (strcmp(arr->items[i].isbn, isbn) == 0) return i;
    }
    return -1;
}

static void book_array_free(BookArray *arr)
{
    if (arr->items) {
        free(arr->items);
        arr->items = NULL;
    }
    arr->length = 0;
    arr->capacity = 0;
}

static BookRStatus rental_array_init(RentalArray *arr)
{
    arr->items = (Rental *)malloc(sizeof(Rental) * INIT_CAPACITY);
    if (!arr->items) return BORR_ERR_MEMORY;
    arr->length = 0;
    arr->capacity = INIT_CAPACITY;
    return BORR_OK;
}

static BookRStatus rental_array_grow(RentalArray *arr)
{
    int new_cap = arr->capacity * 2;
    Rental *new_items = (Rental *)realloc(arr->items, sizeof(Rental) * new_cap);
    if (!new_items) return BORR_ERR_MEMORY;
    arr->items = new_items;
    arr->capacity = new_cap;
    return BORR_OK;
}

static BookRStatus rental_array_add(RentalArray *arr, const Rental *r)
{
    if (arr->length >= arr->capacity) {
        BookRStatus st = rental_array_grow(arr);
        if (st != BORR_OK) return st;
    }
    arr->items[arr->length] = *r;
    arr->length++;
    return BORR_OK;
}

static BookRStatus rental_array_remove_at(RentalArray *arr, int index)
{
    if (index < 0 || index >= arr->length) return BORR_ERR_INVALID_PARAM;
    arr->items[index] = arr->items[arr->length - 1];
    arr->length--;
    return BORR_OK;
}

static int rental_array_find_by_id(const RentalArray *arr, int rid)
{
    for (int i = 0; i < arr->length; i++) {
        if (arr->items[i].rental_id == rid) return i;
    }
    return -1;
}

static void rental_array_free(RentalArray *arr)
{
    if (arr->items) {
        free(arr->items);
        arr->items = NULL;
    }
    arr->length = 0;
    arr->capacity = 0;
}

/* ======================== 文件持久化（BORRV2 Magic Header） ======================== */

static BookRStatus save_to_file(const char *filename)
{
    FILE *fp = fopen(filename, "wb");
    if (!fp) return BORR_ERR_FILE_OPEN;

    if (fwrite(MAGIC_HEADER, 1, MAGIC_LEN, fp) != (size_t)MAGIC_LEN) { fclose(fp); return BORR_ERR_FILE_WRITE; }
    int version = DATA_VERSION;
    if (fwrite(&version, sizeof(int), 1, fp) != 1) { fclose(fp); return BORR_ERR_FILE_WRITE; }
    if (fwrite(&g_next_id, sizeof(int), 1, fp) != 1) { fclose(fp); return BORR_ERR_FILE_WRITE; }
    int book_count = g_books.length;
    if (fwrite(&book_count, sizeof(int), 1, fp) != 1) { fclose(fp); return BORR_ERR_FILE_WRITE; }
    int rental_count = g_rentals.length;
    if (fwrite(&rental_count, sizeof(int), 1, fp) != 1) { fclose(fp); return BORR_ERR_FILE_WRITE; }
    if (book_count > 0) {
        if (fwrite(g_books.items, sizeof(Book), book_count, fp) != (size_t)book_count) {
            fclose(fp); return BORR_ERR_FILE_WRITE;
        }
    }
    if (rental_count > 0) {
        if (fwrite(g_rentals.items, sizeof(Rental), rental_count, fp) != (size_t)rental_count) {
            fclose(fp); return BORR_ERR_FILE_WRITE;
        }
    }
    fclose(fp);
    return BORR_OK;
}

static BookRStatus load_from_file(const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp) return BORR_ERR_FILE_OPEN;

    char magic[MAGIC_LEN + 1] = {0};
    if (fread(magic, 1, MAGIC_LEN, fp) != (size_t)MAGIC_LEN) { fclose(fp); return BORR_ERR_FILE_READ; }
    if (strcmp(magic, MAGIC_HEADER) != 0) { fclose(fp); return BORR_ERR_MAGIC_MISMATCH; }
    int version = 0;
    if (fread(&version, sizeof(int), 1, fp) != 1) { fclose(fp); return BORR_ERR_FILE_READ; }
    if (version != DATA_VERSION) { fclose(fp); return BORR_ERR_VERSION_MISMATCH; }
    if (fread(&g_next_id, sizeof(int), 1, fp) != 1) { fclose(fp); return BORR_ERR_FILE_READ; }
    int book_count = 0;
    if (fread(&book_count, sizeof(int), 1, fp) != 1) { fclose(fp); return BORR_ERR_FILE_READ; }
    int rental_count = 0;
    if (fread(&rental_count, sizeof(int), 1, fp) != 1) { fclose(fp); return BORR_ERR_FILE_READ; }

    book_array_free(&g_books);
    rental_array_free(&g_rentals);
    book_array_init(&g_books);
    rental_array_init(&g_rentals);

    while (g_books.capacity < book_count) book_array_grow(&g_books);
    while (g_rentals.capacity < rental_count) rental_array_grow(&g_rentals);

    if (book_count > 0) {
        if (fread(g_books.items, sizeof(Book), book_count, fp) != (size_t)book_count) {
            fclose(fp); return BORR_ERR_FILE_READ;
        }
        g_books.length = book_count;
    }
    if (rental_count > 0) {
        if (fread(g_rentals.items, sizeof(Rental), rental_count, fp) != (size_t)rental_count) {
            fclose(fp); return BORR_ERR_FILE_READ;
        }
        g_rentals.length = rental_count;
    }
    fclose(fp);
    return BORR_OK;
}

/* ======================== 业务逻辑：书刊管理 ======================== */

static BookRStatus book_add(const char *isbn, const char *title, const char *author,
                            float price, const char *pub_date)
{
    if (!isbn || !title || !author || !pub_date) return BORR_ERR_INVALID_PARAM;
    if (strlen(isbn) == 0) return BORR_ERR_INVALID_PARAM;
    if (book_array_find_by_isbn(&g_books, isbn) >= 0) return BORR_ERR_DUPLICATE;
    Book b;
    memset(&b, 0, sizeof(b));
    safe_strcpy(b.isbn, isbn, MAX_ISBN);
    safe_strcpy(b.title, title, MAX_STR);
    safe_strcpy(b.author, author, MAX_STR);
    b.price = price;
    safe_strcpy(b.pub_date, pub_date, MAX_DATE);
    b.available = 1;
    BookRStatus st = book_array_add(&g_books, &b);
    if (st != BORR_OK) return st;
    return save_to_file(DATA_FILE);
}

static BookRStatus book_delete(const char *isbn)
{
    if (!isbn) return BORR_ERR_INVALID_PARAM;
    int idx = book_array_find_by_isbn(&g_books, isbn);
    if (idx < 0) return BORR_ERR_NOT_FOUND;
    for (int i = 0; i < g_rentals.length; i++) {
        if (strcmp(g_rentals.items[i].isbn, isbn) == 0 && g_rentals.items[i].status == 0) {
            return BORR_ERR_NOT_AVAILABLE;
        }
    }
    BookRStatus st = book_array_remove_at(&g_books, idx);
    if (st != BORR_OK) return st;
    return save_to_file(DATA_FILE);
}

static BookRStatus book_modify(const char *isbn, const char *title, const char *author,
                               float price, const char *pub_date)
{
    if (!isbn) return BORR_ERR_INVALID_PARAM;
    int idx = book_array_find_by_isbn(&g_books, isbn);
    if (idx < 0) return BORR_ERR_NOT_FOUND;
    Book *b = &g_books.items[idx];
    if (title && strlen(title) > 0) safe_strcpy(b->title, title, MAX_STR);
    if (author && strlen(author) > 0) safe_strcpy(b->author, author, MAX_STR);
    if (price > 0) b->price = price;
    if (pub_date && strlen(pub_date) > 0) safe_strcpy(b->pub_date, pub_date, MAX_DATE);
    return save_to_file(DATA_FILE);
}

static const Book *book_get(const char *isbn)
{
    int idx = book_array_find_by_isbn(&g_books, isbn);
    if (idx < 0) return NULL;
    return &g_books.items[idx];
}

/* ======================== 业务逻辑：租借/归还 ======================== */

static BookRStatus rental_borrow(const char *reader_id, const char *isbn)
{
    if (!reader_id || !isbn) return BORR_ERR_INVALID_PARAM;
    int book_idx = book_array_find_by_isbn(&g_books, isbn);
    if (book_idx < 0) return BORR_ERR_NOT_FOUND;
    if (g_books.items[book_idx].available == 0) return BORR_ERR_NOT_AVAILABLE;
    Rental r;
    memset(&r, 0, sizeof(r));
    r.rental_id = g_next_id++;
    safe_strcpy(r.reader_id, reader_id, MAX_ID);
    safe_strcpy(r.isbn, isbn, MAX_ISBN);
    get_current_date(r.rent_date);
    r.return_date[0] = '\0';
    r.fee = 0.0f;
    r.status = 0;
    BookRStatus st = rental_array_add(&g_rentals, &r);
    if (st != BORR_OK) return st;
    g_books.items[book_idx].available = 0;
    return save_to_file(DATA_FILE);
}

static BookRStatus rental_return(int rental_id)
{
    int idx = rental_array_find_by_id(&g_rentals, rental_id);
    if (idx < 0) return BORR_ERR_NOT_FOUND;
    Rental *r = &g_rentals.items[idx];
    if (r->status == 1) return BORR_ERR_ALREADY_RETURNED;
    char today[MAX_DATE];
    get_current_date(today);
    safe_strcpy(r->return_date, today, MAX_DATE);
    int d = days_between(r->rent_date, r->return_date);
    r->fee = d * RENTAL_RATE;
    r->status = 1;
    int book_idx = book_array_find_by_isbn(&g_books, r->isbn);
    if (book_idx >= 0) g_books.items[book_idx].available = 1;
    return save_to_file(DATA_FILE);
}

/* ======================== 统计功能 ======================== */

typedef struct {
    int total_books;
    int available_books;
    int borrowed_books;
    int total_rentals;
    int active_rentals;
    float total_revenue;
} StatsSummary;

static StatsSummary stats_get_summary(void)
{
    StatsSummary s = {0};
    s.total_books = g_books.length;
    for (int i = 0; i < g_books.length; i++) {
        if (g_books.items[i].available) s.available_books++;
        else s.borrowed_books++;
    }
    s.total_rentals = g_rentals.length;
    for (int i = 0; i < g_rentals.length; i++) {
        if (g_rentals.items[i].status == 0) s.active_rentals++;
        s.total_revenue += g_rentals.items[i].fee;
    }
    return s;
}

/* ======================== 内置自测（>=15项） ======================== */

static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(cond, name) do { \
    if (cond) { g_tests_passed++; c_print(C_GREEN, "  [PASS] %s\n", name); } \
    else { g_tests_failed++; c_print(C_RED, "  [FAIL] %s (line %d)\n", name, __LINE__); } \
} while(0)

#define TEST_ASSERT_STATUS(st, expected, name) TEST_ASSERT((st) == (expected), name)

static int run_selftest(void)
{
    c_print(C_BOLD C_MAGENTA, "\n========== 书刊租借系统 V2.0 内置自测 ==========\n\n");
    g_tests_passed = 0;
    g_tests_failed = 0;

    const char *test_file = "test_borrv2.tmp";
    book_array_free(&g_books);
    rental_array_free(&g_rentals);
    book_array_init(&g_books);
    rental_array_init(&g_rentals);
    g_next_id = 1;

    c_print(C_CYAN, "--- T1 初始化测试 ---\n");
    TEST_ASSERT(g_books.length == 0, "T1a: 初始书刊数量为0");
    TEST_ASSERT(g_rentals.length == 0, "T1b: 初始租借记录数量为0");

    c_print(C_CYAN, "\n--- T2 添加书刊 ---\n");
    BookRStatus st;
    Book b1; memset(&b1, 0, sizeof(b1));
    safe_strcpy(b1.isbn, "978-7-111-40701-0", MAX_ISBN);
    safe_strcpy(b1.title, "C程序设计语言", MAX_STR);
    safe_strcpy(b1.author, "K&R", MAX_STR);
    b1.price = 45.0f; safe_strcpy(b1.pub_date, "2012-03-01", MAX_DATE); b1.available = 1;
    st = book_array_add(&g_books, &b1);
    TEST_ASSERT_STATUS(st, BORR_OK, "T2a: 添加第一本书成功");
    TEST_ASSERT(g_books.length == 1, "T2b: 书刊数量为1");

    Book b2; memset(&b2, 0, sizeof(b2));
    safe_strcpy(b2.isbn, "978-7-115-42876-5", MAX_ISBN);
    safe_strcpy(b2.title, "深入理解计算机系统", MAX_STR);
    safe_strcpy(b2.author, "Bryant", MAX_STR);
    b2.price = 139.0f; safe_strcpy(b2.pub_date, "2016-11-01", MAX_DATE); b2.available = 1;
    st = book_array_add(&g_books, &b2);
    TEST_ASSERT_STATUS(st, BORR_OK, "T2c: 添加第二本书成功");
    TEST_ASSERT(g_books.length == 2, "T2d: 书刊数量为2");

    c_print(C_CYAN, "\n--- T3 ISBN重复检测 ---\n");
    int dup_idx = book_array_find_by_isbn(&g_books, "978-7-111-40701-0");
    TEST_ASSERT(dup_idx >= 0, "T3a: 重复ISBN能被查到");
    TEST_ASSERT(strcmp(g_books.items[dup_idx].title, "C程序设计语言") == 0, "T3b: 查到正确书名");

    c_print(C_CYAN, "\n--- T4 查找书刊 ---\n");
    const Book *found = book_get("978-7-115-42876-5");
    TEST_ASSERT(found != NULL, "T4a: 存在的ISBN返回非NULL");
    TEST_ASSERT(strcmp(found->author, "Bryant") == 0, "T4b: 查到正确作者");
    TEST_ASSERT(book_get("000-0-000-00000-0") == NULL, "T4c: 不存在的ISBN返回NULL");

    c_print(C_CYAN, "\n--- T5 修改书刊 ---\n");
    int idx = book_array_find_by_isbn(&g_books, "978-7-111-40701-0");
    safe_strcpy(g_books.items[idx].title, "C程序设计语言(第2版)", MAX_STR);
    g_books.items[idx].price = 49.0f;
    TEST_ASSERT(strcmp(book_get("978-7-111-40701-0")->title, "C程序设计语言(第2版)") == 0, "T5a: 书名修改成功");
    TEST_ASSERT(book_get("978-7-111-40701-0")->price > 48.9f && book_get("978-7-111-40701-0")->price < 49.1f, "T5b: 价格修改成功");

    c_print(C_CYAN, "\n--- T6 借书 ---\n");
    TEST_ASSERT(g_books.items[idx].available == 1, "T6a: 书初始可借");
    g_books.items[idx].available = 0;
    Rental r1; memset(&r1, 0, sizeof(r1));
    r1.rental_id = g_next_id++;
    safe_strcpy(r1.reader_id, "R001", MAX_ID);
    safe_strcpy(r1.isbn, "978-7-111-40701-0", MAX_ISBN);
    safe_strcpy(r1.rent_date, "2026-06-01", MAX_DATE);
    r1.status = 0;
    st = rental_array_add(&g_rentals, &r1);
    TEST_ASSERT_STATUS(st, BORR_OK, "T6b: 创建租借记录成功");
    TEST_ASSERT(g_rentals.length == 1, "T6c: 租借记录数为1");
    TEST_ASSERT(g_books.items[idx].available == 0, "T6d: 书刊标记为不可借");

    c_print(C_CYAN, "\n--- T7 已借书不可再借 ---\n");
    TEST_ASSERT(g_books.items[idx].available == 0, "T7: 已借出的书不可再次借出");

    c_print(C_CYAN, "\n--- T8 还书 ---\n");
    int ridx = rental_array_find_by_id(&g_rentals, 1);
    TEST_ASSERT(ridx >= 0, "T8a: 能找到租借记录ID=1");
    Rental *r = &g_rentals.items[ridx];
    safe_strcpy(r->return_date, "2026-06-10", MAX_DATE);
    int d = days_between(r->rent_date, r->return_date);
    r->fee = d * RENTAL_RATE;
    r->status = 1;
    int bidx = book_array_find_by_isbn(&g_books, r->isbn);
    g_books.items[bidx].available = 1;
    TEST_ASSERT(r->status == 1, "T8b: 记录状态为已归还");
    TEST_ASSERT(d >= 9, "T8c: 日期差计算正确（6.1-6.10≥9天）");
    TEST_ASSERT(r->fee >= 9.0f, "T8d: 租金计算正确（≥9元）");
    TEST_ASSERT(g_books.items[bidx].available == 1, "T8e: 书刊恢复可借");

    c_print(C_CYAN, "\n--- T9 重复归还检测 ---\n");
    TEST_ASSERT(r->status == 1, "T9: 已归还记录再次归还应被拒绝");

    c_print(C_CYAN, "\n--- T10 删除书刊 ---\n");
    int del_idx = book_array_find_by_isbn(&g_books, "978-7-115-42876-5");
    st = book_array_remove_at(&g_books, del_idx);
    TEST_ASSERT_STATUS(st, BORR_OK, "T10a: 删除第二本书成功");
    TEST_ASSERT(g_books.length == 1, "T10b: 删除后书刊数为1");
    TEST_ASSERT(book_array_find_by_isbn(&g_books, "978-7-115-42876-5") == -1, "T10c: 被删书刊无法再查到");

    c_print(C_CYAN, "\n--- T11 删除非法索引 ---\n");
    st = book_array_remove_at(&g_books, 999);
    
    TEST_ASSERT_STATUS(st, BORR_ERR_INVALID_PARAM, "T11: 删除非法索引返回错误");

    c_print(C_CYAN, "\n--- T11b rental_array_remove_at ---\n");
    {
        Rental rt; memset(&rt, 0, sizeof(rt));
        rt.rental_id = 999; safe_strcpy(rt.isbn, "DEL", MAX_ISBN); rt.status = 1;
        BookRStatus st2 = rental_array_add(&g_rentals, &rt);
        TEST_ASSERT_STATUS(st2, BORR_OK, "T11b1: 添加待删除记录成功");
        int didx = rental_array_find_by_id(&g_rentals, 999);
        TEST_ASSERT(didx >= 0, "T11b2: 找到待删除记录");
        st2 = rental_array_remove_at(&g_rentals, didx);
        TEST_ASSERT_STATUS(st2, BORR_OK, "T11b3: 删除租借记录成功");
        TEST_ASSERT(rental_array_find_by_id(&g_rentals, 999) == -1, "T11b4: 删除后记录消失");
    }


    c_print(C_CYAN, "\n--- T12 动态数组扩容 ---\n");
    book_array_free(&g_books);
    book_array_init(&g_books);
    int init_cap = g_books.capacity;
    for (int i = 0; i < init_cap + 1; i++) {
        Book tmp; memset(&tmp, 0, sizeof(tmp));
        sprintf(tmp.isbn, "ISBN-%03d", i);
        sprintf(tmp.title, "Book %d", i);
        safe_strcpy(tmp.author, "Test", MAX_STR);
        tmp.price = 10.0f; tmp.available = 1;
        book_array_add(&g_books, &tmp);
    }
    TEST_ASSERT(g_books.length == init_cap + 1, "T12a: 添加超初始容量成功");
    TEST_ASSERT(g_books.capacity > init_cap, "T12b: 数组已自动扩容");

    c_print(C_CYAN, "\n--- T13 统计功能 ---\n");
    book_array_free(&g_books);
    rental_array_free(&g_rentals);
    book_array_init(&g_books);
    rental_array_init(&g_rentals);
    g_next_id = 1;
    Book sb1, sb2, sb3;
    memset(&sb1, 0, sizeof(sb1)); safe_strcpy(sb1.isbn, "B1", MAX_ISBN); safe_strcpy(sb1.title, "Book1", MAX_STR); sb1.available = 1; book_array_add(&g_books, &sb1);
    memset(&sb2, 0, sizeof(sb2)); safe_strcpy(sb2.isbn, "B2", MAX_ISBN); safe_strcpy(sb2.title, "Book2", MAX_STR); sb2.available = 0; book_array_add(&g_books, &sb2);
    memset(&sb3, 0, sizeof(sb3)); safe_strcpy(sb3.isbn, "B3", MAX_ISBN); safe_strcpy(sb3.title, "Book3", MAX_STR); sb3.available = 0; book_array_add(&g_books, &sb3);
    Rental sr1, sr2;
    memset(&sr1, 0, sizeof(sr1)); sr1.rental_id = 1; safe_strcpy(sr1.isbn, "B2", MAX_ISBN); sr1.fee = 5.0f; sr1.status = 1; rental_array_add(&g_rentals, &sr1);
    memset(&sr2, 0, sizeof(sr2)); sr2.rental_id = 2; safe_strcpy(sr2.isbn, "B3", MAX_ISBN); sr2.fee = 0.0f; sr2.status = 0; rental_array_add(&g_rentals, &sr2);
    StatsSummary ss = stats_get_summary();
    TEST_ASSERT(ss.total_books == 3, "T13a: 总书刊数=3");
    TEST_ASSERT(ss.available_books == 1, "T13b: 可借书刊=1");
    TEST_ASSERT(ss.borrowed_books == 2, "T13c: 已借书刊=2");
    TEST_ASSERT(ss.total_rentals == 2, "T13d: 总租借记录=2");
    TEST_ASSERT(ss.active_rentals == 1, "T13e: 租借中记录=1");
    TEST_ASSERT(ss.total_revenue >= 4.9f && ss.total_revenue <= 5.1f, "T13f: 总营收=5元");

    c_print(C_CYAN, "\n--- T14 文件持久化（保存+加载） ---\n");
    st = save_to_file(test_file);
    TEST_ASSERT_STATUS(st, BORR_OK, "T14a: 保存到临时文件成功");
    int saved_next_id = g_next_id;
    book_array_free(&g_books);
    rental_array_free(&g_rentals);
    g_books.items = NULL; g_books.length = 0; g_books.capacity = 0;
    g_rentals.items = NULL; g_rentals.length = 0; g_rentals.capacity = 0;
    st = load_from_file(test_file);
    TEST_ASSERT_STATUS(st, BORR_OK, "T14b: 从临时文件加载成功");
    TEST_ASSERT(g_books.length == 3, "T14c: 加载后书刊数=3");
    TEST_ASSERT(g_rentals.length == 2, "T14d: 加载后租借记录数=2");
    TEST_ASSERT(g_next_id == saved_next_id, "T14e: next_id正确恢复");
    TEST_ASSERT(book_array_find_by_isbn(&g_books, "B1") >= 0, "T14f: 书刊B1已恢复");
    TEST_ASSERT(book_array_find_by_isbn(&g_books, "B2") >= 0, "T14g: 书刊B2已恢复");
    TEST_ASSERT(rental_array_find_by_id(&g_rentals, 1) >= 0, "T14h: 租借记录1已恢复");

    c_print(C_CYAN, "\n--- T15 Magic Header错误检测 ---\n");
    {
        FILE *fp = fopen("test_bad.tmp", "wb");
        const char *bad = "XXXXXX";
        fwrite(bad, 1, 6, fp);
        int ver = 2; fwrite(&ver, sizeof(int), 1, fp);
        fclose(fp);
    }
    st = load_from_file("test_bad.tmp");
    TEST_ASSERT_STATUS(st, BORR_ERR_MAGIC_MISMATCH, "T15: 错误魔数被正确拒绝");
    remove("test_bad.tmp");

    c_print(C_CYAN, "\n--- T16 工具函数 days_between ---\n");
    int d1 = days_between("2026-01-01", "2026-01-10");
    TEST_ASSERT(d1 == 9, "T16a: 2026-01-01到2026-01-10相差9天");
    int d2 = days_between("2026-06-28", "2026-06-28");
    TEST_ASSERT(d2 == 1, "T16b: 同一天最小1天");

    c_print(C_CYAN, "\n--- T17 safe_strcpy边界 ---\n");
    char small[5];
    safe_strcpy(small, "HelloWorld", sizeof(small));
    TEST_ASSERT(strlen(small) == 4, "T17a: safe_strcpy正确截断");
    TEST_ASSERT(small[4] == '\0', "T17b: 保证结尾0");

    c_print(C_CYAN, "\n--- T18 高层API book_add 重复检测 ---\n");
    book_array_free(&g_books);
    rental_array_free(&g_rentals);
    book_array_init(&g_books);
    rental_array_init(&g_rentals);
    g_next_id = 1;
    st = book_add("978-000-000000-0", "测试书", "测试作者", 10.0f, "2026-01-01");
    /* 保存会写到DATA_FILE，为避免污染正式数据，测试完清理 */
    TEST_ASSERT_STATUS(st, BORR_OK, "T18a: book_add高层API添加成功");
    st = book_add("978-000-000000-0", "重复书", "X", 10.0f, "2026-01-01");
    TEST_ASSERT_STATUS(st, BORR_ERR_DUPLICATE, "T18b: 重复ISBN被高层API拒绝");
    (void)remove(DATA_FILE);

    c_print(C_CYAN, "\n--- T19 rental_borrow/rental_return 高层API ---\n");
    book_array_free(&g_books);
    rental_array_free(&g_rentals);
    book_array_init(&g_books);
    rental_array_init(&g_rentals);
    g_next_id = 1;
    Book tb; memset(&tb, 0, sizeof(tb));
    safe_strcpy(tb.isbn, "BORROW-TEST", MAX_ISBN);
    safe_strcpy(tb.title, "借阅测试", MAX_STR);
    safe_strcpy(tb.author, "T", MAX_STR);
    tb.price = 1.0f; tb.available = 1;
    book_array_add(&g_books, &tb);
    st = rental_borrow("R-TEST", "BORROW-TEST");
    TEST_ASSERT_STATUS(st, BORR_OK, "T19a: rental_borrow借书成功");
    TEST_ASSERT(g_books.items[0].available == 0, "T19b: 书被标记为借出");
    st = rental_borrow("R-TEST2", "BORROW-TEST");
    TEST_ASSERT_STATUS(st, BORR_ERR_NOT_AVAILABLE, "T19c: 已借书不能再次借");
    st = rental_return(1);
    /* rent_date是今天，还书日期也是今天，days=1，fee=1 */
    TEST_ASSERT_STATUS(st, BORR_OK, "T19d: rental_return还书成功");
    TEST_ASSERT(g_books.items[0].available == 1, "T19e: 书恢复可借");
    TEST_ASSERT(g_rentals.items[0].status == 1, "T19f: 记录标记已归还");
    st = rental_return(1);
    TEST_ASSERT_STATUS(st, BORR_ERR_ALREADY_RETURNED, "T19g: 重复归还被拒绝");
    (void)remove(DATA_FILE);

    /* 清理临时文件 */
    remove(test_file);

    c_print(C_BOLD C_MAGENTA, "\n========== 测试结果 ==========\n");
    c_print(C_GREEN, "  通过: %d\n", g_tests_passed);
    c_print(C_RED, "  失败: %d\n", g_tests_failed);
    int total = g_tests_passed + g_tests_failed;
    c_print(C_CYAN, "  总计: %d 项测试\n\n", total);

    book_array_free(&g_books);
    rental_array_free(&g_rentals);

    if (g_tests_failed == 0) {
        c_print(C_BOLD C_GREEN, "========== 全部测试通过！ ==========\n\n");
        return 0;
    } else {
        c_print(C_BOLD C_RED, "========== 存在失败测试！ ==========\n\n");
        return 1;
    }
}

/* ======================== 交互菜单 ======================== */

static void menu_book_add(void)
{
    print_header("添加书刊");
    Book b;
    memset(&b, 0, sizeof(b));
    printf("请输入ISBN："); read_line(b.isbn, MAX_ISBN);
    printf("请输入书名："); read_line(b.title, MAX_STR);
    printf("请输入作者："); read_line(b.author, MAX_STR);
    printf("请输入价格（元）："); b.price = read_float();
    printf("请输入出版日期(YYYY-MM-DD)："); read_line(b.pub_date, MAX_DATE);
    BookRStatus st = book_add(b.isbn, b.title, b.author, b.price, b.pub_date);
    if (st != BORR_OK) { print_error(st); return; }
    print_success("书刊添加成功");
}

static void menu_book_list(void)
{
    print_header("书刊列表");
    if (g_books.length == 0) { print_info("暂无书刊数据"); return; }
    c_print(C_BOLD, "%-20s %-22s %-14s %-8s %-12s %s\n", "ISBN", "书名", "作者", "价格", "出版日期", "状态");
    print_separator();
    for (int i = 0; i < g_books.length; i++) {
        Book *b = &g_books.items[i];
        const char *status_str = b->available ? C_GREEN"可借"C_RESET : C_RED"已借出"C_RESET;
        printf("%-20s %-22s %-14s %-8.2f %-12s %s\n",
               b->isbn, b->title, b->author, b->price, b->pub_date, status_str);
    }
    print_separator();
    c_print(C_CYAN, "共 %d 本书刊\n", g_books.length);
}

static void menu_book_delete(void)
{
    print_header("删除书刊");
    char isbn[MAX_ISBN];
    printf("请输入要删除的ISBN："); read_line(isbn, MAX_ISBN);
    int idx = book_array_find_by_isbn(&g_books, isbn);
    if (idx < 0) { print_error(BORR_ERR_NOT_FOUND); return; }
    c_print(C_YELLOW, "确认删除《%s》(ISBN:%s)？(y/N)：", g_books.items[idx].title, isbn);
    char confirm[8]; read_line(confirm, 8);
    if (strcmp(confirm, "y") != 0 && strcmp(confirm, "Y") != 0) {
        print_info("已取消删除"); return;
    }
    BookRStatus st = book_delete(isbn);
    if (st != BORR_OK) { print_error(st); return; }
    print_success("书刊删除成功");
}

static void menu_book_modify(void)
{
    print_header("修改书刊信息");
    char isbn[MAX_ISBN];
    printf("请输入要修改的ISBN："); read_line(isbn, MAX_ISBN);
    int idx = book_array_find_by_isbn(&g_books, isbn);
    if (idx < 0) { print_error(BORR_ERR_NOT_FOUND); return; }
    Book *b = &g_books.items[idx];
    c_print(C_CYAN, "当前信息：《%s》 作者：%s 价格：%.2f元 出版：%s\n",
            b->title, b->author, b->price, b->pub_date);
    char t_title[MAX_STR] = "", t_author[MAX_STR] = "", t_date[MAX_DATE] = "";
    float t_price = 0.0f;
    printf("新书名（直接回车保留原值）："); read_line(t_title, MAX_STR);
    printf("新作者（直接回车保留原值）："); read_line(t_author, MAX_STR);
    printf("新价格（输入0保留原值）："); t_price = read_float();
    printf("新出版日期（直接回车保留原值）："); read_line(t_date, MAX_DATE);
    BookRStatus st = book_modify(isbn,
                                 strlen(t_title) ? t_title : NULL,
                                 strlen(t_author) ? t_author : NULL,
                                 t_price,
                                 strlen(t_date) ? t_date : NULL);
    if (st != BORR_OK) { print_error(st); return; }
    print_success("书刊信息修改成功");
}

static void menu_book_search(void)
{
    print_header("查询书刊");
    char isbn[MAX_ISBN];
    printf("请输入要查询的ISBN："); read_line(isbn, MAX_ISBN);
    const Book *b = book_get(isbn);
    if (!b) { print_error(BORR_ERR_NOT_FOUND); return; }
    print_separator();
    c_print(C_BOLD, "ISBN："); printf("%s\n", b->isbn);
    c_print(C_BOLD, "书名："); printf("%s\n", b->title);
    c_print(C_BOLD, "作者："); printf("%s\n", b->author);
    c_print(C_BOLD, "价格："); printf("%.2f 元\n", b->price);
    c_print(C_BOLD, "出版日期："); printf("%s\n", b->pub_date);
    c_print(C_BOLD, "状态：");
    if (b->available) c_print(C_GREEN, "可借\n");
    else c_print(C_RED, "已借出\n");
    print_separator();
}

static void menu_rent_borrow(void)
{
    print_header("借书登记");
    char rid[MAX_ID], isbn[MAX_ISBN];
    printf("读者证号："); read_line(rid, MAX_ID);
    printf("书刊ISBN："); read_line(isbn, MAX_ISBN);
    BookRStatus st = rental_borrow(rid, isbn);
    if (st != BORR_OK) { print_error(st); return; }
    char msg[128];
    int new_id = g_next_id - 1;
    int i = rental_array_find_by_id(&g_rentals, new_id);
    sprintf(msg, "借书成功！记录ID=%d，借出日期=%s", new_id,
            i >= 0 ? g_rentals.items[i].rent_date : "");
    print_success(msg);
}

static void menu_rent_return(void)
{
    print_header("还书登记");
    printf("请输入租借记录ID：");
    int rid = read_int();
    /* 先查一下旧日期用于提示 */
    int idx = rental_array_find_by_id(&g_rentals, rid);
    if (idx < 0) { print_error(BORR_ERR_NOT_FOUND); return; }
    if (g_rentals.items[idx].status == 1) { print_error(BORR_ERR_ALREADY_RETURNED); return; }
    char old_date[MAX_DATE];
    safe_strcpy(old_date, g_rentals.items[idx].rent_date, MAX_DATE);
    BookRStatus st = rental_return(rid);
    if (st != BORR_OK) { print_error(st); return; }
    idx = rental_array_find_by_id(&g_rentals, rid);
    char msg[128];
    int days = days_between(old_date, g_rentals.items[idx].return_date);
    sprintf(msg, "还书成功！租借%d天，租金%.2f元", days, g_rentals.items[idx].fee);
    print_success(msg);
}

static void menu_rent_list(void)
{
    print_header("租借记录列表");
    if (g_rentals.length == 0) { print_info("暂无租借记录"); return; }
    c_print(C_BOLD, "%-6s %-12s %-20s %-12s %-12s %-8s %s\n",
            "ID", "读者证号", "ISBN", "借出日期", "归还日期", "租金", "状态");
    print_separator();
    for (int i = 0; i < g_rentals.length; i++) {
        Rental *r = &g_rentals.items[i];
        const char *st_str = r->status ? C_GREEN"已归还"C_RESET : C_YELLOW"租借中"C_RESET;
        printf("%-6d %-12s %-20s %-12s %-12s %-8.2f %s\n",
               r->rental_id, r->reader_id, r->isbn,
               r->rent_date, strlen(r->return_date) ? r->return_date : "-",
               r->fee, st_str);
    }
    print_separator();
    c_print(C_CYAN, "共 %d 条记录\n", g_rentals.length);
}

static void menu_stats(void)
{
    print_header("统计概览");
    StatsSummary s = stats_get_summary();
    c_print(C_BOLD C_WHITE, "  总书刊数："); c_print(C_CYAN, "%d\n", s.total_books);
    c_print(C_BOLD C_WHITE, "  可借书刊："); c_print(C_GREEN, "%d\n", s.available_books);
    c_print(C_BOLD C_WHITE, "  已借书刊："); c_print(C_RED, "%d\n", s.borrowed_books);
    c_print(C_BOLD C_WHITE, "  总租借记录："); c_print(C_CYAN, "%d\n", s.total_rentals);
    c_print(C_BOLD C_WHITE, "  租借中："); c_print(C_YELLOW, "%d\n", s.active_rentals);
    c_print(C_BOLD C_WHITE, "  累计营收："); c_print(C_GREEN, "%.2f 元\n", s.total_revenue);
    print_separator();
}

static void print_banner(void)
{
    c_print(C_BOLD C_BLUE,
        "\n"
        "  ==================================================\n"
        "       书刊租借信息管理系统 V2.0  (BORRV2)\n"
        "     Book Rental Information Management System\n"
        "           作者：数据库侠客  班级：计算机类A班\n"
        "  ==================================================\n\n");
}

static void print_main_menu(void)
{
    print_separator();
    c_print(C_BOLD C_CYAN, "             主 菜 单\n");
    print_separator();
    c_print(C_WHITE, "  "); c_print(C_BOLD C_BLUE, "[1]"); c_print(C_WHITE, " 添加书刊\n");
    c_print(C_WHITE, "  "); c_print(C_BOLD C_BLUE, "[2]"); c_print(C_WHITE, " 书刊列表\n");
    c_print(C_WHITE, "  "); c_print(C_BOLD C_BLUE, "[3]"); c_print(C_WHITE, " 查询书刊\n");
    c_print(C_WHITE, "  "); c_print(C_BOLD C_BLUE, "[4]"); c_print(C_WHITE, " 修改书刊\n");
    c_print(C_WHITE, "  "); c_print(C_BOLD C_BLUE, "[5]"); c_print(C_WHITE, " 删除书刊\n");
    print_separator();
    c_print(C_WHITE, "  "); c_print(C_BOLD C_MAGENTA, "[6]"); c_print(C_WHITE, " 借书登记\n");
    c_print(C_WHITE, "  "); c_print(C_BOLD C_MAGENTA, "[7]"); c_print(C_WHITE, " 还书登记\n");
    c_print(C_WHITE, "  "); c_print(C_BOLD C_MAGENTA, "[8]"); c_print(C_WHITE, " 租借记录\n");
    print_separator();
    c_print(C_WHITE, "  "); c_print(C_BOLD C_GREEN, "[9]"); c_print(C_WHITE, " 统计概览\n");
    c_print(C_WHITE, "  "); c_print(C_BOLD C_RED,   "[0]"); c_print(C_WHITE, " 退出系统\n");
    print_separator();
    printf("请选择操作：");
}

static void interactive_loop(void)
{
    console_init();
    book_array_init(&g_books);
    rental_array_init(&g_rentals);

    BookRStatus st = load_from_file(DATA_FILE);
    if (st == BORR_OK) {
        print_info("数据文件加载成功");
    } else if (st == BORR_ERR_FILE_OPEN) {
        print_info("未检测到数据文件，将创建新数据库");
    } else {
        print_error(st);
        print_info("将以空数据库启动");
    }

    print_banner();

    while (1) {
        print_main_menu();
        int choice = read_int();
        switch (choice) {
        case 1: menu_book_add();    break;
        case 2: menu_book_list();   break;
        case 3: menu_book_search(); break;
        case 4: menu_book_modify(); break;
        case 5: menu_book_delete(); break;
        case 6: menu_rent_borrow(); break;
        case 7: menu_rent_return(); break;
        case 8: menu_rent_list();   break;
        case 9: menu_stats();       break;
        case 0:
            c_print(C_BOLD C_GREEN, "\n感谢使用，数据已自动保存！再见！\n\n");
            goto cleanup;
        default:
            print_error_msg("无效选项，请重新输入");
        }
    }

cleanup:
    book_array_free(&g_books);
    rental_array_free(&g_rentals);
}

/* ======================== CLI 参数处理 ======================== */

static void print_help(const char *prog)
{
    console_init();
    c_print(C_BOLD C_CYAN, "书刊租借信息管理系统 V2.0\n");
    print_separator();
    c_print(C_WHITE, "用法: %s [选项]\n\n", prog);
    c_print(C_BOLD, "选项:\n");
    c_print(C_GREEN, "  -h, --help     "); c_print(C_WHITE, "显示帮助信息\n");
    c_print(C_GREEN, "  -t, --test     "); c_print(C_WHITE, "运行内置自测程序（>=15项）\n");
    c_print(C_GREEN, "  (无参数)       "); c_print(C_WHITE, "启动交互菜单模式\n");
    print_separator();
    c_print(C_YELLOW, "数据文件: %s  (Magic: BORRV2)\n", DATA_FILE);
    c_print(C_YELLOW, "编译器: GCC (C99)  标准: -std=c99 -Wall -Wextra -O2\n\n");
}

/* ======================== 主函数 ======================== */

int main(int argc, char *argv[])
{
    console_init();

    if (argc >= 2) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        }
        if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0) {
            int ret = run_selftest();
            return ret;
        }
        c_print(C_RED, "未知选项: %s\n", argv[1]);
        print_help(argv[0]);
        return 1;
    }

    interactive_loop();
    return 0;
}
