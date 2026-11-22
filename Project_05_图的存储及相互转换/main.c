/*
* ============================================================================
*
*    图的存储及相互转换系统  (V2.0 增强版)
*    Graph Storage & Conversion System  (V2.0 Enhanced Edition)
*
* ============================================================================
*
*  功能概述:
*    实现图的两种经典存储结构（邻接矩阵、邻接表）及其双向转换，支持：
*      [1] 动态内存分配（无固定顶点上限，按需分配）
*      [2] 邻接矩阵 <-> 邻接表 双向无损转换
*      [3] 有向图/无向图、带权图/无权图 四种组合
*      [4] BFS广度优先搜索、DFS深度优先搜索（彩色输出访问顺序）
*      [5] 最短路径：无权图BFS、带权图Dijkstra算法
*      [6] 图统计信息：顶点数、边数、密度、入度/出度（有向图）
*      [7] 彩色终端界面（ANSI颜色，Windows 10+ / Linux）
*      [8] 文件持久化（含魔数头校验，防止非法文件）
*      [9] 命令行模式（非交互调用）
*     [10] 内置演示测试用例
*
*  数据结构:
*    ArcNode      —— 邻接表边结点（链表结点）
*    VNode        —— 邻接表顶点结点（顶点名 + 边表头指针）
*    AdjListGraph —— 邻接表图结构（动态顶点数组 + 统计信息）
*    MatrixGraph  —— 邻接矩阵图结构（动态二维矩阵 + 顶点名数组）
*    GraphHeader  —— 持久化文件头（魔数 + 版本 + 元信息）
*
*  编译命令:
*    gcc -std=c99 -Wall -O2 -o graph.exe main.c
*
*  命令行用法:
*    graph.exe                   交互模式（菜单）
*    graph.exe -h / --help       显示帮助
*    graph.exe -t / --test       运行内置测试用例
*
*  编码: UTF-8
* ============================================================================
*/

/* ======================== 标准头文件 ======================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <limits.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define F_OK 0
#define access _access
#else
#include <unistd.h>
#endif

/* ======================== 常量与宏 ======================== */
#define MAX_NAMELEN     64
#define GRAPH_MAGIC     "GRPH"
#define GRAPH_VERSION   2
#define INF_WEIGHT      INT_MAX / 2

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
#define BG_MAGENTA  "\033[45m"
#define BG_CYAN     "\033[46m"

/* ======================== 错误码 ======================== */
typedef enum {
    GRAPH_OK = 0,
    GRAPH_ERR_OPEN_FILE,
    GRAPH_ERR_READ_FILE,
    GRAPH_ERR_WRITE_FILE,
    GRAPH_ERR_MEMORY,
    GRAPH_ERR_CORRUPT,
    GRAPH_ERR_MAGIC,
    GRAPH_ERR_VERSION,
    GRAPH_ERR_EMPTY,
    GRAPH_ERR_VERTEX_NOT_FOUND,
    GRAPH_ERR_INVALID_PARAM,
    GRAPH_ERR_CANCEL,
} GraphStatus;

/* ======================== 数据结构 ======================== */

typedef struct ArcNode {
    int             adjvex;
    int             weight;
    struct ArcNode *next;
} ArcNode;

typedef struct {
    char      vexname[MAX_NAMELEN];
    ArcNode  *firstarc;
} VNode;

typedef struct {
    VNode   *vertices;
    int      vexnum;
    int      arcnum;
    int      directed;
    int      weighted;
    int      capacity;
} AdjListGraph;

typedef struct {
    char   (*vexname)[MAX_NAMELEN];
    int   **matrix;
    int     vexnum;
    int     arcnum;
    int     directed;
    int     weighted;
    int     capacity;
} MatrixGraph;

typedef struct {
    char   magic[4];
    unsigned short version;
    int    vexnum;
    int    arcnum;
    int    directed;
    int    weighted;
} GraphHeader;

/* ======================== 全局状态 ======================== */
static int colorEnabled = 1;

/* ======================== 前向声明 ======================== */
static void adjlist_to_matrix(AdjListGraph *lg, MatrixGraph *mg);
static void matrix_to_adjlist(MatrixGraph *mg, AdjListGraph *lg);

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
        "  |            Graph Storage & Conversion System  (V2.0 Enhanced)            |\n");
    cPrint(C_BOLD C_CYAN,
        "  |                       图的存储及相互转换系统  V2.0 增强版                 |\n");
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

/* ======================== 邻接表内存管理 ======================== */

static void init_adjlist_graph(AdjListGraph *g, int directed, int weighted, int init_cap) {
    if (init_cap < 4) init_cap = 4;
    g->vertices = (VNode *)calloc((size_t)init_cap, sizeof(VNode));
    if (!g->vertices) {
        g->capacity = 0; g->vexnum = 0; g->arcnum = 0;
        g->directed = directed; g->weighted = weighted;
        return;
    }
    g->capacity = init_cap;
    g->vexnum = 0;
    g->arcnum = 0;
    g->directed = directed;
    g->weighted = weighted;
    for (int i = 0; i < init_cap; i++) {
        g->vertices[i].firstarc = NULL;
        g->vertices[i].vexname[0] = '\0';
    }
}

static int ensure_adjlist_capacity(AdjListGraph *g, int needed) {
    if (needed <= g->capacity) return 1;
    int new_cap = g->capacity;
    while (new_cap < needed) new_cap *= 2;
    VNode *new_v = (VNode *)realloc(g->vertices, (size_t)new_cap * sizeof(VNode));
    if (!new_v) return 0;
    for (int i = g->capacity; i < new_cap; i++) {
        new_v[i].firstarc = NULL;
        new_v[i].vexname[0] = '\0';
    }
    g->vertices = new_v;
    g->capacity = new_cap;
    return 1;
}

static void free_adjlist_graph(AdjListGraph *g) {
    if (!g) return;
    if (g->vertices) {
        for (int i = 0; i < g->vexnum; i++) {
            ArcNode *p = g->vertices[i].firstarc;
            while (p) {
                ArcNode *tmp = p;
                p = p->next;
                free(tmp);
            }
            g->vertices[i].firstarc = NULL;
        }
        free(g->vertices);
        g->vertices = NULL;
    }
    g->vexnum = 0;
    g->arcnum = 0;
    g->capacity = 0;
}

