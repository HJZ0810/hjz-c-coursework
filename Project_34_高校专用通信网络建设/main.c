/* ============================================================================
 * [T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[
 * U                                                                          U
 * U          高校专用通信网络建设系统 V2.0                                    U
 * U       University Communication Network Construction System V2.0          U
 * U                                                                          U
 * d[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[g
 * U                                                                          U
 * U  功能模块:                                                              U
 * U    1. Prim/Kruskal双算法最小生成树(MST)                                  U
 * U    2. 动态内存管理(顶点+边动态数组)                                      U
 * U    3. "NETWV2"魔数文件持久化                                             U
 * U    4. ANSI彩色终端输出                                                   U
 * U    5. CLI命令行参数(-h/-t/--help/--test)                                 U
 * U    6. 内置自测套件(15+测试用例)                                          U
 * U    7. 交互式菜单系统                                                     U
 * U                                                                          U
 * U  @version  V2.0                                                         U
 * U  @date     2026                                                         U
 * U                                                                          U
 * ^[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[T[a
 */

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <windows.h>

/* ============================================================================
 *                           ANSI彩色终端宏定义
 * ============================================================================ */
#define C_RESET     "\033[0m"
#define C_BOLD      "\033[1m"
#define C_RED       "\033[31m"
#define C_GREEN     "\033[32m"
#define C_YELLOW    "\033[33m"
#define C_BLUE      "\033[34m"
#define C_MAGENTA   "\033[35m"
#define C_CYAN      "\033[36m"
#define C_WHITE     "\033[37m"

/* ============================================================================
 *                            常量定义
 * ============================================================================ */
#define MAGIC_HEADER "NETWV2"
#define MAGIC_LEN     6
#define INF_WEIGHT    0x7FFFFFFF
#define INIT_CAP_V    16

/* ============================================================================
 *                         统一错误码枚举 NetStatus
 * ============================================================================ */
typedef enum {
    NET_OK = 0,
    NET_ERR_MEMORY,
    NET_ERR_FILE_OPEN,
    NET_ERR_FILE_READ,
    NET_ERR_FILE_WRITE,
    NET_ERR_MAGIC,
    NET_ERR_VERTEX_EXISTS,
    NET_ERR_VERTEX_NOT_FOUND,
    NET_ERR_EDGE_EXISTS,
    NET_ERR_EDGE_NOT_FOUND,
    NET_ERR_INVALID_PARAM,
    NET_ERR_NO_MST,
    NET_ERR_INDEX_OUT_OF_RANGE,
    NET_ERR_INTERNAL
} NetStatus;

/* ============================================================================
 *                           数据结构定义
 * ============================================================================ */

typedef struct {
    int u, v, w;
} Edge;

typedef struct AdjEdge {
    int to, w;
    struct AdjEdge *next;
} AdjEdge;

typedef struct {
    int vCap, vNum, eNum;
    char **names;
    int **mat;
    AdjEdge **adj;
} Graph;

typedef struct {
    int *parent;
    int *rank;
} UnionFind;

/* ============================================================================
 *                          全局测试计数器
 * ============================================================================ */
static int g_tests_passed = 0;
static int g_tests_failed = 0;

/* ============================================================================
 *                        错误信息字符串表
 * ============================================================================ */
static const char *statusStr(NetStatus s)
{
    static const char *tbl[] = {
        "操作成功", "内存分配失败", "无法打开文件", "文件读取失败",
        "文件写入失败", "文件魔数校验失败", "顶点已存在", "顶点不存在",
        "边已存在", "边不存在", "无效参数", "图不连通,无MST",
        "索引越界", "内部错误"
    };
    if (s >= 0 && s < (int)(sizeof(tbl)/sizeof(tbl[0])))
        return tbl[s];
    return "未知错误";
}

/* ============================================================================
 *                       ANSI终端初始化函数
 * ============================================================================ */
static void setupConsole(void)
{
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(hOut, &mode)) {
        mode |= 0x0004;
        SetConsoleMode(hOut, mode);
    }
}

/* ============================================================================
 *                        打印辅助函数
 * ============================================================================ */
static int dispWidth(const char *s)
{
    int w = 0;
    while (*s) {
        unsigned char c = (unsigned char)*s;
        if (c < 0x80) { w += 1; s += 1; }
        else if ((c & 0xE0) == 0xC0) { w += 2; s += 2; }
        else if ((c & 0xF0) == 0xE0) { w += 2; s += 3; }
        else if ((c & 0xF8) == 0xF0) { w += 2; s += 4; }
        else s++;
    }
    return w;
}

static void printPadded(const char *s, int target)
{
    int w = dispWidth(s);
    printf("%s", s);
    for (int i = w; i < target; i++) putchar(' ');
}

