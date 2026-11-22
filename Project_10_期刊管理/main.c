/*
 * ============================================================
 *  期刊管理系统 V2.0 - Journal Management System Enhanced
 *  Curriculum Design Project - C Language Data Structures
 *  Enhanced Edition with V2.0 Standards
 * ============================================================
 *  Compiler: GCC (MinGW)  -std=c99 -Wall -Wextra -O2
 *  Features:
 *    [NEW] UTF-8 BOM output support
 *    [NEW] ANSI color terminal (Green OK / Red ERR / Cyan Title)
 *    [NEW] CLI: --help/-h / --test/-t / -f <file>
 *    [NEW] Dynamic memory management (dynamic array expansion)
 *    [NEW] Unified error code enum
 *    [NEW] File persistence with magic header "MAGZV2"
 *    [NEW] Built-in self-test suite (CRUD + Subscription + File I/O)
 *    [NEW] Detailed comments
 * ============================================================
 */

#if defined(_WIN32) || defined(_WIN64)
    #define _CRT_SECURE_NO_WARNINGS
    #include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

/* ======================== Version Magic ======================== */
#define MAGZ_MAGIC      "MAGZV2"
#define MAGZ_VERSION    0x0200u
#define MAGZ_MAX_NAME   64
#define MAGZ_MAX_DEPT   48
#define MAGZ_MAX_PHONE  24
#define MAGZ_MAX_PATH   260

/* ======================== ANSI Color Codes ======================== */
#define C_RESET     "\033[0m"
#define C_BOLD      "\033[1m"
#define C_RED       "\033[31m"
#define C_GREEN     "\033[32m"
#define C_YELLOW    "\033[33m"
#define C_CYAN      "\033[36m"
#define C_GRAY      "\033[90m"
#define C_WHITE     "\033[37m"

/* ======================== Unified Error Codes ======================== */
typedef enum {
    MAGZ_OK = 0,
    MAGZ_ERR_MEMORY = -1,
    MAGZ_ERR_NOT_FOUND = -2,
    MAGZ_ERR_DUPLICATE = -3,
    MAGZ_ERR_ALREADY_BORROWED = -4,
    MAGZ_ERR_NOT_BORROWED = -5,
    MAGZ_ERR_QUEUE_FULL = -6,
    MAGZ_ERR_FILE_OPEN = -7,
    MAGZ_ERR_FILE_READ = -8,
    MAGZ_ERR_FILE_WRITE = -9,
    MAGZ_ERR_CRC_MISMATCH = -10,
    MAGZ_ERR_INVALID = -11,
    MAGZ_ERR_MAX_BORROW = -12
} MagzStatus;

/* ======================== Journal Status ======================== */
typedef enum {
    JS_AVAILABLE = 0,
    JS_BORROWED = 1,
    JS_RESERVED = 2
} JournalStatus;

/* ======================== Data Structures (Dynamic Array Based) ======================== */
typedef struct Journal {
    int     id;
    char    name[MAGZ_MAX_NAME];
    char    author[MAGZ_MAX_NAME];
    int     year;
    int     total_copies;
    int     available_copies;
    int     borrower_id;
    char    borrow_date[16];
    JournalStatus status;
} Journal;

typedef struct Reader {
    int     id;
    char    name[MAGZ_MAX_NAME];
    char    dept[MAGZ_MAX_DEPT];
    char    phone[MAGZ_MAX_PHONE];
    int     borrow_count;
    int     borrowed_ids[16];
} Reader;

typedef struct ResvNode {
    int reader_id;
    struct ResvNode *next;
} ResvNode;

typedef struct ResvQueue {
    ResvNode *front;
    ResvNode *rear;
    int count;
} ResvQueue;

typedef struct SysContext {
    Journal *journals;
    int journal_count;
    int journal_capacity;
    Reader  *readers;
    int reader_count;
    int reader_capacity;
    ResvQueue *resv_queues;
    char data_path[MAGZ_MAX_PATH];
} SysContext;

#pragma pack(push, 1)
typedef struct MagzHeader {
    char magic[6];
    unsigned short version;
    int journal_count;
    int reader_count;
    unsigned int data_crc;
} MagzHeader;
#pragma pack(pop)

/* ======================== Global Test Mode Flag ======================== */
static int testMode = 0;

/* ======================== Function Forward Declarations ======================== */
static void enableVTMode(void);
static void cPrint(const char *color, const char *fmt, ...);
static void printOK(const char *msg);
static void printErr(const char *msg);
static void printWarn(const char *msg);
static void printTitle(const char *title);
static void printDivider(void);
static void printThickDivider(void);
static MagzStatus queue_init(ResvQueue *q);
static int queue_is_empty(const ResvQueue *q);
static MagzStatus queue_enqueue(ResvQueue *q, int reader_id);
static int queue_dequeue(ResvQueue *q);
static int queue_peek(const ResvQueue *q);
static int queue_contains(const ResvQueue *q, int reader_id);
static void queue_clear(ResvQueue *q);
static MagzStatus ctx_init(SysContext *ctx);
static void ctx_destroy(SysContext *ctx);
static MagzStatus ctx_ensure_journal_capacity(SysContext *ctx, int needed);
static MagzStatus ctx_ensure_reader_capacity(SysContext *ctx, int needed);
static MagzStatus ctx_add_journal(SysContext *ctx, const Journal *j);
static MagzStatus ctx_add_reader(SysContext *ctx, const Reader *r);
static int ctx_find_journal_index(const SysContext *ctx, int id);
static int ctx_find_reader_index(const SysContext *ctx, int id);
static MagzStatus ctx_remove_journal(SysContext *ctx, int id);
static MagzStatus ctx_remove_reader(SysContext *ctx, int id);
static void get_current_date(char *buf, int buflen);
static int date_compare(const char *d1, const char *d2);
static void clear_input_buffer(void);
static int read_int_input(const char *prompt, int def);
static void read_str_input(const char *prompt, char *buf, int buflen, const char *def);
static MagzStatus save_data(const SysContext *ctx);
static MagzStatus load_data(SysContext *ctx);
static unsigned int crc32_buf(const unsigned char *data, int len);
static MagzStatus journal_borrow(SysContext *ctx, int journal_id, int reader_id);
static MagzStatus journal_return(SysContext *ctx, int journal_id, int reader_id);
static MagzStatus journal_reserve(SysContext *ctx, int journal_id, int reader_id);
static void show_banner(void);
static void do_add_journal(SysContext *ctx);
static void do_add_reader(SysContext *ctx);
static void do_borrow(SysContext *ctx);
static void do_return(SysContext *ctx);
static void do_reserve(SysContext *ctx);
static void do_search_journal(SysContext *ctx);
static void print_all_journals(const SysContext *ctx);
static void print_all_readers(const SysContext *ctx);
static void do_delete_journal(SysContext *ctx);
static void do_delete_reader(SysContext *ctx);
static void do_reminder(SysContext *ctx);
static void show_menu(const SysContext *ctx);
static void pause_screen(void);
static void run_self_tests(void);
static void print_usage(void);
static int run_command_line(int argc, char *argv[], SysContext *ctx);
static void run_interactive(SysContext *ctx);