static int find_vertex_adjlist(AdjListGraph *g, const char *name) {
    for (int i = 0; i < g->vexnum; i++) {
        if (strcmp(g->vertices[i].vexname, name) == 0) return i;
    }
    return -1;
}

static int add_vertex_adjlist(AdjListGraph *g, const char *name) {
    int idx = find_vertex_adjlist(g, name);
    if (idx >= 0) return idx;
    if (!ensure_adjlist_capacity(g, g->vexnum + 1)) return -1;
    snprintf(g->vertices[g->vexnum].vexname, MAX_NAMELEN, "%s", name);
    g->vertices[g->vexnum].firstarc = NULL;
    g->vexnum++;
    return g->vexnum - 1;
}

static void add_arc_adjlist(AdjListGraph *g, int from, int to, int weight) {
    if (from < 0 || from >= g->vexnum || to < 0 || to >= g->vexnum) return;
    ArcNode *node = (ArcNode *)malloc(sizeof(ArcNode));
    if (!node) return;
    node->adjvex = to;
    node->weight = weight;
    node->next = g->vertices[from].firstarc;
    g->vertices[from].firstarc = node;
}

/* ======================== 邻接矩阵内存管理 ======================== */

static void init_matrix_graph(MatrixGraph *g, int directed, int weighted, int init_cap) {
    if (init_cap < 4) init_cap = 4;
    g->vexname = (char (*)[MAX_NAMELEN])calloc((size_t)init_cap, sizeof(char[MAX_NAMELEN]));
    g->matrix = (int **)calloc((size_t)init_cap, sizeof(int *));
    if (!g->vexname || !g->matrix) {
        if (g->vexname) free(g->vexname);
        if (g->matrix) free(g->matrix);
        g->vexname = NULL; g->matrix = NULL;
        g->capacity = 0; g->vexnum = 0; g->arcnum = 0;
        g->directed = directed; g->weighted = weighted;
        return;
    }
    for (int i = 0; i < init_cap; i++) {
        g->matrix[i] = (int *)calloc((size_t)init_cap, sizeof(int));
        if (!g->matrix[i]) {
            for (int j = 0; j < i; j++) free(g->matrix[j]);
            free(g->matrix); free(g->vexname);
            g->matrix = NULL; g->vexname = NULL;
            g->capacity = 0; return;
        }
    }
    g->capacity = init_cap;
    g->vexnum = 0;
    g->arcnum = 0;
    g->directed = directed;
    g->weighted = weighted;
}

static int ensure_matrix_capacity(MatrixGraph *g, int needed) {
    if (needed <= g->capacity) return 1;
    int new_cap = g->capacity;
    while (new_cap < needed) new_cap *= 2;
    char (*new_vex)[MAX_NAMELEN] = (char (*)[MAX_NAMELEN])realloc(g->vexname, (size_t)new_cap * sizeof(char[MAX_NAMELEN]));
    int **new_mat = (int **)realloc(g->matrix, (size_t)new_cap * sizeof(int *));
    if (!new_vex || !new_mat) return 0;
    g->vexname = new_vex;
    g->matrix = new_mat;
    for (int i = 0; i < g->capacity; i++) {
        int *new_row = (int *)realloc(g->matrix[i], (size_t)new_cap * sizeof(int));
        if (!new_row) return 0;
        for (int j = g->capacity; j < new_cap; j++) new_row[j] = 0;
        g->matrix[i] = new_row;
    }
    for (int i = g->capacity; i < new_cap; i++) {
        g->vexname[i][0] = '\0';
        g->matrix[i] = (int *)calloc((size_t)new_cap, sizeof(int));
        if (!g->matrix[i]) return 0;
    }
    g->capacity = new_cap;
    return 1;
}

static void free_matrix_graph(MatrixGraph *g) {
    if (!g) return;
    if (g->matrix) {
        for (int i = 0; i < g->capacity; i++) {
            if (g->matrix[i]) free(g->matrix[i]);
        }
        free(g->matrix);
        g->matrix = NULL;
    }
    if (g->vexname) {
        free(g->vexname);
        g->vexname = NULL;
    }
    g->vexnum = 0;
    g->arcnum = 0;
    g->capacity = 0;
}

static int find_vertex_matrix(MatrixGraph *g, const char *name) {
    for (int i = 0; i < g->vexnum; i++) {
        if (strcmp(g->vexname[i], name) == 0) return i;
    }
    return -1;
}

static int add_vertex_matrix(MatrixGraph *g, const char *name) {
    int idx = find_vertex_matrix(g, name);
    if (idx >= 0) return idx;
    if (!ensure_matrix_capacity(g, g->vexnum + 1)) return -1;
    snprintf(g->vexname[g->vexnum], MAX_NAMELEN, "%s", name);
    for (int i = 0; i <= g->vexnum; i++) {
        g->matrix[g->vexnum][i] = 0;
        g->matrix[i][g->vexnum] = 0;
    }
    g->vexnum++;
    return g->vexnum - 1;
}

/* ======================== 结构转换 ======================== */

static void adjlist_to_matrix(AdjListGraph *lg, MatrixGraph *mg) {
    free_matrix_graph(mg);
    init_matrix_graph(mg, lg->directed, lg->weighted, lg->vexnum > 0 ? lg->vexnum : 4);
    mg->vexnum = lg->vexnum;
    mg->arcnum = lg->arcnum;
    for (int i = 0; i < lg->vexnum; i++) {
        snprintf(mg->vexname[i], MAX_NAMELEN, "%s", lg->vertices[i].vexname);
    }
    for (int i = 0; i < lg->vexnum; i++) {
        ArcNode *p = lg->vertices[i].firstarc;
        while (p) {
            int w = lg->weighted ? p->weight : 1;
            mg->matrix[i][p->adjvex] = w;
            p = p->next;
        }
    }
}

static void matrix_to_adjlist(MatrixGraph *mg, AdjListGraph *lg) {
    free_adjlist_graph(lg);
    init_adjlist_graph(lg, mg->directed, mg->weighted, mg->vexnum > 0 ? mg->vexnum : 4);
    lg->vexnum = mg->vexnum;
    lg->arcnum = mg->arcnum;
    for (int i = 0; i < mg->vexnum; i++) {
        snprintf(lg->vertices[i].vexname, MAX_NAMELEN, "%s", mg->vexname[i]);
        lg->vertices[i].firstarc = NULL;
    }
    for (int i = 0; i < mg->vexnum; i++) {
        for (int j = mg->vexnum - 1; j >= 0; j--) {
            if (mg->matrix[i][j] != 0) {
                add_arc_adjlist(lg, i, j, mg->matrix[i][j]);
            }
        }
    }
}