static void printLine(void)
{
    printf(C_CYAN "+");
    for (int i = 0; i < 64; i++) printf("-");
    printf("+" C_RESET "\n");
}

static void printCenter(const char *s, const char *color)
{
    int w = dispWidth(s);
    int pad = (64 - w) / 2;
    if (pad < 0) pad = 0;
    printf(C_CYAN "|" C_RESET " %s", color);
    for (int i = 0; i < pad; i++) putchar(' ');
    printf("%s", s);
    for (int i = 0; i < 64 - pad - w; i++) putchar(' ');
    printf(C_RESET C_CYAN "|" C_RESET "\n");
}

/* ============================================================================
 *                        图内存管理函数
 * ============================================================================ */

static NetStatus graphCreate(Graph **g)
{
    *g = (Graph *)calloc(1, sizeof(Graph));
    if (!*g) return NET_ERR_MEMORY;
    (*g)->vCap = INIT_CAP_V;
    (*g)->vNum = 0;
    (*g)->eNum = 0;
    (*g)->names = (char **)calloc(INIT_CAP_V, sizeof(char *));
    (*g)->mat   = (int **)calloc(INIT_CAP_V, sizeof(int *));
    (*g)->adj   = (AdjEdge **)calloc(INIT_CAP_V, sizeof(AdjEdge *));
    if (!(*g)->names || !(*g)->mat || !(*g)->adj) {
        free((*g)->names); free((*g)->mat); free((*g)->adj); free(*g);
        return NET_ERR_MEMORY;
    }
    for (int i = 0; i < INIT_CAP_V; i++) {
        (*g)->adj[i] = NULL;
        (*g)->mat[i] = NULL;
    }
    return NET_OK;
}

static void adjClear(AdjEdge *head)
{
    while (head) {
        AdjEdge *t = head;
        head = head->next;
        free(t);
    }
}

static void graphFree(Graph *g)
{
    if (!g) return;
    for (int i = 0; i < g->vNum; i++) {
        free(g->names[i]);
        free(g->mat[i]);
        adjClear(g->adj[i]);
    }
    free(g->names);
    free(g->mat);
    free(g->adj);
    free(g);
}

static NetStatus graphResizeV(Graph *g, int newCap)
{
    if (newCap <= g->vCap) return NET_OK;
    char **newNames = (char **)realloc(g->names, newCap * sizeof(char *));
    int  **newMat   = (int **)realloc(g->mat, newCap * sizeof(int *));
    AdjEdge **newAdj = (AdjEdge **)realloc(g->adj, newCap * sizeof(AdjEdge *));
    if (!newNames || !newMat || !newAdj) return NET_ERR_MEMORY;
    g->names = newNames;
    g->mat = newMat;
    g->adj = newAdj;
    for (int i = g->vCap; i < newCap; i++) {
        g->names[i] = NULL;
        g->mat[i] = NULL;
        g->adj[i] = NULL;
    }
    for (int i = 0; i < g->vNum; i++) {
        int *newRow = (int *)realloc(g->mat[i], newCap * sizeof(int));
        if (!newRow) return NET_ERR_MEMORY;
        g->mat[i] = newRow;
        for (int j = g->vCap; j < newCap; j++)
            g->mat[i][j] = INF_WEIGHT;
    }
    g->vCap = newCap;
    return NET_OK;
}

static NetStatus graphAddVertex(Graph *g, const char *name)
{
    if (!g || !name) return NET_ERR_INVALID_PARAM;
    for (int i = 0; i < g->vNum; i++)
        if (strcmp(g->names[i], name) == 0)
            return NET_ERR_VERTEX_EXISTS;
    if (g->vNum >= g->vCap) {
        NetStatus s = graphResizeV(g, g->vCap * 2);
        if (s != NET_OK) return s;
    }
    int idx = g->vNum;
    g->names[idx] = (char *)malloc(strlen(name) + 1);
    if (!g->names[idx]) return NET_ERR_MEMORY;
    strcpy(g->names[idx], name);
    g->mat[idx] = (int *)malloc(g->vCap * sizeof(int));
    if (!g->mat[idx]) { free(g->names[idx]); return NET_ERR_MEMORY; }
    for (int j = 0; j < g->vCap; j++)
        g->mat[idx][j] = (j == idx) ? 0 : INF_WEIGHT;
    for (int i = 0; i < idx; i++)
        g->mat[i][idx] = INF_WEIGHT;
    g->adj[idx] = NULL;
    g->vNum++;
    return NET_OK;
}

