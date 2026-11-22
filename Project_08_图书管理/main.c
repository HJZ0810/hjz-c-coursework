/*
* ============================================================================
*
*    图书管理系统 V2.0  (Library Management System)
*
* ============================================================================
*
*  功能概述:
*    基于动态数组实现的现代化图书管理系统，支持：
*      [1] 图书信息 CRUD（增删改查）
*      [2] 图书借阅与归还功能
*      [3] 按ID/类型查询图书
*      [4] 文件持久化（自定义BOOKV2二进制格式，带Magic Header校验）
*      [5] 动态数组自动扩容（无需预定义最大图书数）
*      [6] ANSI彩色终端输出（绿色OK/红色ERR/青色标题）
*      [7] 命令行模式（--help/--test/-f/拖放文件）
*      [8] 内置完整自测用例（增删改查+借阅+文件I/O全流程验证）
*      [9] UTF-8编码支持（Windows 10+自动启用VT模式）
*     [10] 统一错误码枚举体系
*
*  数据结构:
*    Book          —— 图书信息结构体（ID/书名/作者/分类/库存/借阅次数）
*    BorrowRecord  —— 借阅记录（图书证号/应还日期）
*    Library       —— 图书馆动态数组容器（自动扩容，capacity*=2策略）
*    BookHeader    —— 数据文件头（Magic "BOOKV2" + 版本号 + 图书数量）
*
*  编译命令:
*    D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o library.exe main.c
*
*  命令行用法:
*    library.exe              交互模式（菜单）
*    library.exe -h/--help    显示帮助
*    library.exe -t/--test    运行内置自测用例
*    library.exe -f <文件>    指定数据文件路径
*    library.exe <文件>       直接打开数据文件（拖放模式）
*
*  交互模式:
*    直接运行 library.exe 进入菜单交互模式
*
*  编码: UTF-8（无BOM）
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
#include <termios.h>
#include <fcntl.h>
#endif

/* ======================== 常量与宏 ======================== */
#define MAX_ID_LEN      32
#define MAX_TITLE_LEN   128
#define MAX_AUTHOR_LEN  64
#define MAX_TYPE_LEN    32
#define MAX_CARD_LEN    32
#define MAX_DATE_LEN    16
#define BOOK_MAGIC      "BOOKV2"
#define BOOK_VERSION    2
#define INIT_CAPACITY   8
#define LOG_FILE        "library.log"

/* 颜色宏（ANSI转义序列） */
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
#define BG_RED      "\033[41m"
#define BG_GREEN    "\033[42m"

/* ======================== 错误码枚举 ======================== */
typedef enum {
    BOOK_OK = 0,
    BOOK_ERR_OPEN_FILE,
    BOOK_ERR_READ_FILE,
    BOOK_ERR_WRITE_FILE,
    BOOK_ERR_MEMORY,
    BOOK_ERR_CORRUPT,
    BOOK_ERR_MAGIC,
    BOOK_ERR_VERSION,
    BOOK_ERR_NOT_FOUND,
    BOOK_ERR_DUPLICATE_ID,
    BOOK_ERR_OUT_OF_STOCK,
    BOOK_ERR_NO_BORROW,
    BOOK_ERR_INVALID_INPUT,
    BOOK_ERR_CANCEL,
    BOOK_ERR_EMPTY,
} BookStatus;

/* ======================== 数据结构 ======================== */

typedef struct BorrowRecord {
    char card_id[MAX_CARD_LEN];
    char due_date[MAX_DATE_LEN];
    time_t borrow_time;
    struct BorrowRecord *next;
} BorrowRecord;

typedef struct {
    char id[MAX_ID_LEN];
    char title[MAX_TITLE_LEN];
    char author[MAX_AUTHOR_LEN];
    char type[MAX_TYPE_LEN];
    int stock;
    int total_borrowed;
    BorrowRecord *borrow_list;
} Book;

typedef struct {
    Book *books;
    int count;
    int capacity;
} Library;

typedef struct {
    char magic[6];
    unsigned short version;
    unsigned int book_count;
    unsigned int header_size;
} BookFileHeader;

/* ======================== 全局状态 ======================== */
static Library g_lib = {NULL, 0, 0};
static int colorEnabled = 1;
static char dataFilePath[512] = "library.dat";
static int testMode = 0;
static int testPassed = 0;
static int testFailed = 0;

/* ======================== 前向声明 ======================== */
static BookStatus saveLibraryToFile(const char *path);
static BookStatus loadLibraryFromFile(const char *path);
static void printDivider(void);

/* ======================== 工具函数：颜色与终端 ======================== */

static void enableVTMode(void) {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) { colorEnabled = 0; return; }
    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) { colorEnabled = 0; return; }
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
    colorEnabled = 1;
#endif
}

static void cPrint(const char *color, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (colorEnabled && color) fputs(color, stdout);
    vprintf(fmt, args);
    if (colorEnabled && color) fputs(C_RESET, stdout);
    va_end(args);
}

static void printOK(const char *msg)    { cPrint(C_GREEN, "  [OK] "); printf("%s\n", msg); }
static void printErr(const char *msg)   { cPrint(C_RED,   "  [ERR] "); printf("%s\n", msg); }
static void printWarn(const char *msg)  { cPrint(C_YELLOW,"  [WARN] "); printf("%s\n", msg); }
static void printInfo(const char *msg)  { cPrint(C_CYAN,  "  [INFO] "); printf("%s\n", msg); }
static void printTitle(const char *msg) { cPrint(C_BOLD C_CYAN, "\n  %s\n", msg); printDivider(); }

static void logMessage(const char *fmt, ...) {
    FILE *fp = fopen(LOG_FILE, "a");
    if (!fp) return;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(fp, "[%04d-%02d-%02d %02d:%02d:%02d] ",
            t->tm_year+1900, t->tm_mon+1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);
    va_list args;
    va_start(args, fmt);
    vfprintf(fp, fmt, args);
    va_end(args);
    fputc('\n', fp);
    fclose(fp);
}