/* ======================== 显示/打印 ======================== */

static void print_adjlist_graph(AdjListGraph *g) {
    cPrint(C_BOLD C_CYAN, "\n  ===== 邻接表展示 =====\n");
    printf("  图类型: %s %s图\n",
           g->directed ? "有向" : "无向",
           g->weighted ? "带权" : "无权");
    printf("  顶点数: "); cPrint(C_WHITE, "%d", g->vexnum);
    printf("  边数: "); cPrint(C_WHITE, "%d\n", g->arcnum);
    printDivider();
    for (int i = 0; i < g->vexnum; i++) {
        cPrint(C_BOLD C_YELLOW, "  [%d] %s", i, g->vertices[i].vexname);
        printf(" ->");
        ArcNode *p = g->vertices[i].firstarc;
        int cnt = 0;
        while (p) {
            if (g->weighted) {
                cPrint(C_GREEN, " %s", g->vertices[p->adjvex].vexname);
                cPrint(C_CYAN, "(%d)", p->weight);
            } else {
                cPrint(C_GREEN, " %s", g->vertices[p->adjvex].vexname);
            }
            p = p->next;
            cnt++;
            if (p) printf(" ->");
        }
        if (cnt == 0) cPrint(C_GRAY, " (无出边)");
        printf("\n");
    }
    printDivider();
}

static void print_matrix_graph(MatrixGraph *g) {
    cPrint(C_BOLD C_CYAN, "\n  ===== 邻接矩阵展示 =====\n");
    printf("  图类型: %s %s图\n",
           g->directed ? "有向" : "无向",
           g->weighted ? "带权" : "无权");
    printf("  顶点数: "); cPrint(C_WHITE, "%d", g->vexnum);
    printf("  边数: "); cPrint(C_WHITE, "%d\n", g->arcnum);
    printDivider();

    int w = 6;
    printf("  %*s", w, "");
    for (int j = 0; j < g->vexnum; j++) {
        cPrint(C_BOLD C_YELLOW, "%*s", w, g->vexname[j]);
    }
    printf("\n");

    for (int i = 0; i < g->vexnum; i++) {
        cPrint(C_BOLD C_YELLOW, "  %*s ", w-2, g->vexname[i]);
        for (int j = 0; j < g->vexnum; j++) {
            int val = g->matrix[i][j];
            if (g->weighted && val == 0) {
                cPrint(C_GRAY, "%*s", w, "inf");
            } else if (val == 0) {
                cPrint(C_GRAY, "%*d", w, val);
            } else {
                cPrint(C_GREEN, "%*d", w, val);
            }
        }
        printf("\n");
    }
    printDivider();
}

/* ======================== 图统计 ======================== */

static void print_graph_stats(AdjListGraph *lg, MatrixGraph *mg) {
    int use_lg = (lg != NULL && lg->vexnum > 0);
    int vexnum = use_lg ? lg->vexnum : (mg ? mg->vexnum : 0);
    int arcnum = use_lg ? lg->arcnum : (mg ? mg->arcnum : 0);
    int directed = use_lg ? lg->directed : (mg ? mg->directed : 0);
    int weighted = use_lg ? lg->weighted : (mg ? mg->weighted : 0);

    if (vexnum == 0) {
        printWarn("当前没有图数据");
        return;
    }

    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  图统计信息\n");
    printDivider();
    printf("  顶点数 (V):       "); cPrint(C_WHITE, "%d\n", vexnum);
    printf("  边数   (E):       "); cPrint(C_WHITE, "%d\n", arcnum);
    printf("  图类型:           "); cPrint(C_WHITE, "%s %s图\n", directed?"有向":"无向", weighted?"带权":"无权");
    double max_edges = directed ? (double)vexnum * (vexnum - 1) : (double)vexnum * (vexnum - 1) / 2.0;
    double density = (max_edges > 0) ? (double)arcnum / max_edges : 0.0;
    printf("  图密度:           ");
    if (density < 0.3) cPrint(C_GREEN, "%.4f  (稀疏图)", density);
    else if (density < 0.7) cPrint(C_YELLOW, "%.4f  (中等密度)", density);
    else cPrint(C_RED, "%.4f  (稠密图)", density);
    printf("\n");

    if (directed && use_lg) {
        printDivider();
        cPrint(C_BOLD, "  %-12s  %-8s  %-8s\n", "顶点", "出度", "入度");
        printDivider();
        int *in_deg = (int *)calloc((size_t)vexnum, sizeof(int));
        int *out_deg = (int *)calloc((size_t)vexnum, sizeof(int));
        if (in_deg && out_deg) {
            for (int i = 0; i < vexnum; i++) {
                ArcNode *p = lg->vertices[i].firstarc;
                while (p) {
                    out_deg[i]++;
                    in_deg[p->adjvex]++;
                    p = p->next;
                }
            }
            for (int i = 0; i < vexnum; i++) {
                printf("  %-12s  ", lg->vertices[i].vexname);
                cPrint(C_CYAN, "%8d", out_deg[i]);
                printf("  ");
                cPrint(C_MAGENTA, "%8d\n", in_deg[i]);
            }
            free(in_deg); free(out_deg);
        }
        printDivider();
    } else if (!directed && use_lg) {
        printDivider();
        cPrint(C_BOLD, "  %-12s  %-8s\n", "顶点", "度");
        printDivider();
        for (int i = 0; i < vexnum; i++) {
            int deg = 0;
            ArcNode *p = lg->vertices[i].firstarc;
            while (p) { deg++; p = p->next; }
            printf("  %-12s  ", lg->vertices[i].vexname);
            cPrint(C_CYAN, "%8d\n", deg);
        }
        printDivider();
    }
    printThickDivider();
}

/* ======================== BFS/DFS 遍历 ======================== */

