/*
* ============================================================================
*
*    一元稀疏多项式计算器  (完全体·V2.0升级版)
*    Univariate Sparse Polynomial Calculator  (V2.0 Full Enhanced)
*
* ============================================================================
*
*  功能概述:
*    基于带头结点单链表实现的一元稀疏多项式运算系统，支持：
*      [1] 多项式创建 —— 键盘逐项输入 / 从二进制文件(.poly)加载
*      [2] 多项式输出 —— 原始格式(系数,指数) / 数学格式(如 -3x^8+6x^3-18)
*      [3] 基本运算 —— 加法 / 减法 / 乘法
*      [4] 高级运算 —— 求值(代入x) / 求导
*      [5] 多项式统计 —— 项数 / 最高次数 / 首项系数 / 常数项
*      [6] 文件保存 —— 带魔数版本头的二进制.poly格式
*      [7] 命令行模式 —— 支持 -a/-s/-m/-e/-d/-t/-o 非交互调用
*      [8] 彩色终端界面 —— ANSI颜色支持（Windows 10+ / Linux）
*      [9] 内置自检 —— --test 参数运行完整测试套件
*     [10] 完善的帮助信息 —— --help 显示完整用法
*
*  数据结构:
*    PolyNode  —— 多项式项结点（带头结点单链表）
*                coef: 系数(double)
*                exp : 指数(int, 降序排列)
*                next: 后继指针
*    PolyHeader —— .poly二进制文件头
*                magic[4]="POLY", version, n(项数)
*
*  编译命令:
*    gcc -std=c99 -Wall -Wextra -O2 -o poly.exe main.c -lm
*
*  命令行用法:
*    poly.exe --help                      显示帮助
*    poly.exe --test                      运行自检
*    poly.exe -a <f1> <f2> [-o out]       加法
*    poly.exe -s <f1> <f2> [-o out]       减法
*    poly.exe -m <f1> <f2> [-o out]       乘法
*    poly.exe -e <f> <x>                  求值
*    poly.exe -d <f> [-o out]             求导
*    poly.exe -t <f>                      显示统计
*
*  交互模式:
*    直接运行 poly.exe 进入菜单交互模式
*
*  编码: UTF-8
* ============================================================================
*/

/* ======================== 标准头文件 ======================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#include <io.h>
#define F_OK 0
#define access _access
#else
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#endif

/* ======================== 常量与宏 ======================== */
#define POLY_MAGIC      "POLY"
#define POLY_VERSION    2
#define EPS             1e-9
#define MAX_PATH_LEN    512
#define MAX_LINE_LEN    256
#define POLY_A          1
#define POLY_B          2

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
#define BG_YELLOW   "\033[43m"

/* ======================== 错误码 ======================== */
typedef enum {
    POLY_OK = 0,
    POLY_ERR_OPEN_FILE,
    POLY_ERR_READ_FILE,
    POLY_ERR_WRITE_FILE,
    POLY_ERR_MEMORY,
    POLY_ERR_CORRUPT,
    POLY_ERR_MAGIC,
    POLY_ERR_VERSION,
    POLY_ERR_EMPTY,
    POLY_ERR_CANCEL,
    POLY_ERR_INVALID,
} PolyStatus;

/* ======================== 数据结构 ======================== */

typedef struct PolyNode {
    double coef;
    int    exp;
    struct PolyNode *next;
} PolyNode, *Polynomial;

#pragma pack(push, 1)
typedef struct {
    char magic[4];
    unsigned short version;
    unsigned short reserved;
    unsigned int n;
} PolyHeader;
#pragma pack(pop)

typedef struct {
    int degree;
    int termCount;
    double leadingCoef;
    double constTerm;
} PolyStats;

/* ======================== 全局状态 ======================== */
static int colorEnabled = 1;