/* ======================== 工具函数：UI辅助 ======================== */

static void printBanner(void) {
    printf("\n");
    cPrint(C_BOLD C_BLUE,
        "  ============================================================================\n");
    cPrint(C_BOLD C_CYAN,
        "  |               Library Management System V2.0 (Enhanced Edition)          |\n");
    cPrint(C_BOLD C_CYAN,
        "  |                         图书管理系统  完全体增强版 V2.0                   |\n");
    cPrint(C_BOLD C_BLUE,
        "  ============================================================================\n");
    printf("\n");
}

static void printDivider(void) {
    cPrint(C_GRAY, "  ----------------------------------------------------------------------------\n");
}

static void printThickDivider(void) {
    cPrint(C_BLUE, "  ============================================================================\n");
}

static int readInt(const char *prompt, int min, int max) {
    int val;
    char buf[256];
    while (1) {
        printf("  %s", prompt);
        fflush(stdout);
        if (fgets(buf, sizeof(buf), stdin) == NULL) { clearerr(stdin); continue; }
        char *nl = strchr(buf, '\n');
        if (nl) *nl = '\0'; else { int c; while ((c=getchar())!='\n' && c!=EOF); }
        if (sscanf(buf, "%d", &val) == 1 && val >= min && val <= max)
            return val;
        cPrint(C_YELLOW, "  [!] 请输入 [%d, %d] 范围内的整数\n", min, max);
    }
}

static void readString(const char *prompt, char *buf, int size) {
    while (1) {
        printf("  %s", prompt);
        fflush(stdout);
        if (fgets(buf, size, stdin) == NULL) { clearerr(stdin); continue; }
        char *nl = strchr(buf, '\n');
        if (nl) *nl = '\0'; else { int c; while ((c=getchar())!='\n' && c!=EOF); }
        if (buf[0] != '\0') return;
        cPrint(C_YELLOW, "  [!] 输入不能为空，请重新输入\n");
    }
}

static int readYesNo(const char *prompt) {
    char buf[64];
    while (1) {
        printf("  %s (Y/N): ", prompt);
        fflush(stdout);
        if (fgets(buf, sizeof(buf), stdin) == NULL) return 0;
        char c = (char)toupper((unsigned char)buf[0]);
        if (c == 'Y') return 1;
        if (c == 'N') return 0;
        cPrint(C_YELLOW, "  [!] 请输入 Y 或 N\n");
    }
}

static int fileExists(const char *path) {
    return access(path, 0) == 0;
}

/* ======================== 动态数组内存管理 ======================== */

static void freeBorrowList(BorrowRecord *head) {
    BorrowRecord *cur = head;
    while (cur != NULL) {
        BorrowRecord *tmp = cur;
        cur = cur->next;
        free(tmp);
    }
}

static void freeLibrary(void) {
    if (g_lib.books == NULL) return;
    for (int i = 0; i < g_lib.count; i++) {
        freeBorrowList(g_lib.books[i].borrow_list);
    }
    free(g_lib.books);
    g_lib.books = NULL;
    g_lib.count = 0;
    g_lib.capacity = 0;
}

static BookStatus ensureCapacity(int needed) {
    if (needed <= g_lib.capacity) return BOOK_OK;
    int newCap = g_lib.capacity == 0 ? INIT_CAPACITY : g_lib.capacity;
    while (newCap < needed) newCap *= 2;
    Book *newBooks = (Book *)realloc(g_lib.books, (size_t)newCap * sizeof(Book));
    if (newBooks == NULL) return BOOK_ERR_MEMORY;
    memset(newBooks + g_lib.capacity, 0, (size_t)(newCap - g_lib.capacity) * sizeof(Book));
    g_lib.books = newBooks;
    g_lib.capacity = newCap;
    return BOOK_OK;
}

/* ======================== 核心业务逻辑 ======================== */

static int findBookIndexById(const char *id) {
    for (int i = 0; i < g_lib.count; i++) {
        if (strcmp(g_lib.books[i].id, id) == 0) return i;
    }
    return -1;
}

static BookStatus addBook(const char *id, const char *title, const char *author,
                          const char *type, int stock) {
    if (findBookIndexById(id) >= 0) return BOOK_ERR_DUPLICATE_ID;
    BookStatus s = ensureCapacity(g_lib.count + 1);
    if (s != BOOK_OK) return s;
    Book *b = &g_lib.books[g_lib.count];
    memset(b, 0, sizeof(Book));
    strncpy(b->id, id, MAX_ID_LEN - 1);
    b->id[MAX_ID_LEN - 1] = '\0';
    strncpy(b->title, title, MAX_TITLE_LEN - 1);
    b->title[MAX_TITLE_LEN - 1] = '\0';
    strncpy(b->author, author, MAX_AUTHOR_LEN - 1);
    b->author[MAX_AUTHOR_LEN - 1] = '\0';
    strncpy(b->type, type, MAX_TYPE_LEN - 1);
    b->type[MAX_TYPE_LEN - 1] = '\0';
    b->stock = stock;
    b->total_borrowed = 0;
    b->borrow_list = NULL;
    g_lib.count++;
    return BOOK_OK;
}

static BookStatus addStock(const char *id, int qty) {
    int idx = findBookIndexById(id);
    if (idx < 0) return BOOK_ERR_NOT_FOUND;
    g_lib.books[idx].stock += qty;
    return BOOK_OK;
}

static BookStatus removeBook(const char *id) {
    int idx = findBookIndexById(id);
    if (idx < 0) return BOOK_ERR_NOT_FOUND;
    freeBorrowList(g_lib.books[idx].borrow_list);
    for (int i = idx; i < g_lib.count - 1; i++) {
        g_lib.books[i] = g_lib.books[i + 1];
    }
    memset(&g_lib.books[g_lib.count - 1], 0, sizeof(Book));
    g_lib.count--;
    return BOOK_OK;
}