static void bfs_adjlist(AdjListGraph *g, int start) {
    if (g->vexnum == 0 || start < 0 || start >= g->vexnum) {
        printErr("无效的起始顶点");
        return;
    }
    int *visited = (int *)calloc((size_t)g->vexnum, sizeof(int));
    int *queue = (int *)malloc((size_t)g->vexnum * sizeof(int));
    if (!visited || !queue) { printErr("内存不足"); free(visited); free(queue); return; }

    int front = 0, rear = 0, order = 0;
    cPrint(C_BOLD C_MAGENTA, "\n  BFS 广度优先遍历 (从 %s 开始):\n", g->vertices[start].vexname);
    printDivider();
    printf("  ");

    visited[start] = 1;
    queue[rear++] = start;
    while (front < rear) {
        int v = queue[front++];
        order++;
        cPrint(C_BOLD C_GREEN, "%d:", order);
        cPrint(C_BOLD C_YELLOW, "%s", g->vertices[v].vexname);
        if (front < rear || order == 1) printf(" -> ");
        ArcNode *p = g->vertices[v].firstarc;
        while (p) {
            if (!visited[p->adjvex]) {
                visited[p->adjvex] = 1;
                queue[rear++] = p->adjvex;
            }
            p = p->next;
        }
    }
    printf("\n");
    printDivider();

    int unvisited = 0;
    for (int i = 0; i < g->vexnum; i++) if (!visited[i]) unvisited++;
    if (unvisited > 0) {
        cPrint(C_YELLOW, "  [INFO] 还有 %d 个顶点不可达（非连通图）\n", unvisited);
    }
    free(visited); free(queue);
}

static void dfs_adjlist_recursive(AdjListGraph *g, int v, int *visited, int *order) {
    visited[v] = 1;
    (*order)++;
    cPrint(C_BOLD C_GREEN, "%d:", *order);
    cPrint(C_BOLD C_YELLOW, "%s", g->vertices[v].vexname);
    printf(" -> ");
    ArcNode *p = g->vertices[v].firstarc;
    while (p) {
        if (!visited[p->adjvex]) {
            dfs_adjlist_recursive(g, p->adjvex, visited, order);
        }
        p = p->next;
    }
}

static void dfs_adjlist(AdjListGraph *g, int start) {
    if (g->vexnum == 0 || start < 0 || start >= g->vexnum) {
        printErr("无效的起始顶点");
        return;
    }
    int *visited = (int *)calloc((size_t)g->vexnum, sizeof(int));
    if (!visited) { printErr("内存不足"); return; }

    int order = 0;
    cPrint(C_BOLD C_MAGENTA, "\n  DFS 深度优先遍历 (从 %s 开始):\n", g->vertices[start].vexname);
    printDivider();
    printf("  ");
    dfs_adjlist_recursive(g, start, visited, &order);
    cPrint(C_GRAY, "(结束)\n");
    printDivider();

    int unvisited = 0;
    for (int i = 0; i < g->vexnum; i++) if (!visited[i]) unvisited++;
    if (unvisited > 0) {
        cPrint(C_YELLOW, "  [INFO] 还有 %d 个顶点不可达（非连通图）\n", unvisited);
    }
    free(visited);
}

/* ======================== 最短路径 ======================== */

static void shortest_path_bfs(AdjListGraph *g, int start, int end) {
    if (g->vexnum == 0) { printErr("图为空"); return; }
    int *dist = (int *)malloc((size_t)g->vexnum * sizeof(int));
    int *prev = (int *)malloc((size_t)g->vexnum * sizeof(int));
    int *queue = (int *)malloc((size_t)g->vexnum * sizeof(int));
    if (!dist || !prev || !queue) { printErr("内存不足"); free(dist); free(prev); free(queue); return; }

    for (int i = 0; i < g->vexnum; i++) { dist[i] = -1; prev[i] = -1; }
    int front = 0, rear = 0;
    dist[start] = 0;
    queue[rear++] = start;

    while (front < rear) {
        int v = queue[front++];
        if (v == end) break;
        ArcNode *p = g->vertices[v].firstarc;
        while (p) {
            if (dist[p->adjvex] == -1) {
                dist[p->adjvex] = dist[v] + 1;
                prev[p->adjvex] = v;
                queue[rear++] = p->adjvex;
            }
            p = p->next;
        }
    }

    cPrint(C_BOLD C_CYAN, "\n  BFS 最短路径 (%s -> %s):\n", g->vertices[start].vexname, g->vertices[end].vexname);
    printDivider();
    if (dist[end] == -1) {
        cPrint(C_RED, "  两点不可达！\n");
    } else {
        printf("  路径长度: "); cPrint(C_GREEN, "%d 步\n", dist[end]);
        printf("  路径: ");
        int path[1024]; int pl = 0;
        for (int v = end; v != -1; v = prev[v]) { path[pl++] = v; }
        for (int i = pl - 1; i >= 0; i--) {
            cPrint(C_BOLD C_YELLOW, "%s", g->vertices[path[i]].vexname);
            if (i > 0) cPrint(C_CYAN, " -> ");
        }
        printf("\n");
    }
    printDivider();
    free(dist); free(prev); free(queue);
}

static void shortest_path_dijkstra(AdjListGraph *g, int start, int end) {
    if (g->vexnum == 0) { printErr("图为空"); return; }
    int *dist = (int *)malloc((size_t)g->vexnum * sizeof(int));
    int *prev = (int *)malloc((size_t)g->vexnum * sizeof(int));
    int *visited = (int *)calloc((size_t)g->vexnum, sizeof(int));
    if (!dist || !prev || !visited) { printErr("内存不足"); free(dist); free(prev); free(visited); return; }

    for (int i = 0; i < g->vexnum; i++) { dist[i] = INF_WEIGHT; prev[i] = -1; }
    dist[start] = 0;

    for (int i = 0; i < g->vexnum; i++) {
        int u = -1, min_d = INF_WEIGHT;
        for (int j = 0; j < g->vexnum; j++) {
            if (!visited[j] && dist[j] < min_d) { min_d = dist[j]; u = j; }
        }
        if (u == -1) break;
        visited[u] = 1;
        if (u == end) break;
        ArcNode *p = g->vertices[u].firstarc;
        while (p) {
            if (!visited[p->adjvex] && dist[u] + p->weight < dist[p->adjvex]) {
                dist[p->adjvex] = dist[u] + p->weight;
                prev[p->adjvex] = u;
            }
            p = p->next;
        }
    }

    cPrint(C_BOLD C_CYAN, "\n  Dijkstra 最短路径 (%s -> %s):\n", g->vertices[start].vexname, g->vertices[end].vexname);
    printDivider();
    if (dist[end] == INF_WEIGHT) {
        cPrint(C_RED, "  两点不可达！\n");
    } else {
        printf("  路径总权值: "); cPrint(C_GREEN, "%d\n", dist[end]);
        printf("  路径: ");
        int path[1024]; int pl = 0;
        for (int v = end; v != -1; v = prev[v]) { path[pl++] = v; }
        for (int i = pl - 1; i >= 0; i--) {
            cPrint(C_BOLD C_YELLOW, "%s", g->vertices[path[i]].vexname);
            if (i > 0) cPrint(C_CYAN, " -> ");
        }
        printf("\n");
    }
    printDivider();
    free(dist); free(prev); free(visited);
}