/* ======================== 前向声明 ======================== */
static void initPoly(Polynomial *p);
static void destroyPoly(Polynomial *p);
static PolyStatus createPoly(Polynomial p);
static PolyStatus addPoly(Polynomial pa, Polynomial pb, Polynomial pc);
static PolyStatus subtractPoly(Polynomial pa, Polynomial pb, Polynomial pc);
static PolyStatus multiplyPoly(Polynomial pa, Polynomial pb, Polynomial pc);
static PolyStatus derivativePoly(Polynomial pa, Polynomial pb);
static double evaluatePoly(Polynomial p, double x);
static void printRaw(Polynomial p);
static void printMath(Polynomial p);
static PolyStatus savePoly(Polynomial p, const char *filename);
static PolyStatus loadPoly(Polynomial p, const char *filename);
static void getStats(Polynomial p, PolyStats *stats);
static void swapPolys(Polynomial *pa, Polynomial *pb);
static void copyPoly(Polynomial src, Polynomial dest);
static int  isZero(Polynomial p);

/* ======================== 工具函数 ======================== */

static double my_fabs(double x) {
    return x < 0 ? -x : x;
}

static double my_pow(double base, int exp) {
    double result = 1.0;
    int e = exp < 0 ? -exp : exp;
    if (exp == 0) return 1.0;
    if (base == 0) return 0.0;
    while (e > 0) {
        if (e & 1) result *= base;
        base *= base;
        e >>= 1;
    }
    return exp < 0 ? 1.0 / result : result;
}

static int is_integer(double x) {
    return my_fabs(x - (double)(int)(x + (x >= 0 ? 0.5 : -0.5))) < EPS;
}

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

static void printBanner(void) {
    cPrint(C_CYAN, "\n");
    printf("  ╔══════════════════════════════════════════════════════════╗\n");
    printf("  ║");
    cPrint(C_BOLD C_YELLOW, "      一元稀疏多项式计算器  V2.0  (完全体·增强版)      ");
    cPrint(C_CYAN, "║\n");
    printf("  ║");
    cPrint(C_GRAY, "   Univariate Sparse Polynomial Calculator  V2.0     ");
    cPrint(C_CYAN, "║\n");
    printf("  ╚══════════════════════════════════════════════════════════╝\n");
    cPrint(C_RESET, "\n");
}

static void clearInputBuffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

static void pauseScreen(void) {
    cPrint(C_GRAY, "\n  按回车键继续...");
    clearInputBuffer();
    getchar();
}

static int readMenuChoice(void) {
    char buf[64];
    printf("  请选择操作: ");
    if (fgets(buf, sizeof(buf), stdin) == NULL) return -1;
    return atoi(buf);
}

/* ======================== 多项式核心操作 ======================== */

static void initPoly(Polynomial *p) {
    *p = (Polynomial)malloc(sizeof(PolyNode));
    if (!*p) {
        printErr("内存分配失败");
        exit(1);
    }
    (*p)->coef = 0.0;
    (*p)->exp = 0;
    (*p)->next = NULL;
}

static void destroyPoly(Polynomial *p) {
    PolyNode *q;
    while (*p) {
        q = (*p)->next;
        free(*p);
        *p = q;
    }
}

static int isZero(Polynomial p) {
    return p->next == NULL;
}

static PolyNode* makeNode(double coef, int exp) {
    PolyNode *node = (PolyNode*)malloc(sizeof(PolyNode));
    if (!node) return NULL;
    node->coef = coef;
    node->exp = exp;
    node->next = NULL;
    return node;
}

static void insertTerm(Polynomial p, double coef, int exp) {
    PolyNode *q, *pre;
    if (my_fabs(coef) < EPS) return;
    pre = p;
    q = p->next;
    while (q && q->exp > exp) {
        pre = q;
        q = q->next;
    }
    if (q && q->exp == exp) {
        q->coef += coef;
        if (my_fabs(q->coef) < EPS) {
            pre->next = q->next;
            free(q);
        }
    } else {
        PolyNode *node = makeNode(coef, exp);
        if (!node) return;
        node->next = q;
        pre->next = node;
    }
}

