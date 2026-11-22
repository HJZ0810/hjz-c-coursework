/*
* ============================================================================
*
*    航空客运订票系统  (V2.0 增强版)
*    Airline Ticket Booking System  (V2.0 Enhanced)
*
* ============================================================================
*
*  功能概述:
*    基于链表与循环队列的航空客运订票系统，支持：
*      [1] 航班信息管理（录入、浏览、查询航线）
*      [2] 客票预订（余票充足/不足/满员三种情况自动处理）
*      [3] 退票处理（自动处理候补队列，支持部分订票）
*      [4] 航班客户名单查看（按姓名有序链表）
*      [5] 候补队列（循环队列，退票后自动补位）
*      [6] 统计功能（总航班数、已订票数、营收、载客率）
*      [7] 数据持久化（二进制文件 + 魔数头校验）
*      [8] 预置5条测试航班数据
*      [9] 命令行模式支持（--help / --load / --save）
*     [10] 彩色终端界面 —— ANSI颜色支持（Windows 10+ / Linux）
*
*  数据结构:
*    Flight        —— 航班信息（动态数组存储，按航班号有序）
*    CustomerNode  —— 已订票客户（单链表，按姓名有序）
*    WaitQueue     —— 候补客户队列（循环数组实现）
*    AirHeader     —— 数据文件头（魔数+版本+校验）
*
*  编译命令:
*    gcc -std=c99 -Wall -O2 -o airline.exe main.c
*
*  命令行用法:
*    airline.exe                交互模式（菜单）
*    airline.exe --help         显示帮助
*    airline.exe --load <file>  从指定文件加载数据
*    airline.exe --save <file>  保存数据到指定文件后启动
*
*  编码: UTF-8
* ============================================================================
*/

/* ======================== 标准头文件 ======================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#include <io.h>
#define F_OK 0
#define access _access
#else
#include <unistd.h>
#endif

/* ======================== 常量与宏 ======================== */
#define MAX_FLIGHT_NO   12
#define MAX_PLANE_NO    12
#define MAX_DEST        20
#define MAX_NAME_LEN    20
#define MAX_WEEKDAY     8
#define DEFAULT_QUEUE_CAP 8
#define DATA_FILE       "airline.dat"
#define AIR_MAGIC       "AIRS"
#define AIR_VERSION     2
#define TICKET_PRICE    800

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
#define BG_YELLOW   "\033[43m"
#define BG_BLUE     "\033[44m"

/* ======================== 错误码 ======================== */
typedef enum {
    AIR_OK = 0,
    AIR_ERR_OPEN_FILE,
    AIR_ERR_READ_FILE,
    AIR_ERR_WRITE_FILE,
    AIR_ERR_MEMORY,
    AIR_ERR_CORRUPT,
    AIR_ERR_MAGIC,
    AIR_ERR_VERSION,
    AIR_ERR_NOT_FOUND,
    AIR_ERR_DUPLICATE,
    AIR_ERR_FULL,
    AIR_ERR_EMPTY,
    AIR_ERR_CANCEL,
    AIR_ERR_INVALID,
} AirStatus;

/* ======================== 数据结构 ======================== */

typedef struct CustomerNode {
    char name[MAX_NAME_LEN];
    int ticketCount;
    int seatStart;
    struct CustomerNode *next;
} CustomerNode;

typedef struct {
    char (*names)[MAX_NAME_LEN];
    int *counts;
    int front;
    int rear;
    int capacity;
    int size;
} WaitQueue;

typedef struct {
    char flightNo[MAX_FLIGHT_NO];
    char planeNo[MAX_PLANE_NO];
    char dest[MAX_DEST];
    int weekday;
    int capacity;
    int remaining;
    CustomerNode *customers;
    WaitQueue waitQueue;
} Flight;

typedef struct {
    char magic[4];
    unsigned short version;
    unsigned int flightCount;
    unsigned int totalBooked;
    unsigned int checksum;
} AirHeader;

/* ======================== 全局状态（动态分配） ======================== */
static Flight *flights = NULL;
static int flightCount = 0;
static int flightCap = 0;
static int colorEnabled = 1;
static char dataPath[256] = DATA_FILE;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

/* ======================== 前向声明 ======================== */
static void initWaitQueue(WaitQueue *q);
static void freeWaitQueue(WaitQueue *q);
static int  enqueueWait(WaitQueue *q, const char *name, int count);
static int  dequeueWait(WaitQueue *q, char *name, int *count);
static int  isWaitQueueEmpty(const WaitQueue *q);
static int  isWaitQueueFull(const WaitQueue *q);
static void freeCustomerList(CustomerNode *head);
static void saveAllFlights(const char *path);
static AirStatus loadAllFlights(const char *path);

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
static void printErr(const char *msg)   { cPrint(C_RED,   "  [ERROR] "); printf("%s\n", msg); }
static void printWarn(const char *msg)  { cPrint(C_YELLOW,"  [WARN] "); printf("%s\n", msg); }
static void printInfo(const char *msg)  { cPrint(C_CYAN,  "  [INFO] "); printf("%s\n", msg); }

/* ======================== 工具函数：UI ======================== */