/* ======================== 文件持久化（带魔数头） ======================== */

static GraphStatus save_adjlist_to_file(AdjListGraph *g, const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) { printErr("无法打开文件写入"); return GRAPH_ERR_OPEN_FILE; }

    GraphHeader hdr;
    memcpy(hdr.magic, GRAPH_MAGIC, 4);
    hdr.version = GRAPH_VERSION;
    hdr.vexnum = g->vexnum;
    hdr.arcnum = g->arcnum;
    hdr.directed = g->directed;
    hdr.weighted = g->weighted;
    fwrite(&hdr, sizeof(hdr), 1, fp);

    for (int i = 0; i < g->vexnum; i++) {
        unsigned char len = (unsigned char)strlen(g->vertices[i].vexname);
        fwrite(&len, 1, 1, fp);
        fwrite(g->vertices[i].vexname, 1, len, fp);
    }

    for (int i = 0; i < g->vexnum; i++) {
        ArcNode *p = g->vertices[i].firstarc;
        while (p) {
            if (g->directed || i < p->adjvex) {
                fwrite(&i, sizeof(int), 1, fp);
                fwrite(&p->adjvex, sizeof(int), 1, fp);
                fwrite(&p->weight, sizeof(int), 1, fp);
            }
            p = p->next;
        }
    }

    fclose(fp);
    printOK("邻接表已保存到文件");
    return GRAPH_OK;
}

static GraphStatus save_matrix_to_file(MatrixGraph *g, const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) { printErr("无法打开文件写入"); return GRAPH_ERR_OPEN_FILE; }

    GraphHeader hdr;
    memcpy(hdr.magic, GRAPH_MAGIC, 4);
    hdr.version = GRAPH_VERSION;
    hdr.vexnum = g->vexnum;
    hdr.arcnum = g->arcnum;
    hdr.directed = g->directed;
    hdr.weighted = g->weighted;
    fwrite(&hdr, sizeof(hdr), 1, fp);

    for (int i = 0; i < g->vexnum; i++) {
        unsigned char len = (unsigned char)strlen(g->vexname[i]);
        fwrite(&len, 1, 1, fp);
        fwrite(g->vexname[i], 1, len, fp);
    }

    for (int i = 0; i < g->vexnum; i++) {
        fwrite(g->matrix[i], sizeof(int), (size_t)g->vexnum, fp);
    }

    fclose(fp);
    printOK("邻接矩阵已保存到文件");
    return GRAPH_OK;
}

static GraphStatus load_adjlist_from_file(AdjListGraph *g, const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) { printErr("无法打开文件"); return GRAPH_ERR_OPEN_FILE; }

    GraphHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) { fclose(fp); printErr("读取文件头失败"); return GRAPH_ERR_CORRUPT; }
    if (memcmp(hdr.magic, GRAPH_MAGIC, 4) != 0) { fclose(fp); printErr("无效的图文件（魔数错误）"); return GRAPH_ERR_MAGIC; }
    if (hdr.version > GRAPH_VERSION) { fclose(fp); printErr("不支持的文件版本"); return GRAPH_ERR_VERSION; }

    free_adjlist_graph(g);
    init_adjlist_graph(g, hdr.directed, hdr.weighted, hdr.vexnum > 0 ? hdr.vexnum : 4);
    g->vexnum = hdr.vexnum;
    g->arcnum = hdr.arcnum;

    for (int i = 0; i < hdr.vexnum; i++) {
        unsigned char len;
        if (fread(&len, 1, 1, fp) != 1) { fclose(fp); return GRAPH_ERR_CORRUPT; }
        if (fread(g->vertices[i].vexname, 1, len, fp) != len) { fclose(fp); return GRAPH_ERR_CORRUPT; }
        g->vertices[i].vexname[len] = '\0';
        g->vertices[i].firstarc = NULL;
    }

    int edges_to_read = g->directed ? g->arcnum : g->arcnum;
    for (int i = 0; i < edges_to_read; i++) {
        int from, to, w;
        if (fread(&from, sizeof(int), 1, fp) != 1 ||
            fread(&to, sizeof(int), 1, fp) != 1 ||
            fread(&w, sizeof(int), 1, fp) != 1) break;
        add_arc_adjlist(g, from, to, w);
        if (!g->directed) add_arc_adjlist(g, to, from, w);
    }

    fclose(fp);
    printOK("邻接表加载成功");
    return GRAPH_OK;
}

static GraphStatus load_matrix_from_file(MatrixGraph *g, const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) { printErr("无法打开文件"); return GRAPH_ERR_OPEN_FILE; }

    GraphHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) { fclose(fp); printErr("读取文件头失败"); return GRAPH_ERR_CORRUPT; }
    if (memcmp(hdr.magic, GRAPH_MAGIC, 4) != 0) { fclose(fp); printErr("无效的图文件（魔数错误）"); return GRAPH_ERR_MAGIC; }
    if (hdr.version > GRAPH_VERSION) { fclose(fp); printErr("不支持的文件版本"); return GRAPH_ERR_VERSION; }

    free_matrix_graph(g);
    init_matrix_graph(g, hdr.directed, hdr.weighted, hdr.vexnum > 0 ? hdr.vexnum : 4);
    g->vexnum = hdr.vexnum;
    g->arcnum = hdr.arcnum;

    for (int i = 0; i < hdr.vexnum; i++) {
        unsigned char len;
        if (fread(&len, 1, 1, fp) != 1) { fclose(fp); return GRAPH_ERR_CORRUPT; }
        if (fread(g->vexname[i], 1, len, fp) != len) { fclose(fp); return GRAPH_ERR_CORRUPT; }
        g->vexname[i][len] = '\0';
    }

    for (int i = 0; i < hdr.vexnum; i++) {
        if (fread(g->matrix[i], sizeof(int), (size_t)hdr.vexnum, fp) != (size_t)hdr.vexnum) {
            fclose(fp); return GRAPH_ERR_CORRUPT;
        }
    }

    fclose(fp);
    printOK("邻接矩阵加载成功");
    return GRAPH_OK;
}