static PolyStatus createPoly(Polynomial p) {
    int n, i, exp;
    double coef;
    PolyNode *q;
    destroyPoly(&p->next);
    p->next = NULL;
    cPrint(C_CYAN, "  请输入多项式的项数: ");
    if (scanf("%d", &n) != 1 || n < 0) {
        clearInputBuffer();
        printErr("项数输入无效");
        return POLY_ERR_INVALID;
    }
    cPrint(C_CYAN, "  请逐项输入 (系数 指数)，共 %d 项:\n", n);
    for (i = 0; i < n; i++) {
        printf("    第 %d 项: ", i + 1);
        if (scanf("%lf %d", &coef, &exp) != 2) {
            clearInputBuffer();
            printErr("输入格式错误");
            return POLY_ERR_INVALID;
        }
        insertTerm(p, coef, exp);
    }
    clearInputBuffer();
    for (q = p->next; q; q = q->next) {
        printf("  -> 加入项: ");
        cPrint(C_YELLOW, "(%.6g, %d)\n", q->coef, q->exp);
    }
    printOK("多项式创建完成");
    return POLY_OK;
}

static void copyPoly(Polynomial src, Polynomial dest) {
    PolyNode *p, *q;
    destroyPoly(&dest->next);
    dest->next = NULL;
    q = dest;
    for (p = src->next; p; p = p->next) {
        q->next = makeNode(p->coef, p->exp);
        if (!q->next) return;
        q = q->next;
    }
}

static void swapPolys(Polynomial *pa, Polynomial *pb) {
    Polynomial t = *pa;
    *pa = *pb;
    *pb = t;
}

static PolyStatus addPoly(Polynomial pa, Polynomial pb, Polynomial pc) {
    PolyNode *qa, *qb, *qc;
    double sum;
    destroyPoly(&pc->next);
    pc->next = NULL;
    qc = pc;
    qa = pa->next;
    qb = pb->next;
    while (qa && qb) {
        if (qa->exp > qb->exp) {
            qc->next = makeNode(qa->coef, qa->exp);
            qa = qa->next;
        } else if (qa->exp < qb->exp) {
            qc->next = makeNode(qb->coef, qb->exp);
            qb = qb->next;
        } else {
            sum = qa->coef + qb->coef;
            if (my_fabs(sum) > EPS) {
                qc->next = makeNode(sum, qa->exp);
            } else {
                qc->next = NULL;
            }
            qa = qa->next;
            qb = qb->next;
        }
        if (qc->next) qc = qc->next;
    }
    while (qa) {
        qc->next = makeNode(qa->coef, qa->exp);
        qc = qc->next;
        qa = qa->next;
    }
    while (qb) {
        qc->next = makeNode(qb->coef, qb->exp);
        qc = qc->next;
        qb = qb->next;
    }
    if (qc) qc->next = NULL;
    return POLY_OK;
}

static PolyStatus subtractPoly(Polynomial pa, Polynomial pb, Polynomial pc) {
    Polynomial temp;
    PolyNode *q;
    PolyStatus st;
    initPoly(&temp);
    for (q = pb->next; q; q = q->next) {
        insertTerm(temp, -q->coef, q->exp);
    }
    st = addPoly(pa, temp, pc);
    destroyPoly(&temp);
    return st;
}

static PolyStatus multiplyPoly(Polynomial pa, Polynomial pb, Polynomial pc) {
    PolyNode *qa, *qb;
    destroyPoly(&pc->next);
    pc->next = NULL;
    for (qa = pa->next; qa; qa = qa->next) {
        for (qb = pb->next; qb; qb = qb->next) {
            insertTerm(pc, qa->coef * qb->coef, qa->exp + qb->exp);
        }
    }
    return POLY_OK;
}

static PolyStatus derivativePoly(Polynomial pa, Polynomial pb) {
    PolyNode *q;
    destroyPoly(&pb->next);
    pb->next = NULL;
    for (q = pa->next; q; q = q->next) {
        if (q->exp > 0) {
            insertTerm(pb, q->coef * q->exp, q->exp - 1);
        }
    }
    return POLY_OK;
}

static double evaluatePoly(Polynomial p, double x) {
    double result = 0.0;
    PolyNode *q;
    for (q = p->next; q; q = q->next) {
        result += q->coef * my_pow(x, q->exp);
    }
    return result;
}

