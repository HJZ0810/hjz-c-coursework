/* ===========================================================================
 * 关键路径方法 (Critical Path Method) V2.0
 * ===========================================================================
 *
 * 项目名称：关键路径问题求解系统 V2.0 (Project_36_CriticalPath_V2)
 *
 * 功能概述：基于 AOE 网 (Activity On Edge Network) 的关键路径求解系统，
 *           通过拓扑排序计算顶点最早/最晚发生时间，识别关键活动和关键路径，
 *           为项目进度优化提供决策数据。
 *
 * 核心算法：CPM (Critical Path Method)
 *           - 拓扑排序 (Kahn算法) 计算 etv (顶点最早发生时间)
 *           - 逆拓扑排序计算 ltv (顶点最晚发生时间)
 *           - 计算 ete/lte，ete==lte 即为关键活动
 *
 * 数据结构：动态邻接表存储 AOE 网
 *           - 顶点 (Vertex)：动态数组，按需扩容
 *           - 边 (Edge/Arc)：动态链表头，每条边动态分配
 *           - 图 (Graph)：顶点数组 + 顶点数 + 边数
 *
 * V2.0 新特性：
 *   - UTF-8 BOM + SetConsoleOutputCP(65001) 中文支持
 *   - ANSI 彩色终端输出
 *   - CLI 支持 (-h/--help, -t/--test)
 *   - 动态内存 (顶点+边动态数组，自动扩容)
 *   - 统一错误码 enum CpmStatus
 *   - "CPMV2" magic header 二进制文件持久化
 *   - 内置 15+ 项自测 (关键路径+环检测+文件I/O)
 *   - 交互菜单
 *
 * 编译命令：gcc -std=c99 -Wall -Wextra -O2 -o cpm.exe main.c
 *
 * 作者：数据结构
 * 版本：V2.0
 * =========================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

/* ===========================================================================
 * 第一部分：常量与配置
 * =========================================================================== */

#define CPM_MAGIC       "CPMV2"
#define CPM_MAGIC_LEN   5
#define CPM_VERSION     2
#define MAX_NAME_LEN    64
#define INIT_VERT_CAP   16
#define VERT_GROW_FACTOR 2

/* ANSI 颜色码 */
#define COLOR_RESET     "\033[0m"
#define COLOR_RED       "\033[31m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_BLUE      "\033[34m"
#define COLOR_MAGENTA   "\033[35m"
#define COLOR_CYAN      "\033[36m"
#define COLOR_WHITE     "\033[37m"
#define COLOR_BOLD      "\033[1m"
#define COLOR_BG_RED    "\033[41m"
#define COLOR_BG_GREEN  "\033[42m"
#define COLOR_BG_BLUE   "\033[44m"

/* ===========================================================================
 * 第二部分：统一错误码枚举
 * =========================================================================== */

typedef enum {
    CPM_OK = 0,
    CPM_ERR_NULL_PTR,
    CPM_ERR_MEMORY,
    CPM_ERR_FILE_OPEN,
    CPM_ERR_FILE_READ,
    CPM_ERR_FILE_WRITE,
    CPM_ERR_FILE_MAGIC,
    CPM_ERR_FILE_VERSION,
    CPM_ERR_FILE_CORRUPT,
    CPM_ERR_VERTEX_NOT_FOUND,
    CPM_ERR_DUPLICATE_VERTEX,
    CPM_ERR_DUPLICATE_EDGE,
    CPM_ERR_INVALID_WEIGHT,
    CPM_ERR_CYCLE_DETECTED,
    CPM_ERR_EMPTY_GRAPH,
    CPM_ERR_NO_SOURCE,
    CPM_ERR_NO_SINK,
    CPM_ERR_NOT_SAVED,
    CPM_ERR_INVALID_INPUT,
    CPM_ERR_INDEX_OUT_OF_RANGE
} CpmStatus;

/* ===========================================================================
 * 第三部分：数据结构定义 (动态内存版本)
 * =========================================================================== */

typedef struct Edge {
    int          to;
    int          weight;
    char         name[MAX_NAME_LEN];
    struct Edge *next;
} Edge;

typedef struct {
    char  name[MAX_NAME_LEN];
    Edge *first_edge;
} Vertex;

typedef struct {
    Vertex *vertices;
    int     vert_count;
    int     vert_cap;
    int     edge_count;
} Graph;

typedef struct {
    int  *etv;
    int  *ltv;
    int  *topo;
    int   topo_cnt;
    int   has_cycle;
    int   source;
    int   sink;
    int   total_duration;
    int   crit_edge_count;
} CpmResult;

/* ===========================================================================
 * 第四部分：ANSI 彩色终端工具
 * =========================================================================== */

static int use_color = 1;

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

static const char *status_msg(CpmStatus s)
{
    switch (s) {
        case CPM_OK:                   return "成功";
        case CPM_ERR_NULL_PTR:         return "空指针错误";
        case CPM_ERR_MEMORY:           return "内存分配失败";
        case CPM_ERR_FILE_OPEN:        return "无法打开文件";
        case CPM_ERR_FILE_READ:        return "文件读取失败";
        case CPM_ERR_FILE_WRITE:       return "文件写入失败";
        case CPM_ERR_FILE_MAGIC:       return "文件魔数错误";
        case CPM_ERR_FILE_VERSION:     return "文件版本不兼容";
        case CPM_ERR_FILE_CORRUPT:     return "文件损坏";
        case CPM_ERR_VERTEX_NOT_FOUND: return "顶点不存在";
        case CPM_ERR_DUPLICATE_VERTEX: return "顶点已存在";
        case CPM_ERR_DUPLICATE_EDGE:   return "边已存在";
        case CPM_ERR_INVALID_WEIGHT:   return "权值无效(需非负)";
        case CPM_ERR_CYCLE_DETECTED:   return "检测到环";
        case CPM_ERR_EMPTY_GRAPH:      return "图为空";
        case CPM_ERR_NO_SOURCE:        return "无源顶点(入度为0)";
        case CPM_ERR_NO_SINK:          return "无汇顶点(出度为0)";
        case CPM_ERR_NOT_SAVED:        return "数据未保存";
        case CPM_ERR_INVALID_INPUT:    return "输入无效";
        case CPM_ERR_INDEX_OUT_OF_RANGE:return "索引越界";
        default:                       return "未知错误";
    }
}