/* ======================== 用户输入建图 ======================== */

static void input_graph_adjlist(AdjListGraph *g) {
    int directed, weighted, vexnum, arcnum;
    char name[MAX_NAMELEN], name2[MAX_NAMELEN];
    int weight;

    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  手动建图 - 邻接表方式\n");
    printDivider();
    directed = readInt("是否有向图？(1=有向 0=无向): ", 0, 1);
    weighted = readInt("是否带权图？(1=带权 0=无权): ", 0, 1);

    free_adjlist_graph(g);
    init_adjlist_graph(g, directed, weighted, 4);

    vexnum = readInt("请输入顶点数 (1~9999): ", 1, 9999);
    printInfo("请输入各顶点名称（每个一行）:");
    for (int i = 0; i < vexnum; i++) {
        char prompt[64];
        snprintf(prompt, sizeof(prompt), "顶点 %d: ", i + 1);
        readString(prompt, name, MAX_NAMELEN);
        add_vertex_adjlist(g, name);
    }

    arcnum = readInt("请输入边数 (0~99999): ", 0, 99999);
    cPrint(C_CYAN, "  请输入各边，格式: ");
    if (weighted) printf("起点 终点 权值\n");
    else printf("起点 终点\n");

    int added = 0;
    for (int i = 0; i < arcnum; i++) {
        char prompt[64];
        snprintf(prompt, sizeof(prompt), "边 %d: ", i + 1);
        char line[256];
        readString(prompt, line, sizeof(line));
        if (weighted) {
            if (sscanf(line, "%63s %63s %d", name, name2, &weight) != 3) {
                printWarn("输入格式错误，跳过此边"); continue;
            }
        } else {
            if (sscanf(line, "%63s %63s", name, name2) != 2) {
                printWarn("输入格式错误，跳过此边"); continue;
            }
            weight = 1;
        }
        int from = find_vertex_adjlist(g, name);
        int to   = find_vertex_adjlist(g, name2);
        if (from < 0 || to < 0) { printWarn("顶点不存在，跳过"); continue; }
        add_arc_adjlist(g, from, to, weight);
        if (!directed) add_arc_adjlist(g, to, from, weight);
        added++;
    }
    g->arcnum = directed ? added : added;
    printOK("邻接表建图完成");
}

static void input_graph_matrix(MatrixGraph *g) {
    int directed, weighted, vexnum;
    char name[MAX_NAMELEN];

    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  手动建图 - 邻接矩阵方式\n");
    printDivider();
    directed = readInt("是否有向图？(1=有向 0=无向): ", 0, 1);
    weighted = readInt("是否带权图？(1=带权 0=无权): ", 0, 1);

    free_matrix_graph(g);
    init_matrix_graph(g, directed, weighted, 4);

    vexnum = readInt("请输入顶点数 (1~500): ", 1, 500);
    printInfo("请输入各顶点名称（每个一行）:");
    for (int i = 0; i < vexnum; i++) {
        char prompt[64];
        snprintf(prompt, sizeof(prompt), "顶点 %d: ", i + 1);
        readString(prompt, name, MAX_NAMELEN);
        add_vertex_matrix(g, name);
    }

    cPrint(C_CYAN, "  请输入 %dx%d 邻接矩阵:\n", vexnum, vexnum);
    if (weighted) printf("  无边填0\n");
    else printf("  0=无边 1=有边\n");

    g->arcnum = 0;
    for (int i = 0; i < vexnum; i++) {
        while (1) {
            char prompt[128];
            snprintf(prompt, sizeof(prompt), "第 %d 行 (%s): ", i + 1, g->vexname[i]);
            char line[4096];
            readString(prompt, line, sizeof(line));
            int ok = 1;
            char *p = line;
            for (int j = 0; j < vexnum; j++) {
                while (*p && isspace((unsigned char)*p)) p++;
                if (!sscanf(p, "%d", &g->matrix[i][j])) { ok = 0; break; }
                if (g->matrix[i][j] != 0) g->arcnum++;
                while (*p && !isspace((unsigned char)*p)) p++;
            }
            if (ok) break;
            cPrint(C_YELLOW, "  [!] 输入有误，请重新输入此行\n");
        }
    }
    if (!directed) g->arcnum /= 2;
    printOK("邻接矩阵建图完成");
}

/* ======================== 内置测试用例 ======================== */