static void getStats(Polynomial p, PolyStats *stats) {
    PolyNode *q;
    memset(stats, 0, sizeof(PolyStats));
    stats->degree = -1;
    stats->termCount = 0;
    stats->leadingCoef = 0.0;
    stats->constTerm = 0.0;
    for (q = p->next; q; q = q->next) {
        stats->termCount++;
        if (q->exp > stats->degree) {
            stats->degree = q->exp;
            stats->leadingCoef = q->coef;
        }
        if (q->exp == 0) {
            stats->constTerm = q->coef;
        }
    }
    if (stats->termCount == 0) {
        stats->degree = 0;
    }
}

/* ======================== 打印函数 ======================== */

static void printRaw(Polynomial p) {
    PolyNode *q = p->next;
    printf("  ");
    cPrint(C_BOLD, "原始格式: ");
    if (!q) {
        cPrint(C_GREEN, "0  (零多项式)\n");
        return;
    }
    while (q) {
        cPrint(C_YELLOW, "(%.6g, %d)", q->coef, q->exp);
        if (q->next) printf(" -> ");
        q = q->next;
    }
    printf("\n");
}

static void printMath(Polynomial p) {
    PolyNode *q = p->next;
    int first = 1;
    printf("  ");
    cPrint(C_BOLD, "数学格式: ");
    cPrint(C_GREEN, "");
    if (!q) {
        printf("0\n");
        cPrint(C_RESET, "");
        return;
    }
    while (q) {
        double coef = q->coef;
        int exp = q->exp;
        double absCoef = my_fabs(coef);
        if (!first) {
            printf(coef < 0 ? "-" : "+");
        } else {
            if (coef < 0) printf("-");
        }
        if (exp == 0) {
            if (is_integer(absCoef)) {
                printf("%d", (int)(absCoef + 0.5));
            } else {
                printf("%g", absCoef);
            }
        } else {
            int showCoef = (my_fabs(absCoef - 1.0) > EPS);
            if (showCoef) {
                if (is_integer(absCoef)) {
                    printf("%d", (int)(absCoef + 0.5));
                } else {
                    printf("%g", absCoef);
                }
            }
            printf("x");
            if (exp > 1) printf("^%d", exp);
        }
        first = 0;
        q = q->next;
    }
    printf("\n");
    cPrint(C_RESET, "");
}

static void printStats(Polynomial p, const char *name) {
    PolyStats st;
    getStats(p, &st);
    cPrint(C_CYAN, "\n  ── %s 统计信息 ──\n", name);
    printf("    ");
    cPrint(C_WHITE, "项数:         ");
    cPrint(C_YELLOW, "%d\n", st.termCount);
    printf("    ");
    cPrint(C_WHITE, "最高次数:     ");
    cPrint(C_YELLOW, "%d\n", st.degree);
    printf("    ");
    cPrint(C_WHITE, "首项系数:     ");
    cPrint(C_YELLOW, "%.6g\n", st.leadingCoef);
    printf("    ");
    cPrint(C_WHITE, "常数项:       ");
    cPrint(C_YELLOW, "%.6g\n", st.constTerm);
}

/* ======================== 文件I/O ======================== */

static PolyStatus savePoly(Polynomial p, const char *filename) {
    FILE *fp;
    PolyHeader hdr;
    PolyNode *q;
    unsigned int n = 0;
    for (q = p->next; q; q = q->next) n++;
    memcpy(hdr.magic, POLY_MAGIC, 4);
    hdr.version = POLY_VERSION;
    hdr.reserved = 0;
    hdr.n = n;
    fp = fopen(filename, "wb");
    if (!fp) {
        printErr("无法创建文件");
        return POLY_ERR_OPEN_FILE;
    }
    if (fwrite(&hdr, sizeof(hdr), 1, fp) != 1) {
        fclose(fp);
        printErr("写入文件头失败");
        return POLY_ERR_WRITE_FILE;
    }
    for (q = p->next; q; q = q->next) {
        if (fwrite(&q->coef, sizeof(double), 1, fp) != 1 ||
            fwrite(&q->exp, sizeof(int), 1, fp) != 1) {
            fclose(fp);
            printErr("写入数据失败");
            return POLY_ERR_WRITE_FILE;
        }
    }
    fclose(fp);
    printOK("保存成功");
    return POLY_OK;
}