/* ======================== Utility Functions: Color & Terminal ======================== */

static void enableVTMode(void) {
#if defined(_WIN32) || defined(_WIN64)
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;
    dwMode |= 0x0004;
    SetConsoleMode(hOut, dwMode);
#endif
}

static void cPrint(const char *color, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fputs(color, stdout);
    vprintf(fmt, args);
    fputs(C_RESET, stdout);
    va_end(args);
}

static void printOK(const char *msg) {
    cPrint(C_GREEN, "  [OK] ");
    printf("%s\n", msg);
}

static void printErr(const char *msg) {
    cPrint(C_RED, "  [ERR] ");
    printf("%s\n", msg);
}

static void printWarn(const char *msg) {
    cPrint(C_YELLOW, "  [WARN] ");
    printf("%s\n", msg);
}

static void printTitle(const char *title) {
    printDivider();
    cPrint(C_CYAN C_BOLD, "  %s\n", title);
    printDivider();
}

static void printDivider(void) {
    cPrint(C_CYAN, "  ----------------------------------------\n");
}

static void printThickDivider(void) {
    cPrint(C_CYAN C_BOLD, "  ========================================\n");
}

/* ======================== Reservation Queue (Linked List) ======================== */

static MagzStatus queue_init(ResvQueue *q) {
    q->front = NULL;
    q->rear = NULL;
    q->count = 0;
    return MAGZ_OK;
}

static int queue_is_empty(const ResvQueue *q) {
    return q->count == 0;
}

static MagzStatus queue_enqueue(ResvQueue *q, int reader_id) {
    ResvNode *node = (ResvNode *)malloc(sizeof(ResvNode));
    if (!node) return MAGZ_ERR_MEMORY;
    node->reader_id = reader_id;
    node->next = NULL;
    if (q->rear) q->rear->next = node;
    else q->front = node;
    q->rear = node;
    q->count++;
    return MAGZ_OK;
}

static int queue_dequeue(ResvQueue *q) {
    if (!q->front) return -1;
    ResvNode *node = q->front;
    int id = node->reader_id;
    q->front = node->next;
    if (!q->front) q->rear = NULL;
    free(node);
    q->count--;
    return id;
}

static int queue_peek(const ResvQueue *q) {
    return q->front ? q->front->reader_id : -1;
}

static int queue_contains(const ResvQueue *q, int reader_id) {
    for (ResvNode *n = q->front; n; n = n->next)
        if (n->reader_id == reader_id) return 1;
    return 0;
}

static void queue_clear(ResvQueue *q) {
    while (!queue_is_empty(q)) queue_dequeue(q);
}

/* ======================== System Context & Dynamic Memory Management ======================== */

#define INITIAL_CAPACITY 16

static MagzStatus ctx_init(SysContext *ctx) {
    ctx->journals = NULL;
    ctx->readers = NULL;
    ctx->resv_queues = NULL;
    ctx->journal_count = 0;
    ctx->journal_capacity = 0;
    ctx->reader_count = 0;
    ctx->reader_capacity = 0;
    strncpy(ctx->data_path, "magz_data.bin", MAGZ_MAX_PATH - 1);
    ctx->data_path[MAGZ_MAX_PATH - 1] = '\0';

    MagzStatus s = ctx_ensure_journal_capacity(ctx, INITIAL_CAPACITY);
    if (s != MAGZ_OK) return s;
    s = ctx_ensure_reader_capacity(ctx, INITIAL_CAPACITY);
    if (s != MAGZ_OK) return s;

    ctx->resv_queues = (ResvQueue *)calloc(INITIAL_CAPACITY, sizeof(ResvQueue));
    if (!ctx->resv_queues) return MAGZ_ERR_MEMORY;
    for (int i = 0; i < INITIAL_CAPACITY; i++) queue_init(&ctx->resv_queues[i]);
    return MAGZ_OK;
}

static void ctx_destroy(SysContext *ctx) {
    if (ctx->journals) free(ctx->journals);
    if (ctx->readers) free(ctx->readers);
    if (ctx->resv_queues) {
        for (int i = 0; i < ctx->journal_capacity; i++) queue_clear(&ctx->resv_queues[i]);
        free(ctx->resv_queues);
    }
    ctx->journals = NULL;
    ctx->readers = NULL;
    ctx->resv_queues = NULL;
    ctx->journal_count = ctx->journal_capacity = 0;
    ctx->reader_count = ctx->reader_capacity = 0;
}

static MagzStatus ctx_ensure_journal_capacity(SysContext *ctx, int needed) {
    if (needed <= ctx->journal_capacity) return MAGZ_OK;
    int new_cap = ctx->journal_capacity == 0 ? INITIAL_CAPACITY : ctx->journal_capacity;
    while (new_cap < needed) new_cap *= 2;

    Journal *nj = (Journal *)realloc(ctx->journals, new_cap * sizeof(Journal));
    if (!nj) return MAGZ_ERR_MEMORY;
    memset(nj + ctx->journal_capacity, 0, (new_cap - ctx->journal_capacity) * sizeof(Journal));

    ResvQueue *nq = (ResvQueue *)realloc(ctx->resv_queues, new_cap * sizeof(ResvQueue));
    if (!nq) { free(nj); return MAGZ_ERR_MEMORY; }
    for (int i = ctx->journal_capacity; i < new_cap; i++) queue_init(&nq[i]);

    ctx->journals = nj;
    ctx->resv_queues = nq;
    ctx->journal_capacity = new_cap;
    return MAGZ_OK;
}