static BookStatus updateBook(const char *id, const char *title, const char *author,
                             const char *type) {
    int idx = findBookIndexById(id);
    if (idx < 0) return BOOK_ERR_NOT_FOUND;
    Book *b = &g_lib.books[idx];
    if (title[0] != '\0') {
        strncpy(b->title, title, MAX_TITLE_LEN - 1);
        b->title[MAX_TITLE_LEN - 1] = '\0';
    }
    if (author[0] != '\0') {
        strncpy(b->author, author, MAX_AUTHOR_LEN - 1);
        b->author[MAX_AUTHOR_LEN - 1] = '\0';
    }
    if (type[0] != '\0') {
        strncpy(b->type, type, MAX_TYPE_LEN - 1);
        b->type[MAX_TYPE_LEN - 1] = '\0';
    }
    return BOOK_OK;
}

static BookStatus borrowBook(const char *id, const char *card_id, const char *due_date) {
    int idx = findBookIndexById(id);
    if (idx < 0) return BOOK_ERR_NOT_FOUND;
    Book *b = &g_lib.books[idx];
    if (b->stock <= 0) return BOOK_ERR_OUT_OF_STOCK;
    BorrowRecord *rec = (BorrowRecord *)calloc(1, sizeof(BorrowRecord));
    if (rec == NULL) return BOOK_ERR_MEMORY;
    strncpy(rec->card_id, card_id, MAX_CARD_LEN - 1);
    rec->card_id[MAX_CARD_LEN - 1] = '\0';
    strncpy(rec->due_date, due_date, MAX_DATE_LEN - 1);
    rec->due_date[MAX_DATE_LEN - 1] = '\0';
    rec->borrow_time = time(NULL);
    rec->next = NULL;
    if (b->borrow_list == NULL) {
        b->borrow_list = rec;
    } else {
        BorrowRecord *tail = b->borrow_list;
        while (tail->next != NULL) tail = tail->next;
        tail->next = rec;
    }
    b->stock--;
    b->total_borrowed++;
    return BOOK_OK;
}

static BookStatus returnBook(const char *id, const char *card_id) {
    int idx = findBookIndexById(id);
    if (idx < 0) return BOOK_ERR_NOT_FOUND;
    Book *b = &g_lib.books[idx];
    if (b->borrow_list == NULL) return BOOK_ERR_NO_BORROW;
    BorrowRecord *prev = NULL;
    BorrowRecord *cur = b->borrow_list;
    while (cur != NULL) {
        if (strcmp(cur->card_id, card_id) == 0) break;
        prev = cur;
        cur = cur->next;
    }
    if (cur == NULL) return BOOK_ERR_NOT_FOUND;
    if (prev == NULL) b->borrow_list = cur->next;
    else prev->next = cur->next;
    free(cur);
    b->stock++;
    return BOOK_OK;
}

/* ======================== 文件持久化（BOOKV2格式） ======================== */

static BookStatus saveLibraryToFile(const char *path) {
    FILE *fp = fopen(path, "wb");
    if (fp == NULL) return BOOK_ERR_OPEN_FILE;
    BookFileHeader hdr;
    memcpy(hdr.magic, BOOK_MAGIC, 6);
    hdr.version = BOOK_VERSION;
    hdr.book_count = (unsigned int)g_lib.count;
    hdr.header_size = (unsigned int)sizeof(BookFileHeader);
    if (fwrite(&hdr, sizeof(hdr), 1, fp) != 1) { fclose(fp); return BOOK_ERR_WRITE_FILE; }
    for (int i = 0; i < g_lib.count; i++) {
        Book *b = &g_lib.books[i];
        if (fwrite(b, sizeof(Book) - sizeof(BorrowRecord *), 1, fp) != 1) {
            fclose(fp); return BOOK_ERR_WRITE_FILE;
        }
        int borrowCount = 0;
        BorrowRecord *br = b->borrow_list;
        while (br) { borrowCount++; br = br->next; }
        if (fwrite(&borrowCount, sizeof(int), 1, fp) != 1) { fclose(fp); return BOOK_ERR_WRITE_FILE; }
        br = b->borrow_list;
        while (br) {
            if (fwrite(br, sizeof(BorrowRecord) - sizeof(BorrowRecord *), 1, fp) != 1) {
                fclose(fp); return BOOK_ERR_WRITE_FILE;
            }
            br = br->next;
        }
    }
    fclose(fp);
    return BOOK_OK;
}

static BookStatus loadLibraryFromFile(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) return BOOK_ERR_OPEN_FILE;
    BookFileHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) { fclose(fp); return BOOK_ERR_READ_FILE; }
    if (memcmp(hdr.magic, BOOK_MAGIC, 6) != 0) { fclose(fp); return BOOK_ERR_MAGIC; }
    if (hdr.version > BOOK_VERSION) { fclose(fp); return BOOK_ERR_VERSION; }
    freeLibrary();
    BookStatus s = ensureCapacity((int)hdr.book_count);
    if (s != BOOK_OK) { fclose(fp); return s; }
    g_lib.count = (int)hdr.book_count;
    for (int i = 0; i < (int)hdr.book_count; i++) {
        Book *b = &g_lib.books[i];
        memset(b, 0, sizeof(Book));
        if (fread(b, sizeof(Book) - sizeof(BorrowRecord *), 1, fp) != 1) {
            fclose(fp); freeLibrary(); return BOOK_ERR_READ_FILE;
        }
        b->borrow_list = NULL;
        int borrowCount = 0;
        if (fread(&borrowCount, sizeof(int), 1, fp) != 1) {
            fclose(fp); freeLibrary(); return BOOK_ERR_READ_FILE;
        }
        BorrowRecord *tail = NULL;
        for (int j = 0; j < borrowCount; j++) {
            BorrowRecord *rec = (BorrowRecord *)calloc(1, sizeof(BorrowRecord));
            if (rec == NULL) { fclose(fp); freeLibrary(); return BOOK_ERR_MEMORY; }
            if (fread(rec, sizeof(BorrowRecord) - sizeof(BorrowRecord *), 1, fp) != 1) {
                free(rec); fclose(fp); freeLibrary(); return BOOK_ERR_READ_FILE;
            }
            rec->next = NULL;
            if (b->borrow_list == NULL) {
                b->borrow_list = rec;
                tail = rec;
            } else {
                tail->next = rec;
                tail = rec;
            }
        }
    }
    fclose(fp);
    return BOOK_OK;
}