static PolyStatus loadPoly(Polynomial p, const char *filename) {
    FILE *fp;
    PolyHeader hdr;
    unsigned int i;
    double coef;
    int exp;
    fp = fopen(filename, "rb");
    if (!fp) {
        printErr("无法打开文件");
        return POLY_ERR_OPEN_FILE;
    }
    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) {
        fclose(fp);
        printErr("读取文件头失败");
        return POLY_ERR_READ_FILE;
    }
    if (memcmp(hdr.magic, POLY_MAGIC, 4) != 0) {
        fclose(fp);
        printErr("文件格式无效(魔数不匹配)");
        return POLY_ERR_MAGIC;
    }
    if (hdr.version != POLY_VERSION) {
        cPrint(C_YELLOW, "  [WARN] 文件版本 %d，当前版本 %d，尝试兼容加载\n", hdr.version, POLY_VERSION);
    }
    destroyPoly(&p->next);
    p->next = NULL;
    for (i = 0; i < hdr.n; i++) {
        if (fread(&coef, sizeof(double), 1, fp) != 1 ||
            fread(&exp, sizeof(int), 1, fp) != 1) {
            fclose(fp);
            printErr("读取数据失败");
            return POLY_ERR_READ_FILE;
        }
        insertTerm(p, coef, exp);
    }
    fclose(fp);
    printOK("加载成功");
    return POLY_OK;
}

/* ======================== 菜单交互 ======================== */

static void printMenu(void) {
    cPrint(C_CYAN, "\n  ┌─────────────────────────────────────────┐\n");
    cPrint(C_CYAN, "  │"); cPrint(C_BOLD C_WHITE, "           主菜单 (Main Menu)           "); cPrint(C_CYAN, "│\n");
    cPrint(C_CYAN, "  ├─────────────────────────────────────────┤\n");
    cPrint(C_CYAN, "  │"); cPrint(C_YELLOW, "  1."); cPrint(C_WHITE, " 创建多项式 A                        "); cPrint(C_CYAN, "│\n");
    cPrint(C_CYAN, "  │"); cPrint(C_YELLOW, "  2."); cPrint(C_WHITE, " 创建多项式 B                        "); cPrint(C_CYAN, "│\n");
    cPrint(C_CYAN, "  │"); cPrint(C_YELLOW, "  3."); cPrint(C_WHITE, " 从文件加载多项式 A                  "); cPrint(C_CYAN, "│\n");
    cPrint(C_CYAN, "  │"); cPrint(C_YELLOW, "  4."); cPrint(C_WHITE, " 从文件加载多项式 B                  "); cPrint(C_CYAN, "│\n");
    cPrint(C_CYAN, "  │"); cPrint(C_YELLOW, "  5."); cPrint(C_WHITE, " 显示多项式 A                        "); cPrint(C_CYAN, "│\n");
    cPrint(C_CYAN, "  │"); cPrint(C_YELLOW, "  6."); cPrint(C_WHITE, " 显示多项式 B                        "); cPrint(C_CYAN, "│\n");
    cPrint(C_CYAN, "  │"); cPrint(C_YELLOW, "  7."); cPrint(C_WHITE, " A + B = C (加法)                    "); cPrint(C_CYAN, "│\n");
    cPrint(C_CYAN, "  │"); cPrint(C_YELLOW, "  8."); cPrint(C_WHITE, " A - B = C (减法)                    "); cPrint(C_CYAN, "│\n");
    cPrint(C_CYAN, "  │"); cPrint(C_YELLOW, "  9."); cPrint(C_WHITE, " A * B = C (乘法)                    "); cPrint(C_CYAN, "│\n");
    cPrint(C_CYAN, "  │"); cPrint(C_YELLOW, " 10."); cPrint(C_WHITE, " 求导 (A' 或 B')                     "); cPrint(C_CYAN, "│\n");
    cPrint(C_CYAN, "  │"); cPrint(C_YELLOW, " 11."); cPrint(C_WHITE, " 多项式求值 (代入x)                   "); cPrint(C_CYAN, "│\n");
    cPrint(C_CYAN, "  │"); cPrint(C_YELLOW, " 12."); cPrint(C_WHITE, " 显示多项式统计信息                  "); cPrint(C_CYAN, "│\n");
    cPrint(C_CYAN, "  │"); cPrint(C_YELLOW, " 13."); cPrint(C_WHITE, " 保存结果 C 到文件                   "); cPrint(C_CYAN, "│\n");
    cPrint(C_CYAN, "  │"); cPrint(C_YELLOW, " 14."); cPrint(C_WHITE, " C → A (结果复制到A继续运算)         "); cPrint(C_CYAN, "│\n");
    cPrint(C_CYAN, "  │"); cPrint(C_YELLOW, " 15."); cPrint(C_WHITE, " 交换 A 和 B                         "); cPrint(C_CYAN, "│\n");
    cPrint(C_CYAN, "  │"); cPrint(C_YELLOW, "  0."); cPrint(C_WHITE, " 退出程序                            "); cPrint(C_CYAN, "│\n");
    cPrint(C_CYAN, "  └─────────────────────────────────────────┘\n\n");
}