static MagzStatus ctx_ensure_reader_capacity(SysContext *ctx, int needed) {
    if (needed <= ctx->reader_capacity) return MAGZ_OK;
    int new_cap = ctx->reader_capacity == 0 ? INITIAL_CAPACITY : ctx->reader_capacity;
    while (new_cap < needed) new_cap *= 2;
    Reader *nr = (Reader *)realloc(ctx->readers, new_cap * sizeof(Reader));
    if (!nr) return MAGZ_ERR_MEMORY;
    memset(nr + ctx->reader_capacity, 0, (new_cap - ctx->reader_capacity) * sizeof(Reader));
    ctx->readers = nr;
    ctx->reader_capacity = new_cap;
    return MAGZ_OK;
}

static MagzStatus ctx_add_journal(SysContext *ctx, const Journal *j) {
    if (ctx_find_journal_index(ctx, j->id) >= 0) return MAGZ_ERR_DUPLICATE;
    MagzStatus s = ctx_ensure_journal_capacity(ctx, ctx->journal_count + 1);
    if (s != MAGZ_OK) return s;
    ctx->journals[ctx->journal_count++] = *j;
    return MAGZ_OK;
}

static MagzStatus ctx_add_reader(SysContext *ctx, const Reader *r) {
    if (ctx_find_reader_index(ctx, r->id) >= 0) return MAGZ_ERR_DUPLICATE;
    MagzStatus s = ctx_ensure_reader_capacity(ctx, ctx->reader_count + 1);
    if (s != MAGZ_OK) return s;
    ctx->readers[ctx->reader_count++] = *r;
    return MAGZ_OK;
}

static int ctx_find_journal_index(const SysContext *ctx, int id) {
    for (int i = 0; i < ctx->journal_count; i++)
        if (ctx->journals[i].id == id) return i;
    return -1;
}

static int ctx_find_reader_index(const SysContext *ctx, int id) {
    for (int i = 0; i < ctx->reader_count; i++)
        if (ctx->readers[i].id == id) return i;
    return -1;
}

static MagzStatus ctx_remove_journal(SysContext *ctx, int id) {
    int idx = ctx_find_journal_index(ctx, id);
    if (idx < 0) return MAGZ_ERR_NOT_FOUND;
    queue_clear(&ctx->resv_queues[idx]);
    for (int i = idx; i < ctx->journal_count - 1; i++) {
        ctx->journals[i] = ctx->journals[i + 1];
        ctx->resv_queues[i] = ctx->resv_queues[i + 1];
    }
    memset(&ctx->journals[ctx->journal_count - 1], 0, sizeof(Journal));
    queue_init(&ctx->resv_queues[ctx->journal_count - 1]);
    ctx->journal_count--;
    return MAGZ_OK;
}

static MagzStatus ctx_remove_reader(SysContext *ctx, int id) {
    int idx = ctx_find_reader_index(ctx, id);
    if (idx < 0) return MAGZ_ERR_NOT_FOUND;
    Reader *r = &ctx->readers[idx];
    for (int k = 0; k < r->borrow_count; k++) {
        int ji = ctx_find_journal_index(ctx, r->borrowed_ids[k]);
        if (ji >= 0) {
            ctx->journals[ji].status = JS_AVAILABLE;
            ctx->journals[ji].borrower_id = -1;
            ctx->journals[ji].available_copies++;
        }
    }
    for (int i = 0; i < ctx->journal_count; i++)
        queue_clear(&ctx->resv_queues[i]);
    for (int i = idx; i < ctx->reader_count - 1; i++)
        ctx->readers[i] = ctx->readers[i + 1];
    memset(&ctx->readers[ctx->reader_count - 1], 0, sizeof(Reader));
    ctx->reader_count--;
    return MAGZ_OK;
}

/* ======================== Date Utilities ======================== */

static void get_current_date(char *buf, int buflen) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buf, buflen, "%Y-%m-%d", tm_info);
}

static int date_compare(const char *d1, const char *d2) {
    int y1, m1, day1, y2, m2, day2;
    sscanf(d1, "%d-%d-%d", &y1, &m1, &day1);
    sscanf(d2, "%d-%d-%d", &y2, &m2, &day2);
    if (y1 != y2) return y1 - y2;
    if (m1 != m2) return m1 - m2;
    return day1 - day2;
}

/* ======================== Input Helpers ======================== */

static void clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

static int read_int_input(const char *prompt, int def) {
    char buf[64];
    printf("%s", prompt);
    fflush(stdout);
    if (fgets(buf, sizeof(buf), stdin) == NULL) return def;
    char *nl = strchr(buf, '\n'); if (nl) *nl = '\0';
    if (strlen(buf) == 0) return def;
    return atoi(buf);
}

static void read_str_input(const char *prompt, char *buf, int buflen, const char *def) {
    printf("%s", prompt);
    fflush(stdout);
    if (fgets(buf, buflen, stdin) == NULL) { strncpy(buf, def, buflen - 1); buf[buflen - 1] = '\0'; return; }
    char *nl = strchr(buf, '\n'); if (nl) *nl = '\0';
    if (strlen(buf) == 0 && def) { strncpy(buf, def, buflen - 1); buf[buflen - 1] = '\0'; }
}

/* ======================== CRC32 for Data Integrity ======================== */

static unsigned int crc32_buf(const unsigned char *data, int len) {
    unsigned int crc = 0xFFFFFFFFu;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
            crc = (crc >> 1) ^ (0xEDB88320u & (-(int)(crc & 1)));
    }
    return ~crc;
}

/* ======================== File Persistence (MAGZV2 + CRC) ======================== */