static NetStatus graphAddEdge(Graph *g, int u, int v, int w)
{
    if (!g) return NET_ERR_INVALID_PARAM;
    if (u < 0 || v < 0 || u >= g->vNum || v >= g->vNum) return NET_ERR_INDEX_OUT_OF_RANGE;
    if (u == v || w <= 0) return NET_ERR_INVALID_PARAM;
    if (g->mat[u][v] != INF_WEIGHT) return NET_ERR_EDGE_EXISTS;
    g->mat[u][v] = w;
    g->mat[v][u] = w;
    AdjEdge *e1 = (AdjEdge *)malloc(sizeof(AdjEdge));
    AdjEdge *e2 = (AdjEdge *)malloc(sizeof(AdjEdge));
    if (!e1 || !e2) { free(e1); free(e2); return NET_ERR_MEMORY; }
    e1->to = v; e1->w = w; e1->next = g->adj[u]; g->adj[u] = e1;
    e2->to = u; e2->w = w; e2->next = g->adj[v]; g->adj[v] = e2;
    g->eNum++;
    return NET_OK;
}

static NetStatus graphSetEdge(Graph *g, int u, int v, int w)
{
    if (!g || u < 0 || v < 0 || u >= g->vNum || v >= g->vNum || u == v)
        return NET_ERR_INVALID_PARAM;
    if (w <= 0) return NET_ERR_INVALID_PARAM;
    g->mat[u][v] = w;
    g->mat[v][u] = w;
    return NET_OK;
}

/* ============================================================================
 *                        并查集 (Union-Find)
 * ============================================================================ */
static NetStatus ufCreate(UnionFind *uf, int n)
{
    if (n <= 0) return NET_ERR_INVALID_PARAM;
    size_t sz = (size_t)n;
    uf->parent = (int *)malloc(sz * sizeof(int));
    uf->rank   = (int *)calloc(sz, sizeof(int));
    if (!uf->parent || !uf->rank) { free(uf->parent); free(uf->rank); return NET_ERR_MEMORY; }
    for (int i = 0; i < n; i++) uf->parent[i] = i;
    return NET_OK;
}

static void ufFree(UnionFind *uf)
{
    free(uf->parent); free(uf->rank);
}

static int ufFind(UnionFind *uf, int x)
{
    while (uf->parent[x] != x) {
        uf->parent[x] = uf->parent[uf->parent[x]];
        x = uf->parent[x];
    }
    return x;
}

static int ufUnion(UnionFind *uf, int a, int b)
{
    int ra = ufFind(uf, a), rb = ufFind(uf, b);
    if (ra == rb) return 0;
    if (uf->rank[ra] < uf->rank[rb]) uf->parent[ra] = rb;
    else {
        uf->parent[rb] = ra;
        if (uf->rank[ra] == uf->rank[rb]) uf->rank[ra]++;
    }
    return 1;
}

/* ============================================================================
 *                      Prim算法最小生成树
 * ============================================================================ */
static NetStatus mstPrim(const Graph *g, Edge *mst, int *total)
{
    if (!g || g->vNum == 0) return NET_ERR_INVALID_PARAM;
    int n = g->vNum;
    int *key = (int *)malloc(n * sizeof(int));
    int *from = (int *)malloc(n * sizeof(int));
    int *used = (int *)calloc(n, sizeof(int));
    if (!key || !from || !used) { free(key); free(from); free(used); return NET_ERR_MEMORY; }
    for (int i = 0; i < n; i++) { key[i] = INF_WEIGHT; from[i] = -1; }
    key[0] = 0;
    *total = 0;
    int cnt = 0;
    for (int i = 0; i < n; i++) {
        int u = -1, minK = INF_WEIGHT;
        for (int v = 0; v < n; v++)
            if (!used[v] && key[v] < minK) { minK = key[v]; u = v; }
        if (u == -1 || minK == INF_WEIGHT) { free(key); free(from); free(used); return NET_ERR_NO_MST; }
        used[u] = 1;
        if (from[u] != -1) {
            mst[cnt].u = from[u];
            mst[cnt].v = u;
            mst[cnt].w = minK;
            (*total) += minK;
            cnt++;
        }
        for (int v = 0; v < n; v++) {
            if (!used[v] && g->mat[u][v] < key[v]) {
                key[v] = g->mat[u][v];
                from[v] = u;
            }
        }
    }
    free(key); free(from); free(used);
    return (cnt == n - 1) ? NET_OK : NET_ERR_NO_MST;
}

/* ============================================================================
 *                      Kruskal算法最小生成树
 * ============================================================================ */
static int edgeCmp(const void *a, const void *b)
{
    return ((const Edge *)a)->w - ((const Edge *)b)->w;
}