static void doInteractive(void) {
    Polynomial polyA, polyB, polyC;
    int choice, which;
    char filename[MAX_PATH_LEN];
    double x, result;
    initPoly(&polyA);
    initPoly(&polyB);
    initPoly(&polyC);
    printBanner();
    while (1) {
        printMenu();
        choice = readMenuChoice();
        switch (choice) {
        case 1:
            cPrint(C_BOLD C_MAGENTA, "\n  ── 创建多项式 A ──\n");
            createPoly(polyA);
            break;
        case 2:
            cPrint(C_BOLD C_MAGENTA, "\n  ── 创建多项式 B ──\n");
            createPoly(polyB);
            break;
        case 3:
            cPrint(C_BOLD C_MAGENTA, "\n  ── 加载多项式 A ──\n");
            printf("  请输入文件名: ");
            scanf("%s", filename);
            clearInputBuffer();
            loadPoly(polyA, filename);
            break;
        case 4:
            cPrint(C_BOLD C_MAGENTA, "\n  ── 加载多项式 B ──\n");
            printf("  请输入文件名: ");
            scanf("%s", filename);
            clearInputBuffer();
            loadPoly(polyB, filename);
            break;
        case 5:
            cPrint(C_BOLD C_MAGENTA, "\n  ── 多项式 A ──\n");
            printRaw(polyA);
            printMath(polyA);
            break;
        case 6:
            cPrint(C_BOLD C_MAGENTA, "\n  ── 多项式 B ──\n");
            printRaw(polyB);
            printMath(polyB);
            break;
        case 7:
            cPrint(C_BOLD C_MAGENTA, "\n  ── A + B = C ──\n");
            addPoly(polyA, polyB, polyC);
            printRaw(polyC);
            printMath(polyC);
            break;
        case 8:
            cPrint(C_BOLD C_MAGENTA, "\n  ── A - B = C ──\n");
            subtractPoly(polyA, polyB, polyC);
            printRaw(polyC);
            printMath(polyC);
            break;
        case 9:
            cPrint(C_BOLD C_MAGENTA, "\n  ── A * B = C ──\n");
            multiplyPoly(polyA, polyB, polyC);
            printRaw(polyC);
            printMath(polyC);
            break;
        case 10:
            cPrint(C_BOLD C_MAGENTA, "\n  ── 求导 ──\n");
            printf("  求哪个多项式的导? (1=A, 2=B): ");
            scanf("%d", &which);
            clearInputBuffer();
            if (which == 1) {
                derivativePoly(polyA, polyC);
                cPrint(C_CYAN, "  A' = ");
            } else if (which == 2) {
                derivativePoly(polyB, polyC);
                cPrint(C_CYAN, "  B' = ");
            } else {
                printErr("无效选择");
                break;
            }
            printRaw(polyC);
            printMath(polyC);
            break;
        case 11:
            cPrint(C_BOLD C_MAGENTA, "\n  ── 多项式求值 ──\n");
            printf("  求哪个多项式的值? (1=A, 2=B, 3=C): ");
            scanf("%d", &which);
            printf("  请输入 x 的值: ");
            scanf("%lf", &x);
            clearInputBuffer();
            if (which == 1) result = evaluatePoly(polyA, x);
            else if (which == 2) result = evaluatePoly(polyB, x);
            else if (which == 3) result = evaluatePoly(polyC, x);
            else { printErr("无效选择"); break; }
            printf("  ");
            cPrint(C_CYAN, "f(");
            cPrint(C_YELLOW, "%g", x);
            cPrint(C_CYAN, ") = ");
            cPrint(C_GREEN C_BOLD, "%.10g\n", result);
            break;
        case 12:
            cPrint(C_BOLD C_MAGENTA, "\n  ── 统计信息 ──\n");
            printStats(polyA, "多项式 A");
            printStats(polyB, "多项式 B");
            if (!isZero(polyC)) printStats(polyC, "结果 C");
            break;
        case 13:
            cPrint(C_BOLD C_MAGENTA, "\n  ── 保存结果 C ──\n");
            if (isZero(polyC)) {
                printWarn("结果 C 为空 (零多项式)，仍保存");
            }
            printf("  请输入保存文件名 (.poly): ");
            scanf("%s", filename);
            clearInputBuffer();
            savePoly(polyC, filename);
            break;
        case 14:
            cPrint(C_BOLD C_MAGENTA, "\n  ── C → A ──\n");
            copyPoly(polyC, polyA);
            printOK("已将结果 C 复制到 A");
            break;
        case 15:
            swapPolys(&polyA, &polyB);
            printOK("已交换 A 和 B");
            break;
        case 0:
            cPrint(C_GREEN C_BOLD, "\n  感谢使用，再见！\n\n");
            destroyPoly(&polyA);
            destroyPoly(&polyB);
            destroyPoly(&polyC);
            return;
        default:
            printErr("无效选择，请输入 0-15");
        }
        pauseScreen();
    }
}