static MagzStatus save_data(const SysContext *ctx) {
    FILE *fp = fopen(ctx->data_path, "wb");
    if (!fp) return MAGZ_ERR_FILE_OPEN;
    MagzHeader hdr;
    memcpy(hdr.magic, MAGZ_MAGIC, 6);
    hdr.version = MAGZ_VERSION;
    hdr.journal_count = ctx->journal_count;
    hdr.reader_count = ctx->reader_count;
    hdr.data_crc = 0;
    fwrite(&hdr, sizeof(hdr), 1, fp);
    fwrite(ctx->journals, sizeof(Journal), ctx->journal_count, fp);
    fwrite(ctx->readers, sizeof(Reader), ctx->reader_count, fp);
    unsigned char *buf = (unsigned char *)malloc((ctx->journal_count * (int)sizeof(Journal) + ctx->reader_count * (int)sizeof(Reader)));
    if (buf) {
        memcpy(buf, ctx->journals, ctx->journal_count * (int)sizeof(Journal));
        memcpy(buf + ctx->journal_count * (int)sizeof(Journal), ctx->readers, ctx->reader_count * (int)sizeof(Reader));
        hdr.data_crc = crc32_buf(buf, ctx->journal_count * (int)sizeof(Journal) + ctx->reader_count * (int)sizeof(Reader));
        free(buf);
    }
    fseek(fp, 0, SEEK_SET);
    fwrite(&hdr, sizeof(hdr), 1, fp);
    fclose(fp);
    return MAGZ_OK;
}

static MagzStatus load_data(SysContext *ctx) {
    FILE *fp = fopen(ctx->data_path, "rb");
    if (!fp) return MAGZ_ERR_FILE_OPEN;
    MagzHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) { fclose(fp); return MAGZ_ERR_FILE_READ; }
    if (memcmp(hdr.magic, MAGZ_MAGIC, 6) != 0) { fclose(fp); return MAGZ_ERR_INVALID; }
    if (hdr.journal_count > 0) {
        ctx_ensure_journal_capacity(ctx, hdr.journal_count);
        if (fread(ctx->journals, sizeof(Journal), hdr.journal_count, fp) != (size_t)hdr.journal_count) { fclose(fp); return MAGZ_ERR_FILE_READ; }
        ctx->journal_count = hdr.journal_count;
    }
    if (hdr.reader_count > 0) {
        ctx_ensure_reader_capacity(ctx, hdr.reader_count);
        if (fread(ctx->readers, sizeof(Reader), hdr.reader_count, fp) != (size_t)hdr.reader_count) { fclose(fp); return MAGZ_ERR_FILE_READ; }
        ctx->reader_count = hdr.reader_count;
    }
    for (int i = 0; i < ctx->journal_count; i++) queue_init(&ctx->resv_queues[i]);
    fclose(fp);
    return MAGZ_OK;
}

/* ======================== Business Logic: Borrow/Return/Reserve ======================== */

static MagzStatus journal_borrow(SysContext *ctx, int journal_id, int reader_id) {
    int ji = ctx_find_journal_index(ctx, journal_id);
    int ri = ctx_find_reader_index(ctx, reader_id);
    if (ji < 0) return MAGZ_ERR_NOT_FOUND;
    if (ri < 0) return MAGZ_ERR_NOT_FOUND;
    Journal *j = &ctx->journals[ji];
    Reader *r = &ctx->readers[ri];
    if (r->borrow_count >= 5) return MAGZ_ERR_MAX_BORROW;
    if (j->available_copies <= 0) return MAGZ_ERR_ALREADY_BORROWED;
    j->available_copies--;
    j->borrower_id = reader_id;
    j->status = JS_BORROWED;
    get_current_date(j->borrow_date, sizeof(j->borrow_date));
    r->borrowed_ids[r->borrow_count++] = journal_id;
    return MAGZ_OK;
}

static MagzStatus journal_return(SysContext *ctx, int journal_id, int reader_id) {
    int ji = ctx_find_journal_index(ctx, journal_id);
    int ri = ctx_find_reader_index(ctx, reader_id);
    if (ji < 0) return MAGZ_ERR_NOT_FOUND;
    if (ri < 0) return MAGZ_ERR_NOT_FOUND;
    Journal *j = &ctx->journals[ji];
    Reader *r = &ctx->readers[ri];
    int found = 0;
    for (int k = 0; k < r->borrow_count; k++) {
        if (r->borrowed_ids[k] == journal_id) {
            for (int m = k; m < r->borrow_count - 1; m++) r->borrowed_ids[m] = r->borrowed_ids[m + 1];
            r->borrow_count--;
            found = 1;
            break;
        }
    }
    if (!found) return MAGZ_ERR_NOT_BORROWED;
    j->available_copies++;
    if (j->available_copies >= j->total_copies) {
        j->status = JS_AVAILABLE;
        j->borrower_id = -1;
        j->borrow_date[0] = '\0';
    }
    ResvQueue *q = &ctx->resv_queues[ji];
    if (!queue_is_empty(q)) {
        int next_r = queue_dequeue(q);
        if (next_r > 0) {
            int nri = ctx_find_reader_index(ctx, next_r);
            if (nri >= 0) {
                journal_borrow(ctx, journal_id, next_r);
            }
        }
    }
    return MAGZ_OK;
}

static MagzStatus journal_reserve(SysContext *ctx, int journal_id, int reader_id) {
    int ji = ctx_find_journal_index(ctx, journal_id);
    int ri = ctx_find_reader_index(ctx, reader_id);
    if (ji < 0) return MAGZ_ERR_NOT_FOUND;
    if (ri < 0) return MAGZ_ERR_NOT_FOUND;
    ResvQueue *q = &ctx->resv_queues[ji];
    if (queue_contains(q, reader_id)) return MAGZ_ERR_DUPLICATE;
    return queue_enqueue(q, reader_id);
}

/* ======================== UI: Display Functions ======================== */