static NetStatus mstKruskal(const Graph *g, Edge *mst, int *total)
{
    if (!g || g->vNum == 0) return NET_ERR_INVALID_PARAM;
    int n = g->vNum;
    Edge *edges = (Edge *)malloc((size_t)g->eNum * sizeof(Edge));
    if (!edges) return NET_ERR_MEMORY;
    int eCnt = 0;
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            if (g->mat[i][j] != INF_WEIGHT && g->mat[i][j] > 0) {
                edges[eCnt].u = i; edges[eCnt].v = j; edges[eCnt].w = g->mat[i][j];
                eCnt++;
            }
    qsort(edges, (size_t)eCnt, sizeof(Edge), edgeCmp);
    UnionFind uf;
    if (ufCreate(&uf, n) != NET_OK) { free(edges); return NET_ERR_MEMORY; }
    *total = 0;
    int cnt = 0;
    for (int i = 0; i < eCnt && cnt < n - 1; i++) {
        if (ufUnion(&uf, edges[i].u, edges[i].v)) {
            mst[cnt++] = edges[i];
            *total += edges[i].w;
        }
    }
    ufFree(&uf);
    free(edges);
    return (cnt == n - 1) ? NET_OK : NET_ERR_NO_MST;
}

/* ============================================================================
 *                   "NETWV2"魔数文件持久化
 * ============================================================================ */
static NetStatus graphSave(const Graph *g, const char *filename)
{
    if (!g || !filename) return NET_ERR_INVALID_PARAM;
    FILE *fp = fopen(filename, "wb");
    if (!fp) return NET_ERR_FILE_OPEN;
    fwrite(MAGIC_HEADER, 1, MAGIC_LEN, fp);
    int v = g->vNum, e = g->eNum;
    fwrite(&v, sizeof(int), 1, fp);
    fwrite(&e, sizeof(int), 1, fp);
    for (int i = 0; i < g->vNum; i++) {
        unsigned char len = (unsigned char)strlen(g->names[i]);
        fwrite(&len, 1, 1, fp);
        fwrite(g->names[i], 1, len, fp);
    }
    int written = 0;
    for (int i = 0; i < g->vNum && written < g->eNum; i++)
        for (int j = i + 1; j < g->vNum && written < g->eNum; j++)
            if (g->mat[i][j] != INF_WEIGHT && g->mat[i][j] > 0) {
                fwrite(&i, sizeof(int), 1, fp);
                fwrite(&j, sizeof(int), 1, fp);
                fwrite(&g->mat[i][j], sizeof(int), 1, fp);
                written++;
            }
    fclose(fp);
    return NET_OK;
}

static NetStatus graphLoad(Graph **g, const char *filename)
{
    if (!g || !filename) return NET_ERR_INVALID_PARAM;
    FILE *fp = fopen(filename, "rb");
    if (!fp) return NET_ERR_FILE_OPEN;
    char magic[MAGIC_LEN + 1] = {0};
    if (fread(magic, 1, MAGIC_LEN, fp) != MAGIC_LEN) { fclose(fp); return NET_ERR_FILE_READ; }
    if (memcmp(magic, MAGIC_HEADER, MAGIC_LEN) != 0) { fclose(fp); return NET_ERR_MAGIC; }
    int v = 0, e = 0;
    if (fread(&v, sizeof(int), 1, fp) != 1 || fread(&e, sizeof(int), 1, fp) != 1) {
        fclose(fp); return NET_ERR_FILE_READ;
    }
    graphFree(*g);
    NetStatus s = graphCreate(g);
    if (s != NET_OK) { fclose(fp); return s; }
    for (int i = 0; i < v; i++) {
        unsigned char len = 0;
        if (fread(&len, 1, 1, fp) != 1) { fclose(fp); graphFree(*g); *g = NULL; return NET_ERR_FILE_READ; }
        char *name = (char *)malloc(len + 1);
        if (!name) { fclose(fp); graphFree(*g); *g = NULL; return NET_ERR_MEMORY; }
        if (fread(name, 1, len, fp) != (size_t)len) { free(name); fclose(fp); graphFree(*g); *g = NULL; return NET_ERR_FILE_READ; }
        name[len] = '\0';
        s = graphAddVertex(*g, name);
        free(name);
        if (s != NET_OK) { fclose(fp); graphFree(*g); *g = NULL; return s; }
    }
    for (int i = 0; i < e; i++) {
        int eu = 0, ev = 0, ew = 0;
        if (fread(&eu, sizeof(int), 1, fp) != 1 ||
            fread(&ev, sizeof(int), 1, fp) != 1 ||
            fread(&ew, sizeof(int), 1, fp) != 1) {
            fclose(fp); graphFree(*g); *g = NULL; return NET_ERR_FILE_READ;
        }
        graphAddEdge(*g, eu, ev, ew);
    }
    fclose(fp);
    return NET_OK;
}