static void run_tests(void) {
    printf("\n");
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  运行内置测试用例\n");
    printThickDivider();

    AdjListGraph lg;
    MatrixGraph  mg;
    AdjListGraph lg2;

    /* 测试1: 有向带权图 */
    cPrint(C_BOLD C_YELLOW, "\n  [测试1] 有向带权图\n");
    printDivider();
    init_adjlist_graph(&lg, 1, 1, 4);
    add_vertex_adjlist(&lg, "A");
    add_vertex_adjlist(&lg, "B");
    add_vertex_adjlist(&lg, "C");
    add_vertex_adjlist(&lg, "D");
    lg.arcnum = 5;
    add_arc_adjlist(&lg, 0, 1, 3);
    add_arc_adjlist(&lg, 0, 2, 5);
    add_arc_adjlist(&lg, 1, 3, 2);
    add_arc_adjlist(&lg, 2, 3, 1);
    add_arc_adjlist(&lg, 3, 0, 4);
    print_adjlist_graph(&lg);

    adjlist_to_matrix(&lg, &mg);
    print_matrix_graph(&mg);

    matrix_to_adjlist(&mg, &lg2);
    print_adjlist_graph(&lg2);

    bfs_adjlist(&lg2, 0);
    dfs_adjlist(&lg2, 0);
    shortest_path_dijkstra(&lg2, 0, 3);

    save_adjlist_to_file(&lg, "test1_adjlist.grph");
    save_matrix_to_file(&mg, "test1_matrix.grph");

    free_adjlist_graph(&lg);
    free_matrix_graph(&mg);
    free_adjlist_graph(&lg2);

    /* 测试2: 无向无权图 */
    cPrint(C_BOLD C_YELLOW, "\n  [测试2] 无向无权图\n");
    printDivider();
    init_adjlist_graph(&lg, 0, 0, 5);
    add_vertex_adjlist(&lg, "1");
    add_vertex_adjlist(&lg, "2");
    add_vertex_adjlist(&lg, "3");
    add_vertex_adjlist(&lg, "4");
    add_vertex_adjlist(&lg, "5");
    lg.arcnum = 6;
    add_arc_adjlist(&lg, 0, 1, 1); add_arc_adjlist(&lg, 1, 0, 1);
    add_arc_adjlist(&lg, 0, 2, 1); add_arc_adjlist(&lg, 2, 0, 1);
    add_arc_adjlist(&lg, 1, 3, 1); add_arc_adjlist(&lg, 3, 1, 1);
    add_arc_adjlist(&lg, 2, 3, 1); add_arc_adjlist(&lg, 3, 2, 1);
    add_arc_adjlist(&lg, 3, 4, 1); add_arc_adjlist(&lg, 4, 3, 1);
    add_arc_adjlist(&lg, 1, 4, 1); add_arc_adjlist(&lg, 4, 1, 1);
    print_adjlist_graph(&lg);

    adjlist_to_matrix(&lg, &mg);
    print_matrix_graph(&mg);

    matrix_to_adjlist(&mg, &lg2);
    bfs_adjlist(&lg2, 0);
    shortest_path_bfs(&lg2, 0, 4);

    save_adjlist_to_file(&lg, "test2_adjlist.grph");
    save_matrix_to_file(&mg, "test2_matrix.grph");

    free_adjlist_graph(&lg);
    free_matrix_graph(&mg);
    free_adjlist_graph(&lg2);

    printThickDivider();
    cPrint(C_BOLD C_GREEN, "  所有测试用例执行完成！\n");
    printThickDivider();
    printf("\n");
}

/* ======================== 帮助菜单 ======================== */

static void showHelp(void) {
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  使用帮助\n");
    printThickDivider();
    printf("\n");
    cPrint(C_BOLD, "  工作流程:\n");
    printf("    1. 选择 1 或 2 创建图（邻接表/邻接矩阵方式）\n");
    printf("    2. 选择 5 或 6 进行结构双向转换\n");
    printf("    3. 选择 7 或 8 显示当前图结构\n");
    printf("    4. 选择 S 查看图统计信息\n");
    printf("    5. 选择 B/D 进行 BFS/DFS 遍历\n");
    printf("    6. 选择 P 计算最短路径\n");
    printf("    7. 选择 9/10 保存到文件，3/4 从文件加载\n");
    printf("    8. 选择 T 运行内置测试用例\n");
    printf("\n");
    cPrint(C_BOLD, "  命令行模式:\n");
    cPrint(C_CYAN, "    graph.exe -h / --help    "); printf("显示帮助\n");
    cPrint(C_CYAN, "    graph.exe -t / --test    "); printf("运行测试用例\n");
    printf("\n");
    cPrint(C_BOLD, "  文件格式:\n");
    cPrint(C_CYAN, "    *.grph         "); printf("图数据文件（含魔数头，二进制格式）\n");
    printf("\n");
}

/* ======================== 菜单系统 ======================== */

static void showMenu(int has_adjlist, int has_matrix) {
    printDivider();
    cPrint(C_BOLD C_BLUE, "  主菜单\n");
    printDivider();
    cPrint(C_CYAN, "  1 "); printf("- 手动建图（邻接表方式）\n");
    cPrint(C_CYAN, "  2 "); printf("- 手动建图（邻接矩阵方式）\n");
    cPrint(C_CYAN, "  3 "); printf("- 从文件加载邻接表\n");
    cPrint(C_CYAN, "  4 "); printf("- 从文件加载邻接矩阵\n");
    cPrint(C_CYAN, "  5 "); printf("- 邻接表 -> 邻接矩阵\n");
    cPrint(C_CYAN, "  6 "); printf("- 邻接矩阵 -> 邻接表\n");
    cPrint(C_CYAN, "  7 "); printf("- 显示当前邻接表\n");
    cPrint(C_CYAN, "  8 "); printf("- 显示当前邻接矩阵\n");
    cPrint(C_CYAN, "  9 "); printf("- 保存邻接表到文件\n");
    cPrint(C_CYAN, " 10 "); printf("- 保存邻接矩阵到文件\n");
    cPrint(C_CYAN, "  S "); printf("- 图统计信息\n");
    cPrint(C_CYAN, "  B "); printf("- BFS 广度优先遍历\n");
    cPrint(C_CYAN, "  D "); printf("- DFS 深度优先遍历\n");
    cPrint(C_CYAN, "  P "); printf("- 最短路径计算\n");
    cPrint(C_CYAN, "  T "); printf("- 运行内置测试\n");
    cPrint(C_CYAN, "  H/?"); printf("- 帮助信息\n");
    cPrint(C_CYAN, "  Q "); printf("- 退出系统\n");
    printDivider();
    printf("  当前状态: 邻接表 ");
    if (has_adjlist) cPrint(C_GREEN, "OK"); else cPrint(C_GRAY, "--");
    printf(" | 邻接矩阵 ");
    if (has_matrix) cPrint(C_GREEN, "OK"); else cPrint(C_GRAY, "--");
    printf("\n");
    printDivider();
}

static int select_vertex(AdjListGraph *g, const char *prompt) {
    cPrint(C_CYAN, "  可用顶点: ");
    for (int i = 0; i < g->vexnum; i++) {
        cPrint(C_YELLOW, "%s", g->vertices[i].vexname);
        if (i < g->vexnum - 1) printf(", ");
    }
    printf("\n");
    char name[MAX_NAMELEN];
    while (1) {
        readString(prompt, name, MAX_NAMELEN);
        int idx = find_vertex_adjlist(g, name);
        if (idx >= 0) return idx;
        cPrint(C_YELLOW, "  [!] 顶点不存在，请重新输入\n");
    }
}

/* ======================== 命令行处理 ======================== */

