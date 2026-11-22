/*
 * ╔══════════════════════════════════════════════════════════════════════════╗
 * ║                                                                          ║
 * ║                    校园导游咨询系统 V2.0                                  ║
 * ║                    Campus Tour Guide System V2.0                          ║
 * ║                                                                          ║
 * ╠══════════════════════════════════════════════════════════════════════════╣
 * ║                                                                          ║
 * ║  版本特性：                                                              ║
 * ║    - UTF-8编码+BOM，Windows控制台UTF-8输出                               ║
 * ║    - ANSI彩色终端界面                                                    ║
 * ║    - CLI参数支持(-h/--help, -t/--test)                                    ║
 * ║    - Dijkstra + Floyd双算法最短路径                                      ║
 * ║    - 动态内存管理(景点+邻接矩阵动态数组)                                 ║
 * ║    - 统一错误码枚举(GuideStatus)                                         ║
 * ║    - "GUIDV2"魔数文件持久化(二进制格式)                                  ║
 * ║    - 内置15+项单元自测                                                   ║
 * ║    - 交互式菜单系统                                                      ║
 * ║    - 完整中文注释                                                        ║
 * ║                                                                          ║
 * ║  编译命令：                                                              ║
 * ║    D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o guide.exe main.c         ║
 * ║                                                                          ║
 * ║  作者：数据结构课程设计                                                  ║
 * ║  版本：V2.0                                                              ║
 * ║                                                                          ║
 * ╚══════════════════════════════════════════════════════════════════════════╝
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* =========================================================================
 *
 *                          常量与宏定义
 *
 * ========================================================================= */

#define DATA_FILE_V2     "campus_v2.dat"
#define MAGIC_HEADER     "GUIDV2"
#define MAGIC_LEN        6
#define MAX_NAME         64
#define MAX_DESC         256
#define INF_DIST         0x3FFFFFFF
#define INITIAL_CAPACITY 16

/* ANSI颜色码 */
#define COLOR_RESET     "\033[0m"
#define COLOR_RED       "\033[31m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_BLUE      "\033[34m"
#define COLOR_MAGENTA   "\033[35m"
#define COLOR_CYAN      "\033[36m"
#define COLOR_WHITE     "\033[37m"
#define COLOR_BOLD      "\033[1m"
#define COLOR_BG_BLUE   "\033[44m"

/* =========================================================================
 *
 *                          统一错误码枚举
 *
 * ========================================================================= */

typedef enum {
    GUIDE_OK                = 0,
    GUIDE_ERR_MEMORY        = -1,
    GUIDE_ERR_FILE_OPEN     = -2,
    GUIDE_ERR_FILE_READ     = -3,
    GUIDE_ERR_FILE_WRITE    = -4,
    GUIDE_ERR_MAGIC         = -5,
    GUIDE_ERR_VERSION       = -6,
    GUIDE_ERR_NOT_FOUND     = -7,
    GUIDE_ERR_INVALID_PARAM = -8,
    GUIDE_ERR_INVALID_INPUT = -9,
    GUIDE_ERR_NO_PATH       = -10,
    GUIDE_ERR_FULL          = -11
} GuideStatus;

/* =========================================================================
 *
 *                          数据结构定义
 *
 * ========================================================================= */

typedef struct {
    int  id;
    char name[MAX_NAME];
    char desc[MAX_DESC];
} ScenicSpot;

typedef struct {
    int capacity;
    int vexNum;
    int edgeNum;
    ScenicSpot *spots;
    int **matrix;
} CampusGraph;

/* =========================================================================
 *
 *                          全局变量
 *
 * ========================================================================= */

static CampusGraph g_graph;
static int g_colorEnabled = 1;
static int g_testMode = 0;
static int g_testsPassed = 0;
static int g_testsFailed = 0;

/* =========================================================================
 *
 *                          函数声明
 *
 * ========================================================================= */

/* 初始化与销毁 */
GuideStatus graphInit(CampusGraph *g, int initialCap);
void        graphFree(CampusGraph *g);
GuideStatus graphAddSpot(CampusGraph *g, int id, const char *name, const char *desc);
GuideStatus graphAddEdge(CampusGraph *g, int u, int v, int weight);
int         graphFindSpot(const CampusGraph *g, int id);

/* 最短路径算法 */
GuideStatus dijkstra(const CampusGraph *g, int start, int end, int *dist, int *prev);
GuideStatus floydWarshall(CampusGraph *g, int ***dist, int ***next);
void        freeFloydMatrices(int **dist, int **next, int n);
void        reconstructPath(const int *prev, int start, int end, int *path, int *pathLen);
void        reconstructFloydPath(int **next, int start, int end, int *path, int *pathLen);

/* 文件持久化 */
GuideStatus saveToFileV2(const CampusGraph *g, const char *filename);
GuideStatus loadFromFileV2(CampusGraph *g, const char *filename);

/* 初始化测试数据 */
GuideStatus initDefaultData(CampusGraph *g);

/* 终端彩色输出 */
void colorPrint(const char *color, const char *fmt, ...);
void setupConsole(void);
void printDivider(void);
void printHeader(const char *title);

/* 用户界面 */
void showHelp(void);
void showMenu(void);
void showAllSpots(void);
void querySpotInfo(void);
void queryShortestPath(void);
void queryAllPathsFloyd(void);
void showAsciiMap(void);
int  inputInt(const char *prompt);
void clearInputBuffer(void);

/* 单元测试 */
void runSelfTests(void);
void testAssert(int condition, const char *testName);
void testSummary(void);