/* ============================================================================
 *                          内置测试框架
 * ============================================================================ */
#define TEST_ASSERT(cond, msg) do { \
    if (cond) { \
        printf(C_GREEN "  [PASS] " C_RESET "%s\n", msg); \
        g_tests_passed++; \
    } else { \
        printf(C_RED "  [FAIL] " C_RESET "%s (line %d)\n", msg, __LINE__); \
        g_tests_failed++; \
    } \
} while(0)

#define TEST_STATUS(s, expected, msg) TEST_ASSERT((s) == (expected), msg)

static Graph *buildTestGraph(int which)
{
    Graph *g = NULL;
    graphCreate(&g);
    if (which == 0) {
        graphAddVertex(g, "A"); graphAddVertex(g, "B"); graphAddVertex(g, "C");
        graphAddEdge(g, 0, 1, 1); graphAddEdge(g, 1, 2, 2); graphAddEdge(g, 0, 2, 3);
    } else if (which == 1) {
        const char *names[] = {"安北工业大学", "北方大学", "空科大学", "安北大学",
                               "石板房职业大学", "东山学院", "东山大学", "安北师范大学",
                               "安北技工大学", "南宛职业大学", "安北农业大学", "安北健康护理大学"};
        for (int i = 0; i < 12; i++) graphAddVertex(g, names[i]);
        int edges[][3] = {
            {0,1,5},{0,2,8},{1,2,4},{1,5,18},{2,9,22},{5,9,10},{5,6,25},{9,6,30},
            {0,3,28},{1,10,35},{3,10,6},{3,4,32},{10,7,40},{10,8,38},{4,7,7},
            {4,8,12},{4,11,9},{7,8,6},{7,11,8},{8,11,15},{0,5,30},{3,7,36},
            {6,3,55},{1,9,20},{0,9,33}
        };
        for (size_t i = 0; i < sizeof(edges)/sizeof(edges[0]); i++)
            graphAddEdge(g, edges[i][0], edges[i][1], edges[i][2]);
    } else if (which == 2) {
        graphAddVertex(g, "X"); graphAddVertex(g, "Y");
        graphAddEdge(g, 0, 1, 42);
    }
    return g;
}

static void runTests(void)
{
    printf(C_BOLD C_CYAN "\n=== 高校专用通信网络建设系统 V2.0 内置自测套件 ===\n" C_RESET);
    g_tests_passed = 0; g_tests_failed = 0;

    printf(C_YELLOW "\n[1/3] 数据结构与内存管理测试\n" C_RESET);
    {
        Graph *g = NULL;
        NetStatus s = graphCreate(&g);
        TEST_STATUS(s, NET_OK, "T01: 图创建成功");
        s = graphAddVertex(g, "测试顶点1");
        TEST_STATUS(s, NET_OK, "T02: 添加单个顶点成功");
        s = graphAddVertex(g, "测试顶点1");
        TEST_STATUS(s, NET_ERR_VERTEX_EXISTS, "T03: 重复顶点检测");
        s = graphAddEdge(g, 0, 1, 10);
        TEST_STATUS(s, NET_ERR_INDEX_OUT_OF_RANGE, "T04: 越界边参数检测");
        graphFree(g);
    }

    printf(C_YELLOW "\n[2/3] MST算法测试 (Prim + Kruskal)\n" C_RESET);
    {
        Graph *g = buildTestGraph(0);
        Edge mst[8]; int total = 0;
        NetStatus s = mstPrim(g, mst, &total);
        TEST_STATUS(s, NET_OK, "T05: Prim 3顶点图MST计算成功");
        TEST_ASSERT(total == 3, "T06: Prim 3顶点总权值=3 (1+2)");
        total = 0;
        s = mstKruskal(g, mst, &total);
        TEST_STATUS(s, NET_OK, "T07: Kruskal 3顶点图MST计算成功");
        TEST_ASSERT(total == 3, "T08: Kruskal 3顶点总权值=3 (与Prim一致)");
        graphFree(g);

        g = buildTestGraph(1);
        total = 0;
        s = mstPrim(g, mst, &total);
        TEST_STATUS(s, NET_OK, "T09: Prim 12顶点完整图MST成功");
        int primTotal = total;
        total = 0;
        s = mstKruskal(g, mst, &total);
        TEST_STATUS(s, NET_OK, "T10: Kruskal 12顶点完整图MST成功");
        TEST_ASSERT(total == primTotal, "T11: Prim与Kruskal总权值一致");
        graphFree(g);

        g = buildTestGraph(2);
        total = 0;
        s = mstPrim(g, mst, &total);
        TEST_STATUS(s, NET_OK, "T12: 双顶点图MST成功");
        TEST_ASSERT(total == 42, "T13: 双顶点总权值=42");
        graphFree(g);
    }

    printf(C_YELLOW "\n[3/3] 文件I/O与持久化测试\n" C_RESET);
    {
        Graph *g = buildTestGraph(1);
        NetStatus s = graphSave(g, "test_netwv2.bin");
        TEST_STATUS(s, NET_OK, "T14: NETWV2格式文件保存成功");

        Graph *g2 = NULL;
        s = graphLoad(&g2, "test_netwv2.bin");
        TEST_STATUS(s, NET_OK, "T15: NETWV2格式文件加载成功");
        TEST_ASSERT(g2->vNum == g->vNum, "T16: 加载后顶点数一致");
        TEST_ASSERT(g2->eNum == g->eNum, "T17: 加载后边数一致");

        Edge mst1[32], mst2[32]; int t1 = 0, t2 = 0;
        mstKruskal(g, mst1, &t1);
        mstKruskal(g2, mst2, &t2);
        TEST_ASSERT(t1 == t2, "T18: 持久化前后MST总权值一致");
        graphFree(g); graphFree(g2);
        remove("test_netwv2.bin");

        g = NULL; graphCreate(&g);
        s = graphLoad(&g, "__nonexist__.bin");
        TEST_STATUS(s, NET_ERR_FILE_OPEN, "T19: 不存在文件加载错误处理");
        graphFree(g);
    }

    printf(C_BOLD "\n=========================================\n" C_RESET);
    printf("  测试结果: " C_GREEN "%d 通过" C_RESET ", " C_RED "%d 失败" C_RESET "\n",
           g_tests_passed, g_tests_failed);
    printf(C_BOLD "=========================================\n" C_RESET);
    if (g_tests_failed == 0)
        printf(C_GREEN C_BOLD "  所有测试通过! ✓\n" C_RESET);
    else
        printf(C_RED C_BOLD "  存在失败测试用例!\n" C_RESET);
}