/* ======================== 显示与查询 ======================== */

static void printBookDetail(const Book *b, int idx) {
    cPrint(C_BOLD C_WHITE, "  ┌─ 图书 #%d ", idx + 1);
    cPrint(C_GRAY, "──────────────────────────────────────────────\n");
    printf("  │ 书号:   %s\n", b->id);
    printf("  │ 书名:   %s\n", b->title);
    printf("  │ 作者:   %s\n", b->author);
    printf("  │ 分类:   %s\n", b->type);
    printf("  │ 库存:   ");
    if (b->stock > 0) cPrint(C_GREEN, "%d 册\n", b->stock);
    else cPrint(C_RED, "%d 册（已借完）\n", b->stock);
    printf("  │ 累计借阅: %d 次\n", b->total_borrowed);
    if (b->borrow_list != NULL) {
        cPrint(C_CYAN, "  │ 当前借阅记录:\n");
        BorrowRecord *br = b->borrow_list;
        while (br != NULL) {
            printf("  │   - 图书证: %-20s 应还日期: %s\n", br->card_id, br->due_date);
            br = br->next;
        }
    } else {
        cPrint(C_GRAY, "  │ (无借阅记录)\n");
    }
    cPrint(C_GRAY, "  └──────────────────────────────────────────────────────────\n");
}

static void listAllBooks(void) {
    printTitle("图书列表");
    if (g_lib.count == 0) {
        printWarn("当前书库为空");
        return;
    }
    cPrint(C_BOLD, "  %-4s %-14s %-24s %-14s %-8s %-6s\n",
           "序号", "书号", "书名", "作者", "分类", "库存");
    printDivider();
    for (int i = 0; i < g_lib.count; i++) {
        Book *b = &g_lib.books[i];
        printf("  %-4d %-14s %-24s %-14s %-8s ",
               i + 1, b->id, b->title, b->author, b->type);
        if (b->stock > 0) cPrint(C_GREEN, "%-6d\n", b->stock);
        else cPrint(C_RED, "%-6d\n", b->stock);
    }
    printDivider();
    cPrint(C_CYAN, "  共计 %d 本图书\n", g_lib.count);
}

static void queryById(void) {
    char id[MAX_ID_LEN];
    printTitle("按书号查询");
    readString("请输入书号: ", id, MAX_ID_LEN);
    int idx = findBookIndexById(id);
    if (idx < 0) {
        printErr("未找到该书号的图书");
        return;
    }
    printBookDetail(&g_lib.books[idx], idx);
}

static void queryByType(void) {
    char type[MAX_TYPE_LEN];
    printTitle("按分类查询");
    readString("请输入图书分类: ", type, MAX_TYPE_LEN);
    int found = 0;
    for (int i = 0; i < g_lib.count; i++) {
        if (strcmp(g_lib.books[i].type, type) == 0) {
            printBookDetail(&g_lib.books[i], i);
            found++;
        }
    }
    if (found == 0) {
        printWarn("未找到该分类的图书");
    } else {
        cPrint(C_GREEN, "  共找到 %d 本 [%s] 分类的图书\n", found, type);
    }
}

static void searchByKeyword(void) {
    char kw[128];
    printTitle("关键词搜索（书名/作者）");
    readString("请输入关键词: ", kw, 128);
    int found = 0;
    for (int i = 0; i < g_lib.count; i++) {
        if (strstr(g_lib.books[i].title, kw) != NULL ||
            strstr(g_lib.books[i].author, kw) != NULL) {
            printBookDetail(&g_lib.books[i], i);
            found++;
        }
    }
    if (found == 0) {
        printWarn("未找到包含该关键词的图书");
    } else {
        cPrint(C_GREEN, "  共找到 %d 本匹配图书\n", found);
    }
}

/* ======================== 交互菜单功能 ======================== */

static void doAddBook(void) {
    char id[MAX_ID_LEN], title[MAX_TITLE_LEN], author[MAX_AUTHOR_LEN], type[MAX_TYPE_LEN];
    int stock;
    printTitle("新增图书 / 补充库存");
    readString("请输入书号(ISBN): ", id, MAX_ID_LEN);
    int existing = findBookIndexById(id);
    if (existing >= 0) {
        printInfo("该书号已存在，将执行补充库存操作");
        printf("  当前库存: %d 册\n", g_lib.books[existing].stock);
        int qty = readInt("请输入要补充的数量: ", 1, 10000);
        if (addStock(id, qty) == BOOK_OK) {
            printOK("库存补充成功");
            saveLibraryToFile(dataFilePath);
        } else {
            printErr("操作失败");
        }
        return;
    }
    readString("请输入书名: ", title, MAX_TITLE_LEN);
    readString("请输入作者: ", author, MAX_AUTHOR_LEN);
    readString("请输入分类: ", type, MAX_TYPE_LEN);
    stock = readInt("请输入初始库存数量: ", 0, 10000);
    BookStatus s = addBook(id, title, author, type, stock);
    if (s == BOOK_OK) {
        printOK("图书入库成功");
        saveLibraryToFile(dataFilePath);
        logMessage("ADD: %s - %s x%d", id, title, stock);
    } else {
        printErr("入库失败（可能内存不足）");
    }
}