/* ======================== 命令行处理 ======================== */

static void printHelp(void) {
    printf("一元稀疏多项式计算器 V2.0\n");
    printf("用法:\n");
    printf("  poly.exe                  进入交互模式\n");
    printf("  poly.exe --help           显示本帮助\n");
    printf("  poly.exe --test           运行内置自检\n");
    printf("  poly.exe -a <f1> <f2>     加法: f1 + f2\n");
    printf("  poly.exe -s <f1> <f2>     减法: f1 - f2\n");
    printf("  poly.exe -m <f1> <f2>     乘法: f1 * f2\n");
    printf("  poly.exe -d <f>           求导\n");
    printf("  poly.exe -e <f> <x>       求值\n");
    printf("  poly.exe -t <f>           显示统计\n");
    printf("  poly.exe ... -o <out>     输出到文件.poly\n");
}

static int runTests(void) {
    Polynomial a, b, c;
    double v;
    int passed = 0, total = 0;
    printf("Running self-tests...\n");
    initPoly(&a); initPoly(&b); initPoly(&c);
    insertTerm(a, 3.0, 2); insertTerm(a, -2.0, 1); insertTerm(a, 1.0, 0);
    insertTerm(b, 1.0, 2); insertTerm(b, 1.0, 0);
    total++; addPoly(a, b, c);
    v = evaluatePoly(c, 1.0);
    if (my_fabs(v - 4.0) < EPS) { passed++; printf("[PASS] add: (3x^2-2x+1)+(x^2+1)=4x^2-2x+2, f(1)=4\n"); }
    else { printf("[FAIL] add: expected 4 got %g\n", v); }
    total++; subtractPoly(a, b, c);
    v = evaluatePoly(c, 1.0);
    if (my_fabs(v - 0.0) < EPS) { passed++; printf("[PASS] subtract: f(1)=0\n"); }
    else { printf("[FAIL] subtract: expected 0 got %g\n", v); }
    total++; multiplyPoly(a, b, c);
    v = evaluatePoly(c, 2.0);
    if (my_fabs(v - 45.0) < EPS) { passed++; printf("[PASS] multiply: f(2)=45\n"); }
    else { printf("[FAIL] multiply: expected 45 got %g\n", v); }
    total++; derivativePoly(a, c);
    v = evaluatePoly(c, 1.0);
    if (my_fabs(v - 4.0) < EPS) { passed++; printf("[PASS] derivative: (3x^2-2x+1)'=6x-2, f(1)=4\n"); }
    else { printf("[FAIL] derivative: expected 4 got %g\n", v); }
    savePoly(a, "_test.poly");
    destroyPoly(&b->next);
    total++; loadPoly(b, "_test.poly");
    v = evaluatePoly(b, 1.0);
    if (my_fabs(v - 2.0) < EPS) { passed++; printf("[PASS] save/load: f(1)=2\n"); }
    else { printf("[FAIL] save/load: expected 2 got %g\n", v); }
    remove("_test.poly");
    destroyPoly(&a); destroyPoly(&b); destroyPoly(&c);
    printf("\nTests: %d/%d passed\n", passed, total);
    return (passed == total) ? 0 : 1;
}