/* =========================================================================
 *
 *                          彩色输出与控制台设置
 *
 * ========================================================================= */

void setupConsole(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(hOut, &mode)) {
            mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT;
            SetConsoleMode(hOut, mode);
        }
    }
#endif
}

void colorPrint(const char *color, const char *fmt, ...)
{
    va_list args;
    if (g_colorEnabled && color) {
        printf("%s", color);
    }
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    if (g_colorEnabled && color) {
        printf("%s", COLOR_RESET);
    }
}

void printDivider(void)
{
    colorPrint(COLOR_CYAN, "═");
    for (int i = 0; i < 68; i++) {
        colorPrint(COLOR_CYAN, "═");
    }
    colorPrint(COLOR_CYAN, "═\n");
}

void printHeader(const char *title)
{
    printf("\n");
    printDivider();
    colorPrint(COLOR_CYAN, "║");
    colorPrint(COLOR_BOLD COLOR_YELLOW, "  %-66s", title);
    colorPrint(COLOR_CYAN, "║\n");
    printDivider();
}

/* =========================================================================
 *
 *                          图的初始化与管理
 *
 * ========================================================================= */

GuideStatus graphInit(CampusGraph *g, int initialCap)
{
    if (!g || initialCap <= 0) return GUIDE_ERR_INVALID_PARAM;
    
    g->capacity = initialCap;
    g->vexNum = 0;
    g->edgeNum = 0;
    
    g->spots = (ScenicSpot *)calloc(initialCap, sizeof(ScenicSpot));
    if (!g->spots) return GUIDE_ERR_MEMORY;
    
    g->matrix = (int **)calloc(initialCap, sizeof(int *));
    if (!g->matrix) {
        free(g->spots);
        return GUIDE_ERR_MEMORY;
    }
    
    for (int i = 0; i < initialCap; i++) {
        g->matrix[i] = (int *)calloc(initialCap, sizeof(int));
        if (!g->matrix[i]) {
            for (int j = 0; j < i; j++) free(g->matrix[j]);
            free(g->matrix);
            free(g->spots);
            return GUIDE_ERR_MEMORY;
        }
        for (int j = 0; j < initialCap; j++) {
            g->matrix[i][j] = (i == j) ? 0 : INF_DIST;
        }
    }
    
    return GUIDE_OK;
}

void graphFree(CampusGraph *g)
{
    if (!g) return;
    if (g->spots) free(g->spots);
    if (g->matrix) {
        for (int i = 0; i < g->capacity; i++) {
            if (g->matrix[i]) free(g->matrix[i]);
        }
        free(g->matrix);
    }
    g->spots = NULL;
    g->matrix = NULL;
    g->vexNum = 0;
    g->edgeNum = 0;
    g->capacity = 0;
}

static GuideStatus graphExpand(CampusGraph *g)
{
    int newCap = g->capacity * 2;
    ScenicSpot *newSpots = (ScenicSpot *)realloc(g->spots, newCap * sizeof(ScenicSpot));
    if (!newSpots) return GUIDE_ERR_MEMORY;
    g->spots = newSpots;
    
    int **newMatrix = (int **)realloc(g->matrix, newCap * sizeof(int *));
    if (!newMatrix) return GUIDE_ERR_MEMORY;
    g->matrix = newMatrix;
    
    for (int i = g->capacity; i < newCap; i++) {
        g->matrix[i] = (int *)calloc(newCap, sizeof(int));
        if (!g->matrix[i]) return GUIDE_ERR_MEMORY;
    }
    
    for (int i = 0; i < newCap; i++) {
        int *newRow = (int *)realloc(g->matrix[i], newCap * sizeof(int));
        if (!newRow) return GUIDE_ERR_MEMORY;
        g->matrix[i] = newRow;
        for (int j = g->capacity; j < newCap; j++) {
            g->matrix[i][j] = (i == j) ? 0 : INF_DIST;
        }
    }
    
    for (int i = g->capacity; i < newCap; i++) {
        for (int j = 0; j < g->capacity; j++) {
            g->matrix[i][j] = (i == j) ? 0 : INF_DIST;
        }
    }
    
    g->capacity = newCap;
    return GUIDE_OK;
}

int graphFindSpot(const CampusGraph *g, int id)
{
    if (!g) return -1;
    for (int i = 0; i < g->vexNum; i++) {
        if (g->spots[i].id == id) return i;
    }
    return -1;
}

GuideStatus graphAddSpot(CampusGraph *g, int id, const char *name, const char *desc)
{
    if (!g || !name || !desc) return GUIDE_ERR_INVALID_PARAM;
    if (graphFindSpot(g, id) >= 0) return GUIDE_ERR_INVALID_PARAM;
    
    if (g->vexNum >= g->capacity) {
        GuideStatus st = graphExpand(g);
        if (st != GUIDE_OK) return st;
    }
    
    int idx = g->vexNum;
    g->spots[idx].id = id;
    strncpy(g->spots[idx].name, name, MAX_NAME - 1);
    g->spots[idx].name[MAX_NAME - 1] = '\0';
    strncpy(g->spots[idx].desc, desc, MAX_DESC - 1);
    g->spots[idx].desc[MAX_DESC - 1] = '\0';
    
    for (int i = 0; i <= g->vexNum; i++) {
        g->matrix[idx][i] = (idx == i) ? 0 : INF_DIST;
        g->matrix[i][idx] = (idx == i) ? 0 : INF_DIST;
    }
    g->vexNum++;
    
    return GUIDE_OK;
}