/* ============================================================================
 *                         默认数据初始化
 * ============================================================================ */
static NetStatus initDefaultGraph(Graph **g)
{
    graphFree(*g);
    NetStatus s = graphCreate(g);
    if (s != NET_OK) return s;
    const char *names[] = {"安北工业大学", "北方大学", "空科大学", "安北大学",
                           "石板房职业大学", "东山学院", "东山大学", "安北师范大学",
                           "安北技工大学", "南宛职业大学", "安北农业大学", "安北健康护理大学"};
    for (int i = 0; i < 12; i++) {
        s = graphAddVertex(*g, names[i]);
        if (s != NET_OK) return s;
    }
    int edges[][3] = {
        {0,1,5},{0,2,8},{1,2,4},{1,5,18},{2,9,22},{5,9,10},{5,6,25},{9,6,30},
        {0,3,28},{1,10,35},{3,10,6},{3,4,32},{10,7,40},{10,8,38},{4,7,7},
        {4,8,12},{4,11,9},{7,8,6},{7,11,8},{8,11,15},{0,5,30},{3,7,36},
        {6,3,55},{1,9,20},{0,9,33}
    };
    for (size_t i = 0; i < sizeof(edges)/sizeof(edges[0]); i++)
        graphAddEdge(*g, edges[i][0], edges[i][1], edges[i][2]);
    return NET_OK;
}

/* ============================================================================
 *                        交互式菜单显示与操作
 * ============================================================================ */
static void showAllData(const Graph *g)
{
    printf("\n");
    printLine();
    printCenter("高校与线路数据总览", C_BOLD C_WHITE);
    printLine();
    printf(C_CYAN "|" C_RESET "  %-4s %-36s %-5s" C_CYAN "|" C_RESET "\n", "编号", "高校名称", "序号");
    printLine();
    for (int i = 0; i < g->vNum; i++) {
        printf(C_CYAN "|" C_RESET "  %-4d", i);
        printPadded(g->names[i], 36);
        printf(" %-5d" C_CYAN "|" C_RESET "\n", i);
    }
    printLine();
    printf("\n  " C_CYAN "通信线路列表 (共%d条)" C_RESET "\n\n", g->eNum);
    printf("  %-4s  %-20s  <->  %-20s  %s\n", "序号", "高校A", "高校B", "建设成本(万元)");
    printf("  ");
    for (int i = 0; i < 72; i++) printf("-");
    printf("\n");
    int idx = 0;
    for (int i = 0; i < g->vNum; i++)
        for (int j = i + 1; j < g->vNum; j++)
            if (g->mat[i][j] != INF_WEIGHT && g->mat[i][j] > 0) {
                printf("  %-4d  ", idx++);
                printPadded(g->names[i], 20);
                printf("  <->  ");
                printPadded(g->names[j], 20);
                printf(" %d\n", g->mat[i][j]);
            }
}