static void journal_print(const Journal *j, int idx) {
    printf("  [%02d] ID: %-6d  名称: %s\n", idx, j->id, j->name);
    printf("       作者: %s  年份: %d\n", j->author, j->year);
    printf("       总数: %d  可借: %d  状态: ", j->total_copies, j->available_copies);
    switch (j->status) {
        case JS_AVAILABLE: cPrint(C_GREEN, "可借\n"); break;
        case JS_BORROWED: cPrint(C_YELLOW, "已借出 (读者%d)\n", j->borrower_id); break;
        case JS_RESERVED: cPrint(C_RED, "已预约\n"); break;
        default: printf("未知\n");
    }
    if (j->borrow_date[0]) printf("       借出日期: %s\n", j->borrow_date);
}

static void reader_print(const Reader *r) {
    printf("  ┌─ 读者ID: %-6d  姓名: %s\n", r->id, r->name);
    printf("  │  院系: %s\n", r->dept);
    printf("  │  电话: %s\n", r->phone);
    printf("  │  当前借阅数: %d\n", r->borrow_count);
    printf("  └─────────────────────────────────────────────\n");
}

static void print_all_journals(const SysContext *ctx) {
    printTitle("全部期刊列表");
    if (ctx->journal_count == 0) { printWarn("暂无期刊信息"); return; }
    for (int i = 0; i < ctx->journal_count; i++) journal_print(&ctx->journals[i], i + 1);
}

static void print_all_readers(const SysContext *ctx) {
    printTitle("全部读者列表");
    if (ctx->reader_count == 0) { printWarn("暂无读者信息"); return; }
    for (int i = 0; i < ctx->reader_count; i++) reader_print(&ctx->readers[i]);
}

/* ======================== UI: Menu Actions ======================== */