GuideStatus graphAddEdge(CampusGraph *g, int u, int v, int weight)
{
    if (!g) return GUIDE_ERR_INVALID_PARAM;
    if (u < 0 || u >= g->vexNum || v < 0 || v >= g->vexNum) return GUIDE_ERR_INVALID_PARAM;
    if (weight <= 0) return GUIDE_ERR_INVALID_PARAM;
    
    if (g->matrix[u][v] == INF_DIST) {
        g->edgeNum++;
    }
    g->matrix[u][v] = weight;
    g->matrix[v][u] = weight;
    
    return GUIDE_OK;
}

/* =========================================================================
 *
 *                          Dijkstra算法
 *
 * ========================================================================= */

GuideStatus dijkstra(const CampusGraph *g, int start, int end, int *dist, int *prev)
{
    if (!g || !dist || !prev) return GUIDE_ERR_INVALID_PARAM;
    int n = g->vexNum;
    if (start < 0 || start >= n || end < 0 || end >= n) return GUIDE_ERR_INVALID_PARAM;
    
    int *visited = (int *)calloc(n, sizeof(int));
    if (!visited) return GUIDE_ERR_MEMORY;
    
    for (int i = 0; i < n; i++) {
        dist[i] = g->matrix[start][i];
        prev[i] = (dist[i] < INF_DIST && i != start) ? start : -1;
        visited[i] = 0;
    }
    dist[start] = 0;
    visited[start] = 1;
    
    for (int count = 1; count < n; count++) {
        int minDist = INF_DIST, u = -1;
        for (int j = 0; j < n; j++) {
            if (!visited[j] && dist[j] < minDist) {
                minDist = dist[j];
                u = j;
            }
        }
        if (u == -1) break;
        visited[u] = 1;
        
        for (int j = 0; j < n; j++) {
            if (!visited[j] && g->matrix[u][j] < INF_DIST) {
                int newDist = dist[u] + g->matrix[u][j];
                if (newDist < dist[j]) {
                    dist[j] = newDist;
                    prev[j] = u;
                }
            }
        }
    }
    
    free(visited);
    return (dist[end] >= INF_DIST) ? GUIDE_ERR_NO_PATH : GUIDE_OK;
}

void reconstructPath(const int *prev, int start, int end, int *path, int *pathLen)
{
    int len = 0;
    int cur = end;
    while (cur != -1) {
        path[len++] = cur;
        if (cur == start) break;
        cur = prev[cur];
    }
    *pathLen = len;
}

/* =========================================================================
 *
 *                          Floyd-Warshall算法
 *
 * ========================================================================= */

GuideStatus floydWarshall(CampusGraph *g, int ***outDist, int ***outNext)
{
    int n = g->vexNum;
    int **dist = (int **)calloc(n, sizeof(int *));
    int **next = (int **)calloc(n, sizeof(int *));
    if (!dist || !next) {
        if (dist) free(dist);
        if (next) free(next);
        return GUIDE_ERR_MEMORY;
    }
    
    for (int i = 0; i < n; i++) {
        dist[i] = (int *)calloc(n, sizeof(int));
        next[i] = (int *)calloc(n, sizeof(int));
        if (!dist[i] || !next[i]) {
            for (int k = 0; k <= i; k++) {
                if (dist[k]) free(dist[k]);
                if (next[k]) free(next[k]);
            }
            free(dist); free(next);
            return GUIDE_ERR_MEMORY;
        }
        for (int j = 0; j < n; j++) {
            dist[i][j] = g->matrix[i][j];
            next[i][j] = (i != j && g->matrix[i][j] < INF_DIST) ? j : -1;
        }
    }
    
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (dist[i][k] < INF_DIST && dist[k][j] < INF_DIST) {
                    if (dist[i][j] > dist[i][k] + dist[k][j]) {
                        dist[i][j] = dist[i][k] + dist[k][j];
                        next[i][j] = next[i][k];
                    }
                }
            }
        }
    }
    
    *outDist = dist;
    *outNext = next;
    return GUIDE_OK;
}

void freeFloydMatrices(int **dist, int **next, int n)
{
    if (dist) {
        for (int i = 0; i < n; i++) if (dist[i]) free(dist[i]);
        free(dist);
    }
    if (next) {
        for (int i = 0; i < n; i++) if (next[i]) free(next[i]);
        free(next);
    }
}

void reconstructFloydPath(int **next, int start, int end, int *path, int *pathLen)
{
    int len = 0;
    int cur = start;
    path[len++] = cur;
    while (cur != end) {
        cur = next[cur][end];
        if (cur == -1) {
            *pathLen = 0;
            return;
        }
        path[len++] = cur;
    }
    *pathLen = len;
}

/* =========================================================================
 *
 *                          文件持久化(GUIDV2格式)
 *
 * ========================================================================= */

GuideStatus saveToFileV2(const CampusGraph *g, const char *filename)
{
    if (!g || !filename) return GUIDE_ERR_INVALID_PARAM;
    FILE *fp = fopen(filename, "wb");
    if (!fp) return GUIDE_ERR_FILE_OPEN;
    
    fwrite(MAGIC_HEADER, 1, MAGIC_LEN, fp);
    int version = 2;
    fwrite(&version, sizeof(int), 1, fp);
    fwrite(&g->vexNum, sizeof(int), 1, fp);
    fwrite(&g->edgeNum, sizeof(int), 1, fp);
    
    for (int i = 0; i < g->vexNum; i++) {
        fwrite(&g->spots[i].id, sizeof(int), 1, fp);
        fwrite(g->spots[i].name, 1, MAX_NAME, fp);
        fwrite(g->spots[i].desc, 1, MAX_DESC, fp);
    }
    
    for (int i = 0; i < g->vexNum; i++) {
        fwrite(g->matrix[i], sizeof(int), g->vexNum, fp);
    }
    
    fclose(fp);
    return GUIDE_OK;
}