static void c_print(const char *color, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if (use_color && color) fputs(color, stdout);
    vprintf(fmt, args);
    if (use_color && color) fputs(COLOR_RESET, stdout);
    va_end(args);
}

static void print_ok(const char *msg)    { c_print(COLOR_GREEN, "[OK] %s\n", msg); }
static void print_err(const char *msg)   { c_print(COLOR_RED,   "[错误] %s\n", msg); }
static void print_warn(const char *msg)  { c_print(COLOR_YELLOW,"[警告] %s\n", msg); }
static void print_info(const char *msg)  { c_print(COLOR_CYAN,  "[信息] %s\n", msg); }

static void print_sep(void)
{
    c_print(COLOR_BLUE, "──────────────────────────────────────────────────────────\n");
}

static void print_sep_dbl(void)
{
    c_print(COLOR_BOLD COLOR_BLUE,
            "══════════════════════════════════════════════════════════\n");
}

/* ===========================================================================
 * 第五部分：图操作 (动态内存管理)
 * =========================================================================== */

static CpmStatus graph_init(Graph *g)
{
    if (!g) return CPM_ERR_NULL_PTR;
    g->vert_cap = INIT_VERT_CAP;
    g->vert_count = 0;
    g->edge_count = 0;
    g->vertices = (Vertex *)calloc((size_t)g->vert_cap, sizeof(Vertex));
    if (!g->vertices) return CPM_ERR_MEMORY;
    return CPM_OK;
}

static void graph_free(Graph *g)
{
    int i;
    if (!g || !g->vertices) return;
    for (i = 0; i < g->vert_count; i++) {
        Edge *e = g->vertices[i].first_edge;
        while (e) { Edge *t = e; e = e->next; free(t); }
        g->vertices[i].first_edge = NULL;
    }
    free(g->vertices);
    g->vertices = NULL;
    g->vert_count = 0;
    g->vert_cap = 0;
    g->edge_count = 0;
}

static CpmStatus graph_ensure_cap(Graph *g)
{
    Vertex *nv;
    int new_cap;
    if (!g) return CPM_ERR_NULL_PTR;
    if (g->vert_count < g->vert_cap) return CPM_OK;
    new_cap = g->vert_cap * VERT_GROW_FACTOR;
    nv = (Vertex *)realloc(g->vertices, (size_t)new_cap * sizeof(Vertex));
    if (!nv) return CPM_ERR_MEMORY;
    g->vertices = nv;
    memset(&g->vertices[g->vert_cap], 0,
           (size_t)(new_cap - g->vert_cap) * sizeof(Vertex));
    g->vert_cap = new_cap;
    return CPM_OK;
}

static CpmStatus graph_find_vertex(const Graph *g, const char *name, int *out_idx)
{
    int i;
    if (!g || !name || !out_idx) return CPM_ERR_NULL_PTR;
    for (i = 0; i < g->vert_count; i++) {
        if (strcmp(g->vertices[i].name, name) == 0) {
            *out_idx = i;
            return CPM_OK;
        }
    }
    return CPM_ERR_VERTEX_NOT_FOUND;
}

static CpmStatus graph_add_vertex(Graph *g, const char *name)
{
    CpmStatus st;
    int idx;
    if (!g || !name) return CPM_ERR_NULL_PTR;
    if (strlen(name) >= MAX_NAME_LEN) return CPM_ERR_INVALID_INPUT;
    if (graph_find_vertex(g, name, &idx) == CPM_OK) return CPM_ERR_DUPLICATE_VERTEX;
    st = graph_ensure_cap(g);
    if (st != CPM_OK) return st;
    memset(&g->vertices[g->vert_count], 0, sizeof(Vertex));
    strncpy(g->vertices[g->vert_count].name, name, MAX_NAME_LEN - 1);
    g->vertices[g->vert_count].name[MAX_NAME_LEN - 1] = '\0';
    g->vertices[g->vert_count].first_edge = NULL;
    g->vert_count++;
    return CPM_OK;
}

static CpmStatus graph_add_edge(Graph *g, const char *from, const char *to,
                                int weight, const char *ename)
{
    int fi, ti;
    Edge *e, *cur;
    CpmStatus st;
    if (!g || !from || !to || !ename) return CPM_ERR_NULL_PTR;
    if (weight < 0) return CPM_ERR_INVALID_WEIGHT;
    if (strlen(ename) >= MAX_NAME_LEN) return CPM_ERR_INVALID_INPUT;
    if ((st = graph_find_vertex(g, from, &fi)) != CPM_OK) return st;
    if ((st = graph_find_vertex(g, to, &ti)) != CPM_OK) return st;
    for (cur = g->vertices[fi].first_edge; cur; cur = cur->next) {
        if (cur->to == ti) return CPM_ERR_DUPLICATE_EDGE;
    }
    e = (Edge *)malloc(sizeof(Edge));
    if (!e) return CPM_ERR_MEMORY;
    e->to = ti;
    e->weight = weight;
    strncpy(e->name, ename, MAX_NAME_LEN - 1);
    e->name[MAX_NAME_LEN - 1] = '\0';
    e->next = g->vertices[fi].first_edge;
    g->vertices[fi].first_edge = e;
    g->edge_count++;
    return CPM_OK;
}