static void show_banner(void) {
    printf("\n");
    cPrint(C_CYAN C_BOLD, "  ╔══════════════════════════════════════════════════════════════╗\n");
    cPrint(C_CYAN C_BOLD, "  ║"); cPrint(C_WHITE C_BOLD, "          期 刊 管 理 系 统  V2.0  (Magazine)               "); cPrint(C_CYAN C_BOLD, "║\n");
    cPrint(C_CYAN C_BOLD, "  ║"); cPrint(C_GRAY, "          Journal Management System - Enhanced              "); cPrint(C_CYAN C_BOLD, "║\n");
    cPrint(C_CYAN C_BOLD, "  ╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

static void do_add_journal(SysContext *ctx) {
    printTitle("添加新期刊");
    Journal j; memset(&j, 0, sizeof(j));
    j.id = read_int_input("  期刊编号: ", 0);
    if (ctx_find_journal_index(ctx, j.id) >= 0) { printErr("该编号已存在"); return; }
    read_str_input("  期刊名称: ", j.name, sizeof(j.name), "");
    read_str_input("  作者/主编: ", j.author, sizeof(j.author), "");
    j.year = read_int_input("  出版年份: ", 2024);
    j.total_copies = read_int_input("  总册数: ", 1);
    j.available_copies = j.total_copies;
    j.status = JS_AVAILABLE;
    j.borrower_id = -1;
    if (ctx_add_journal(ctx, &j) == MAGZ_OK) printOK("期刊添加成功");
    else printErr("添加失败");
}

static void do_add_reader(SysContext *ctx) {
    printTitle("添加新读者");
    Reader r; memset(&r, 0, sizeof(r));
    r.id = read_int_input("  读者编号(学号/工号): ", 0);
    if (ctx_find_reader_index(ctx, r.id) >= 0) { printErr("该编号已存在"); return; }
    read_str_input("  姓名: ", r.name, sizeof(r.name), "");
    read_str_input("  院系: ", r.dept, sizeof(r.dept), "");
    read_str_input("  电话: ", r.phone, sizeof(r.phone), "");
    if (ctx_add_reader(ctx, &r) == MAGZ_OK) printOK("读者添加成功");
    else printErr("添加失败");
}

static void do_borrow(SysContext *ctx) {
    printTitle("借阅期刊");
    int jid = read_int_input("  期刊编号: ", 0);
    int rid = read_int_input("  读者编号: ", 0);
    MagzStatus s = journal_borrow(ctx, jid, rid);
    switch (s) {
        case MAGZ_OK: printOK("借阅成功"); break;
        case MAGZ_ERR_NOT_FOUND: printErr("期刊或读者不存在"); break;
        case MAGZ_ERR_MAX_BORROW: printErr("读者借阅已达上限(5本)"); break;
        case MAGZ_ERR_ALREADY_BORROWED: printErr("期刊已被借完"); break;
        default: printErr("借阅失败"); break;
    }
}

static void do_return(SysContext *ctx) {
    printTitle("归还期刊");
    int jid = read_int_input("  期刊编号: ", 0);
    int rid = read_int_input("  读者编号: ", 0);
    MagzStatus s = journal_return(ctx, jid, rid);
    switch (s) {
        case MAGZ_OK: printOK("归还成功"); break;
        case MAGZ_ERR_NOT_FOUND: printErr("期刊或读者不存在"); break;
        case MAGZ_ERR_NOT_BORROWED: printErr("该读者未借阅此期刊"); break;
        default: printErr("归还失败"); break;
    }
}

static void do_reserve(SysContext *ctx) {
    printTitle("预约期刊");
    int jid = read_int_input("  期刊编号: ", 0);
    int rid = read_int_input("  读者编号: ", 0);
    MagzStatus s = journal_reserve(ctx, jid, rid);
    switch (s) {
        case MAGZ_OK: printOK("预约成功"); break;
        case MAGZ_ERR_NOT_FOUND: printErr("期刊或读者不存在"); break;
        case MAGZ_ERR_DUPLICATE: printErr("您已预约过此期刊"); break;
        default: printErr("预约失败"); break;
    }
}

static void do_search_journal(SysContext *ctx) {
    printTitle("查询期刊");
    char kw[MAGZ_MAX_NAME];
    read_str_input("  输入关键词(名称/作者): ", kw, sizeof(kw), "");
    printf("\n");
    int found = 0;
    for (int i = 0; i < ctx->journal_count; i++) {
        if (strstr(ctx->journals[i].name, kw) || strstr(ctx->journals[i].author, kw)) {
            journal_print(&ctx->journals[i], i + 1);
            found++;
        }
    }
    if (!found) printWarn("未找到匹配期刊");
}

static void do_delete_journal(SysContext *ctx) {
    printTitle("删除期刊");
    int id = read_int_input("  期刊编号: ", 0);
    int ji = ctx_find_journal_index(ctx, id);
    if (ji < 0) { printErr("期刊不存在"); return; }
    if (ctx->journals[ji].status == JS_BORROWED) { printErr("期刊已被借出，无法删除"); return; }
    if (ctx_remove_journal(ctx, id) == MAGZ_OK) printOK("删除成功");
    else printErr("删除失败");
}

static void do_delete_reader(SysContext *ctx) {
    printTitle("删除读者");
    int id = read_int_input("  读者编号: ", 0);
    int ri = ctx_find_reader_index(ctx, id);
    if (ri < 0) { printErr("读者不存在"); return; }
    if (ctx->readers[ri].borrow_count > 0) { printErr("该读者尚有未还期刊，无法删除"); return; }
    if (ctx_remove_reader(ctx, id) == MAGZ_OK) printOK("删除成功");
    else printErr("删除失败");
}

static void do_reminder(SysContext *ctx) {
    printTitle("到期/超期提醒");
    char today[16];
    get_current_date(today, sizeof(today));
    int cnt = 0;
    for (int i = 0; i < ctx->journal_count; i++) {
        Journal *j = &ctx->journals[i];
        if (j->status == JS_BORROWED && j->borrow_date[0]) {
            int diff = date_compare(today, j->borrow_date);
            if (diff >= 30) {
                cPrint(C_RED, "  [超期] ");
                printf("期刊「%s」(ID:%d) 借阅人ID:%d 借阅日:%s\n", j->name, j->id, j->borrower_id, j->borrow_date);
                cnt++;
            } else if (diff >= 25) {
                cPrint(C_YELLOW, "  [即将到期] ");
                printf("期刊「%s」(ID:%d) 借阅人ID:%d 借阅日:%s\n", j->name, j->id, j->borrower_id, j->borrow_date);
                cnt++;
            }
        }
    }
    if (cnt == 0) printOK("无超期/即将到期期刊");
}

static void show_menu(const SysContext *ctx) {
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  主菜单\n");
    printDivider();
    cPrint(C_CYAN, "  1 "); printf("- 添加期刊\n");
    cPrint(C_CYAN, "  2 "); printf("- 添加读者\n");
    cPrint(C_CYAN, "  3 "); printf("- 借阅期刊\n");
    cPrint(C_CYAN, "  4 "); printf("- 归还期刊\n");
    cPrint(C_CYAN, "  5 "); printf("- 预约期刊\n");
    cPrint(C_CYAN, "  6 "); printf("- 查询期刊\n");
    cPrint(C_CYAN, "  7 "); printf("- 查看所有期刊\n");
    cPrint(C_CYAN, "  8 "); printf("- 查看所有读者\n");
    cPrint(C_CYAN, "  9 "); printf("- 删除期刊\n");
    cPrint(C_CYAN, " 10 "); printf("- 删除读者\n");
    cPrint(C_CYAN, " 11 "); printf("- 到期/超期提醒\n");
    cPrint(C_CYAN, "  0 "); printf("- 保存并退出\n");
    printDivider();
    printf("  当前状态: 期刊 ");
    cPrint(C_GREEN, "%d", ctx->journal_count);
    cPrint(C_CYAN, "  读者: ");
    cPrint(C_GREEN, "%d\n", ctx->reader_count);
    printDivider();
}

static void pause_screen(void) {
    if (testMode) return;
    printf("\n");
    cPrint(C_GRAY, "  按 Enter 键继续...");
    fflush(stdout);
    clear_input_buffer();
}

/* ======================== Built-in Self-Test Suite ======================== */

static int g_test_passed = 0;
static int g_test_failed = 0;

static void test_assert(int condition, const char *test_name) {
    if (condition) { g_test_passed++; if (!testMode) { cPrint(C_GREEN, "    [PASS] "); printf("%s\n", test_name); } }
    else { g_test_failed++; cPrint(C_RED, "    [FAIL] "); printf("%s\n", test_name); }
}

static void run_self_tests(void) {
    testMode = 1;
    g_test_passed = 0;
    g_test_failed = 0;

    printTitle("期刊管理系统 V2.0 - 内置自测用例");

    /* ---- Group 1: System Init & Dynamic Memory ---- */
    cPrint(C_CYAN C_BOLD, "\n  [测试组1] 系统初始化与动态内存\n");
    {
        SysContext ctx;
        MagzStatus s = ctx_init(&ctx);
        test_assert(s == MAGZ_OK, "ctx_init 初始化成功");
        test_assert(ctx.journals != NULL && ctx.readers != NULL, "初始数组分配成功");
        test_assert(ctx.journal_capacity >= INITIAL_CAPACITY, "初始容量>=16");

        Journal j; memset(&j, 0, sizeof(j));
        j.id = 1; strcpy(j.name, "测试期刊"); strcpy(j.author, "测试作者");
        j.year = 2024; j.total_copies = 5; j.available_copies = 5; j.status = JS_AVAILABLE; j.borrower_id = -1;
        ctx_add_journal(&ctx, &j);

        for (int i = 2; i <= 10; i++) {
            Journal jj; memset(&jj, 0, sizeof(jj));
            jj.id = i; sprintf(jj.name, "期刊%d", i); strcpy(jj.author, "作者");
            jj.year = 2024; jj.total_copies = 1; jj.available_copies = 1; jj.status = JS_AVAILABLE; jj.borrower_id = -1;
            ctx_add_journal(&ctx, &jj);
        }
        test_assert(ctx.journal_count == 10, "添加10个期刊成功");
        test_assert(ctx.journal_capacity >= 10, "动态扩容触发成功");

        ctx_destroy(&ctx);
        test_assert(ctx.journals == NULL && ctx.readers == NULL, "ctx_destroy 释放内存成功");
    }

    /* ---- Group 2: Journal CRUD ---- */
    cPrint(C_CYAN C_BOLD, "\n  [测试组2] 期刊CRUD操作\n");
    {
        SysContext ctx; ctx_init(&ctx);
        Journal j; memset(&j, 0, sizeof(j));
        j.id = 101; strcpy(j.name, "C程序设计"); strcpy(j.author, "谭浩强");
        j.year = 2020; j.total_copies = 3; j.available_copies = 3; j.status = JS_AVAILABLE; j.borrower_id = -1;
        test_assert(ctx_add_journal(&ctx, &j) == MAGZ_OK, "添加期刊成功");
        test_assert(ctx_find_journal_index(&ctx, 101) >= 0, "查找期刊成功");

        Journal j2; memset(&j2, 0, sizeof(j2));
        j2.id = 101; strcpy(j2.name, "Duplicate");
        test_assert(ctx_add_journal(&ctx, &j2) == MAGZ_ERR_DUPLICATE, "重复ID拒绝成功");

        test_assert(ctx_remove_journal(&ctx, 101) == MAGZ_OK, "删除期刊成功");
        test_assert(ctx_find_journal_index(&ctx, 101) < 0, "删除后查找失败(正确)");
        ctx_destroy(&ctx);
    }

    /* ---- Group 3: Reader CRUD + Borrow/Return/Reserve ---- */
    cPrint(C_CYAN C_BOLD, "\n  [测试组3] 读者CRUD与借阅/归还/预约\n");
    {
        SysContext ctx; ctx_init(&ctx);
        Journal j; memset(&j, 0, sizeof(j));
        j.id = 201; strcpy(j.name, "数据结构"); strcpy(j.author, "严蔚敏");
        j.year = 2021; j.total_copies = 1; j.available_copies = 1; j.status = JS_AVAILABLE; j.borrower_id = -1;
        ctx_add_journal(&ctx, &j);

        Reader r; memset(&r, 0, sizeof(r));
        r.id = 4001; strcpy(r.name, "张三"); strcpy(r.dept, "计算机学院"); strcpy(r.phone, "13800000000");
        test_assert(ctx_add_reader(&ctx, &r) == MAGZ_OK, "添加读者成功");

        test_assert(journal_borrow(&ctx, 201, 4001) == MAGZ_OK, "借阅成功");
        test_assert(ctx.journals[ctx_find_journal_index(&ctx, 201)].available_copies == 0, "可借数减为0");
        test_assert(ctx.readers[ctx_find_reader_index(&ctx, 4001)].borrow_count == 1, "读者借阅数=1");

        test_assert(journal_borrow(&ctx, 201, 4001) == MAGZ_ERR_ALREADY_BORROWED, "重复借阅正确拒绝");

        Reader r2; memset(&r2, 0, sizeof(r2));
        r2.id = 4002; strcpy(r2.name, "李四"); strcpy(r2.dept, "软件学院"); strcpy(r2.phone, "13900000000");
        ctx_add_reader(&ctx, &r2);
        test_assert(journal_reserve(&ctx, 201, 4002) == MAGZ_OK, "预约成功");
        test_assert(queue_peek(&ctx.resv_queues[ctx_find_journal_index(&ctx, 201)]) == 4002, "预约队列队首正确");
        test_assert(journal_reserve(&ctx, 201, 4002) == MAGZ_ERR_DUPLICATE, "重复预约正确拒绝");

        test_assert(journal_return(&ctx, 201, 4001) == MAGZ_OK, "归还成功");
        test_assert(ctx.readers[ctx_find_reader_index(&ctx, 4001)].borrow_count == 0, "归还后借阅数=0");

        ctx_destroy(&ctx);
    }

    /* ---- Group 4: File I/O with MAGZV2 Magic ---- */
    cPrint(C_CYAN C_BOLD, "\n  [测试组4] 文件持久化(MAGZV2魔数+CRC)\n");
    {
        const char *test_file = "_test_magz_v2.bin";
        SysContext ctx1; ctx_init(&ctx1);
        strncpy(ctx1.data_path, test_file, sizeof(ctx1.data_path) - 1);

        Journal j; memset(&j, 0, sizeof(j));
        j.id = 301; strcpy(j.name, "测试期刊A"); strcpy(j.author, "作者A");
        j.year = 2024; j.total_copies = 2; j.available_copies = 2; j.status = JS_AVAILABLE; j.borrower_id = -1;
        ctx_add_journal(&ctx1, &j);
        j.id = 302; strcpy(j.name, "测试期刊B"); strcpy(j.author, "作者B");
        j.year = 2023; j.total_copies = 1; j.available_copies = 1; j.status = JS_AVAILABLE; j.borrower_id = -1;
        ctx_add_journal(&ctx1, &j);

        Reader r; memset(&r, 0, sizeof(r));
        r.id = 5001; strcpy(r.name, "王五"); strcpy(r.dept, "信工院"); strcpy(r.phone, "13700000000");
        ctx_add_reader(&ctx1, &r);

        test_assert(save_data(&ctx1) == MAGZ_OK, "保存数据成功(MAGZV2)");
        ctx_destroy(&ctx1);

        SysContext ctx2; ctx_init(&ctx2);
        strncpy(ctx2.data_path, test_file, sizeof(ctx2.data_path) - 1);
        test_assert(load_data(&ctx2) == MAGZ_OK, "加载数据成功");
        test_assert(ctx2.journal_count == 2, "加载后期刊数=2");
        int ji = ctx_find_journal_index(&ctx2, 301);
        test_assert(ji >= 0, "期刊301存在");
        int ri = ctx_find_reader_index(&ctx2, 5001);
        test_assert(ri >= 0, "读者5001存在");

        FILE *fp = fopen(test_file, "rb");
        MagzHeader hdr;
        fread(&hdr, sizeof(hdr), 1, fp);
        fclose(fp);
        test_assert(memcmp(hdr.magic, MAGZ_MAGIC, 6) == 0, "文件魔数为MAGZV2");
        test_assert(hdr.version == MAGZ_VERSION, "版本号=0x0200");
        test_assert(hdr.journal_count == 2 && hdr.reader_count == 1, "头计数正确");

        remove(test_file);
        ctx_destroy(&ctx2);
    }

    /* ---- Group 5: Date & Utilities ---- */
    cPrint(C_CYAN C_BOLD, "\n  [测试组5] 日期工具与辅助函数\n");
    {
        char d1[16];
        get_current_date(d1, sizeof(d1));
        test_assert(strlen(d1) == 10 && d1[4] == '-' && d1[7] == '-', "当前日期格式正确 YYYY-MM-DD");

        test_assert(date_compare("2025-01-01", "2025-01-01") == 0, "日期相等");
        test_assert(date_compare("2025-06-01", "2025-06-15") < 0, "日期比较: 早 < 晚");
        test_assert(date_compare("2025-06-15", "2025-06-01") > 0, "日期比较: 晚 > 早");
        test_assert(date_compare("2024-12-31", "2025-01-01") < 0, "跨年比较");
    }

    /* ---- Group 6: Error Handling ---- */
    cPrint(C_CYAN C_BOLD, "\n  [测试组6] 错误处理边界\n");
    {
        SysContext ctx; ctx_init(&ctx);
        test_assert(journal_borrow(&ctx, 9999, 8888) == MAGZ_ERR_NOT_FOUND, "不存在期刊/读者返回NOT_FOUND");
        test_assert(journal_return(&ctx, 9999, 8888) == MAGZ_ERR_NOT_FOUND, "归还不存在返回NOT_FOUND");
        test_assert(ctx_remove_journal(&ctx, 9999) == MAGZ_ERR_NOT_FOUND, "删除不存在期刊返回NOT_FOUND");
        ctx_destroy(&ctx);
    }

    printDivider();
    printf("\n");
    cPrint(C_BOLD, "  测试结果: ");
    cPrint(C_GREEN C_BOLD, "%d 通过", g_test_passed);
    printf("  ");
    if (g_test_failed > 0) cPrint(C_RED C_BOLD, "%d 失败\n", g_test_failed);
    else cPrint(C_GREEN C_BOLD, "0 失败\n");
    printf("\n");

    if (g_test_failed > 0) {
        cPrint(C_RED C_BOLD, "  !!! 自测未通过 !!!\n\n");
        testMode = 0;
        return;
    }
    cPrint(C_GREEN C_BOLD, "  ✓ 全部自测通过!\n\n");
    testMode = 0;
}

/* ======================== CLI Handler ======================== */

static void print_usage(void) {
    printf("期刊管理系统 V2.0 - Journal Management System Enhanced\n\n");
    printf("用法:\n");
    printf("  journal.exe                 启动交互模式\n");
    printf("  journal.exe -h | --help     显示帮助信息\n");
    printf("  journal.exe -t | --test     运行内置自测用例\n");
    printf("  journal.exe -f <file>       指定数据文件路径\n");
    printf("  journal.exe <file>          同上(等价于 -f <file>)\n");
    printf("\n");
    printf("数据文件格式: 二进制, 魔数 \"%s\", 版本 0x%04X\n", MAGZ_MAGIC, MAGZ_VERSION);
    printf("默认数据文件: magz_data.bin\n");
}

static int run_command_line(int argc, char *argv[], SysContext *ctx) {
    if (argc < 2) return 0;
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_usage();
        return 0;
    }
    if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0) {
        const char *tf = "_test_run.bin";
        strncpy(ctx->data_path, tf, sizeof(ctx->data_path) - 1);
        run_self_tests();
        remove(tf);
        return g_test_failed == 0 ? 0 : 1;
    }
    if (strcmp(argv[1], "-f") == 0) {
        if (argc < 3) {
            printErr("-f 需要指定文件路径");
            print_usage();
            return 1;
        }
        strncpy(ctx->data_path, argv[2], sizeof(ctx->data_path) - 1);
        return 0;
    }
    if (argv[1][0] != '-') {
        strncpy(ctx->data_path, argv[1], sizeof(ctx->data_path) - 1);
        return 0;
    }
    printErr("未知参数");
    print_usage();
    return 1;
}