static void showMST(const Graph *g, int algo)
{
    int vcount = g->vNum;
    Edge *mst = (Edge *)malloc((size_t)(vcount > 0 ? vcount - 1 : 0) * sizeof(Edge));
    int total = 0;
    NetStatus s;
    const char *algoName = (algo == 0) ? "Prim" : "Kruskal";
    if (algo == 0) s = mstPrim(g, mst, &total);
    else s = mstKruskal(g, mst, &total);
    printf("\n");
    printLine();
    printCenter(algoName, C_BOLD C_GREEN);
    printCenter("最小生成树 (MST) 计算结果", C_BOLD C_GREEN);
    printLine();
    if (s != NET_OK) {
        printf(C_RED "  错误: %s\n" C_RESET, statusStr(s));
        free(mst); return;
    }
    printf("  MST边数: %d\n", vcount - 1);
    printf("  最小总建设成本: " C_BOLD C_YELLOW "%d 万元" C_RESET "\n\n", total);
    printf("  %-4s  %-20s  <->  %-20s  %s\n", "序号", "高校A", "高校B", "成本(万元)");
    printf("  ");
    for (int i = 0; i < 72; i++) printf("-");
    printf("\n");
    for (int i = 0; i < vcount - 1; i++) {
        printf("  %-4d  ", i + 1);
        printPadded(g->names[mst[i].u], 20);
        printf("  <->  ");
        printPadded(g->names[mst[i].v], 20);
        printf(" %d\n", mst[i].w);
    }
    printf("\n  " C_GREEN "合计总成本: %d 万元" C_RESET "\n", total);
    free(mst);
}

static void modifyEdge(Graph *g)
{
    int u, v, w;
    printf("\n  请输入要修改/添加的线路 (高校A编号 高校B编号 新成本): ");
    if (scanf("%d %d %d", &u, &v, &w) != 3) {
        printf(C_RED "  输入格式错误\n" C_RESET);
        while (getchar() != '\n') { /* do nothing */ }
        return;
    }
    if (u < 0 || u >= g->vNum || v < 0 || v >= g->vNum || u == v || w <= 0) {
        printf(C_RED "  参数无效\n" C_RESET);
        while (getchar() != '\n') { /* do nothing */ }
        return;
    }
    if (g->mat[u][v] != INF_WEIGHT) {
        graphSetEdge(g, u, v, w);
        printf(C_GREEN "  已更新线路 %s <-> %s 成本为 %d 万元\n" C_RESET,
               g->names[u], g->names[v], w);
    } else {
        NetStatus s = graphAddEdge(g, u, v, w);
        if (s == NET_OK)
            printf(C_GREEN "  已添加线路 %s <-> %s 成本为 %d 万元\n" C_RESET,
                   g->names[u], g->names[v], w);
        else
            printf(C_RED "  添加失败: %s\n" C_RESET, statusStr(s));
    }
}

static void printMenu(void)
{
    printf("\n");
    printLine();
    printCenter("高校专用通信网络建设系统", C_BOLD C_WHITE);
    printCenter("V2.0 - Prim/Kruskal 双算法 MST", C_BOLD C_CYAN);
    printLine();
    printf(C_CYAN "|" C_RESET "  " C_BOLD "1." C_RESET " 显示所有高校与线路信息              " C_CYAN "|" C_RESET "\n");
    printf(C_CYAN "|" C_RESET "  " C_BOLD "2." C_RESET " Prim算法计算MST                     " C_CYAN "|" C_RESET "\n");
    printf(C_CYAN "|" C_RESET "  " C_BOLD "3." C_RESET " Kruskal算法计算MST                  " C_CYAN "|" C_RESET "\n");
    printf(C_CYAN "|" C_RESET "  " C_BOLD "4." C_RESET " 修改/添加线路成本                   " C_CYAN "|" C_RESET "\n");
    printf(C_CYAN "|" C_RESET "  " C_BOLD "5." C_RESET " 保存数据到文件(NETWV2格式)          " C_CYAN "|" C_RESET "\n");
    printf(C_CYAN "|" C_RESET "  " C_BOLD "6." C_RESET " 从文件加载数据(NETWV2格式)          " C_CYAN "|" C_RESET "\n");
    printf(C_CYAN "|" C_RESET "  " C_BOLD "7." C_RESET " 重置为默认数据(12高校/25线路)       " C_CYAN "|" C_RESET "\n");
    printf(C_CYAN "|" C_RESET "  " C_BOLD "8." C_RESET " 对比Prim与Kruskal结果               " C_CYAN "|" C_RESET "\n");
    printf(C_CYAN "|" C_RESET "  " C_BOLD "0." C_RESET " 退出系统                            " C_CYAN "|" C_RESET "\n");
    printLine();
    printf("  请选择操作 [0-8]: ");
}