/* ===========================================================================
 * 第六部分：拓扑排序与关键路径算法
 * =========================================================================== */

static void cpm_result_free(CpmResult *r)
{
    if (!r) return;
    free(r->etv); free(r->ltv); free(r->topo);
    r->etv = NULL; r->ltv = NULL; r->topo = NULL;
}

static CpmStatus cpm_calc(const Graph *g, CpmResult *res)
{
    int *in_deg, *stack;
    int top, cnt, i, max_out, max_out_idx;
    Edge *e;
    if (!g || !res) return CPM_ERR_NULL_PTR;
    if (g->vert_count == 0) return CPM_ERR_EMPTY_GRAPH;

    memset(res, 0, sizeof(CpmResult));
    res->etv = (int *)calloc((size_t)g->vert_count, sizeof(int));
    res->ltv = (int *)malloc((size_t)g->vert_count * sizeof(int));
    res->topo = (int *)malloc((size_t)g->vert_count * sizeof(int));
    in_deg = (int *)calloc((size_t)g->vert_count, sizeof(int));
    stack = (int *)malloc((size_t)g->vert_count * sizeof(int));
    if (!res->etv || !res->ltv || !res->topo || !in_deg || !stack) {
        free(in_deg); free(stack); cpm_result_free(res);
        return CPM_ERR_MEMORY;
    }

    for (i = 0; i < g->vert_count; i++) {
        for (e = g->vertices[i].first_edge; e; e = e->next) {
            in_deg[e->to]++;
        }
    }

    top = -1; res->source = -1;
    for (i = 0; i < g->vert_count; i++) {
        if (in_deg[i] == 0) { stack[++top] = i; res->source = i; }
    }
    if (top < 0) {
        res->has_cycle = 1;
        free(in_deg); free(stack);
        /* ltv not freed yet but etv/topo allocated; free all */
        free(res->ltv); res->ltv = NULL;
        cpm_result_free(res);
        return CPM_ERR_CYCLE_DETECTED;
    }

    cnt = 0;
    while (top >= 0) {
        int v = stack[top--];
        res->topo[cnt++] = v;
        for (e = g->vertices[v].first_edge; e; e = e->next) {
            int w = e->to;
            if (res->etv[v] + e->weight > res->etv[w])
                res->etv[w] = res->etv[v] + e->weight;
            if (--in_deg[w] == 0) stack[++top] = w;
        }
    }

    free(in_deg); free(stack);

    if (cnt < g->vert_count) {
        res->has_cycle = 1;
        cpm_result_free(res);
        return CPM_ERR_CYCLE_DETECTED;
    }
    res->topo_cnt = cnt;
    res->has_cycle = 0;

    max_out = -1; max_out_idx = -1;
    for (i = 0; i < g->vert_count; i++) {
        int out = 0;
        for (e = g->vertices[i].first_edge; e; e = e->next) out++;
        if (out == 0) { if (max_out < 0 || res->etv[i] > max_out) { max_out = res->etv[i]; max_out_idx = i; } }
    }
    res->sink = max_out_idx;
    res->total_duration = max_out_idx >= 0 ? res->etv[max_out_idx] : 0;

    for (i = 0; i < g->vert_count; i++) res->ltv[i] = res->total_duration;

    for (i = cnt - 1; i >= 0; i--) {
        int v = res->topo[i];
        for (e = g->vertices[v].first_edge; e; e = e->next) {
            int w = e->to;
            if (res->ltv[w] - e->weight < res->ltv[v])
                res->ltv[v] = res->ltv[w] - e->weight;
        }
    }

    res->crit_edge_count = 0;
    for (i = 0; i < g->vert_count; i++) {
        for (e = g->vertices[i].first_edge; e; e = e->next) {
            int ete = res->etv[i];
            int lte = res->ltv[e->to] - e->weight;
            if (ete == lte) res->crit_edge_count++;
        }
    }

    return CPM_OK;
}

/* ===========================================================================
 * 第七部分："CPMV2" 二进制文件持久化
 * =========================================================================== */

static CpmStatus graph_save_binary(const Graph *g, const char *path)
{
    FILE *fp;
    int i, ver = CPM_VERSION;
    Edge *e;
    char magic[CPM_MAGIC_LEN] = CPM_MAGIC;
    if (!g || !path) return CPM_ERR_NULL_PTR;
    fp = fopen(path, "wb");
    if (!fp) return CPM_ERR_FILE_OPEN;
    fwrite(magic, 1, CPM_MAGIC_LEN, fp);
    fwrite(&ver, sizeof(int), 1, fp);
    fwrite(&g->vert_count, sizeof(int), 1, fp);
    fwrite(&g->edge_count, sizeof(int), 1, fp);
    for (i = 0; i < g->vert_count; i++) {
        unsigned char nlen = (unsigned char)strlen(g->vertices[i].name);
        fwrite(&nlen, 1, 1, fp);
        fwrite(g->vertices[i].name, 1, nlen, fp);
    }
    for (i = 0; i < g->vert_count; i++) {
        int out_cnt = 0;
        for (e = g->vertices[i].first_edge; e; e = e->next) out_cnt++;
        fwrite(&out_cnt, sizeof(int), 1, fp);
        for (e = g->vertices[i].first_edge; e; e = e->next) {
            unsigned char nlen;
            fwrite(&e->to, sizeof(int), 1, fp);
            fwrite(&e->weight, sizeof(int), 1, fp);
            nlen = (unsigned char)strlen(e->name);
            fwrite(&nlen, 1, 1, fp);
            fwrite(e->name, 1, nlen, fp);
        }
    }
    fclose(fp);
    return CPM_OK;
}