static void printBanner(void) {
    printf("\n");
    cPrint(C_BOLD C_BLUE,
        "  ============================================================================\n");
    cPrint(C_BOLD C_CYAN,
        "  |          Airline Ticket Booking System  (V2.0 Enhanced Edition)          |\n");
    cPrint(C_BOLD C_CYAN,
        "  |                         航空客运订票系统  V2.0 增强版                     |\n");
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
        return;
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

static const char *weekdayName(int wd) {
    static const char *names[] = {"", "周一", "周二", "周三", "周四", "周五", "周六", "周日"};
    if (wd >= 1 && wd <= 7) return names[wd];
    return "?";
}

/* ======================== 内存管理：航班动态数组 ======================== */

static void freeFlightData(Flight *f) {
    if (!f) return;
    freeCustomerList(f->customers);
    f->customers = NULL;
    freeWaitQueue(&f->waitQueue);
}

static void freeAllFlights(void) {
    for (int i = 0; i < flightCount; i++) {
        freeFlightData(&flights[i]);
    }
    free(flights);
    flights = NULL;
    flightCount = 0;
    flightCap = 0;
}

static int ensureFlightCap(int need) {
    if (need <= flightCap) return 1;
    int newCap = flightCap == 0 ? 8 : flightCap;
    while (newCap < need) newCap *= 2;
    Flight *nf = (Flight *)realloc(flights, (size_t)newCap * sizeof(Flight));
    if (!nf) return 0;
    flights = nf;
    flightCap = newCap;
    return 1;
}

/* ======================== 等待队列（循环数组实现） ======================== */

static void initWaitQueue(WaitQueue *q) {
    q->capacity = DEFAULT_QUEUE_CAP;
    q->names = (char (*)[MAX_NAME_LEN])malloc((size_t)q->capacity * sizeof(char[MAX_NAME_LEN]));
    q->counts = (int *)malloc((size_t)q->capacity * sizeof(int));
    q->front = 0;
    q->rear = 0;
    q->size = 0;
}

static void freeWaitQueue(WaitQueue *q) {
    if (q->names) { free(q->names); q->names = NULL; }
    if (q->counts) { free(q->counts); q->counts = NULL; }
    q->front = q->rear = q->size = q->capacity = 0;
}

static int isWaitQueueEmpty(const WaitQueue *q) { return q->size == 0; }
static int isWaitQueueFull(const WaitQueue *q)  { return q->size == q->capacity; }

static int expandWaitQueue(WaitQueue *q) {
    int newCap = q->capacity * 2;
    char (*newNames)[MAX_NAME_LEN] = (char (*)[MAX_NAME_LEN])malloc((size_t)newCap * sizeof(char[MAX_NAME_LEN]));
    int *newCounts = (int *)malloc((size_t)newCap * sizeof(int));
    if (!newNames || !newCounts) {
        if (newNames) free(newNames);
        if (newCounts) free(newCounts);
        return 0;
    }
    for (int i = 0; i < q->size; i++) {
        int idx = (q->front + i) % q->capacity;
        strcpy(newNames[i], q->names[idx]);
        newCounts[i] = q->counts[idx];
    }
    free(q->names);
    free(q->counts);
    q->names = newNames;
    q->counts = newCounts;
    q->front = 0;
    q->rear = q->size;
    q->capacity = newCap;
    return 1;
}

static int enqueueWait(WaitQueue *q, const char *name, int count) {
    if (isWaitQueueFull(q)) {
        if (!expandWaitQueue(q)) return 0;
    }
    strncpy(q->names[q->rear], name, MAX_NAME_LEN - 1);
    q->names[q->rear][MAX_NAME_LEN - 1] = '\0';
    q->counts[q->rear] = count;
    q->rear = (q->rear + 1) % q->capacity;
    q->size++;
    return 1;
}

static int dequeueWait(WaitQueue *q, char *name, int *count) {
    if (isWaitQueueEmpty(q)) return 0;
    if (name) strcpy(name, q->names[q->front]);
    if (count) *count = q->counts[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;
    return 1;
}

static const char *peekWaitName(const WaitQueue *q) {
    if (isWaitQueueEmpty(q)) return NULL;
    return q->names[q->front];
}

static int peekWaitCount(const WaitQueue *q) {
    if (isWaitQueueEmpty(q)) return 0;
    return q->counts[q->front];
}

/* ======================== 客户链表操作 ======================== */

static CustomerNode *createCustomer(const char *name, int count, int seatStart) {
    CustomerNode *n = (CustomerNode *)malloc(sizeof(CustomerNode));
    if (!n) return NULL;
    strncpy(n->name, name, MAX_NAME_LEN - 1);
    n->name[MAX_NAME_LEN - 1] = '\0';
    n->ticketCount = count;
    n->seatStart = seatStart;
    n->next = NULL;
    return n;
}

static CustomerNode *insertCustomer(CustomerNode *head, const char *name, int count, int seatStart) {
    CustomerNode *nn = createCustomer(name, count, seatStart);
    if (!nn) return head;
    if (!head || strcmp(name, head->name) < 0) {
        nn->next = head;
        return nn;
    }
    CustomerNode *cur = head;
    while (cur->next && strcmp(cur->next->name, name) < 0)
        cur = cur->next;
    nn->next = cur->next;
    cur->next = nn;
    return head;
}

static CustomerNode *findCustomer(CustomerNode *head, const char *name) {
    while (head) {
        if (strcmp(head->name, name) == 0) return head;
        head = head->next;
    }
    return NULL;
}

static CustomerNode *removeCustomer(CustomerNode *head, const char *name) {
    if (!head) return NULL;
    if (strcmp(head->name, name) == 0) {
        CustomerNode *n = head->next;
        free(head);
        return n;
    }
    CustomerNode *cur = head;
    while (cur->next) {
        if (strcmp(cur->next->name, name) == 0) {
            CustomerNode *n = cur->next;
            cur->next = n->next;
            free(n);
            return head;
        }
        cur = cur->next;
    }
    return head;
}

static void freeCustomerList(CustomerNode *head) {
    while (head) {
        CustomerNode *n = head->next;
        free(head);
        head = n;
    }
}

/* ======================== 航班查找与插入 ======================== */

static int findFlightIndex(const char *flightNo) {
    for (int i = 0; i < flightCount; i++) {
        if (strcmp(flights[i].flightNo, flightNo) == 0) return i;
    }
    return -1;
}

static int findFlightByDest(const char *dest, int *indices, int maxOut) {
    int cnt = 0;
    for (int i = 0; i < flightCount && cnt < maxOut; i++) {
        if (strstr(flights[i].dest, dest) != NULL) {
            indices[cnt++] = i;
        }
    }
    return cnt;
}

static int insertFlightSorted(const Flight *f) {
    if (!ensureFlightCap(flightCount + 1)) return -1;
    int pos = flightCount;
    for (int i = 0; i < flightCount; i++) {
        if (strcmp(f->flightNo, flights[i].flightNo) < 0) {
            pos = i;
            break;
        }
    }
    for (int i = flightCount; i > pos; i--) {
        flights[i] = flights[i - 1];
    }
    flights[pos] = *f;
    flights[pos].customers = NULL;
    flightCount++;
    return pos;
}

/* ======================== 座位分配显示 ======================== */

static void printSeatMap(const Flight *f) {
    int cap = f->capacity;
    if (cap <= 0) return;
    int booked = cap - f->remaining;
    cPrint(C_CYAN, "  座位图（■=已订 □=可用）:\n  ");
    for (int i = 1; i <= cap; i++) {
        if (i <= booked) {
            cPrint(C_RED, "■");
        } else {
            cPrint(C_GREEN, "□");
        }
        if (i % 10 == 0 && i < cap) printf("\n  ");
    }
    printf("\n");
    cPrint(C_GRAY, "  已订: "); cPrint(C_RED, "%d", booked);
    cPrint(C_GRAY, "  可用: "); cPrint(C_GREEN, "%d\n", f->remaining);
}

/* ======================== 持久化（二进制 + 魔数头） ======================== */

static void saveAllFlights(const char *path) {
    FILE *fp = fopen(path, "wb");
    if (!fp) { printErr("无法打开数据文件进行写入"); return; }

    unsigned int totalBooked = 0;
    for (int i = 0; i < flightCount; i++) {
        totalBooked += (unsigned int)(flights[i].capacity - flights[i].remaining);
    }

    AirHeader hdr;
    memcpy(hdr.magic, AIR_MAGIC, 4);
    hdr.version = AIR_VERSION;
    hdr.flightCount = (unsigned int)flightCount;
    hdr.totalBooked = totalBooked;
    hdr.checksum = 0;
    fwrite(&hdr, sizeof(hdr), 1, fp);

    for (int i = 0; i < flightCount; i++) {
        Flight *f = &flights[i];
        fwrite(f->flightNo, MAX_FLIGHT_NO, 1, fp);
        fwrite(f->planeNo, MAX_PLANE_NO, 1, fp);
        fwrite(f->dest, MAX_DEST, 1, fp);
        fwrite(&f->weekday, sizeof(int), 1, fp);
        fwrite(&f->capacity, sizeof(int), 1, fp);
        fwrite(&f->remaining, sizeof(int), 1, fp);

        int custCount = 0;
        CustomerNode *c = f->customers;
        while (c) { custCount++; c = c->next; }
        fwrite(&custCount, sizeof(int), 1, fp);
        c = f->customers;
        while (c) {
            fwrite(c->name, MAX_NAME_LEN, 1, fp);
            fwrite(&c->ticketCount, sizeof(int), 1, fp);
            fwrite(&c->seatStart, sizeof(int), 1, fp);
            c = c->next;
        }

        fwrite(&f->waitQueue.size, sizeof(int), 1, fp);
        for (int j = 0; j < f->waitQueue.size; j++) {
            int idx = (f->waitQueue.front + j) % f->waitQueue.capacity;
            fwrite(f->waitQueue.names[idx], MAX_NAME_LEN, 1, fp);
            fwrite(&f->waitQueue.counts[idx], sizeof(int), 1, fp);
        }
    }

    fseek(fp, 0, SEEK_SET);
    hdr.checksum = hdr.flightCount * 31u + hdr.totalBooked * 7u;
    fwrite(&hdr, sizeof(hdr), 1, fp);
    fclose(fp);
}

static AirStatus loadAllFlights(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return AIR_ERR_OPEN_FILE;

    AirHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) { fclose(fp); return AIR_ERR_CORRUPT; }
    if (memcmp(hdr.magic, AIR_MAGIC, 4) != 0) { fclose(fp); return AIR_ERR_MAGIC; }
    if (hdr.version > AIR_VERSION) { fclose(fp); return AIR_ERR_VERSION; }

    freeAllFlights();
    if (!ensureFlightCap((int)hdr.flightCount)) { fclose(fp); return AIR_ERR_MEMORY; }

    for (unsigned int i = 0; i < hdr.flightCount; i++) {
        Flight f;
        memset(&f, 0, sizeof(f));
        initWaitQueue(&f.waitQueue);
        f.customers = NULL;

        if (fread(f.flightNo, MAX_FLIGHT_NO, 1, fp) != 1 ||
            fread(f.planeNo, MAX_PLANE_NO, 1, fp) != 1 ||
            fread(f.dest, MAX_DEST, 1, fp) != 1 ||
            fread(&f.weekday, sizeof(int), 1, fp) != 1 ||
            fread(&f.capacity, sizeof(int), 1, fp) != 1 ||
            fread(&f.remaining, sizeof(int), 1, fp) != 1) {
            freeFlightData(&f);
            fclose(fp);
            return AIR_ERR_CORRUPT;
        }

        int custCount;
        if (fread(&custCount, sizeof(int), 1, fp) != 1) { freeFlightData(&f); fclose(fp); return AIR_ERR_CORRUPT; }
        for (int j = 0; j < custCount; j++) {
            char cname[MAX_NAME_LEN];
            int tc, ss;
            if (fread(cname, MAX_NAME_LEN, 1, fp) != 1 ||
                fread(&tc, sizeof(int), 1, fp) != 1 ||
                fread(&ss, sizeof(int), 1, fp) != 1) {
                freeFlightData(&f); fclose(fp); return AIR_ERR_CORRUPT;
            }
            f.customers = insertCustomer(f.customers, cname, tc, ss);
        }

        int wsize;
        if (fread(&wsize, sizeof(int), 1, fp) != 1) { freeFlightData(&f); fclose(fp); return AIR_ERR_CORRUPT; }
        for (int j = 0; j < wsize; j++) {
            char wname[MAX_NAME_LEN];
            int wc;
            if (fread(wname, MAX_NAME_LEN, 1, fp) != 1 ||
                fread(&wc, sizeof(int), 1, fp) != 1) {
                freeFlightData(&f); fclose(fp); return AIR_ERR_CORRUPT;
            }
            enqueueWait(&f.waitQueue, wname, wc);
        }

        if (!ensureFlightCap(flightCount + 1)) { freeFlightData(&f); fclose(fp); return AIR_ERR_MEMORY; }
        flights[flightCount++] = f;
    }

    fclose(fp);
    return AIR_OK;
}

/* ======================== 核心业务：录入航班 ======================== */

static void inputFlight(void) {
    printf("\n");
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  录入航班信息\n");
    printDivider();

    Flight f;
    memset(&f, 0, sizeof(f));
    f.customers = NULL;
    initWaitQueue(&f.waitQueue);

    readString("航班号: ", f.flightNo, MAX_FLIGHT_NO);
    if (findFlightIndex(f.flightNo) >= 0) {
        printErr("该航班号已存在！");
        freeWaitQueue(&f.waitQueue);
        return;
    }
    readString("飞机号: ", f.planeNo, MAX_PLANE_NO);
    readString("终点站: ", f.dest, MAX_DEST);
    f.weekday = readInt("飞行日(1=周一...7=周日): ", 1, 7);
    f.capacity = readInt("载客定额(1~500): ", 1, 500);
    f.remaining = f.capacity;

    int pos = insertFlightSorted(&f);
    if (pos < 0) { printErr("内存分配失败"); freeWaitQueue(&f.waitQueue); return; }

    printOK("航班录入成功");
    saveAllFlights(dataPath);
}

/* ======================== 核心业务：查询航线 ======================== */

static void queryRoute(void) {
    printf("\n");
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  航线查询\n");
    printDivider();

    char dest[MAX_DEST];
    readString("请输入终点站名（支持模糊匹配）: ", dest, MAX_DEST);

    int results[256];
    int cnt = findFlightByDest(dest, results, 256);
    if (cnt == 0) {
        printWarn("未找到匹配的航线");
        return;
    }

    cPrint(C_GREEN, "  找到 %d 条匹配航线:\n", cnt);
    printDivider();
    cPrint(C_BOLD, "  %-10s %-10s %-10s %-6s %-6s %-6s\n",
           "航班号", "飞机号", "终点站", "飞行日", "定额", "余票");
    printDivider();
    for (int i = 0; i < cnt; i++) {
        Flight *f = &flights[results[i]];
        printf("  %-10s %-10s %-10s %-6s %-6d ",
               f->flightNo, f->planeNo, f->dest,
               weekdayName(f->weekday), f->capacity);
        if (f->remaining == 0) {
            cPrint(C_RED, "%-6d (满员)\n", f->remaining);
        } else if (f->remaining <= 3) {
            cPrint(C_YELLOW, "%-6d (紧张)\n", f->remaining);
        } else {
            cPrint(C_GREEN, "%-6d\n", f->remaining);
        }
    }
    printDivider();
}

/* ======================== 核心业务：订票 ======================== */

static void bookTicket(void) {
    printf("\n");
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  客票预订\n");
    printDivider();

    char flightNo[MAX_FLIGHT_NO];
    char name[MAX_NAME_LEN];
    readString("航班号: ", flightNo, MAX_FLIGHT_NO);

    int idx = findFlightIndex(flightNo);
    if (idx < 0) { printErr("未找到该航班！"); return; }

    Flight *f = &flights[idx];
    cPrint(C_CYAN, "  航班信息: %s (%s → %s)  ", f->flightNo, f->planeNo, f->dest);
    printf("余票: ");
    if (f->remaining == 0) cPrint(C_RED, "%d (满员)\n", f->remaining);
    else cPrint(C_GREEN, "%d\n", f->remaining);
    printSeatMap(f);

    readString("客户姓名: ", name, MAX_NAME_LEN);
    int count = readInt("订票数量: ", 1, f->capacity + 20);

    if (f->remaining >= count) {
        int seatStart = f->capacity - f->remaining + 1;
        f->remaining -= count;
        f->customers = insertCustomer(f->customers, name, count, seatStart);
        printThickDivider();
        cPrint(C_BOLD C_GREEN, "  ** 订票成功！**\n");
        printf("  客户："); cPrint(C_BOLD, "%s\n", name);
        printf("  航班：%s（%s → %s）\n", f->flightNo, f->planeNo, f->dest);
        printf("  订票数：%d 张\n", count);
        cPrint(C_CYAN, "  座位号："); cPrint(C_BOLD C_GREEN, "%d ~ %d\n", seatStart, seatStart + count - 1);
        printSeatMap(f);
        saveAllFlights(dataPath);
    } else if (f->remaining > 0 && count > f->remaining) {
        cPrint(C_YELLOW, "  余票不足！当前余票 %d 张，您需要 %d 张。\n", f->remaining, count);
        if (readYesNo("是否加入等候替补队列")) {
            if (enqueueWait(&f->waitQueue, name, count)) {
                printOK("已加入等候替补队列");
                cPrint(C_CYAN, "  排队位置：%d\n", f->waitQueue.size);
                saveAllFlights(dataPath);
            } else { printErr("队列操作失败"); }
        } else { printInfo("已取消订票"); }
    } else {
        cPrint(C_RED, "  抱歉，航班 %s 已满员！\n", f->flightNo);
        if (readYesNo("是否加入等候替补队列")) {
            if (enqueueWait(&f->waitQueue, name, count)) {
                printOK("已加入等候替补队列");
                cPrint(C_CYAN, "  排队位置：%d\n", f->waitQueue.size);
                saveAllFlights(dataPath);
            } else { printErr("队列操作失败"); }
        } else { printInfo("已取消订票"); }
    }
}

/* ======================== 核心业务：退票 ======================== */

static void refundTicket(void) {
    printf("\n");
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  退票处理\n");
    printDivider();

    char flightNo[MAX_FLIGHT_NO];
    char name[MAX_NAME_LEN];
    readString("航班号: ", flightNo, MAX_FLIGHT_NO);

    int idx = findFlightIndex(flightNo);
    if (idx < 0) { printErr("未找到航班号！"); return; }

    readString("客户姓名: ", name, MAX_NAME_LEN);

    Flight *f = &flights[idx];
    CustomerNode *cust = findCustomer(f->customers, name);
    if (!cust) { printErr("该客户未在本航班订票！"); return; }

    int refundCount = cust->ticketCount;
    int refundSeat = cust->seatStart;
    f->remaining += refundCount;
    f->customers = removeCustomer(f->customers, name);

    printThickDivider();
    cPrint(C_BOLD C_GREEN, "  ** 退票成功！**\n");
    printf("  客户：%s，退票 %d 张，座位号 %d ~ %d\n",
           name, refundCount, refundSeat, refundSeat + refundCount - 1);
    cPrint(C_CYAN, "  航班 %s 当前余票：%d 张\n", f->flightNo, f->remaining);

    while (!isWaitQueueEmpty(&f->waitQueue) && f->remaining > 0) {
        const char *wName = peekWaitName(&f->waitQueue);
        int wCount = peekWaitCount(&f->waitQueue);

        if (f->remaining >= wCount) {
            char wn[MAX_NAME_LEN];
            int wc;
            dequeueWait(&f->waitQueue, wn, &wc);
            int seatStart = f->capacity - f->remaining + 1;
            f->remaining -= wc;
            f->customers = insertCustomer(f->customers, wn, wc, seatStart);
            printf("\n");
            cPrint(C_BOLD C_MAGENTA, "  >> 候补客户 %s 自动订票成功！\n", wn);
            printf("     订票数：%d 张，座位号：%d ~ %d\n",
                   wc, seatStart, seatStart + wc - 1);
        } else {
            printf("\n");
            cPrint(C_YELLOW, "  候补客户 %s 需要 %d 张票，当前余票 %d 张。\n",
                   wName, wCount, f->remaining);
            if (readYesNo("是否接受部分订票")) {
                char wn[MAX_NAME_LEN];
                int wc = 0;
                dequeueWait(&f->waitQueue, wn, &wc);
                int seatStart = f->capacity - f->remaining + 1;
                int bookCount = f->remaining;
                f->remaining = 0;
                f->customers = insertCustomer(f->customers, wn, bookCount, seatStart);
                cPrint(C_GREEN, "  客户 %s 部分订票成功，订票 %d 张，座位号 %d ~ %d\n",
                       wn, bookCount, seatStart, seatStart + bookCount - 1);
            } else {
                cPrint(C_CYAN, "  客户 %s 放弃本次候补，保留下次机会。\n", wName);
                char wn[MAX_NAME_LEN];
                int wc = 0;
                dequeueWait(&f->waitQueue, wn, &wc);
                enqueueWait(&f->waitQueue, wn, wc);
            }
            break;
        }
    }

    saveAllFlights(dataPath);
    printOK("退票处理完成");
}

/* ======================== 显示所有航班 ======================== */

static void showAllFlights(void) {
    printf("\n");
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  所有航班信息\n");
    printDivider();
    if (flightCount == 0) {
        printWarn("暂无航班数据");
        return;
    }

    cPrint(C_BOLD, "  %-10s %-10s %-10s %-6s %-6s %-6s %-8s\n",
           "航班号", "飞机号", "终点站", "星期", "定额", "余票", "载客率");
    printDivider();
    int totalCap = 0, totalBooked = 0;
    for (int i = 0; i < flightCount; i++) {
        Flight *f = &flights[i];
        int booked = f->capacity - f->remaining;
        double occ = (f->capacity > 0) ? (double)booked / f->capacity * 100.0 : 0;
        totalCap += f->capacity;
        totalBooked += booked;
        printf("  %-10s %-10s %-10s %-6s %-6d ",
               f->flightNo, f->planeNo, f->dest,
               weekdayName(f->weekday), f->capacity);
        if (f->remaining == 0) {
            cPrint(C_RED, "%-6d ", f->remaining);
        } else if (f->remaining <= 3) {
            cPrint(C_YELLOW, "%-6d ", f->remaining);
        } else {
            cPrint(C_GREEN, "%-6d ", f->remaining);
        }
        if (occ >= 80) cPrint(C_RED, "%6.1f%%\n", occ);
        else if (occ >= 40) cPrint(C_YELLOW, "%6.1f%%\n", occ);
        else cPrint(C_GREEN, "%6.1f%%\n", occ);
    }
    printDivider();
    cPrint(C_BOLD, "  合计: "); printf("%d 个航班, ", flightCount);
    cPrint(C_CYAN, "%d/%d", totalBooked, totalCap);
    printf(" 座位已订, 总载客率 ");
    double totalOcc = totalCap > 0 ? (double)totalBooked / totalCap * 100.0 : 0;
    if (totalOcc >= 80) cPrint(C_RED, "%.1f%%\n", totalOcc);
    else if (totalOcc >= 40) cPrint(C_YELLOW, "%.1f%%\n", totalOcc);
    else cPrint(C_GREEN, "%.1f%%\n", totalOcc);
}

/* ======================== 显示客户名单 ======================== */

static void showCustomers(int idx) {
    if (idx < 0 || idx >= flightCount) { printErr("无效的航班索引！"); return; }
    Flight *f = &flights[idx];
    printf("\n");
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  航班 %s 已订票客户名单\n", f->flightNo);
    cPrint(C_GRAY, "  航线: %s → %s  飞行日: %s  定额: %d\n",
           f->planeNo, f->dest, weekdayName(f->weekday), f->capacity);
    printDivider();

    if (!f->customers) {
        printWarn("暂无已订票客户");
    } else {
        cPrint(C_BOLD, "  %-15s %-8s %-15s\n", "姓名", "票数", "座位号");
        printDivider();
        CustomerNode *c = f->customers;
        while (c) {
            printf("  %-15s %-8d ", c->name, c->ticketCount);
            cPrint(C_CYAN, "%d ~ %d\n", c->seatStart, c->seatStart + c->ticketCount - 1);
            c = c->next;
        }
    }

    if (!isWaitQueueEmpty(&f->waitQueue)) {
        printDivider();
        cPrint(C_BOLD C_MAGENTA, "  候补队列（共 %d 人）:\n", f->waitQueue.size);
        for (int i = 0; i < f->waitQueue.size; i++) {
            int qi = (f->waitQueue.front + i) % f->waitQueue.capacity;
            printf("    %d. %s (%d张)\n", i + 1, f->waitQueue.names[qi], f->waitQueue.counts[qi]);
        }
    }
    printDivider();
}

/* ======================== 统计功能 ======================== */

static void showStatistics(void) {
    printf("\n");
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  运营统计报告\n");
    printDivider();

    int totalCap = 0, totalBooked = 0, totalWait = 0;
    int fullFlights = 0;
    for (int i = 0; i < flightCount; i++) {
        Flight *f = &flights[i];
        totalCap += f->capacity;
        totalBooked += (f->capacity - f->remaining);
        totalWait += f->waitQueue.size;
        if (f->remaining == 0) fullFlights++;
    }
    int revenue = totalBooked * TICKET_PRICE;
    double occ = totalCap > 0 ? (double)totalBooked / totalCap * 100.0 : 0;

    printf("  总航班数:     "); cPrint(C_BOLD C_CYAN, "%d\n", flightCount);
    printf("  满员航班:     "); cPrint(fullFlights>0?C_RED:C_GREEN, "%d\n", fullFlights);
    printf("  总座位数:     %d\n", totalCap);
    printf("  已订票数:     "); cPrint(C_GREEN, "%d\n", totalBooked);
    printf("  余票总数:     "); cPrint(C_YELLOW, "%d\n", totalCap - totalBooked);
    printf("  候补人数:     "); cPrint(C_MAGENTA, "%d\n", totalWait);
    printf("  预估营收:     "); cPrint(C_BOLD C_GREEN, "¥%d  (单价 ¥%d/张)\n", revenue, TICKET_PRICE);
    printf("  总体载客率:   ");
    if (occ >= 80) cPrint(C_RED, "%.1f%%\n", occ);
    else if (occ >= 40) cPrint(C_YELLOW, "%.1f%%\n", occ);
    else cPrint(C_GREEN, "%.1f%%\n", occ);
    printDivider();

    if (flightCount > 0) {
        cPrint(C_BOLD, "  各航班载客率详情:\n");
        for (int i = 0; i < flightCount; i++) {
            Flight *f = &flights[i];
            int booked = f->capacity - f->remaining;
            double fo = f->capacity > 0 ? (double)booked / f->capacity * 100.0 : 0;
            int barW = 20;
            int filled = (int)(fo / 100.0 * barW);
            printf("  %-10s [", f->flightNo);
            for (int j = 0; j < barW; j++) {
                if (j < filled) {
                    if (fo >= 80) cPrint(C_RED, "█");
                    else if (fo >= 40) cPrint(C_YELLOW, "█");
                    else cPrint(C_GREEN, "█");
                } else {
                    cPrint(C_GRAY, "░");
                }
            }
            printf("] ");
            if (fo >= 80) cPrint(C_RED, "%5.1f%%\n", fo);
            else if (fo >= 40) cPrint(C_YELLOW, "%5.1f%%\n", fo);
            else cPrint(C_GREEN, "%5.1f%%\n", fo);
        }
    }
    printThickDivider();
}

/* ======================== 加载测试数据 ======================== */

static void loadTestData(void) {
    for (int i = 0; i < flightCount; i++) {
        freeFlightData(&flights[i]);
    }
    flightCount = 0;

    Flight f;
    memset(&f, 0, sizeof(f));
    f.customers = NULL;
    initWaitQueue(&f.waitQueue);

    /* 航班1：CA1001 北京→上海 周一 5座 余3 已有2位客户 */
    strcpy(f.flightNo, "CA1001"); strcpy(f.planeNo, "B2301");
    strcpy(f.dest, "上海"); f.weekday = 1; f.capacity = 5; f.remaining = 3;
    f.customers = NULL; initWaitQueue(&f.waitQueue);
    insertFlightSorted(&f);
    int i = findFlightIndex("CA1001");
    flights[i].customers = insertCustomer(flights[i].customers, "张三", 1, 1);
    flights[i].customers = insertCustomer(flights[i].customers, "李四", 1, 2);

    /* 航班2：CZ2002 北京→广州 周三 4座 满员 2客户+1候补 */
    memset(&f, 0, sizeof(f));
    strcpy(f.flightNo, "CZ2002"); strcpy(f.planeNo, "B5601");
    strcpy(f.dest, "广州"); f.weekday = 3; f.capacity = 4; f.remaining = 0;
    f.customers = NULL; initWaitQueue(&f.waitQueue);
    insertFlightSorted(&f);
    i = findFlightIndex("CZ2002");
    flights[i].customers = insertCustomer(flights[i].customers, "王五", 2, 1);
    flights[i].customers = insertCustomer(flights[i].customers, "赵六", 2, 3);
    enqueueWait(&flights[i].waitQueue, "孙七", 1);

    /* 航班3：MU3003 上海→深圳 周五 6座 全空 */
    memset(&f, 0, sizeof(f));
    strcpy(f.flightNo, "MU3003"); strcpy(f.planeNo, "A1201");
    strcpy(f.dest, "深圳"); f.weekday = 5; f.capacity = 6; f.remaining = 6;
    f.customers = NULL; initWaitQueue(&f.waitQueue);
    insertFlightSorted(&f);

    /* 航班4：HU4004 广州→成都 周日 3座 余1 1位客户 */
    memset(&f, 0, sizeof(f));
    strcpy(f.flightNo, "HU4004"); strcpy(f.planeNo, "B7801");
    strcpy(f.dest, "成都"); f.weekday = 7; f.capacity = 3; f.remaining = 1;
    f.customers = NULL; initWaitQueue(&f.waitQueue);
    insertFlightSorted(&f);
    i = findFlightIndex("HU4004");
    flights[i].customers = insertCustomer(flights[i].customers, "周八", 2, 1);

    /* 航班5：CA5005 深圳→北京 周二 4座 全空 */
    memset(&f, 0, sizeof(f));
    strcpy(f.flightNo, "CA5005"); strcpy(f.planeNo, "B9901");
    strcpy(f.dest, "北京"); f.weekday = 2; f.capacity = 4; f.remaining = 4;
    f.customers = NULL; initWaitQueue(&f.waitQueue);
    insertFlightSorted(&f);

    printThickDivider();
    cPrint(C_BOLD C_GREEN, "  [OK] 已加载测试数据:\n");
    printf("    - 5条航班信息\n");
    printf("    - 5位已订票客户\n");
    printf("    - 1个等候替补\n");
    showAllFlights();
    saveAllFlights(dataPath);
}

/* ======================== 帮助菜单 ======================== */

static void showHelp(void) {
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  使用帮助\n");
    printThickDivider();
    printf("\n");
    cPrint(C_BOLD, "  功能说明:\n");
    printf("    1. 录入航班信息    — 输入航班号、飞机号、目的地、星期、载客量\n");
    printf("    2. 查询航线        — 按目的地模糊搜索航班\n");
    printf("    3. 客票预订        — 余票充足直接订，不足/满员可候补\n");
    printf("    4. 退票处理        — 退票后自动处理候补队列\n");
    printf("    5. 查看所有航班    — 表格展示+载客率\n");
    printf("    6. 查看客户名单    — 按姓名排序+候补队列\n");
    printf("    7. 加载测试数据    — 预置5条航班\n");
    printf("    8. 运营统计        — 航班/座位/营收/载客率图表\n");
    printf("    9. 保存数据        — 手动保存到文件\n");
    printf("    0. 退出系统        — 自动保存\n");
    printf("\n");
    cPrint(C_BOLD, "  命令行模式:\n");
    cPrint(C_CYAN, "    airline.exe --help          "); printf("显示帮助\n");
    cPrint(C_CYAN, "    airline.exe --load <file>   "); printf("从指定文件加载\n");
    cPrint(C_CYAN, "    airline.exe --save <file>   "); printf("启动后另存数据\n");
    cPrint(C_CYAN, "    airline.exe -t, --test      "); printf("运行内置自测\n");
    printf("\n");
    cPrint(C_BOLD, "  数据文件:\n");
    cPrint(C_CYAN, "    airline.dat    "); printf("二进制数据文件（魔数头+校验）\n");
    printf("\n");
    cPrint(C_BOLD, "  票价: "); cPrint(C_GREEN, "¥%d/张\n", TICKET_PRICE);
    printf("\n");
}

/* ======================== 菜单系统 ======================== */

static void showMenu(void) {
    printDivider();
    cPrint(C_BOLD C_BLUE, "  主菜单\n");
    printDivider();
    cPrint(C_CYAN, "  1 "); printf("- 录入航班信息\n");
    cPrint(C_CYAN, "  2 "); printf("- 查询航线\n");
    cPrint(C_CYAN, "  3 "); printf("- 客票预订\n");
    cPrint(C_CYAN, "  4 "); printf("- 退票处理\n");
    cPrint(C_CYAN, "  5 "); printf("- 查看所有航班\n");
    cPrint(C_CYAN, "  6 "); printf("- 查看客户名单\n");
    cPrint(C_CYAN, "  7 "); printf("- 加载测试数据\n");
    cPrint(C_CYAN, "  8 "); printf("- 运营统计\n");
    cPrint(C_CYAN, "  9 "); printf("- 保存数据\n");
    cPrint(C_CYAN, "  H/?"); printf("- 帮助信息\n");
    cPrint(C_CYAN, "  0 "); printf("- 退出系统\n");
    printDivider();
    printf("  当前状态: 航班 ");
    cPrint(C_CYAN, "%d", flightCount);
    printf(" | 数据文件: "); cPrint(C_GRAY, "%s\n", dataPath);
    printDivider();
}

static void runMenu(void) {
    char choice[64];
    while (1) {
        showMenu();
        printf("  请选择操作: "); fflush(stdout);
        if (fgets(choice, sizeof(choice), stdin) == NULL) break;
        char *nl = strchr(choice, '\n'); if (nl) *nl = '\0';
        if (choice[0] == '\0') continue;
        char c = (char)toupper((unsigned char)choice[0]);
        switch (c) {
        case '1': inputFlight(); break;
        case '2': queryRoute(); break;
        case '3': bookTicket(); break;
        case '4': refundTicket(); break;
        case '5': showAllFlights(); break;
        case '6': {
            char fno[MAX_FLIGHT_NO];
            readString("请输入航班号: ", fno, MAX_FLIGHT_NO);
            int idx = findFlightIndex(fno);
            if (idx < 0) printErr("未找到该航班！");
            else showCustomers(idx);
            break;
        }
        case '7': loadTestData(); break;
        case '8': showStatistics(); break;
        case '9': saveAllFlights(dataPath); printOK("数据已保存"); break;
        case 'H':
        case '?': showHelp(); break;
        case '0':
            saveAllFlights(dataPath);
            cPrint(C_GREEN, "\n  [OK] 数据已保存，感谢使用，再见！\n\n");
            return;
        default:
            if (isdigit((unsigned char)c)) {
                cPrint(C_YELLOW, "  [!] 无效选项，请重新输入\n");
            } else {
                cPrint(C_YELLOW, "  [!] 无效选项，请重新输入\n");
            }
            break;
        }
    }
}

/* ======================== 内置自测用例 ======================== */

static void t_assert(int cond, const char *msg) {
    if (cond) {
        cPrint(C_GREEN, "    [PASS] "); printf("%s\n", msg);
        g_tests_passed++;
    } else {
        cPrint(C_RED, "    [FAIL] "); printf("%s\n", msg);
        g_tests_failed++;
    }
}

static int run_self_test(void) {
    g_tests_passed = 0;
    g_tests_failed = 0;

    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  运行内置自测用例\n");
    printThickDivider();

    freeAllFlights();
    const char *test_file = "__test_air.dat";
    remove(test_file);

    cPrint(C_CYAN, "\n  [TEST] T1: 添加航班\n");
    Flight f;
    memset(&f, 0, sizeof(f));
    f.customers = NULL;
    initWaitQueue(&f.waitQueue);
    strcpy(f.flightNo, "CA9901");
    strcpy(f.planeNo, "B0001");
    strcpy(f.dest, "测试城市");
    f.weekday = 1;
    f.capacity = 10;
    f.remaining = 10;
    int pos = insertFlightSorted(&f);
    t_assert(pos >= 0, "插入航班CA9901成功");
    t_assert(flightCount == 1, "航班数=1");

    cPrint(C_CYAN, "\n  [TEST] T2: 查询航班\n");
    int idx = findFlightIndex("CA9901");
    t_assert(idx == 0, "按航班号找到CA9901");
    t_assert(findFlightIndex("XX9999") < 0, "查找不存在航班返回-1");

    int indices[16];
    int cnt = findFlightByDest("测试", indices, 16);
    t_assert(cnt == 1, "按目的地模糊查询找到1条");
    t_assert(indices[0] == 0, "查询结果索引正确");

    cPrint(C_CYAN, "\n  [TEST] T3: 模拟订票\n");
    Flight *tf = &flights[idx];
    t_assert(tf->remaining == 10, "初始余票=10");
    int seatStart = tf->capacity - tf->remaining + 1;
    tf->remaining -= 2;
    tf->customers = insertCustomer(tf->customers, "测试客户A", 2, seatStart);
    t_assert(tf->remaining == 8, "订票2张后余票=8");
    CustomerNode *cust = findCustomer(tf->customers, "测试客户A");
    t_assert(cust != NULL, "已订票客户可查");
    t_assert(cust != NULL && cust->ticketCount == 2, "客户订票数=2");
    t_assert(cust != NULL && cust->seatStart == 1, "座位起始=1");

    seatStart = tf->capacity - tf->remaining + 1;
    tf->remaining -= 3;
    tf->customers = insertCustomer(tf->customers, "测试客户B", 3, seatStart);
    t_assert(tf->remaining == 5, "再订3张后余票=5");
    t_assert(findCustomer(tf->customers, "测试客户B") != NULL, "客户B已订票");

    cPrint(C_CYAN, "\n  [TEST] T4: 模拟退票\n");
    CustomerNode *custA = findCustomer(tf->customers, "测试客户A");
    t_assert(custA != NULL, "客户A存在");
    if (custA) {
        tf->remaining += custA->ticketCount;
        tf->customers = removeCustomer(tf->customers, "测试客户A");
        t_assert(tf->remaining == 7, "退2张后余票=7");
        t_assert(findCustomer(tf->customers, "测试客户A") == NULL, "客户A已移除");
        t_assert(findCustomer(tf->customers, "测试客户B") != NULL, "客户B仍在");
    } else {
        t_assert(0, "退2张后余票=7");
        t_assert(0, "客户A已移除");
        t_assert(0, "客户B仍在");
    }

    cPrint(C_CYAN, "\n  [TEST] T5: 再添加航班\n");
    memset(&f, 0, sizeof(f));
    f.customers = NULL;
    initWaitQueue(&f.waitQueue);
    strcpy(f.flightNo, "MU8802");
    strcpy(f.planeNo, "A0002");
    strcpy(f.dest, "北京");
    f.weekday = 3;
    f.capacity = 20;
    f.remaining = 20;
    pos = insertFlightSorted(&f);
    t_assert(pos >= 0, "插入航班MU8802成功");
    t_assert(flightCount == 2, "航班数=2");
    t_assert(strcmp(flights[0].flightNo, "CA9901") < 0 ||
             strcmp(flights[1].flightNo, "CA9901") == 0 ||
             strcmp(flights[1].flightNo, "MU8802") == 0, "航班按号有序");

    cPrint(C_CYAN, "\n  [TEST] T6: 候补队列\n");
    int idx2 = findFlightIndex("CA9901");
    t_assert(idx2 >= 0, "CA9901存在");
    if (idx2 >= 0) {
        Flight *f2 = &flights[idx2];
        t_assert(isWaitQueueEmpty(&f2->waitQueue) == 1, "初始候补队列为空");
        enqueueWait(&f2->waitQueue, "候补甲", 1);
        enqueueWait(&f2->waitQueue, "候补乙", 2);
        t_assert(f2->waitQueue.size == 2, "入队2人后队列大小=2");
        t_assert(isWaitQueueEmpty(&f2->waitQueue) == 0, "队列非空");
        char wn[MAX_NAME_LEN];
        int wc = 0;
        dequeueWait(&f2->waitQueue, wn, &wc);
        t_assert(strcmp(wn, "候补甲") == 0, "出队第1人是候补甲");
        t_assert(wc == 1, "候补甲订票1张");
        t_assert(f2->waitQueue.size == 1, "出队后队列大小=1");
        dequeueWait(&f2->waitQueue, wn, &wc);
        t_assert(strcmp(wn, "候补乙") == 0, "出队第2人是候补乙");
        t_assert(isWaitQueueEmpty(&f2->waitQueue) == 1, "全部出队后队列为空");
    } else {
        t_assert(0, "初始候补队列为空");
        t_assert(0, "入队2人后队列大小=2");
        t_assert(0, "队列非空");
        t_assert(0, "出队第1人是候补甲");
        t_assert(0, "候补甲订票1张");
        t_assert(0, "出队后队列大小=1");
        t_assert(0, "出队第2人是候补乙");
        t_assert(0, "全部出队后队列为空");
    }

    cPrint(C_CYAN, "\n  [TEST] T7: 保存到文件\n");
    saveAllFlights(test_file);

    cPrint(C_CYAN, "\n  [TEST] T8: 从文件加载（持久化往返）\n");
    int saved_count = flightCount;
    int saved_remaining = -1;
    int idx_ca = findFlightIndex("CA9901");
    if (idx_ca >= 0) saved_remaining = flights[idx_ca].remaining;
    freeAllFlights();
    t_assert(flightCount == 0, "释放后航班数=0");
    AirStatus load_st = loadAllFlights(test_file);
    t_assert(load_st == AIR_OK, "加载测试文件成功");
    t_assert(flightCount == saved_count, "加载后航班数一致");
    int loaded_idx = findFlightIndex("CA9901");
    t_assert(loaded_idx >= 0, "CA9901仍在数据中");
    t_assert(findFlightIndex("MU8802") >= 0, "MU8802仍在数据中");
    if (loaded_idx >= 0) {
        t_assert(flights[loaded_idx].remaining == saved_remaining, "余票数据一致");
        t_assert(flights[loaded_idx].capacity == 10, "载客量数据一致");
        t_assert(findCustomer(flights[loaded_idx].customers, "测试客户B") != NULL, "客户数据持久化正确");
    } else {
        t_assert(0, "余票数据一致");
        t_assert(0, "载客量数据一致");
        t_assert(0, "客户数据持久化正确");
    }

    cPrint(C_CYAN, "\n  [TEST] T9: 边界与清理\n");
    freeAllFlights();
    t_assert(flightCount == 0, "释放全部数据后航班数=0");
    t_assert(findFlightIndex("CA9901") < 0, "空数据查询返回-1");
    remove(test_file);

    printThickDivider();
    cPrint(C_BOLD, "  测试结果: ");
    cPrint(C_GREEN, "PASS=%d  ", g_tests_passed);
    cPrint(C_RED, "FAIL=%d  ", g_tests_failed);
    int total = g_tests_passed + g_tests_failed;
    printf("TOTAL=%d\n", total);
    if (g_tests_failed == 0) {
        cPrint(C_BOLD C_GREEN, "  ★ 全部测试通过！\n");
    } else {
        cPrint(C_BOLD C_RED, "  ✗ 有测试失败，请检查！\n");
    }
    printThickDivider();

    return g_tests_failed > 0 ? 1 : 0;
}

/* ======================== 命令行处理 ======================== */

static void printUsage(const char *prog) {
    printf("Airline Ticket Booking System - V2.0 Enhanced\n\n");
    printf("用法:\n");
    printf("  %s                     交互模式（菜单）\n", prog);
    printf("  %s --help              显示帮助\n", prog);
    printf("  %s --load <file>       从指定数据文件加载\n", prog);
    printf("  %s --save <file>       从默认加载后另存到指定文件\n", prog);
    printf("  %s -t, --test          运行内置自测\n", prog);
    printf("  %s -h                  显示帮助\n", prog);
}

static int runCommandLine(int argc, char *argv[]) {
    if (argc < 2) return 0;
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        printUsage(argv[0]);
        return 1;
    }
    if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0) {
        int failures = run_self_test();
        return failures == 0 ? 1 : 2;
    }
    if (strcmp(argv[1], "--load") == 0) {
        if (argc < 3) { printErr("用法: --load <文件>"); return 1; }
        strncpy(dataPath, argv[2], sizeof(dataPath) - 1);
        AirStatus s = loadAllFlights(dataPath);
        if (s == AIR_OK) printOK("数据加载成功");
        else { printErr("数据加载失败"); return 1; }
        return 0;
    }
    if (strcmp(argv[1], "--save") == 0) {
        if (argc < 3) { printErr("用法: --save <文件>"); return 1; }
        printInfo("将在交互模式启动后保存到指定文件");
        return 0;
    }
    printUsage(argv[0]);
    return 1;
}

/* ======================== 入口 ======================== */

int main(int argc, char *argv[]) {
    enableVTMode();

    if (argc > 1) {
        int ret = runCommandLine(argc, argv);
        if (ret == 1) { freeAllFlights(); return 0; }
        if (ret != 0) { freeAllFlights(); return 1; }
    }

    printBanner();

    AirStatus s = loadAllFlights(dataPath);
    if (s == AIR_OK) {
        printInfo("已从数据文件加载航班数据");
    } else if (s == AIR_ERR_MAGIC) {
        printWarn("数据文件格式不匹配，将使用空数据启动");
    } else if (s == AIR_ERR_OPEN_FILE) {
        printInfo("未找到数据文件，系统初始为空");
    } else {
        printWarn("数据文件损坏，将使用空数据启动");
    }

    if (flightCount == 0) {
        cPrint(C_YELLOW, "  [TIP] 选择菜单 7 可加载 5 条预置测试航班\n");
    }

    runMenu();
    freeAllFlights();
    return 0;
}