int main(int argc, char *argv[]) {
    int i;
    const char *outFile = NULL;
    enableVTMode();
    if (argc < 2) {
        doInteractive();
        return 0;
    }
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printHelp();
            return 0;
        }
        if (strcmp(argv[i], "--test") == 0) {
            return runTests();
        }
    }
    if (argc >= 3 && (strcmp(argv[1], "-a") == 0 || strcmp(argv[1], "-s") == 0 ||
                      strcmp(argv[1], "-m") == 0) && argc >= 4) {
        Polynomial a, b, c;
        initPoly(&a); initPoly(&b); initPoly(&c);
        for (i = 4; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
                outFile = argv[i + 1];
                i++;
            }
        }
        if (loadPoly(a, argv[2]) != POLY_OK || loadPoly(b, argv[3]) != POLY_OK) {
            destroyPoly(&a); destroyPoly(&b); destroyPoly(&c);
            return 1;
        }
        if (strcmp(argv[1], "-a") == 0) addPoly(a, b, c);
        else if (strcmp(argv[1], "-s") == 0) subtractPoly(a, b, c);
        else multiplyPoly(a, b, c);
        printMath(c);
        if (outFile) savePoly(c, outFile);
        destroyPoly(&a); destroyPoly(&b); destroyPoly(&c);
        return 0;
    }
    if (argc >= 3 && strcmp(argv[1], "-d") == 0) {
        Polynomial a, c;
        initPoly(&a); initPoly(&c);
        for (i = 3; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) { outFile = argv[i + 1]; i++; }
        }
        if (loadPoly(a, argv[2]) != POLY_OK) { destroyPoly(&a); destroyPoly(&c); return 1; }
        derivativePoly(a, c);
        printMath(c);
        if (outFile) savePoly(c, outFile);
        destroyPoly(&a); destroyPoly(&c);
        return 0;
    }
    if (argc >= 4 && strcmp(argv[1], "-e") == 0) {
        Polynomial a;
        double x, v;
        initPoly(&a);
        if (loadPoly(a, argv[2]) != POLY_OK) { destroyPoly(&a); return 1; }
        x = atof(argv[3]);
        v = evaluatePoly(a, x);
        printf("%.10g\n", v);
        destroyPoly(&a);
        return 0;
    }
    if (argc >= 3 && strcmp(argv[1], "-t") == 0) {
        Polynomial a;
        PolyStats st;
        initPoly(&a);
        if (loadPoly(a, argv[2]) != POLY_OK) { destroyPoly(&a); return 1; }
        getStats(a, &st);
        printf("termCount=%d degree=%d leadingCoef=%.10g constTerm=%.10g\n",
               st.termCount, st.degree, st.leadingCoef, st.constTerm);
        destroyPoly(&a);
        return 0;
    }
    printHelp();
    return 1;
}