static CpmStatus graph_load_binary(Graph *g, const char *path)
{
    FILE *fp;
    char magic[CPM_MAGIC_LEN];
    int ver, vc, i, j;
    CpmStatus st;
    if (!g || !path) return CPM_ERR_NULL_PTR;
    fp = fopen(path, "rb");
    if (!fp) return CPM_ERR_FILE_OPEN;

    if (fread(magic, 1, CPM_MAGIC_LEN, fp) != CPM_MAGIC_LEN ||
        memcmp(magic, CPM_MAGIC, CPM_MAGIC_LEN) != 0) {
        fclose(fp); return CPM_ERR_FILE_MAGIC;
    }
    if (fread(&ver, sizeof(int), 1, fp) != 1 || ver != CPM_VERSION) {
        fclose(fp); return CPM_ERR_FILE_VERSION;
    }
    if (fread(&vc, sizeof(int), 1, fp) != 1) {
        fclose(fp); return CPM_ERR_FILE_CORRUPT;
    }
    {
        int ec_check;
        if (fread(&ec_check, sizeof(int), 1, fp) != 1) {
            fclose(fp); return CPM_ERR_FILE_CORRUPT;
        }
    }

    graph_free(g);
    st = graph_init(g);
    if (st != CPM_OK) { fclose(fp); return st; }

    for (i = 0; i < vc; i++) {
        unsigned char nlen;
        char name[MAX_NAME_LEN];
        if (fread(&nlen, 1, 1, fp) != 1 || nlen == 0 || nlen >= MAX_NAME_LEN) {
            fclose(fp); graph_free(g); return CPM_ERR_FILE_CORRUPT;
        }
        if (fread(name, 1, nlen, fp) != nlen) { fclose(fp); graph_free(g); return CPM_ERR_FILE_CORRUPT; }
        name[nlen] = '\0';
        if ((st = graph_add_vertex(g, name)) != CPM_OK) { fclose(fp); graph_free(g); return st; }
    }

    for (i = 0; i < vc; i++) {
        int out_cnt;
        if (fread(&out_cnt, sizeof(int), 1, fp) != 1) { fclose(fp); graph_free(g); return CPM_ERR_FILE_CORRUPT; }
        for (j = 0; j < out_cnt; j++) {
            int to, wt;
            unsigned char nlen;
            char ename[MAX_NAME_LEN];
            if (fread(&to, sizeof(int), 1, fp) != 1 ||
                fread(&wt, sizeof(int), 1, fp) != 1 ||
                fread(&nlen, 1, 1, fp) != 1 || nlen == 0 || nlen >= MAX_NAME_LEN) {
                fclose(fp); graph_free(g); return CPM_ERR_FILE_CORRUPT;
            }
            if (fread(ename, 1, nlen, fp) != nlen) { fclose(fp); graph_free(g); return CPM_ERR_FILE_CORRUPT; }
            ename[nlen] = '\0';
            if (to < 0 || to >= g->vert_count || wt < 0) { fclose(fp); graph_free(g); return CPM_ERR_FILE_CORRUPT; }
            if ((st = graph_add_edge(g, g->vertices[i].name, g->vertices[to].name, wt, ename)) != CPM_OK) {
                fclose(fp); graph_free(g); return st;
            }
        }
    }

    fclose(fp);
    return CPM_OK;
}

/* ===========================================================================
 * 第八部分：输出显示
 * =========================================================================== */

struct CritCtx {
    const Graph *g;
    const CpmResult *res;
    int *visited;
};

static void dfs_crit_path(struct CritCtx *ctx, int v, int depth)
{
    Edge *e;
    const Graph *g = ctx->g;
    const CpmResult *res = ctx->res;

    if (v == res->sink) {
        c_print(COLOR_BOLD COLOR_RED, " -> %s [终点, 总工期=%d]\n",
                g->vertices[v].name, res->total_duration);
        return;
    }
    if (depth > 0) c_print(COLOR_BOLD COLOR_RED, " -> %s", g->vertices[v].name);
    else c_print(COLOR_BOLD COLOR_RED, "  %s", g->vertices[v].name);
    ctx->visited[v] = 1;
    for (e = g->vertices[v].first_edge; e; e = e->next) {
        int ete2 = res->etv[v];
        int lte2 = res->ltv[e->to] - e->weight;
        if (ete2 == lte2 && !ctx->visited[e->to]) {
            c_print(COLOR_BOLD COLOR_RED, " --%s(%d)--", e->name, e->weight);
            dfs_crit_path(ctx, e->to, depth + 1);
        }
    }
    ctx->visited[v] = 0;
    if (v == res->source) putchar('\n');
}

static void graph_print(const Graph *g, const CpmResult *res)
{
    int i;
    Edge *e;
    print_sep_dbl();
    c_print(COLOR_BOLD COLOR_CYAN, "  AOE 网邻接表\n");
    print_sep();
    for (i = 0; i < g->vert_count; i++) {
        c_print(COLOR_YELLOW, "  %s", g->vertices[i].name);
        if (res) {
            c_print(COLOR_WHITE, " (etv=%d, ltv=%d)", res->etv[i], res->ltv[i]);
        }
        c_print(COLOR_WHITE, ":");
        for (e = g->vertices[i].first_edge; e; e = e->next) {
            int crit = 0;
            if (res) {
                int ete = res->etv[i];
                int lte = res->ltv[e->to] - e->weight;
                if (ete == lte) crit = 1;
            }
            if (crit) c_print(COLOR_BOLD COLOR_RED, " ->%s(%s,%d*CRITICAL*)",
                              g->vertices[e->to].name, e->name, e->weight);
            else c_print(COLOR_GREEN, " ->%s(%s,%d)",
                         g->vertices[e->to].name, e->name, e->weight);
        }
        putchar('\n');
    }
    print_sep();
    printf("  顶点数: %d    边数: %d\n", g->vert_count, g->edge_count);
}