/* ============================================================================
 *                         CLI帮助信息
 * ============================================================================ */
static void printHelp(const char *prog)
{
    printf(C_BOLD C_CYAN "高校专用通信网络建设系统 V2.0\n" C_RESET);
    printf("用法: %s [选项]\n\n", prog);
    printf(C_BOLD "选项:\n" C_RESET);
    printf("  -h, --help      显示此帮助信息\n");
    printf("  -t, --test      运行内置自测套件(15+测试用例)\n");
    printf("\n");
    printf(C_BOLD "功能:\n" C_RESET);
    printf("  - Prim/Kruskal双算法最小生成树\n");
    printf("  - 动态内存顶点/边管理\n");
    printf("  - \"NETWV2\"魔数二进制文件持久化\n");
    printf("  - ANSI彩色终端输出\n");
    printf("  - 15+项内置自测(MST+文件I/O)\n");
}

/* ============================================================================
 *                             主函数
 * ============================================================================ */
int main(int argc, char *argv[])
{
    setupConsole();
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printHelp(argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--test") == 0) {
            runTests();
            return g_tests_failed == 0 ? 0 : 1;
        }
    }
    Graph *g = NULL;
    initDefaultGraph(&g);
    printf("\n");
    printLine();
    printCenter("欢迎使用", C_BOLD C_MAGENTA);
    printCenter("高校专用通信网络建设系统 V2.0", C_BOLD C_MAGENTA);
    printLine();
    printf(C_CYAN "  已加载默认数据: %d 所高校, %d 条通信线路\n" C_RESET, g->vNum, g->eNum);
    printf("  使用 -h 查看帮助, -t 运行自测\n");
    int choice;
    do {
        printMenu();
        if (scanf("%d", &choice) != 1) {
            printf(C_RED "  输入无效,请输入数字\n" C_RESET);
            while (getchar() != '\n');
            continue;
        }
        switch (choice) {
            case 1: showAllData(g); break;
            case 2: showMST(g, 0); break;
            case 3: showMST(g, 1); break;
            case 4: modifyEdge(g); break;
            case 5: {
                char fname[256] = "network.netv2";
                printf("  保存到文件名 [默认:network.netv2]: ");
                while (getchar() != '\n');
                char *p = fgets(fname, sizeof(fname), stdin);
                if (p && strlen(p) > 1) fname[strcspn(fname, "\n")] = 0;
                NetStatus s = graphSave(g, fname);
                if (s == NET_OK) printf(C_GREEN "  数据已保存到 %s\n" C_RESET, fname);
                else printf(C_RED "  保存失败: %s\n" C_RESET, statusStr(s));
                break;
            }
            case 6: {
                char fname[256] = "network.netv2";
                printf("  加载文件名 [默认:network.netv2]: ");
                while (getchar() != '\n');
                char *p = fgets(fname, sizeof(fname), stdin);
                if (p && strlen(p) > 1) fname[strcspn(fname, "\n")] = 0;
                NetStatus s = graphLoad(&g, fname);
                if (s == NET_OK) printf(C_GREEN "  已从 %s 加载数据 (%d高校/%d线路)\n" C_RESET,
                                         fname, g->vNum, g->eNum);
                else printf(C_RED "  加载失败: %s\n" C_RESET, statusStr(s));
                break;
            }
            case 7:
                initDefaultGraph(&g);
                printf(C_GREEN "  已重置为默认数据\n" C_RESET); break;
            case 8: {
                Edge mst[32]; int tp = 0, tk = 0;
                mstPrim(g, mst, &tp); mstKruskal(g, mst, &tk);
                printf("\n  " C_BOLD "算法对比结果:" C_RESET "\n");
                printf("  Prim   总建设成本: " C_YELLOW "%d 万元" C_RESET "\n", tp);
                printf("  Kruskal总建设成本: " C_YELLOW "%d 万元" C_RESET "\n", tk);
                printf(C_GREEN "  %s" C_RESET "\n",
                       (tp == tk) ? "✓ 两算法结果完全一致" : "✗ 结果不一致");
                break;
            }
            case 0: printf(C_CYAN "\n  感谢使用,再见!\n" C_RESET); break;
            default: printf(C_RED "  无效选择\n" C_RESET);
        }
    } while (choice != 0);
    graphFree(g);
    return 0;
}