static void doRemoveBook(void) {
    char id[MAX_ID_LEN];
    printTitle("注销图书");
    readString("请输入要注销的书号: ", id, MAX_ID_LEN);
    int idx = findBookIndexById(id);
    if (idx < 0) { printErr("未找到该图书"); return; }
    Book *b = &g_lib.books[idx];
    printf("  将注销图书: [%s] %s (作者:%s 库存:%d)\n", b->id, b->title, b->author, b->stock);
    if (b->borrow_list != NULL) {
        printWarn("该图书当前有借出记录，注销后借阅记录将一并删除！");
    }
    if (!readYesNo("确认注销该图书？")) {
        printInfo("操作已取消");
        return;
    }
    if (removeBook(id) == BOOK_OK) {
        printOK("图书已注销");
        saveLibraryToFile(dataFilePath);
        logMessage("REMOVE: %s", id);
    } else {
        printErr("注销失败");
    }
}

static void doUpdateBook(void) {
    char id[MAX_ID_LEN], title[MAX_TITLE_LEN], author[MAX_AUTHOR_LEN], type[MAX_TYPE_LEN];
    printTitle("修改图书信息");
    readString("请输入要修改的书号: ", id, MAX_ID_LEN);
    int idx = findBookIndexById(id);
    if (idx < 0) { printErr("未找到该图书"); return; }
    Book *b = &g_lib.books[idx];
    printf("  当前信息 - 书名: %s | 作者: %s | 分类: %s\n", b->title, b->author, b->type);
    printf("  （直接回车表示不修改该项）\n");
    printf("  新书名: "); fflush(stdout);
    if (fgets(title, MAX_TITLE_LEN, stdin) == NULL) { clearerr(stdin); return; }
    char *nl = strchr(title, '\n'); if (nl) *nl = '\0'; else { int c; while((c=getchar())!='\n'&&c!=EOF); }
    printf("  新作者: "); fflush(stdout);
    if (fgets(author, MAX_AUTHOR_LEN, stdin) == NULL) { clearerr(stdin); return; }
    nl = strchr(author, '\n'); if (nl) *nl = '\0'; else { int c; while((c=getchar())!='\n'&&c!=EOF); }
    printf("  新分类: "); fflush(stdout);
    if (fgets(type, MAX_TYPE_LEN, stdin) == NULL) { clearerr(stdin); return; }
    nl = strchr(type, '\n'); if (nl) *nl = '\0'; else { int c; while((c=getchar())!='\n'&&c!=EOF); }
    if (updateBook(id, title, author, type) == BOOK_OK) {
        printOK("图书信息已更新");
        saveLibraryToFile(dataFilePath);
        logMessage("UPDATE: %s", id);
    } else {
        printErr("更新失败");
    }
}

static void doBorrowBook(void) {
    char id[MAX_ID_LEN], card[MAX_CARD_LEN], date[MAX_DATE_LEN];
    printTitle("借阅图书");
    readString("请输入书号: ", id, MAX_ID_LEN);
    int idx = findBookIndexById(id);
    if (idx < 0) { printErr("未找到该图书"); return; }
    Book *b = &g_lib.books[idx];
    if (b->stock <= 0) { printErr("该图书已全部借出，暂无库存"); return; }
    printf("  当前库存: %d 册\n", b->stock);
    readString("请输入图书证号: ", card, MAX_CARD_LEN);
    readString("请输入应还日期(YYYY-MM-DD): ", date, MAX_DATE_LEN);
    if (borrowBook(id, card, date) == BOOK_OK) {
        printOK("借阅成功");
        printf("  《%s》已借给 %s，剩余库存 %d 册\n", b->title, card, b->stock);
        saveLibraryToFile(dataFilePath);
        logMessage("BORROW: %s by %s, due %s", id, card, date);
    } else {
        printErr("借阅失败");
    }
}

static void doReturnBook(void) {
    char id[MAX_ID_LEN], card[MAX_CARD_LEN];
    printTitle("归还图书");
    readString("请输入书号: ", id, MAX_ID_LEN);
    int idx = findBookIndexById(id);
    if (idx < 0) { printErr("未找到该图书"); return; }
    Book *b = &g_lib.books[idx];
    if (b->borrow_list == NULL) { printWarn("该图书当前没有借出记录"); return; }
    cPrint(C_CYAN, "  当前借阅记录:\n");
    BorrowRecord *br = b->borrow_list;
    int n = 0;
    while (br) {
        n++;
        printf("  %d. 图书证: %-20s 应还: %s\n", n, br->card_id, br->due_date);
        br = br->next;
    }
    readString("请输入归还人图书证号: ", card, MAX_CARD_LEN);
    if (returnBook(id, card) == BOOK_OK) {
        printOK("归还成功");
        printf("  《%s》已由 %s 归还，当前库存 %d 册\n", b->title, card, b->stock);
        saveLibraryToFile(dataFilePath);
        logMessage("RETURN: %s by %s", id, card);
    } else {
        printErr("归还失败（未找到该借阅记录）");
    }
}

/* ======================== 主菜单 ======================== */

static void showMainMenu(void) {
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  主菜单\n");
    printDivider();
    cPrint(C_CYAN, "  1 "); printf("- 图书入库 / 补充库存\n");
    cPrint(C_CYAN, "  2 "); printf("- 注销图书\n");
    cPrint(C_CYAN, "  3 "); printf("- 修改图书信息\n");
    cPrint(C_CYAN, "  4 "); printf("- 借阅图书\n");
    cPrint(C_CYAN, "  5 "); printf("- 归还图书\n");
    printDivider();
    cPrint(C_CYAN, "  6 "); printf("- 显示全部图书\n");
    cPrint(C_CYAN, "  7 "); printf("- 按书号查询\n");
    cPrint(C_CYAN, "  8 "); printf("- 按分类查询\n");
    cPrint(C_CYAN, "  9 "); printf("- 关键词搜索\n");
    printDivider();
    cPrint(C_CYAN, "  S "); printf("- 保存数据\n");
    cPrint(C_CYAN, "  L "); printf("- 重新加载数据\n");
    cPrint(C_CYAN, "  H/?"); printf("- 帮助\n");
    cPrint(C_CYAN, "  0 "); printf("- 退出系统\n");
    printDivider();
    printf("  书库状态: ");
    cPrint(C_GREEN, "%d 本图书", g_lib.count);
    printf(" | 数据文件: "); cPrint(C_WHITE, "%s", dataFilePath);
    printf("\n");
    printDivider();
}