static void result_print(const Graph *g, const CpmResult *res)
{
    int i;
    Edge *e;
    print_sep_dbl();
    c_print(COLOR_BOLD COLOR_MAGENTA, "  关键路径分析结果 (CPM V2.0)\n");
    print_sep_dbl();

    c_print(COLOR_CYAN, "\n[1] 拓扑序列: ");
    for (i = 0; i < res->topo_cnt; i++) {
        printf("%s", g->vertices[res->topo[i]].name);
        if (i < res->topo_cnt - 1) printf(" -> ");
    }
    printf("\n");

    c_print(COLOR_CYAN, "\n[2] 顶点最早/最晚发生时间:\n");
    print_sep();
    printf("  %-8s %-8s %-8s %-8s\n", "顶点", "etv", "ltv", "时差");
    print_sep();
    for (i = 0; i < g->vert_count; i++) {
        int slack = res->ltv[i] - res->etv[i];
        const char *c = (slack == 0) ? COLOR_BOLD COLOR_RED : COLOR_WHITE;
        c_print(c, "  %-8s %-8d %-8d %-8d\n",
                g->vertices[i].name, res->etv[i], res->ltv[i], slack);
    }

    c_print(COLOR_CYAN, "\n[3] 活动时间参数与关键活动:\n");
    print_sep();
    printf("  %-6s %-5s->%-5s %-5s %-5s %-5s %-8s\n",
           "活动", "起点", "终点", "权值", "ete", "lte", "状态");
    print_sep();
    for (i = 0; i < g->vert_count; i++) {
        for (e = g->vertices[i].first_edge; e; e = e->next) {
            int ete = res->etv[i];
            int lte = res->ltv[e->to] - e->weight;
            int crit = (ete == lte);
            if (crit) c_print(COLOR_BOLD COLOR_RED,
                "  %-6s %-5s->%-5s %-5d %-5d %-5d *关键*\n",
                e->name, g->vertices[i].name, g->vertices[e->to].name,
                e->weight, ete, lte);
            else printf("  %-6s %-5s->%-5s %-5d %-5d %-5d\n",
                e->name, g->vertices[i].name, g->vertices[e->to].name,
                e->weight, ete, lte);
        }
    }

    c_print(COLOR_CYAN, "\n[4] 关键路径追踪:\n");
    print_sep();
    {
        struct CritCtx ctx;
        int *visited_arr = (int *)calloc((size_t)g->vert_count, sizeof(int));
        if (visited_arr) {
            ctx.g = g;
            ctx.res = res;
            ctx.visited = visited_arr;
            dfs_crit_path(&ctx, res->source, 0);
            free(visited_arr);
        }
    }

    print_sep_dbl();
    c_print(COLOR_BOLD COLOR_GREEN, "  源点: %s\n", g->vertices[res->source].name);
    c_print(COLOR_BOLD COLOR_GREEN, "  汇点: %s\n", g->vertices[res->sink].name);
    c_print(COLOR_BOLD COLOR_GREEN, "  关键活动数: %d\n", res->crit_edge_count);
    c_print(COLOR_BOLD COLOR_GREEN, "  项目总工期(关键路径长度): %d\n", res->total_duration);
    print_sep_dbl();
}

/* ===========================================================================
 * 第九部分：内置自测 (15+ 项)
 * =========================================================================== */

static int tests_passed = 0;
static int tests_failed = 0;

static void test_check(int cond, const char *name)
{
    if (cond) { c_print(COLOR_GREEN, "  [PASS] %s\n", name); tests_passed++; }
    else      { c_print(COLOR_RED,   "  [FAIL] %s\n", name); tests_failed++; }
}