GuideStatus loadFromFileV2(CampusGraph *g, const char *filename)
{
    if (!g || !filename) return GUIDE_ERR_INVALID_PARAM;
    FILE *fp = fopen(filename, "rb");
    if (!fp) return GUIDE_ERR_FILE_OPEN;
    
    char magic[MAGIC_LEN + 1] = {0};
    if (fread(magic, 1, MAGIC_LEN, fp) != MAGIC_LEN) {
        fclose(fp);
        return GUIDE_ERR_FILE_READ;
    }
    if (memcmp(magic, MAGIC_HEADER, MAGIC_LEN) != 0) {
        fclose(fp);
        return GUIDE_ERR_MAGIC;
    }
    
    int version;
    if (fread(&version, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return GUIDE_ERR_FILE_READ;
    }
    if (version != 2) {
        fclose(fp);
        return GUIDE_ERR_VERSION;
    }
    
    int vexNum, edgeNum;
    if (fread(&vexNum, sizeof(int), 1, fp) != 1 ||
        fread(&edgeNum, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return GUIDE_ERR_FILE_READ;
    }
    
    graphFree(g);
    GuideStatus st = graphInit(g, (vexNum > INITIAL_CAPACITY) ? vexNum : INITIAL_CAPACITY);
    if (st != GUIDE_OK) {
        fclose(fp);
        return st;
    }
    
    for (int i = 0; i < vexNum; i++) {
        if (fread(&g->spots[i].id, sizeof(int), 1, fp) != 1 ||
            fread(g->spots[i].name, 1, MAX_NAME, fp) != MAX_NAME ||
            fread(g->spots[i].desc, 1, MAX_DESC, fp) != MAX_DESC) {
            fclose(fp);
            graphFree(g);
            return GUIDE_ERR_FILE_READ;
        }
    }
    g->vexNum = vexNum;
    
    for (int i = 0; i < vexNum; i++) {
        if (fread(g->matrix[i], sizeof(int), vexNum, fp) != (size_t)vexNum) {
            fclose(fp);
            graphFree(g);
            return GUIDE_ERR_FILE_READ;
        }
    }
    g->edgeNum = edgeNum;
    
    fclose(fp);
    return GUIDE_OK;
}

/* =========================================================================
 *
 *                          默认测试数据初始化
 *
 * ========================================================================= */

GuideStatus initDefaultData(CampusGraph *g)
{
    graphFree(g);
    GuideStatus st = graphInit(g, INITIAL_CAPACITY);
    if (st != GUIDE_OK) return st;
    
    struct { int id; const char *name; const char *desc; } spots[] = {
        {1,  "校门",     "学校正门，校园标志性建筑，出入校园的主要通道。"},
        {2,  "图书馆",   "馆藏图书200余万册，提供自习、借阅、电子资源服务。"},
        {3,  "教学楼A",  "主要公共教学楼，共6层，含多媒体教室。"},
        {4,  "教学楼B",  "主要专业课教学楼，共5层，配备智慧黑板。"},
        {5,  "实验楼",   "理工科实验中心，各类实验设备先进齐全。"},
        {6,  "学生活动中心", "学生社团活动场所，10余间活动室和多功能厅。"},
        {7,  "食堂",     "三层餐饮楼，提供各地风味美食，可容纳2000人就餐。"},
        {8,  "体育馆",   "综合性体育场馆，有篮球、羽毛球、乒乓球设施。"},
        {9,  "宿舍楼",   "学生公寓区，分东区和西区，宿舍条件良好。"},
        {10, "校医院",   "校内医疗机构，提供基本医疗和健康咨询服务。"},
        {11, "超市",     "校园连锁超市，日用品、食品、文具一应俱全。"},
        {12, "湖心亭",   "校园中心湖景，绿树成荫，是休闲散步的好去处。"}
    };
    
    int spotCount = sizeof(spots) / sizeof(spots[0]);
    for (int i = 0; i < spotCount; i++) {
        st = graphAddSpot(g, spots[i].id, spots[i].name, spots[i].desc);
        if (st != GUIDE_OK) return st;
    }
    
    struct { int u, v, w; } edges[] = {
        {0, 1,  100}, {0, 8,  150}, {0, 2,  200},
        {1, 2,   80}, {1, 11, 120}, {1, 3,  150},
        {2, 4,  100}, {2, 3,   80},
        {3, 8,  120}, {3, 11, 100},
        {4, 5,  200}, {4, 6,  150},
        {5, 6,   80}, {5, 10, 100}, {5, 7,  150},
        {6, 7,  120}, {6, 11, 180},
        {8, 9,  100}, {8, 7,  200},
        {9, 10, 120},
        {11,10, 150}, {7, 10,  80}
    };
    
    int edgeCount = sizeof(edges) / sizeof(edges[0]);
    for (int i = 0; i < edgeCount; i++) {
        st = graphAddEdge(g, edges[i].u, edges[i].v, edges[i].w);
        if (st != GUIDE_OK) return st;
    }
    
    return GUIDE_OK;
}

/* =========================================================================
 *
 *                          输入工具函数
 *
 * ========================================================================= */

void clearInputBuffer(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int inputInt(const char *prompt)
{
    int val;
    printf("%s", prompt);
    if (scanf("%d", &val) != 1) {
        clearInputBuffer();
        return -999;
    }
    clearInputBuffer();
    return val;
}

/* =========================================================================
 *
 *                          帮助信息
 *
 * ========================================================================= */

void showHelp(void)
{
    colorPrint(COLOR_BOLD COLOR_CYAN, "╔══════════════════════════════════════════════════════════════╗\n");
    colorPrint(COLOR_BOLD COLOR_CYAN, "║");
    colorPrint(COLOR_BOLD COLOR_YELLOW, "              校园导游咨询系统 V2.0 - 使用帮助                 ");
    colorPrint(COLOR_BOLD COLOR_CYAN, "║\n");
    colorPrint(COLOR_BOLD COLOR_CYAN, "╚══════════════════════════════════════════════════════════════╝\n\n");
    
    colorPrint(COLOR_GREEN, "用法：\n");
    colorPrint(COLOR_WHITE, "  guide.exe [选项]\n\n");
    colorPrint(COLOR_GREEN, "选项：\n");
    colorPrint(COLOR_WHITE, "  -h, --help      显示本帮助信息\n");
    colorPrint(COLOR_WHITE, "  -t, --test      运行内置单元自测\n");
    colorPrint(COLOR_WHITE, "  (无参数)        启动交互式菜单系统\n\n");
    colorPrint(COLOR_GREEN, "交互菜单功能：\n");
    colorPrint(COLOR_WHITE, "  1 - 查询景点信息\n");
    colorPrint(COLOR_WHITE, "  2 - 查询两点最短路径(Dijkstra)\n");
    colorPrint(COLOR_WHITE, "  3 - 查询所有点对最短路径(Floyd)\n");
    colorPrint(COLOR_WHITE, "  4 - 显示所有景点列表\n");
    colorPrint(COLOR_WHITE, "  5 - 显示校园平面图\n");
    colorPrint(COLOR_WHITE, "  6 - 保存数据到文件\n");
    colorPrint(COLOR_WHITE, "  0 - 退出系统\n\n");
}

/* =========================================================================
 *
 *                          业务功能函数
 *
 * ========================================================================= */

void showAllSpots(void)
{
    printHeader("所有景点列表");
    colorPrint(COLOR_BOLD, "\n  %-6s %-14s %s\n", "编号", "名称", "简介");
    colorPrint(COLOR_CYAN, "  ────── ────────────── ");
    for (int i = 0; i < 46; i++) colorPrint(COLOR_CYAN, "─");
    printf("\n");
    for (int i = 0; i < g_graph.vexNum; i++) {
        colorPrint(COLOR_YELLOW, "  %-6d", g_graph.spots[i].id);
        colorPrint(COLOR_GREEN, " %-14s", g_graph.spots[i].name);
        printf(" %s\n", g_graph.spots[i].desc);
    }
    printf("\n");
}

void querySpotInfo(void)
{
    printHeader("景点信息查询");
    showAllSpots();
    
    int id = inputInt("  请输入景点编号：");
    int idx = graphFindSpot(&g_graph, id);
    if (idx < 0) {
        colorPrint(COLOR_RED, "  [错误] 未找到编号为 %d 的景点。\n", id);
        return;
    }
    
    colorPrint(COLOR_BOLD COLOR_BLUE, "\n  ┌─────────────────────────────────────────────────────────┐\n");
    colorPrint(COLOR_BOLD COLOR_BLUE, "  │");
    colorPrint(COLOR_BOLD COLOR_YELLOW, " 景点详细信息                                              ");
    colorPrint(COLOR_BOLD COLOR_BLUE, "│\n");
    colorPrint(COLOR_BOLD COLOR_BLUE, "  ├─────────────────────────────────────────────────────────┤\n");
    colorPrint(COLOR_BOLD COLOR_BLUE, "  │");
    colorPrint(COLOR_WHITE, " 编  号：%d", g_graph.spots[idx].id);
    for (int i = 0; i < 51 - 6 - 1; i++) printf(" ");
    colorPrint(COLOR_BOLD COLOR_BLUE, "│\n");
    colorPrint(COLOR_BOLD COLOR_BLUE, "  │");
    colorPrint(COLOR_WHITE, " 名  称：%s", g_graph.spots[idx].name);
    for (int i = 0; i < 51 - (int)strlen(g_graph.spots[idx].name) - 6; i++) printf(" ");
    colorPrint(COLOR_BOLD COLOR_BLUE, "│\n");
    colorPrint(COLOR_BOLD COLOR_BLUE, "  │");
    colorPrint(COLOR_WHITE, " 简  介：%s", g_graph.spots[idx].desc);
    printf("\n");
    colorPrint(COLOR_BOLD COLOR_BLUE, "  └─────────────────────────────────────────────────────────┘\n\n");
    
    colorPrint(COLOR_CYAN, "  相邻景点：\n");
    int found = 0;
    for (int k = 0; k < g_graph.vexNum; k++) {
        if (k != idx && g_graph.matrix[idx][k] < INF_DIST) {
            colorPrint(COLOR_GREEN, "    → %s ", g_graph.spots[k].name);
            colorPrint(COLOR_YELLOW, "(距离 %d 米)\n", g_graph.matrix[idx][k]);
            found = 1;
        }
    }
    if (!found) colorPrint(COLOR_YELLOW, "    (无相邻景点)\n");
    printf("\n");
}

void queryShortestPath(void)
{
    printHeader("最短路径查询 - Dijkstra算法");
    showAllSpots();
    
    int startId = inputInt("  请输入起点编号：");
    int si = graphFindSpot(&g_graph, startId);
    if (si < 0) { colorPrint(COLOR_RED, "  [错误] 起点不存在。\n"); return; }
    
    int endId = inputInt("  请输入终点编号：");
    int ei = graphFindSpot(&g_graph, endId);
    if (ei < 0) { colorPrint(COLOR_RED, "  [错误] 终点不存在。\n"); return; }
    
    if (si == ei) {
        colorPrint(COLOR_YELLOW, "  [提示] 起点和终点相同，距离为 0 米。\n");
        return;
    }
    
    int *dist = (int *)malloc(g_graph.vexNum * sizeof(int));
    int *prev = (int *)malloc(g_graph.vexNum * sizeof(int));
    int *path = (int *)malloc(g_graph.vexNum * sizeof(int));
    if (!dist || !prev || !path) {
        colorPrint(COLOR_RED, "  [错误] 内存分配失败。\n");
        free(dist); free(prev); free(path);
        return;
    }
    
    GuideStatus st = dijkstra(&g_graph, si, ei, dist, prev);
    if (st != GUIDE_OK) {
        colorPrint(COLOR_RED, "  [错误] 从 %s 到 %s 没有可达路径。\n",
                   g_graph.spots[si].name, g_graph.spots[ei].name);
        free(dist); free(prev); free(path);
        return;
    }
    
    int pathLen;
    reconstructPath(prev, si, ei, path, &pathLen);
    
    colorPrint(COLOR_BOLD COLOR_GREEN, "\n  ★ 最短距离：%d 米\n\n", dist[ei]);
    colorPrint(COLOR_CYAN, "  ★ 推荐路线：");
    for (int i = pathLen - 1; i >= 0; i--) {
        colorPrint(COLOR_YELLOW, "%s", g_graph.spots[path[i]].name);
        if (i > 0) {
            int u = path[i], v = path[i-1];
            colorPrint(COLOR_WHITE, " --(%dm)--> ", g_graph.matrix[u][v]);
        }
    }
    printf("\n\n");
    
    free(dist); free(prev); free(path);
}

void queryAllPathsFloyd(void)
{
    printHeader("所有点对最短路径 - Floyd-Warshall算法");
    
    int **dist = NULL, **next = NULL;
    GuideStatus st = floydWarshall(&g_graph, &dist, &next);
    if (st != GUIDE_OK) {
        colorPrint(COLOR_RED, "  [错误] Floyd算法执行失败。\n");
        return;
    }
    
    colorPrint(COLOR_BOLD, "\n  距离矩阵(单位：米)：\n\n");
    colorPrint(COLOR_CYAN, "        ");
    for (int j = 0; j < g_graph.vexNum; j++) {
        colorPrint(COLOR_YELLOW, " %-6d", g_graph.spots[j].id);
    }
    printf("\n");
    
    for (int i = 0; i < g_graph.vexNum; i++) {
        colorPrint(COLOR_YELLOW, "  %-5d", g_graph.spots[i].id);
        for (int j = 0; j < g_graph.vexNum; j++) {
            if (dist[i][j] >= INF_DIST) {
                colorPrint(COLOR_RED, "  ∞    ");
            } else {
                colorPrint(COLOR_GREEN, " %-6d", dist[i][j]);
            }
        }
        printf("\n");
    }
    
    int sid = inputInt("\n  输入起点编号查看详细路径(0返回)：");
    if (sid > 0) {
        int eid = inputInt("  输入终点编号：");
        int si = graphFindSpot(&g_graph, sid);
        int ei = graphFindSpot(&g_graph, eid);
        if (si >= 0 && ei >= 0 && si != ei) {
            if (dist[si][ei] >= INF_DIST) {
                colorPrint(COLOR_RED, "  无可达路径。\n");
            } else {
                int *path = (int *)malloc(g_graph.vexNum * sizeof(int));
                int pathLen;
                reconstructFloydPath(next, si, ei, path, &pathLen);
                colorPrint(COLOR_BOLD COLOR_GREEN, "\n  ★ 最短距离：%d 米\n", dist[si][ei]);
                colorPrint(COLOR_CYAN, "  ★ 路线：");
                for (int i = 0; i < pathLen; i++) {
                    colorPrint(COLOR_YELLOW, "%s", g_graph.spots[path[i]].name);
                    if (i < pathLen - 1) {
                        colorPrint(COLOR_WHITE, " → ");
                    }
                }
                printf("\n");
                free(path);
            }
        }
    }
    
    freeFloydMatrices(dist, next, g_graph.vexNum);
}

void showAsciiMap(void)
{
    printHeader("校园平面图 (ASCII示意)");
    printf("\n");
    colorPrint(COLOR_MAGENTA, "                          [宿舍楼(9)]\n");
    colorPrint(COLOR_WHITE, "                         /         \\\n");
    colorPrint(COLOR_YELLOW, "                      100          200\n");
    colorPrint(COLOR_WHITE, "                       /              \\\n");
    colorPrint(COLOR_MAGENTA, "    [校门(1)]---150---[体育馆(8)]      \\\n");
    colorPrint(COLOR_WHITE, "      |  \\               |    |        |\n");
    colorPrint(COLOR_YELLOW, "    200  100           120   200       |\n");
    colorPrint(COLOR_WHITE, "      |    \\             |    |        |\n");
    colorPrint(COLOR_MAGENTA, "      |  [图书馆(2)]---150---[湖心亭(12)]\n");
    colorPrint(COLOR_WHITE, "      |    / |  \\          |   / |\n");
    colorPrint(COLOR_YELLOW, "     80  120  80  150    100 120 150\n");
    colorPrint(COLOR_WHITE, "      |  /   |    \\       | /   |\n");
    colorPrint(COLOR_MAGENTA, "  [教学楼A(3)] [教学楼B(4)] [校医院(10)]\n");
    colorPrint(COLOR_WHITE, "      |         / \\\n");
    colorPrint(COLOR_YELLOW, "    100      120  100\n");
    colorPrint(COLOR_WHITE, "      |     /      \\\n");
    colorPrint(COLOR_MAGENTA, "  [实验楼(5)]     [超市(11)]\n");
    colorPrint(COLOR_WHITE, "      |  \\             |\n");
    colorPrint(COLOR_YELLOW, "    200  150         180\n");
    colorPrint(COLOR_WHITE, "      |    \\           |\n");
    colorPrint(COLOR_MAGENTA, "  [活动中心(6)]---80---[食堂(7)]\n");
    printf("\n");
    colorPrint(COLOR_CYAN, "  说明：数字表示两点间步行距离，单位为米。\n\n");
}

void showMenu(void)
{
    int choice;
    while (1) {
        printHeader("校园导游咨询系统 V2.0 - 主菜单");
        colorPrint(COLOR_GREEN, "  【1】");
        printf(" 查询景点信息\n");
        colorPrint(COLOR_GREEN, "  【2】");
        printf(" 两点最短路径查询 (Dijkstra)\n");
        colorPrint(COLOR_GREEN, "  【3】");
        printf(" 所有点对最短路径 (Floyd)\n");
        colorPrint(COLOR_GREEN, "  【4】");
        printf(" 显示所有景点列表\n");
        colorPrint(COLOR_GREEN, "  【5】");
        printf(" 显示校园平面图\n");
        colorPrint(COLOR_GREEN, "  【6】");
        printf(" 保存数据到文件\n");
        colorPrint(COLOR_GREEN, "  【0】");
        printf(" 退出系统\n");
        printDivider();
        choice = inputInt("  请输入您的选择：");
        
        switch (choice) {
            case 1: querySpotInfo(); break;
            case 2: queryShortestPath(); break;
            case 3: queryAllPathsFloyd(); break;
            case 4: showAllSpots(); break;
            case 5: showAsciiMap(); break;
            case 6: {
                GuideStatus st = saveToFileV2(&g_graph, DATA_FILE_V2);
                if (st == GUIDE_OK) {
                    colorPrint(COLOR_GREEN, "  [成功] 数据已保存到 %s\n", DATA_FILE_V2);
                } else {
                    colorPrint(COLOR_RED, "  [错误] 保存失败，错误码：%d\n", st);
                }
                break;
            }
            case 0:
                printHeader("感谢使用，再见！");
                return;
            default:
                colorPrint(COLOR_RED, "  [提示] 无效选择，请重新输入。\n");
        }
        if (!g_testMode) {
            printf("\n  按回车键继续...");
            getchar();
        }
    }
}

/* =========================================================================
 *
 *                          内置自测模块
 *
 * ========================================================================= */

void testAssert(int condition, const char *testName)
{
    if (condition) {
        g_testsPassed++;
        colorPrint(COLOR_GREEN, "  [PASS] %s\n", testName);
    } else {
        g_testsFailed++;
        colorPrint(COLOR_RED, "  [FAIL] %s\n", testName);
    }
}

void runSelfTests(void)
{
    g_testMode = 1;
    g_testsPassed = 0;
    g_testsFailed = 0;
    
    colorPrint(COLOR_BOLD COLOR_CYAN, "\n╔══════════════════════════════════════════════════════════════╗\n");
    colorPrint(COLOR_BOLD COLOR_CYAN, "║");
    colorPrint(COLOR_BOLD COLOR_YELLOW, "                 V2.0 内置自测程序开始执行                     ");
    colorPrint(COLOR_BOLD COLOR_CYAN, "║\n");
    colorPrint(COLOR_BOLD COLOR_CYAN, "╚══════════════════════════════════════════════════════════════╝\n\n");
    
    /* T01: 图初始化 */
    CampusGraph testGraph;
    testAssert(graphInit(&testGraph, 4) == GUIDE_OK, "T01: 图初始化成功");
    
    /* T02: 添加景点 */
    testAssert(graphAddSpot(&testGraph, 1, "测试点A", "测试描述A") == GUIDE_OK, "T02: 添加景点A成功");
    testAssert(graphAddSpot(&testGraph, 2, "测试点B", "测试描述B") == GUIDE_OK, "T03: 添加景点B成功");
    testAssert(graphAddSpot(&testGraph, 3, "测试点C", "测试描述C") == GUIDE_OK, "T04: 添加景点C成功");
    
    /* T05: 查找景点 */
    testAssert(graphFindSpot(&testGraph, 1) == 0, "T05: 查找景点ID=1返回索引0");
    testAssert(graphFindSpot(&testGraph, 99) == -1, "T06: 查找不存在景点返回-1");
    
    /* T07: 添加边 */
    testAssert(graphAddEdge(&testGraph, 0, 1, 100) == GUIDE_OK, "T07: 添加边A-B(100)成功");
    testAssert(graphAddEdge(&testGraph, 1, 2, 200) == GUIDE_OK, "T08: 添加边B-C(200)成功");
    
    /* T09: Dijkstra最短路径 A->C */
    int dist[16], prev[16], path[16], pathLen;
    testAssert(dijkstra(&testGraph, 0, 2, dist, prev) == GUIDE_OK, "T09: Dijkstra(A->C)执行成功");
    testAssert(dist[2] == 300, "T10: Dijkstra(A->C)距离正确(300米)");
    reconstructPath(prev, 0, 2, path, &pathLen);
    testAssert(pathLen == 3, "T11: Dijkstra(A->C)路径长度正确(3个节点)");
    
    /* T12: Dijkstra同一点 */
    testAssert(dijkstra(&testGraph, 0, 0, dist, prev) == GUIDE_OK, "T12: Dijkstra(A->A)返回OK");
    
    /* T13: 加载默认数据 */
    testAssert(initDefaultData(&g_graph) == GUIDE_OK, "T13: 默认测试数据加载成功");
    testAssert(g_graph.vexNum == 12, "T14: 默认景点数量正确(12个)");
    testAssert(g_graph.edgeNum == 22, "T15: 默认路径数量正确(22条)");
    
    /* T16: Dijkstra校门(1)->图书馆(2) */
    int si = graphFindSpot(&g_graph, 1);
    int ei = graphFindSpot(&g_graph, 2);
    testAssert(dijkstra(&g_graph, si, ei, dist, prev) == GUIDE_OK, "T16: Dijkstra(校门->图书馆)成功");
    testAssert(dist[ei] == 100, "T17: 校门到图书馆距离正确(100米)");
    
    /* T18: Dijkstra教学楼A(3)->食堂(7) */
    si = graphFindSpot(&g_graph, 3);
    ei = graphFindSpot(&g_graph, 7);
    testAssert(dijkstra(&g_graph, si, ei, dist, prev) == GUIDE_OK, "T18: Dijkstra(教学楼A->食堂)成功");
    
    /* T19: Floyd-Warshall算法 */
    int **fdist = NULL, **fnext = NULL;
    testAssert(floydWarshall(&g_graph, &fdist, &fnext) == GUIDE_OK, "T19: Floyd-Warshall算法执行成功");
    si = graphFindSpot(&g_graph, 1);
    ei = graphFindSpot(&g_graph, 12);
    testAssert(fdist[si][ei] < INF_DIST, "T20: Floyd(校门->湖心亭)可达");
    
    /* T21: 文件保存与加载 */
    testAssert(saveToFileV2(&g_graph, "test_v2.dat") == GUIDE_OK, "T21: 文件保存成功");
    CampusGraph loadGraph;
    testAssert(loadFromFileV2(&loadGraph, "test_v2.dat") == GUIDE_OK, "T22: 文件加载成功");
    testAssert(loadGraph.vexNum == g_graph.vexNum, "T23: 加载后景点数量一致");
    testAssert(loadGraph.edgeNum == g_graph.edgeNum, "T24: 加载后边数量一致");
    
    /* T25: 魔数验证 */
    testAssert(loadFromFileV2(&loadGraph, "main.c") == GUIDE_ERR_MAGIC, "T25: 非GUIDV2文件返回魔数错误");
    
    graphFree(&testGraph);
    graphFree(&loadGraph);
    freeFloydMatrices(fdist, fnext, g_graph.vexNum);
    remove("test_v2.dat");
    
    testSummary();
}

void testSummary(void)
{
    printf("\n");
    printDivider();
    colorPrint(COLOR_BOLD, "  测试结果汇总：\n");
    colorPrint(COLOR_GREEN, "  通过：%d 项\n", g_testsPassed);
    if (g_testsFailed > 0) {
        colorPrint(COLOR_RED, "  失败：%d 项\n", g_testsFailed);
    } else {
        colorPrint(COLOR_GREEN, "  失败：%d 项\n", g_testsFailed);
    }
    printDivider();
    
    if (g_testsFailed == 0) {
        colorPrint(COLOR_BOLD COLOR_GREEN, "\n  ★★★ 所有测试全部通过！ ★★★\n\n");
    } else {
        colorPrint(COLOR_BOLD COLOR_RED, "\n  ✗✗✗ 存在失败的测试用例！ ✗✗✗\n\n");
    }
}

/* =========================================================================
 *
 *                          主函数
 *
 * ========================================================================= */

int main(int argc, char *argv[])
{
    setupConsole();
    
    /* CLI参数解析 */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            showHelp();
            return 0;
        }
        if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--test") == 0) {
            runSelfTests();
            return (g_testsFailed == 0) ? 0 : 1;
        }
    }
    
    /* 正常启动：初始化数据 */
    colorPrint(COLOR_BOLD COLOR_CYAN, "\n╔══════════════════════════════════════════════════════════════╗\n");
    colorPrint(COLOR_BOLD COLOR_CYAN, "║");
    colorPrint(COLOR_BOLD COLOR_YELLOW, "              欢迎使用校园导游咨询系统 V2.0                    ");
    colorPrint(COLOR_BOLD COLOR_CYAN, "║\n");
    colorPrint(COLOR_BOLD COLOR_CYAN, "╚══════════════════════════════════════════════════════════════╝\n");
    
    GuideStatus st = loadFromFileV2(&g_graph, DATA_FILE_V2);
    if (st != GUIDE_OK) {
        colorPrint(COLOR_YELLOW, "\n  [提示] 未找到有效数据文件，正在加载默认数据...\n");
        st = initDefaultData(&g_graph);
        if (st != GUIDE_OK) {
            colorPrint(COLOR_RED, "  [错误] 数据初始化失败！程序退出。\n");
            return 1;
        }
        saveToFileV2(&g_graph, DATA_FILE_V2);
        colorPrint(COLOR_GREEN, "  [成功] 默认数据已加载并保存。\n");
    } else {
        colorPrint(COLOR_GREEN, "  [成功] 已从 %s 加载数据。\n", DATA_FILE_V2);
    }
    
    showMenu();
    
    graphFree(&g_graph);
    return 0;
}