static void showHelp(void) {
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  使用帮助\n");
    printThickDivider();
    printf("\n");
    cPrint(C_BOLD, "  图书管理流程:\n");
    printf("    1. 入库新书（选1）输入书号、书名、作者、分类、库存\n");
    printf("    2. 已有图书可直接输入书号补库存\n");
    printf("    3. 借阅（选4）登记图书证号和应还日期，库存自动-1\n");
    printf("    4. 归还（选5）输入图书证号，库存自动+1\n");
    printf("    5. 查询（6-9）支持列表/书号/分类/关键词\n");
    printf("    6. 修改（3）可更新书名、作者、分类（书号不可改）\n");
    printf("    7. 注销（2）删除图书及其所有借阅记录\n");
    printf("\n");
    cPrint(C_BOLD, "  命令行模式:\n");
    cPrint(C_CYAN, "    library.exe -h/--help    "); printf("显示帮助\n");
    cPrint(C_CYAN, "    library.exe -t/--test    "); printf("运行内置自测\n");
    cPrint(C_CYAN, "    library.exe -f <文件>    "); printf("指定数据文件\n");
    cPrint(C_CYAN, "    （Windows下可直接拖放文件到library.exe打开）\n");
    printf("\n");
    cPrint(C_BOLD, "  数据文件格式:\n");
    cPrint(C_CYAN, "    Magic: "); printf("\"BOOKV2\" (6字节标识)\n");
    cPrint(C_CYAN, "    版本:   "); printf("V%d\n", BOOK_VERSION);
    cPrint(C_CYAN, "    存储:   "); printf("二进制格式，含图书信息+借阅链表\n");
    printf("\n");
}

static void runInteractive(void) {
    char choice[64];
    while (1) {
        showMainMenu();
        printf("  请选择操作: "); fflush(stdout);
        if (fgets(choice, sizeof(choice), stdin) == NULL) break;
        char *nl = strchr(choice, '\n'); if (nl) *nl = '\0';
        if (choice[0] == '\0') continue;
        char c = (char)toupper((unsigned char)choice[0]);
        switch (c) {
        case '1': doAddBook();      break;
        case '2': doRemoveBook();   break;
        case '3': doUpdateBook();   break;
        case '4': doBorrowBook();   break;
        case '5': doReturnBook();   break;
        case '6': listAllBooks();   break;
        case '7': queryById();      break;
        case '8': queryByType();    break;
        case '9': searchByKeyword(); break;
        case 'S':
            if (saveLibraryToFile(dataFilePath) == BOOK_OK)
                printOK("数据已保存");
            else printErr("保存失败");
            break;
        case 'L': {
            BookStatus s = loadLibraryFromFile(dataFilePath);
            if (s == BOOK_OK) printOK("数据已重新加载");
            else if (s == BOOK_ERR_OPEN_FILE) printWarn("数据文件不存在（首次运行？）");
            else printErr("加载失败，文件可能已损坏");
            break;
        }
        case 'H':
        case '?': showHelp(); break;
        case '0':
            saveLibraryToFile(dataFilePath);
            cPrint(C_GREEN, "\n  [OK] 数据已保存，感谢使用，再见！\n\n");
            logMessage("EXIT: 正常退出，共%d本图书", g_lib.count);
            return;
        default:
            cPrint(C_YELLOW, "  [!] 无效选项，请重新输入\n");
            break;
        }
    }
}

/* ======================== 内置自测用例 ======================== */

static void testAssert(int cond, const char *fmt, ...) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (cond) {
        testPassed++;
        if (!testMode) cPrint(C_GREEN, "    PASS: ");
        else printf("    PASS: ");
        printf("%s\n", buf);
    } else {
        testFailed++;
        if (!testMode) cPrint(C_RED, "    FAIL: ");
        else printf("    FAIL: ");
        printf("%s\n", buf);
    }
}

#define TEST_ASSERT(cond, ...) testAssert((cond), __VA_ARGS__)