static int build_test_graph(Graph *g, int case_id)
{
    int i;
    graph_free(g);
    (void)graph_init(g);
    switch (case_id) {
        case 0: { /* 经典教材例：10顶点16边，总工期31 */
            const char *vn[] = {"V0","V1","V2","V3","V4","V5","V6","V7","V8","V9"};
            int ed[][3] = {{0,1,6},{0,2,4},{0,3,5},{1,3,5},{2,3,8},{2,4,3},
                           {3,5,2},{3,6,7},{4,5,4},{4,7,3},{5,7,4},{6,7,5},
                           {6,9,3},{7,8,4},{7,9,5},{8,9,3}};
            const char *en[] = {"a1","a2","a3","a4","a5","a6","a7","a8",
                                "a9","a10","a11","a12","a13","a14","a15","a16"};
            for (i = 0; i < 10; i++) (void)graph_add_vertex(g, vn[i]);
            for (i = 0; i < 16; i++)
                (void)graph_add_edge(g, vn[ed[i][0]], vn[ed[i][1]], ed[i][2], en[i]);
            return 1;
        }
        case 1: { /* 简单线性: V0->V1->V2->V3, 总工期=2+3+4=9 */
            const char *vn[] = {"V0","V1","V2","V3"};
            (void)graph_add_vertex(g, vn[0]);
            for (i = 1; i < 4; i++) (void)graph_add_vertex(g, vn[i]);
            (void)graph_add_edge(g, "V0","V1",2,"a1");
            (void)graph_add_edge(g, "V1","V2",3,"a2");
            (void)graph_add_edge(g, "V2","V3",4,"a3");
            return 1;
        }
        case 2: { /* 并行路径: V0->V1(3), V0->V2(5), V1->V3(2), V2->V3(1), 总工期=5+1=6? 不: max(3+2=5,5+1=6)=6 */
            const char *vn[] = {"S","A","B","T"};
            for (i = 0; i < 4; i++) (void)graph_add_vertex(g, vn[i]);
            (void)graph_add_edge(g, "S","A",3,"a1");
            (void)graph_add_edge(g, "S","B",5,"a2");
            (void)graph_add_edge(g, "A","T",2,"a3");
            (void)graph_add_edge(g, "B","T",1,"a4");
            return 1;
        }
        case 3: { /* 单顶点无边 */
            (void)graph_add_vertex(g, "Only");
            return 1;
        }
        case 4: { /* 菱形: V0->V1(1),V0->V2(2),V1->V3(3),V2->V3(1), 总工期=max(1+3=4,2+1=3)=4 */
            const char *vn[] = {"V0","V1","V2","V3"};
            for (i = 0; i < 4; i++) (void)graph_add_vertex(g, vn[i]);
            (void)graph_add_edge(g, "V0","V1",1,"a1");
            (void)graph_add_edge(g, "V0","V2",2,"a2");
            (void)graph_add_edge(g, "V1","V3",3,"a3");
            (void)graph_add_edge(g, "V2","V3",1,"a4");
            return 1;
        }
        case 5: { /* 含环: V0->V1, V1->V2, V2->V0 */
            const char *vn[] = {"V0","V1","V2"};
            for (i = 0; i < 3; i++) (void)graph_add_vertex(g, vn[i]);
            (void)graph_add_edge(g, "V0","V1",1,"a1");
            (void)graph_add_edge(g, "V1","V2",1,"a2");
            (void)graph_add_edge(g, "V2","V0",1,"a3");
            return 1;
        }
        case 6: { /* 教材简单例: V0->V1(3),V0->V2(2),V1->V3(2),V1->V4(3),V2->V3(4),V2->V4(6),V3->V5(2),V4->V5(1), 总工期=max path? V0->V2->V4->V5=2+6+1=9; V0->V1->V4->V5=3+3+1=7; V0->V2->V3->V5=2+4+2=8; V0->V1->V3->V5=3+2+2=7 → 9 */
            const char *vn[] = {"A","B","C","D","E","F"};
            for (i = 0; i < 6; i++) (void)graph_add_vertex(g, vn[i]);
            (void)graph_add_edge(g,"A","B",3,"a1");
            (void)graph_add_edge(g,"A","C",2,"a2");
            (void)graph_add_edge(g,"B","D",2,"a3");
            (void)graph_add_edge(g,"B","E",3,"a4");
            (void)graph_add_edge(g,"C","D",4,"a5");
            (void)graph_add_edge(g,"C","E",6,"a6");
            (void)graph_add_edge(g,"D","F",2,"a7");
            (void)graph_add_edge(g,"E","F",1,"a8");
            return 1;
        }
        case 7: { /* 自环(非法AOE，应检测为环) */
            (void)graph_add_vertex(g, "X");
            (void)graph_add_edge(g, "X","X",5,"self");
            return 1;
        }
        case 8: { /* 两顶点单边: S->T, 10 */
            (void)graph_add_vertex(g,"S"); (void)graph_add_vertex(g,"T");
            (void)graph_add_edge(g,"S","T",10,"a1");
            return 1;
        }
        case 9: { /* 三条等长路径 */
            const char *vn[] = {"S","A","B","C","T"};
            for (i = 0; i < 5; i++) (void)graph_add_vertex(g, vn[i]);
            (void)graph_add_edge(g,"S","A",2,"a1");
            (void)graph_add_edge(g,"S","B",2,"a2");
            (void)graph_add_edge(g,"S","C",2,"a3");
            (void)graph_add_edge(g,"A","T",3,"a4");
            (void)graph_add_edge(g,"B","T",3,"a5");
            (void)graph_add_edge(g,"C","T",3,"a6");
            return 1; /* 总工期5, 三条都是关键路径 */
        }
        default: return 0;
    }
}