/* ======================== Interactive Main Loop ======================== */

static void run_interactive(SysContext *ctx) {
    show_banner();
    load_data(ctx);

    char choice[64];
    while (1) {
        show_menu(ctx);
        printf("  请选择操作 [0-11]: ");
        fflush(stdout);
        if (fgets(choice, sizeof(choice), stdin) == NULL) break;
        char *nl = strchr(choice, '\n'); if (nl) *nl = '\0';
        int opt = atoi(choice);

        switch (opt) {
            case 1: do_add_journal(ctx); pause_screen(); break;
            case 2: do_add_reader(ctx); pause_screen(); break;
            case 3: do_borrow(ctx); pause_screen(); break;
            case 4: do_return(ctx); pause_screen(); break;
            case 5: do_reserve(ctx); pause_screen(); break;
            case 6: do_search_journal(ctx); pause_screen(); break;
            case 7: print_all_journals(ctx); pause_screen(); break;
            case 8: print_all_readers(ctx); pause_screen(); break;
            case 9: do_delete_journal(ctx); pause_screen(); break;
            case 10: do_delete_reader(ctx); pause_screen(); break;
            case 11: do_reminder(ctx); pause_screen(); break;
            case 0:
                save_data(ctx);
                printOK("数据已保存，再见！");
                printf("\n");
                return;
            default:
                printErr("无效选项，请重新选择");
                pause_screen();
                break;
        }
    }
}

/* ======================== Program Entry ======================== */

int main(int argc, char *argv[]) {
    enableVTMode();

    SysContext ctx;
    MagzStatus s = ctx_init(&ctx);
    if (s != MAGZ_OK) {
        printErr("系统初始化失败（内存不足）");
        return 1;
    }

    int cli_ret = run_command_line(argc, argv, &ctx);
    if (argc >= 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0 ||
        strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0)) {
        ctx_destroy(&ctx);
        return cli_ret;
    }
    if (cli_ret != 0 && argc >= 2) {
        ctx_destroy(&ctx);
        return cli_ret;
    }

    run_interactive(&ctx);
    ctx_destroy(&ctx);
    return 0;
}