static void printUsage(const char *prog) {
    printf("Graph Storage & Conversion System - V2.0 Enhanced Edition\n\n");
    printf("用法:\n");
    printf("  %s                     交互模式（菜单）\n", prog);
    printf("  %s -h, --help          显示帮助\n", prog);
    printf("  %s -t, --test          运行内置测试用例\n", prog);
    printf("\n");
}

static int runCommandLine(int argc, char *argv[]) {
    if (argc < 2) return 0;
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        printUsage(argv[0]);
        return 1;
    }
    if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0) {
        run_tests();
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
        return ret;
    }

    AdjListGraph adjlist;
    MatrixGraph  matrix;
    init_adjlist_graph(&adjlist, 0, 0, 4);
    init_matrix_graph(&matrix, 0, 0, 4);
    int has_adjlist = 0;
    int has_matrix = 0;

    printBanner();
    printInfo("欢迎使用图存储及转换系统 V2.0！输入 H 查看帮助。");

    char choice[64];
    while (1) {
        showMenu(has_adjlist, has_matrix);
        printf("  请选择操作: "); fflush(stdout);
        if (fgets(choice, sizeof(choice), stdin) == NULL) break;
        char *nl = strchr(choice, '\n'); if (nl) *nl = '\0';
        if (choice[0] == '\0') continue;

        if (strcmp(choice, "1") == 0) {
            input_graph_adjlist(&adjlist);
            has_adjlist = 1;
        } else if (strcmp(choice, "2") == 0) {
            input_graph_matrix(&matrix);
            has_matrix = 1;
        } else if (strcmp(choice, "3") == 0) {
            char fname[256] = "graph.grph";
            readString("请输入文件名（回车使用默认 graph.grph）: ", fname, sizeof(fname));
            if (fname[0] == '\0') strcpy(fname, "graph.grph");
            if (has_adjlist) free_adjlist_graph(&adjlist);
            if (load_adjlist_from_file(&adjlist, fname) == GRAPH_OK) has_adjlist = 1;
        } else if (strcmp(choice, "4") == 0) {
            char fname[256] = "graph.grph";
            readString("请输入文件名（回车使用默认 graph.grph）: ", fname, sizeof(fname));
            if (fname[0] == '\0') strcpy(fname, "graph.grph");
            if (load_matrix_from_file(&matrix, fname) == GRAPH_OK) has_matrix = 1;
        } else if (strcmp(choice, "5") == 0) {
            if (!has_adjlist) { printWarn("请先创建或加载邻接表"); continue; }
            adjlist_to_matrix(&adjlist, &matrix);
            has_matrix = 1;
            printOK("转换完成：邻接表 -> 邻接矩阵");
            print_matrix_graph(&matrix);
        } else if (strcmp(choice, "6") == 0) {
            if (!has_matrix) { printWarn("请先创建或加载邻接矩阵"); continue; }
            if (has_adjlist) free_adjlist_graph(&adjlist);
            matrix_to_adjlist(&matrix, &adjlist);
            has_adjlist = 1;
            printOK("转换完成：邻接矩阵 -> 邻接表");
            print_adjlist_graph(&adjlist);
        } else if (strcmp(choice, "7") == 0) {
            if (!has_adjlist) { printWarn("请先创建或加载邻接表"); continue; }
            print_adjlist_graph(&adjlist);
        } else if (strcmp(choice, "8") == 0) {
            if (!has_matrix) { printWarn("请先创建或加载邻接矩阵"); continue; }
            print_matrix_graph(&matrix);
        } else if (strcmp(choice, "9") == 0) {
            if (!has_adjlist) { printWarn("请先创建或加载邻接表"); continue; }
            char fname[256] = "graph_adjlist.grph";
            readString("保存到文件名（回车使用 graph_adjlist.grph）: ", fname, sizeof(fname));
            if (fname[0] == '\0') strcpy(fname, "graph_adjlist.grph");
            save_adjlist_to_file(&adjlist, fname);
        } else if (strcmp(choice, "10") == 0) {
            if (!has_matrix) { printWarn("请先创建或加载邻接矩阵"); continue; }
            char fname[256] = "graph_matrix.grph";
            readString("保存到文件名（回车使用 graph_matrix.grph）: ", fname, sizeof(fname));
            if (fname[0] == '\0') strcpy(fname, "graph_matrix.grph");
            save_matrix_to_file(&matrix, fname);
        } else {
            char c = (char)toupper((unsigned char)choice[0]);
            if (c == 'S') {
                if (has_adjlist) print_graph_stats(&adjlist, NULL);
                else if (has_matrix) print_graph_stats(NULL, &matrix);
                else printWarn("当前没有图数据");
            } else if (c == 'B') {
                if (!has_adjlist) {
                    if (has_matrix) {
                        matrix_to_adjlist(&matrix, &adjlist);
                        has_adjlist = 1;
                    } else { printWarn("请先创建或加载图"); continue; }
                }
                int start = select_vertex(&adjlist, "BFS 起始顶点: ");
                bfs_adjlist(&adjlist, start);
            } else if (c == 'D') {
                if (!has_adjlist) {
                    if (has_matrix) {
                        matrix_to_adjlist(&matrix, &adjlist);
                        has_adjlist = 1;
                    } else { printWarn("请先创建或加载图"); continue; }
                }
                int start = select_vertex(&adjlist, "DFS 起始顶点: ");
                dfs_adjlist(&adjlist, start);
            } else if (c == 'P') {
                if (!has_adjlist) {
                    if (has_matrix) {
                        matrix_to_adjlist(&matrix, &adjlist);
                        has_adjlist = 1;
                    } else { printWarn("请先创建或加载图"); continue; }
                }
                int s = select_vertex(&adjlist, "起点: ");
                int e = select_vertex(&adjlist, "终点: ");
                if (adjlist.weighted) shortest_path_dijkstra(&adjlist, s, e);
                else shortest_path_bfs(&adjlist, s, e);
            } else if (c == 'T') {
                run_tests();
            } else if (c == 'H' || c == '?') {
                showHelp();
            } else if (c == 'Q') {
                free_adjlist_graph(&adjlist);
                free_matrix_graph(&matrix);
                cPrint(C_GREEN, "\n  [OK] 感谢使用，再见！\n\n");
                return 0;
            } else {
                cPrint(C_YELLOW, "  [!] 无效选项，请重新输入\n");
            }
        }
    }

    free_adjlist_graph(&adjlist);
    free_matrix_graph(&matrix);
    return 0;
}