static int run_self_tests(void)
{
    Graph g; CpmResult r; CpmStatus st;
    tests_passed = 0; tests_failed = 0;

    print_sep_dbl();
    c_print(COLOR_BOLD COLOR_CYAN, "  CPM V2.0 内置自测套件\n");
    print_sep_dbl();

    /* T01: 空图 */
    graph_init(&g);
    st = cpm_calc(&g, &r);
    test_check(st == CPM_ERR_EMPTY_GRAPH, "T01 空图返回 CPM_ERR_EMPTY_GRAPH");
    graph_free(&g);

    /* T02: 经典10顶点例，总工期31 */
    build_test_graph(&g, 0);
    st = cpm_calc(&g, &r);
    test_check(st == CPM_OK, "T02 经典例拓扑排序成功");
    test_check(!r.has_cycle, "T03 经典例无环");
    test_check(r.total_duration == 31, "T04 经典例总工期=31");
    test_check(r.crit_edge_count > 0, "T05 经典例有关键活动");
    cpm_result_free(&r); graph_free(&g);

    /* T06-T07: 线性链 */
    build_test_graph(&g, 1);
    st = cpm_calc(&g, &r);
    test_check(st == CPM_OK && r.total_duration == 9, "T06 线性链总工期=9");
    test_check(r.crit_edge_count == 3, "T07 线性链全部3活动为关键");
    cpm_result_free(&r); graph_free(&g);

    /* T08: 并行路径 */
    build_test_graph(&g, 2);
    st = cpm_calc(&g, &r);
    test_check(st == CPM_OK && r.total_duration == 6, "T08 并行路径总工期=6");
    cpm_result_free(&r); graph_free(&g);

    /* T09: 菱形 */
    build_test_graph(&g, 4);
    st = cpm_calc(&g, &r);
    test_check(st == CPM_OK && r.total_duration == 4, "T09 菱形总工期=4");
    cpm_result_free(&r); graph_free(&g);

    /* T10: 环检测 (V0->V1->V2->V0) */
    build_test_graph(&g, 5);
    st = cpm_calc(&g, &r);
    test_check(st == CPM_ERR_CYCLE_DETECTED, "T10 三角环检测成功");
    graph_free(&g);

    /* T11: 自环检测 */
    build_test_graph(&g, 7);
    st = cpm_calc(&g, &r);
    test_check(st == CPM_ERR_CYCLE_DETECTED, "T11 自环检测成功");
    graph_free(&g);

    /* T12: 单顶点无边 */
    build_test_graph(&g, 3);
    st = cpm_calc(&g, &r);
    test_check(st == CPM_OK && r.total_duration == 0, "T12 单顶点总工期=0");
    cpm_result_free(&r); graph_free(&g);

    /* T13: 两顶点单边 */
    build_test_graph(&g, 8);
    st = cpm_calc(&g, &r);
    test_check(st == CPM_OK && r.total_duration == 10, "T13 单边总工期=10");
    cpm_result_free(&r); graph_free(&g);

    /* T14: 教材例(6顶点)总工期=9 */
    build_test_graph(&g, 6);
    st = cpm_calc(&g, &r);
    test_check(st == CPM_OK && r.total_duration == 9, "T14 6顶点教材例总工期=9");
    cpm_result_free(&r); graph_free(&g);

    /* T15: 三条等长关键路径 */
    build_test_graph(&g, 9);
    st = cpm_calc(&g, &r);
    test_check(st == CPM_OK && r.total_duration == 5, "T15 三并行路径总工期=5");
    test_check(r.crit_edge_count == 6, "T16 三路径全部6活动为关键");
    cpm_result_free(&r); graph_free(&g);

    /* T17-T19: 文件I/O 二进制持久化 */
    build_test_graph(&g, 0);
    st = graph_save_binary(&g, "cpm_test.bin");
    test_check(st == CPM_OK, "T17 二进制保存成功");
    {
        Graph g2; CpmResult r2;
        (void)graph_init(&g2);
        st = graph_load_binary(&g2, "cpm_test.bin");
        test_check(st == CPM_OK, "T18 二进制加载成功");
        test_check(g2.vert_count == g.vert_count && g2.edge_count == g.edge_count,
                   "T19 加载后顶点/边数一致");
        st = cpm_calc(&g2, &r2);
        test_check(st == CPM_OK && r2.total_duration == 31, "T20 加载后计算总工期仍=31");
        cpm_result_free(&r2); graph_free(&g2);
    }
    graph_free(&g);
    remove("cpm_test.bin");

    /* T21: 错误文件魔数 */
    {
        FILE *fp = fopen("cpm_bad.bin","wb");
        if (fp) { fprintf(fp,"XXXX"); fclose(fp); }
        Graph g3; (void)graph_init(&g3);
        st = graph_load_binary(&g3, "cpm_bad.bin");
        test_check(st == CPM_ERR_FILE_MAGIC, "T21 错误魔数文件检测");
        graph_free(&g3); remove("cpm_bad.bin");
    }

    /* T22: 重复顶点 */
    {
        Graph g4; (void)graph_init(&g4);
        st = graph_add_vertex(&g4, "A");
        test_check(st == CPM_OK, "T22 首次添加顶点成功");
        st = graph_add_vertex(&g4, "A");
        test_check(st == CPM_ERR_DUPLICATE_VERTEX, "T23 重复顶点返回错误");
        graph_free(&g4);
    }

    /* T24: 负权值 */
    {
        Graph g5; (void)graph_init(&g5);
        (void)graph_add_vertex(&g5,"S");(void)graph_add_vertex(&g5,"T");
        st = graph_add_edge(&g5,"S","T",-1,"bad");
        test_check(st == CPM_ERR_INVALID_WEIGHT, "T24 负权值拒绝");
        graph_free(&g5);
    }

    /* T25: 动态扩容 */
    {
        Graph g6; int k; char name[16];
        (void)graph_init(&g6);
        for (k = 0; k < 50; k++) {
            sprintf(name,"V%d",k);
            (void)graph_add_vertex(&g6, name);
        }
        test_check(g6.vert_count == 50, "T25 动态扩容至50顶点");
        graph_free(&g6);
    }

    print_sep_dbl();
    c_print(COLOR_BOLD, "  测试结果: ");
    c_print(COLOR_GREEN, "%d 通过", tests_passed);
    c_print(COLOR_RESET, " / ");
    c_print(COLOR_RED, "%d 失败", tests_failed);
    c_print(COLOR_RESET, " / 共 %d 项\n", tests_passed + tests_failed);
    print_sep_dbl();

    return tests_failed == 0 ? 0 : 1;
}

/* ===========================================================================
 * 第十部分：交互菜单
 * =========================================================================== */

static void menu_banner(void)
{
    print_sep_dbl();
    c_print(COLOR_BOLD COLOR_CYAN,
            "      关键路径方法 (Critical Path Method) V2.0\n");
    c_print(COLOR_WHITE,
            "      AOE网 · 拓扑排序 · 动态内存 · 二进制持久化\n");
    print_sep_dbl();
}

static void menu_help(void)
{
    c_print(COLOR_CYAN, "\n用法: cpm [选项]\n\n");
    c_print(COLOR_YELLOW, "选项:\n");
    printf("  -h, --help       显示帮助信息\n");
    printf("  -t, --test       运行内置自测套件 (25项)\n");
    printf("  (无参数)         进入交互菜单模式\n\n");
    c_print(COLOR_CYAN, "交互菜单命令:\n");
    printf("  1. 加载示例图 (经典10顶点16边, 总工期31)\n");
    printf("  2. 手动添加顶点/边构建AOE网\n");
    printf("  3. 从文件加载 (CPMV2二进制格式)\n");
    printf("  4. 保存到文件\n");
    printf("  5. 显示邻接表\n");
    printf("  6. 计算关键路径\n");
    printf("  7. 清空当前图\n");
    printf("  0. 退出\n\n");
}