static int runSelfTests(void) {
    testPassed = 0;
    testFailed = 0;
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  运行内置自测用例\n");
    printThickDivider();

    /* 测试1: 初始化与空状态 */
    cPrint(C_BOLD C_CYAN, "\n  [Test 1] 初始化状态测试\n");
    TEST_ASSERT(g_lib.count == 0, "初始图书数量为0");
    TEST_ASSERT(g_lib.capacity == 0, "初始容量为0");

    /* 测试2: 添加图书 */
    cPrint(C_BOLD C_CYAN, "\n  [Test 2] 图书入库测试\n");
    BookStatus s;
    s = addBook("9787111213826", "C程序设计语言", "Brian W. Kernighan", "计算机", 5);
    TEST_ASSERT(s == BOOK_OK, "添加《C程序设计语言》成功");
    s = addBook("9787115428028", "算法导论", "Thomas H. Cormen", "计算机", 3);
    TEST_ASSERT(s == BOOK_OK, "添加《算法导论》成功");
    s = addBook("9787020002207", "红楼梦", "曹雪芹", "文学", 2);
    TEST_ASSERT(s == BOOK_OK, "添加《红楼梦》成功");
    s = addBook("9787544253994", "百年孤独", "加西亚·马尔克斯", "文学", 4);
    TEST_ASSERT(s == BOOK_OK, "添加《百年孤独》成功");
    s = addBook("9787111213826", "重复书号测试", "Test", "Test", 1);
    TEST_ASSERT(s == BOOK_ERR_DUPLICATE_ID, "重复书号正确返回错误");
    TEST_ASSERT(g_lib.count == 4, "入库后图书总数为4");
    TEST_ASSERT(g_lib.capacity >= 4, "自动扩容工作正常");

    /* 测试3: 查找 */
    cPrint(C_BOLD C_CYAN, "\n  [Test 3] 图书查找测试\n");
    int idx = findBookIndexById("9787111213826");
    TEST_ASSERT(idx == 0, "按书号查找《C程序设计语言》位置正确");
    idx = findBookIndexById("9999999999999");
    TEST_ASSERT(idx == -1, "不存在书号返回-1正确");
    TEST_ASSERT(strcmp(g_lib.books[0].title, "C程序设计语言") == 0, "书名数据正确");
    TEST_ASSERT(g_lib.books[0].stock == 5, "库存数据正确");

    /* 测试4: 补充库存 */
    cPrint(C_BOLD C_CYAN, "\n  [Test 4] 库存补充测试\n");
    s = addStock("9787111213826", 3);
    TEST_ASSERT(s == BOOK_OK, "补充库存成功");
    TEST_ASSERT(g_lib.books[0].stock == 8, "库存增加正确(5+3=8)");
    s = addStock("000", 1);
    TEST_ASSERT(s == BOOK_ERR_NOT_FOUND, "不存在图书补库存返回错误");

    /* 测试5: 借阅功能 */
    cPrint(C_BOLD C_CYAN, "\n  [Test 5] 借阅功能测试\n");
    s = borrowBook("9787111213826", "2025001001", "2026-07-01");
    TEST_ASSERT(s == BOOK_OK, "第一次借阅成功");
    TEST_ASSERT(g_lib.books[0].stock == 7, "借阅后库存-1正确(8-1=7)");
    TEST_ASSERT(g_lib.books[0].total_borrowed == 1, "累计借阅次数+1正确");
    TEST_ASSERT(g_lib.books[0].borrow_list != NULL, "借阅记录已创建");
    s = borrowBook("9787111213826", "2025001002", "2026-07-15");
    TEST_ASSERT(s == BOOK_OK, "第二次借阅成功");
    TEST_ASSERT(g_lib.books[0].stock == 6, "二次借阅库存正确(7-1=6)");
    TEST_ASSERT(g_lib.books[0].borrow_list->next != NULL, "借阅记录链表正确");
    s = borrowBook("000", "x", "x");
    TEST_ASSERT(s == BOOK_ERR_NOT_FOUND, "不存在图书借阅失败");

    /* 测试6: 借阅超量测试 */
    cPrint(C_BOLD C_CYAN, "\n  [Test 6] 库存耗尽测试\n");
    addBook("TEST001", "测试书", "测试作者", "测试", 1);
    borrowBook("TEST001", "CARD001", "2026-01-01");
    s = borrowBook("TEST001", "CARD002", "2026-01-02");
    TEST_ASSERT(s == BOOK_ERR_OUT_OF_STOCK, "库存为0时借阅正确返回OUT_OF_STOCK");
    removeBook("TEST001");

    /* 测试7: 归还功能 */
    cPrint(C_BOLD C_CYAN, "\n  [Test 7] 归还功能测试\n");
    s = returnBook("9787111213826", "2025001001");
    TEST_ASSERT(s == BOOK_OK, "归还图书成功");
    TEST_ASSERT(g_lib.books[0].stock == 7, "归还后库存+1正确(6+1=7)");
    s = returnBook("9787111213826", "999999999");
    TEST_ASSERT(s == BOOK_ERR_NOT_FOUND, "归还不存在借阅记录失败");
    s = returnBook("000", "x");
    TEST_ASSERT(s == BOOK_ERR_NOT_FOUND, "归还不存在图书失败");

    /* 测试8: 修改图书信息 */
    cPrint(C_BOLD C_CYAN, "\n  [Test 8] 信息修改测试\n");
    s = updateBook("9787020002207", "红楼梦（评注本）", "", "古典文学");
    TEST_ASSERT(s == BOOK_OK, "修改图书信息成功");
    TEST_ASSERT(strcmp(g_lib.books[2].title, "红楼梦（评注本）") == 0, "书名更新正确");
    TEST_ASSERT(strcmp(g_lib.books[2].type, "古典文学") == 0, "分类更新正确");
    TEST_ASSERT(strcmp(g_lib.books[2].author, "曹雪芹") == 0, "作者未修改正确保留");
    s = updateBook("000", "", "", "");
    TEST_ASSERT(s == BOOK_ERR_NOT_FOUND, "修改不存在图书失败");

    /* 测试9: 文件I/O测试 */
    cPrint(C_BOLD C_CYAN, "\n  [Test 9] 文件持久化测试\n");
    const char *testFile = "_test_library.dat";
    remove(testFile);
    s = saveLibraryToFile(testFile);
    TEST_ASSERT(s == BOOK_OK, "保存到文件成功");
    TEST_ASSERT(fileExists(testFile), "数据文件已创建");
    int savedCount = g_lib.count;
    freeLibrary();
    TEST_ASSERT(g_lib.count == 0, "释放后状态清空");
    s = loadLibraryFromFile(testFile);
    TEST_ASSERT(s == BOOK_OK, "从文件加载成功");
    TEST_ASSERT(g_lib.count == savedCount, "加载后图书数量一致(%d)", savedCount);
    idx = findBookIndexById("9787111213826");
    TEST_ASSERT(idx >= 0, "加载后书号查找正常");
    TEST_ASSERT(g_lib.books[idx].stock == 7, "加载后库存数据正确");
    TEST_ASSERT(strcmp(g_lib.books[2].type, "古典文学") == 0, "加载后分类数据正确");
    remove(testFile);

    /* 测试10: 魔数和版本校验 */
    cPrint(C_BOLD C_CYAN, "\n  [Test 10] 文件格式校验测试\n");
    FILE *fp = fopen("_test_bad.dat", "wb");
    char badmagic[16] = "BADX00";
    fwrite(badmagic, 1, 16, fp);
    fclose(fp);
    s = loadLibraryFromFile("_test_bad.dat");
    TEST_ASSERT(s == BOOK_ERR_MAGIC, "错误魔数正确返回MAGIC错误");
    remove("_test_bad.dat");

    /* 测试11: 注销图书 */
    cPrint(C_BOLD C_CYAN, "\n  [Test 11] 图书注销测试\n");
    int cnt = g_lib.count;
    s = removeBook("9787544253994");
    TEST_ASSERT(s == BOOK_OK, "注销《百年孤独》成功");
    TEST_ASSERT(g_lib.count == cnt - 1, "注销后数量-1正确");
    idx = findBookIndexById("9787544253994");
    TEST_ASSERT(idx == -1, "注销后该书号查找不到");
    s = removeBook("000");
    TEST_ASSERT(s == BOOK_ERR_NOT_FOUND, "注销不存在图书失败");

    /* 测试12: 大量数据扩容 */
    cPrint(C_BOLD C_CYAN, "\n  [Test 12] 动态扩容压力测试\n");
    freeLibrary();
    int stressOK = 1;
    for (int i = 0; i < 100; i++) {
        char isbn[32], t[64], a[32];
        snprintf(isbn, sizeof(isbn), "STRESS%04d", i);
        snprintf(t, sizeof(t), "测试图书%d", i);
        snprintf(a, sizeof(a), "作者%d", i);
        if (addBook(isbn, t, a, "测试", i % 10) != BOOK_OK) { stressOK = 0; break; }
    }
    TEST_ASSERT(stressOK, "批量添加100本图书无内存错误");
    TEST_ASSERT(g_lib.count == 100, "批量添加后数量为100");
    TEST_ASSERT(g_lib.capacity >= 100, "容量自动扩容到>=100");
    TEST_ASSERT(strcmp(g_lib.books[50].id, "STRESS0050") == 0, "第50本书数据正确");
    TEST_ASSERT(strcmp(g_lib.books[99].title, "测试图书99") == 0, "第99本书数据正确");
    freeLibrary();

    /* 测试结果汇总 */
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  测试结果汇总\n");
    printDivider();
    cPrint(C_GREEN, "  通过: %d\n", testPassed);
    if (testFailed > 0) cPrint(C_RED, "  失败: %d\n", testFailed);
    else cPrint(C_GREEN, "  失败: %d\n", testFailed);
    int total = testPassed + testFailed;
    cPrint(C_CYAN, "  总计: %d 个测试用例\n", total);
    if (testFailed == 0) {
        cPrint(C_BOLD C_GREEN, "\n  ✓ 所有测试通过！\n\n");
        logMessage("TEST: %d/%d PASS", testPassed, total);
    } else {
        cPrint(C_BOLD C_RED, "\n  ✗ 有 %d 个测试失败！\n\n", testFailed);
    }
    printThickDivider();
    return testFailed == 0 ? 0 : 1;
}