static char *trim(char *s)
{
    char *end;
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == '\0') return s;
    end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}

static void interactive_loop(void)
{
    Graph g; CpmResult res; CpmStatus st;
    int has_result = 0, loaded = 0;
    char line[256], vfrom[MAX_NAME_LEN], vto[MAX_NAME_LEN], ename[MAX_NAME_LEN];
    int w, idx;
    (void)graph_init(&g);
    memset(&res, 0, sizeof(res));

    menu_banner();
    for (;;) {
        print_sep();
        c_print(COLOR_BOLD COLOR_YELLOW, "[菜单] ");
        c_print(COLOR_WHITE, "请选择操作 (h=帮助, 0=退出): ");
        if (!fgets(line, sizeof(line), stdin)) break;
        { char *t = trim(line); if (*t == '\0') continue; }

        switch (line[0]) {
            case '0':
                print_info("正在退出...");
                goto exit_loop;
            case 'h': case 'H':
                menu_help();
                break;
            case '1':
                build_test_graph(&g, 0);
                loaded = 1; has_result = 0;
                print_ok("已加载经典示例: 10顶点 16边");
                break;
            case '2': {
                print_info("--- 手动构建 AOE 网 ---");
                printf("顶点名（空行结束）: ");
                while (fgets(line, sizeof(line), stdin)) {
                    char *t = trim(line);
                    if (*t == '\0') break;
                    if ((st = graph_add_vertex(&g, t)) != CPM_OK)
                        print_err(status_msg(st));
                    else printf("  + 顶点 %s 添加成功\n", t);
                    printf("顶点名（空行结束）: ");
                }
                print_info("现在添加边 (格式: 起点 终点 权值 活动名, 空行结束)");
                printf("边> ");
                while (fgets(line, sizeof(line), stdin)) {
                    char *t = trim(line);
                    if (*t == '\0') break;
                    if (sscanf(line, "%63s %63s %d %63s", vfrom, vto, &w, ename) == 4) {
                        if ((st = graph_add_edge(&g, vfrom, vto, w, ename)) != CPM_OK)
                            print_err(status_msg(st));
                        else printf("  + 边 %s->%s (%s,%d) 添加成功\n", vfrom, vto, ename, w);
                    } else print_err("格式错误, 应为: 起点 终点 权值 活动名");
                    printf("边> ");
                }
                loaded = 1; has_result = 0;
                print_ok("手动构建完成");
                break;
            }
            case '3': {
                char path[256];
                printf("请输入文件路径 (默认 cpm_data.bin): ");
                if (!fgets(path, sizeof(path), stdin)) break;
                { char *t = trim(path); if (*t == '\0') strcpy(path, "cpm_data.bin"); }
                st = graph_load_binary(&g, trim(path));
                if (st == CPM_OK) { print_ok("文件加载成功"); loaded = 1; has_result = 0; }
                else print_err(status_msg(st));
                break;
            }
            case '4': {
                char path[256];
                if (!loaded) { print_err("图为空, 无法保存"); break; }
                printf("请输入保存路径 (默认 cpm_data.bin): ");
                if (!fgets(path, sizeof(path), stdin)) break;
                { char *t = trim(path); if (*t == '\0') strcpy(path, "cpm_data.bin"); }
                st = graph_save_binary(&g, trim(path));
                if (st == CPM_OK) print_ok("保存成功");
                else print_err(status_msg(st));
                break;
            }
            case '5':
                if (!loaded) { print_err("图为空, 请先加载或构建"); break; }
                graph_print(&g, has_result ? &res : NULL);
                break;
            case '6':
                if (!loaded) { print_err("图为空, 请先加载或构建"); break; }
                cpm_result_free(&res);
                st = cpm_calc(&g, &res);
                if (st == CPM_OK) {
                    result_print(&g, &res);
                    has_result = 1;
                } else {
                    print_err(status_msg(st));
                    has_result = 0;
                }
                break;
            case '7':
                graph_free(&g);
                (void)graph_init(&g);
                has_result = 0; loaded = 0;
                cpm_result_free(&res); memset(&res, 0, sizeof(res));
                print_ok("图已清空");
                break;
            case '8':
                if (!loaded) { print_err("图为空"); break; }
                printf("输入顶点名查找索引: ");
                if (!fgets(line, sizeof(line), stdin)) break;
                trim(line);
                if (graph_find_vertex(&g, line, &idx) == CPM_OK)
                    c_print(COLOR_GREEN, "顶点 %s 索引=%d\n", line, idx);
                else print_err("顶点不存在");
                break;
            default:
                print_warn("无效命令, 输入 h 查看帮助");
        }
    }
exit_loop:
    cpm_result_free(&res);
    graph_free(&g);
}

/* ===========================================================================
 * 第十一部分：CLI 入口 main
 * =========================================================================== */

int main(int argc, char *argv[])
{
    int i;
    console_init();

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            menu_banner();
            menu_help();
            return 0;
        }
        if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--test") == 0) {
            return run_self_tests();
        }
        if (strcmp(argv[i], "--no-color") == 0) {
            use_color = 0;
        }
    }

    if (argc > 1 && use_color == 1) {
        /* 未知参数提示 */
        c_print(COLOR_YELLOW, "[提示] 未知参数，进入交互模式。使用 -h 查看帮助。\n");
    }

    interactive_loop();
    return 0;
}