/* ======================== 命令行处理 ======================== */

static void printUsage(const char *prog) {
    printf("Library Management System V2.0 - Enhanced Edition\n\n");
    printf("用法:\n");
    printf("  %s                     交互模式（菜单）\n", prog);
    printf("  %s -h, --help          显示帮助\n", prog);
    printf("  %s -t, --test          运行内置自测用例\n", prog);
    printf("  %s -f <数据文件>       指定数据文件路径\n", prog);
    printf("  %s <数据文件>          直接打开数据文件（拖放模式）\n", prog);
    printf("\n示例:\n");
    printf("  %s -f mybooks.dat      使用 mybooks.dat 作为数据文件\n", prog);
    printf("  %s -t                  运行所有自测用例验证功能\n", prog);
    printf("\n（Windows下直接拖放文件到%s可打开该数据文件）\n", prog);
}

static int runCommandLine(int argc, char *argv[]) {
    if (argc < 2) return 0;
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        printUsage(argv[0]);
        return 1;
    }
    if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0) {
        testMode = 1;
        int ret = runSelfTests();
        freeLibrary();
        return ret;
    }
    if (strcmp(argv[1], "-f") == 0) {
        if (argc < 3) { printErr("用法: -f <数据文件路径>"); return 1; }
        strncpy(dataFilePath, argv[2], sizeof(dataFilePath) - 1);
        dataFilePath[sizeof(dataFilePath) - 1] = '\0';
        return 0;
    }
    if (argv[1][0] != '-') {
        strncpy(dataFilePath, argv[1], sizeof(dataFilePath) - 1);
        dataFilePath[sizeof(dataFilePath) - 1] = '\0';
        cPrint(C_CYAN, "  [INFO] 拖放模式：打开数据文件 %s\n", dataFilePath);
        return 0;
    }
    printUsage(argv[0]);
    return 1;
}

/* ======================== 程序入口 ======================== */

int main(int argc, char *argv[]) {
    enableVTMode();
    logMessage("PROGRAM START V2.0");

    int cmdRet = runCommandLine(argc, argv);
    if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0 ||
                     strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0)) {
        return cmdRet;
    }

    if (!testMode) {
        printBanner();
        BookStatus s = loadLibraryFromFile(dataFilePath);
        if (s == BOOK_OK) {
            printOK("数据加载成功");
            cPrint(C_CYAN, "  书库现有 %d 本图书\n", g_lib.count);
        } else if (s == BOOK_ERR_OPEN_FILE) {
            printInfo("数据文件不存在，将创建新书库");
        } else if (s == BOOK_ERR_MAGIC) {
            printErr("数据文件格式错误（魔数不匹配），可能是旧版本文件");
            if (readYesNo("是否覆盖为V2.0新格式？（旧数据将丢失）")) {
                freeLibrary();
                printInfo("已初始化空书库");
            } else {
                freeLibrary();
                return 1;
            }
        } else {
            printErr("数据文件损坏，将以空书库启动");
            freeLibrary();
        }
        runInteractive();
    }

    freeLibrary();
    return 0;
}
