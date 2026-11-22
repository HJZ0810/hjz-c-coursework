/*
 * 全国交通咨询模拟系统 V2.0
 * National Traffic Consultation System V2.0
 *
 * 编译器：GCC (C99标准)
 * 编译命令：D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o traff.exe main.c
 *
 * V2.0 新特性：
 *   - UTF-8 BOM编码 + SetConsoleOutputCP(65001) Windows控制台支持
 *   - ANSI彩色终端输出
 *   - CLI参数解析 (-h/--help, -t/--test)
 *   - 动态数组存储（城市+交通线路动态数组，支持自动扩容）
 *   - 统一错误码枚举 TraffStatus
 *   - 文件持久化带Magic Header "TRAFV2"
 *   - 内置自测框架（15+测试用例：最短路径+中转+文件I/O）
 *   - 完整中文注释
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>

/*====================================================================================================
 *
 *  ANSI 终端颜色码定义
 *
 *====================================================================================================*/

#define ANSI_RESET      "\033[0m"
#define ANSI_BOLD       "\033[1m"
#define ANSI_DIM        "\033[2m"
#define ANSI_RED        "\033[31m"
#define ANSI_GREEN      "\033[32m"
#define ANSI_YELLOW     "\033[33m"
#define ANSI_BLUE       "\033[34m"
#define ANSI_MAGENTA    "\033[35m"
#define ANSI_CYAN       "\033[36m"
#define ANSI_WHITE      "\033[37m"
#define ANSI_BG_RED     "\033[41m"
#define ANSI_BG_GREEN   "\033[42m"
#define ANSI_BG_BLUE    "\033[44m"

#define COLOR_SUCCESS   ANSI_GREEN
#define COLOR_ERROR     ANSI_RED
#define COLOR_WARN      ANSI_YELLOW
#define COLOR_INFO      ANSI_CYAN
#define COLOR_HEADER    ANSI_BOLD ANSI_BLUE
#define COLOR_PROMPT    ANSI_BOLD ANSI_MAGENTA

/*====================================================================================================
 *
 *  常量与宏定义
 *
 *====================================================================================================*/

#define MAX_NAME        32
#define MAX_ROUTE_NUM   16
#define MAX_TIME_STR    8
#define INF             999999999

#define INITIAL_CAPACITY    8
#define GROWTH_FACTOR       2

#define DATA_FILE       "trafv2.dat"
#define MAGIC_HEADER    "TRAFV2"
#define MAGIC_LEN       6
#define DATA_VERSION    2

#define TRAIN           1
#define PLANE           2

/*====================================================================================================
 *
 *  统一错误码枚举
 *
 *====================================================================================================*/

typedef enum {
    TRAFF_OK = 0,
    TRAFF_ERR_MEMORY,
    TRAFF_ERR_NOT_FOUND,
    TRAFF_ERR_DUPLICATE,
    TRAFF_ERR_INVALID_INPUT,
    TRAFF_ERR_FILE_OPEN,
    TRAFF_ERR_FILE_READ,
    TRAFF_ERR_FILE_WRITE,
    TRAFF_ERR_MAGIC_MISMATCH,
    TRAFF_ERR_VERSION_MISMATCH,
    TRAFF_ERR_EMPTY,
    TRAFF_ERR_NO_PATH,
    TRAFF_ERR_IO
} TraffStatus;

/*====================================================================================================
 *
 *  数据结构定义
 *
 *====================================================================================================*/

typedef struct Edge {
    int     type;
    char    route_num[MAX_ROUTE_NUM];
    int     from;
    int     to;
    char    dep_time[MAX_TIME_STR];
    char    arr_time[MAX_TIME_STR];
    int     duration;
    int     cost;
    struct Edge *next;
} Edge;

typedef struct {
    char    name[MAX_NAME];
    Edge   *head;
} City;

typedef struct {
    City *items;
    size_t count;
    size_t capacity;
} CityArray;

typedef struct {
    Edge *items;
    size_t count;
    size_t capacity;
} EdgePool;

/*====================================================================================================
 *
 *  全局数据
 *
 *====================================================================================================*/

static CityArray g_cities = {NULL, 0, 0};
static EdgePool  g_edges  = {NULL, 0, 0};

/*====================================================================================================
 *
 *  工具函数
 *
 *====================================================================================================*/

static void clearInputBuffer(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

static void readLine(char *buf, int size)
{
    if (fgets(buf, size, stdin) != NULL) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
        } else {
            clearInputBuffer();
        }
    }
}

static int isEmptyStr(const char *str)
{
    if (str == NULL) return 1;
    while (*str) {
        if (!isspace((unsigned char)*str)) return 0;
        str++;
    }
    return 1;
}

static void printSeparator(void)
{
    printf(ANSI_DIM "------------------------------------------------------------" ANSI_RESET "\n");
}

static void printSuccess(const char *msg)
{
    printf(COLOR_SUCCESS "  [✓] %s" ANSI_RESET "\n", msg);
}

static void printError(const char *msg)
{
    printf(COLOR_ERROR "  [✗] %s" ANSI_RESET "\n", msg);
}

static void printWarning(const char *msg)
{
    printf(COLOR_WARN "  [!] %s" ANSI_RESET "\n", msg);
}

static int timeToMin(const char *t)
{
    int h = 0, m = 0;
    if (!t || strlen(t) < 4) return 0;
    sscanf(t, "%d:%d", &h, &m);
    return h * 60 + m;
}

static int calcDuration(const char *dep, const char *arr)
{
    int d = timeToMin(dep), a = timeToMin(arr);
    int diff = a - d;
    if (diff <= 0) diff += 1440;
    return diff;
}

static const char *typeStr(int type)
{
    return (type == TRAIN) ? "火车" : "飞机";
}

/*====================================================================================================
 *
 *  动态数组管理 - 城市
 *
 *====================================================================================================*/

static TraffStatus cityArrayInit(CityArray *arr)
{
    arr->items = (City*)calloc(INITIAL_CAPACITY, sizeof(City));
    if (arr->items == NULL) return TRAFF_ERR_MEMORY;
    arr->count = 0;
    arr->capacity = INITIAL_CAPACITY;
    return TRAFF_OK;
}

static TraffStatus cityArrayResize(CityArray *arr)
{
    size_t newCap = arr->capacity * GROWTH_FACTOR;
    City *newItems = (City*)realloc(arr->items, newCap * sizeof(City));
    if (newItems == NULL) return TRAFF_ERR_MEMORY;
    arr->items = newItems;
    arr->capacity = newCap;
    return TRAFF_OK;
}

static TraffStatus cityArrayAdd(CityArray *arr, const char *name)
{
    if (arr->count >= arr->capacity) {
        TraffStatus st = cityArrayResize(arr);
        if (st != TRAFF_OK) return st;
    }
    strncpy(arr->items[arr->count].name, name, MAX_NAME - 1);
    arr->items[arr->count].name[MAX_NAME - 1] = '\0';
    arr->items[arr->count].head = NULL;
    arr->count++;
    return TRAFF_OK;
}

static int cityArrayFind(const CityArray *arr, const char *name)
{
    for (size_t i = 0; i < arr->count; i++) {
        if (strcmp(arr->items[i].name, name) == 0)
            return (int)i;
    }
    return -1;
}

static void cityArrayFree(CityArray *arr)
{
    free(arr->items);
    arr->items = NULL;
    arr->count = 0;
    arr->capacity = 0;
}

/*====================================================================================================
 *
 *  动态数组管理 - 边池
 *
 *====================================================================================================*/

static TraffStatus edgePoolInit(EdgePool *pool)
{
    pool->items = (Edge*)calloc(INITIAL_CAPACITY * 4, sizeof(Edge));
    if (pool->items == NULL) return TRAFF_ERR_MEMORY;
    pool->count = 0;
    pool->capacity = INITIAL_CAPACITY * 4;
    return TRAFF_OK;
}

static TraffStatus edgePoolResize(EdgePool *pool)
{
    size_t newCap = pool->capacity * GROWTH_FACTOR;
    Edge *newItems = (Edge*)realloc(pool->items, newCap * sizeof(Edge));
    if (newItems == NULL) return TRAFF_ERR_MEMORY;

    for (size_t i = 0; i < pool->count; i++) {
        Edge *e = &newItems[i];
        if (e->next != NULL) {
            ptrdiff_t idx = e->next - pool->items;
            e->next = &newItems[idx];
        }
    }

    for (size_t i = 0; i < g_cities.count; i++) {
        if (g_cities.items[i].head != NULL) {
            ptrdiff_t idx = g_cities.items[i].head - pool->items;
            g_cities.items[i].head = &newItems[idx];
        }
    }

    pool->items = newItems;
    pool->capacity = newCap;
    return TRAFF_OK;
}

static int edgePoolAdd(EdgePool *pool, int type, const char *rnum,
                       int from, int to, const char *dep, const char *arr,
                       int dur, int cost)
{
    if (pool->count >= pool->capacity) {
        if (edgePoolResize(pool) != TRAFF_OK) return -1;
    }
    Edge *e = &pool->items[pool->count];
    e->type = type;
    strncpy(e->route_num, rnum, MAX_ROUTE_NUM - 1);
    e->route_num[MAX_ROUTE_NUM - 1] = '\0';
    e->from = from;
    e->to = to;
    strncpy(e->dep_time, dep, MAX_TIME_STR - 1);
    e->dep_time[MAX_TIME_STR - 1] = '\0';
    strncpy(e->arr_time, arr, MAX_TIME_STR - 1);
    e->arr_time[MAX_TIME_STR - 1] = '\0';
    e->duration = dur;
    e->cost = cost;
    e->next = g_cities.items[from].head;
    g_cities.items[from].head = e;
    int idx = (int)pool->count;
    pool->count++;
    return idx;
}

static void edgePoolFree(EdgePool *pool)
{
    free(pool->items);
    pool->items = NULL;
    pool->count = 0;
    pool->capacity = 0;
}

/*====================================================================================================
 *
 *  业务函数
 *
 *====================================================================================================*/

static TraffStatus addCity(const char *name)
{
    if (isEmptyStr(name)) return TRAFF_ERR_INVALID_INPUT;
    if (cityArrayFind(&g_cities, name) >= 0) return TRAFF_ERR_DUPLICATE;
    return cityArrayAdd(&g_cities, name);
}

static int findCity(const char *name)
{
    return cityArrayFind(&g_cities, name);
}

static TraffStatus addRoute(int type, const char *rnum, const char *fromName,
                            const char *toName, const char *dep, const char *arr,
                            int cost)
{
    if (isEmptyStr(rnum) || isEmptyStr(fromName) || isEmptyStr(toName) ||
        isEmptyStr(dep) || isEmptyStr(arr) || cost < 0) {
        return TRAFF_ERR_INVALID_INPUT;
    }
    int from = findCity(fromName);
    int to = findCity(toName);
    if (from < 0 || to < 0) return TRAFF_ERR_NOT_FOUND;
    int dur = calcDuration(dep, arr);
    int idx = edgePoolAdd(&g_edges, type, rnum, from, to, dep, arr, dur, cost);
    if (idx < 0) return TRAFF_ERR_MEMORY;
    return TRAFF_OK;
}

/*====================================================================================================
 *
 *  Dijkstra 最短路径算法
 *
 *====================================================================================================*/

static int edgeWeight(Edge *e, int mode, int prevEdgeIdx)
{
    int w = (mode == 0) ? e->duration : e->cost;
    if (mode == 0 && prevEdgeIdx >= 0) {
        int arr_u = timeToMin(g_edges.items[prevEdgeIdx].arr_time);
        int dep_e = timeToMin(e->dep_time);
        int wait = dep_e - arr_u;
        if (wait < 0) wait += 1440;
        w += wait;
    }
    return w;
}

static TraffStatus dijkstraQuery(int src, int dst, int mode,
                                  int *outTotalTime, int *outTotalCost,
                                  int *pathLen, int **outPath, int **outPrevE)
{
    int n = (int)g_cities.count;
    if (src < 0 || src >= n || dst < 0 || dst >= n) return TRAFF_ERR_INVALID_INPUT;

    int *dist = (int*)malloc(n * sizeof(int));
    int *vis = (int*)malloc(n * sizeof(int));
    int *prev_e = (int*)malloc(n * sizeof(int));
    int *prev_c = (int*)malloc(n * sizeof(int));
    int *path = (int*)malloc(n * sizeof(int));

    if (!dist || !vis || !prev_e || !prev_c || !path) {
        free(dist); free(vis); free(prev_e); free(prev_c); free(path);
        return TRAFF_ERR_MEMORY;
    }

    for (int i = 0; i < n; i++) {
        dist[i] = INF;
        vis[i] = 0;
        prev_e[i] = -1;
        prev_c[i] = -1;
    }
    dist[src] = 0;

    for (int i = 0; i < n; i++) {
        int u = -1, min_d = INF;
        for (int j = 0; j < n; j++) {
            if (!vis[j] && dist[j] < min_d) {
                min_d = dist[j];
                u = j;
            }
        }
        if (u < 0 || u == dst) break;
        vis[u] = 1;

        Edge *e = g_cities.items[u].head;
        while (e) {
            int v = e->to;
            if (!vis[v]) {
                int ei = (int)(e - g_edges.items);
                int w = edgeWeight(e, mode, prev_e[u]);
                if (dist[u] + w < dist[v]) {
                    dist[v] = dist[u] + w;
                    prev_e[v] = ei;
                    prev_c[v] = u;
                }
            }
            e = e->next;
        }
    }

    if (dist[dst] >= INF) {
        free(dist); free(vis); free(prev_e); free(prev_c); free(path);
        return TRAFF_ERR_NO_PATH;
    }

    int len = 0, v = dst;
    while (v != src && v >= 0) {
        path[len++] = v;
        v = prev_c[v];
    }
    path[len++] = src;

    int total_t = 0, total_c = 0;
    for (int i = len - 2; i >= 0; i--) {
        int ei = prev_e[path[i]];
        if (i < len - 2) {
            int prev_arr = timeToMin(g_edges.items[prev_e[path[i + 1]]].arr_time);
            int cur_dep = timeToMin(g_edges.items[ei].dep_time);
            int wait = cur_dep - prev_arr;
            if (wait < 0) wait += 1440;
            total_t += wait;
        }
        total_t += g_edges.items[ei].duration;
        total_c += g_edges.items[ei].cost;
    }

    *outTotalTime = total_t;
    *outTotalCost = total_c;
    *pathLen = len;
    *outPath = path;
    *outPrevE = prev_e;

    free(dist);
    free(vis);
    free(prev_c);

    return TRAFF_OK;
}

static void printRouteResult(int src, int dst, int mode,
                              int total_t, int total_c,
                              int pathLen, int *path, int *prev_e)
{
    printf("\n");
    printSeparator();
    if (mode == 0)
        printf(COLOR_HEADER "  ★ 最快路线：%-8s → %-8s" ANSI_RESET "\n",
               g_cities.items[src].name, g_cities.items[dst].name);
    else
        printf(COLOR_HEADER "  ★ 最省钱路线：%-8s → %-8s" ANSI_RESET "\n",
               g_cities.items[src].name, g_cities.items[dst].name);
    printSeparator();

    printf("  路线概览：");
    for (int i = pathLen - 1; i >= 0; i--) {
        printf("%s", g_cities.items[path[i]].name);
        if (i > 0) printf(" → ");
    }
    printf("\n\n");

    for (int i = pathLen - 2, seg = 1; i >= 0; i--, seg++) {
        int ei = prev_e[path[i]];
        Edge *pe = &g_edges.items[ei];
        int wait = 0;
        if (i < pathLen - 2) {
            int prev_arr = timeToMin(g_edges.items[prev_e[path[i + 1]]].arr_time);
            int cur_dep = timeToMin(g_edges.items[ei].dep_time);
            wait = cur_dep - prev_arr;
            if (wait < 0) wait += 1440;
        }
        printf("  第%d段：%s → %s\n", seg,
               g_cities.items[pe->from].name, g_cities.items[pe->to].name);
        printf("    交通方式：%s  班次/航班：%s\n", typeStr(pe->type), pe->route_num);
        printf("    出发时间：%s    到达时间：%s\n", pe->dep_time, pe->arr_time);
        printf("    行程时间：%d分钟    票价：%d元\n", pe->duration, pe->cost);
        if (wait > 0) printf("    中转等候：%d分钟\n", wait);
        printf("\n");
    }

    printSeparator();
    printf("  ★ 总计：总费用 = %d元 | 总耗时 = %d小时%d分钟\n",
           total_c, total_t / 60, total_t % 60);
    printSeparator();
}

/*====================================================================================================
 *
 *  文件持久化 - Magic Header "TRAFV2"
 *
 *====================================================================================================*/

static TraffStatus saveAllData(void)
{
    FILE *fp = fopen(DATA_FILE, "wb");
    if (fp == NULL) return TRAFF_ERR_FILE_OPEN;

    if (fwrite(MAGIC_HEADER, 1, MAGIC_LEN, fp) != MAGIC_LEN) {
        fclose(fp); return TRAFF_ERR_FILE_WRITE;
    }

    unsigned char ver = DATA_VERSION;
    if (fwrite(&ver, 1, 1, fp) != 1) {
        fclose(fp); return TRAFF_ERR_FILE_WRITE;
    }

    size_t cc = g_cities.count;
    size_t ec = g_edges.count;
    if (fwrite(&cc, sizeof(size_t), 1, fp) != 1 ||
        fwrite(&ec, sizeof(size_t), 1, fp) != 1) {
        fclose(fp); return TRAFF_ERR_FILE_WRITE;
    }

    for (size_t i = 0; i < g_cities.count; i++) {
        City *c = &g_cities.items[i];
        if (fwrite(c->name, 1, MAX_NAME, fp) != MAX_NAME) {
            fclose(fp); return TRAFF_ERR_FILE_WRITE;
        }
    }

    for (size_t i = 0; i < g_edges.count; i++) {
        Edge *e = &g_edges.items[i];
        if (fwrite(e, sizeof(Edge), 1, fp) != 1) {
            fclose(fp); return TRAFF_ERR_FILE_WRITE;
        }
    }

    fclose(fp);
    return TRAFF_OK;
}

static void rebuildAdjacencyList(void)
{
    for (size_t i = 0; i < g_cities.count; i++) {
        g_cities.items[i].head = NULL;
    }
    for (size_t i = 0; i < g_edges.count; i++) {
        Edge *e = &g_edges.items[i];
        e->next = g_cities.items[e->from].head;
        g_cities.items[e->from].head = e;
    }
}

static TraffStatus loadAllData(void)
{
    FILE *fp = fopen(DATA_FILE, "rb");
    if (fp == NULL) return TRAFF_ERR_FILE_OPEN;

    char magic[MAGIC_LEN + 1] = {0};
    if (fread(magic, 1, MAGIC_LEN, fp) != MAGIC_LEN) {
        fclose(fp); return TRAFF_ERR_FILE_READ;
    }
    if (strcmp(magic, MAGIC_HEADER) != 0) {
        fclose(fp); return TRAFF_ERR_MAGIC_MISMATCH;
    }

    unsigned char ver;
    if (fread(&ver, 1, 1, fp) != 1) {
        fclose(fp); return TRAFF_ERR_FILE_READ;
    }
    if (ver != DATA_VERSION) {
        fclose(fp); return TRAFF_ERR_VERSION_MISMATCH;
    }

    size_t cc = 0, ec = 0;
    if (fread(&cc, sizeof(size_t), 1, fp) != 1 ||
        fread(&ec, sizeof(size_t), 1, fp) != 1) {
        fclose(fp); return TRAFF_ERR_FILE_READ;
    }

    cityArrayFree(&g_cities);
    edgePoolFree(&g_edges);
    cityArrayInit(&g_cities);
    edgePoolInit(&g_edges);

    while (g_cities.capacity < cc) cityArrayResize(&g_cities);
    while (g_edges.capacity < ec) edgePoolResize(&g_edges);

    for (size_t i = 0; i < cc; i++) {
        char name[MAX_NAME] = {0};
        if (fread(name, 1, MAX_NAME, fp) != MAX_NAME) {
            fclose(fp); return TRAFF_ERR_FILE_READ;
        }
        cityArrayAdd(&g_cities, name);
    }

    for (size_t i = 0; i < ec; i++) {
        Edge e;
        memset(&e, 0, sizeof(e));
        if (fread(&e, sizeof(Edge), 1, fp) != 1) {
            fclose(fp); return TRAFF_ERR_FILE_READ;
        }
        g_edges.items[i] = e;
    }
    g_edges.count = ec;

    rebuildAdjacencyList();

    fclose(fp);
    return TRAFF_OK;
}

/*====================================================================================================
 *
 *  初始化默认数据
 *
 *====================================================================================================*/

static void initDefaultData(void)
{
    addCity("北京"); addCity("上海"); addCity("广州");
    addCity("成都"); addCity("武汉"); addCity("西安");
    addCity("南京"); addCity("杭州");

    addRoute(TRAIN, "G101", "北京", "上海", "08:00", "12:30", 553);
    addRoute(TRAIN, "G103", "北京", "上海", "09:30", "14:10", 553);
    addRoute(TRAIN, "G201", "北京", "广州", "07:00", "14:30", 862);
    addRoute(TRAIN, "G301", "北京", "武汉", "08:30", "12:00", 515);
    addRoute(TRAIN, "G303", "北京", "西安", "10:00", "14:20", 520);
    addRoute(TRAIN, "G401", "上海", "广州", "08:00", "13:20", 793);
    addRoute(TRAIN, "G403", "上海", "南京", "09:00", "10:30", 135);
    addRoute(TRAIN, "G501", "广州", "成都", "09:00", "19:30", 1280);
    addRoute(TRAIN, "G503", "广州", "西安", "08:30", "12:00", 463);
    addRoute(TRAIN, "G601", "成都", "武汉", "08:00", "12:30", 263);
    addRoute(TRAIN, "G701", "武汉", "西安", "09:00", "13:00", 458);
    addRoute(TRAIN, "G801", "西安", "南京", "08:00", "11:30", 348);
    addRoute(TRAIN, "G803", "西安", "杭州", "09:30", "14:00", 428);
    addRoute(TRAIN, "G901", "南京", "杭州", "08:00", "09:00",  89);
    addRoute(TRAIN, "G105", "上海", "杭州", "10:00", "11:20",  73);

    addRoute(PLANE, "CA101", "北京", "上海", "07:00", "09:10", 800);
    addRoute(PLANE, "CA201", "北京", "广州", "08:00", "11:00", 1200);
    addRoute(PLANE, "CA301", "北京", "成都", "09:00", "11:30", 1100);
    addRoute(PLANE, "MU101", "上海", "广州", "07:30", "10:00", 950);
    addRoute(PLANE, "MU201", "上海", "成都", "08:00", "11:00", 1050);
    addRoute(PLANE, "CZ101", "广州", "成都", "09:00", "11:30", 880);
    addRoute(PLANE, "CZ201", "广州", "杭州", "10:00", "12:00", 780);
    addRoute(PLANE, "HU101", "成都", "杭州", "08:30", "11:00", 920);
    addRoute(PLANE, "HU201", "西安", "上海", "09:00", "11:00", 850);
    addRoute(PLANE, "CA401", "武汉", "北京", "07:00", "09:00", 700);
}

/*====================================================================================================
 *
 *  初始化与清理
 *
 *====================================================================================================*/

static TraffStatus initSystem(void)
{
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    TraffStatus s1 = cityArrayInit(&g_cities);
    TraffStatus s2 = edgePoolInit(&g_edges);
    if (s1 != TRAFF_OK || s2 != TRAFF_OK) {
        return TRAFF_ERR_MEMORY;
    }

    TraffStatus st = loadAllData();
    if (st == TRAFF_OK) {
        return TRAFF_OK;
    }

    initDefaultData();
    saveAllData();
    return TRAFF_OK;
}

static void cleanupSystem(void)
{
    cityArrayFree(&g_cities);
    edgePoolFree(&g_edges);
}

/*====================================================================================================
 *
 *  交互式菜单
 *
 *====================================================================================================*/

static void showHelp(void)
{
    printf(COLOR_HEADER "\n  全国交通咨询模拟系统 V2.0\n" ANSI_RESET);
    printf("  用法: traff [选项]\n\n");
    printf(COLOR_INFO "  选项:\n" ANSI_RESET);
    printf("    -h, --help      显示帮助信息\n");
    printf("    -t, --test      运行内置自测\n");
    printf("    (无参数)        启动交互菜单\n\n");
}

static void listCities(void)
{
    printSeparator();
    printf(COLOR_HEADER "  城市列表 (%zu 个)" ANSI_RESET "\n", g_cities.count);
    printSeparator();
    if (g_cities.count == 0) {
        printWarning("暂无城市信息");
        return;
    }
    for (size_t i = 0; i < g_cities.count; i++) {
        printf("  %2zu. %s\n", i + 1, g_cities.items[i].name);
    }
    printSeparator();
}

static void listRoutes(void)
{
    printSeparator();
    printf(COLOR_HEADER "  交通线路列表 (%zu 条)" ANSI_RESET "\n", g_edges.count);
    printSeparator();
    if (g_edges.count == 0) {
        printWarning("暂无交通线路");
        return;
    }
    printf(COLOR_INFO "  %-4s %-4s %-8s %-8s %-8s %-6s %-6s %s" ANSI_RESET "\n",
           "序号", "类型", "班次", "出发", "到达", "出发", "到达", "票价");
    printSeparator();
    for (size_t i = 0; i < g_edges.count; i++) {
        Edge *e = &g_edges.items[i];
        printf("  %-4zu %-4s %-8s %-8s %-8s %-6s %-6s %d元\n",
               i + 1, typeStr(e->type), e->route_num,
               g_cities.items[e->from].name, g_cities.items[e->to].name,
               e->dep_time, e->arr_time, e->cost);
    }
    printSeparator();
}

static void menuAddCity(void)
{
    char name[MAX_NAME];
    printSeparator();
    printf(COLOR_HEADER "  添加城市" ANSI_RESET "\n");
    printSeparator();
    printf(COLOR_PROMPT "  请输入城市名称: " ANSI_RESET);
    readLine(name, MAX_NAME);
    TraffStatus st = addCity(name);
    if (st == TRAFF_OK) {
        printSuccess("城市添加成功");
        saveAllData();
    } else if (st == TRAFF_ERR_DUPLICATE) {
        printError("该城市已存在");
    } else if (st == TRAFF_ERR_INVALID_INPUT) {
        printError("城市名称不能为空");
    } else {
        printError("添加失败");
    }
}

static void menuAddRoute(void)
{
    int type, cost;
    char rnum[MAX_ROUTE_NUM], from[MAX_NAME], to[MAX_NAME];
    char dep[MAX_TIME_STR], arr[MAX_TIME_STR];

    printSeparator();
    printf(COLOR_HEADER "  添加交通线路" ANSI_RESET "\n");
    printSeparator();

    printf(COLOR_PROMPT "  请选择交通类型 (1=火车, 2=飞机): " ANSI_RESET);
    if (scanf("%d", &type) != 1 || (type != TRAIN && type != PLANE)) {
        printError("无效的交通类型");
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    printf(COLOR_PROMPT "  请输入班次/航班号: " ANSI_RESET);
    readLine(rnum, MAX_ROUTE_NUM);
    printf(COLOR_PROMPT "  请输入出发城市: " ANSI_RESET);
    readLine(from, MAX_NAME);
    printf(COLOR_PROMPT "  请输入到达城市: " ANSI_RESET);
    readLine(to, MAX_NAME);
    printf(COLOR_PROMPT "  请输入出发时间 (HH:MM): " ANSI_RESET);
    readLine(dep, MAX_TIME_STR);
    printf(COLOR_PROMPT "  请输入到达时间 (HH:MM): " ANSI_RESET);
    readLine(arr, MAX_TIME_STR);
    printf(COLOR_PROMPT "  请输入票价 (元): " ANSI_RESET);
    if (scanf("%d", &cost) != 1 || cost < 0) {
        printError("无效的票价");
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    TraffStatus st = addRoute(type, rnum, from, to, dep, arr, cost);
    if (st == TRAFF_OK) {
        printSuccess("线路添加成功");
        saveAllData();
    } else if (st == TRAFF_ERR_NOT_FOUND) {
        printError("出发或到达城市不存在");
    } else if (st == TRAFF_ERR_INVALID_INPUT) {
        printError("输入信息无效");
    } else {
        printError("添加失败");
    }
}

static void menuQueryRoute(void)
{
    char from[MAX_NAME], to[MAX_NAME];
    int mode;

    printSeparator();
    printf(COLOR_HEADER "  查询最优路线" ANSI_RESET "\n");
    printSeparator();

    printf(COLOR_PROMPT "  请输入出发城市: " ANSI_RESET);
    readLine(from, MAX_NAME);
    printf(COLOR_PROMPT "  请输入到达城市: " ANSI_RESET);
    readLine(to, MAX_NAME);
    printf(COLOR_PROMPT "  请选择优化方式 (0=最快, 1=最省钱): " ANSI_RESET);
    if (scanf("%d", &mode) != 1 || (mode != 0 && mode != 1)) {
        printError("无效的选择");
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    int src = findCity(from);
    int dst = findCity(to);
    if (src < 0 || dst < 0) {
        printError("出发或到达城市不存在");
        return;
    }
    if (src == dst) {
        printError("出发和到达城市不能相同");
        return;
    }

    int total_t, total_c, pathLen;
    int *path = NULL, *prev_e = NULL;
    TraffStatus st = dijkstraQuery(src, dst, mode, &total_t, &total_c,
                                    &pathLen, &path, &prev_e);
    if (st == TRAFF_ERR_NO_PATH) {
        printError("没有找到可行路线");
    } else if (st == TRAFF_OK) {
        printRouteResult(src, dst, mode, total_t, total_c, pathLen, path, prev_e);
        free(path);
        free(prev_e);
    } else {
        printError("查询失败");
    }
}

static void mainMenu(void)
{
    int choice;
    do {
        printSeparator();
        printf(COLOR_HEADER "\n");
        printf("      ╔══════════════════════════════════════════╗\n");
        printf("      ║       全国交通咨询模拟系统 V2.0           ║\n");
        printf("      ╚══════════════════════════════════════════╝\n" ANSI_RESET);
        printSeparator();
        printf("  1. 查看所有城市\n");
        printf("  2. 查看所有线路\n");
        printf("  3. 添加城市\n");
        printf("  4. 添加交通线路\n");
        printf("  5. 查询最优路线\n");
        printf("  0. 退出系统\n");
        printSeparator();
        printf(COLOR_PROMPT "  请选择功能: " ANSI_RESET);

        if (scanf("%d", &choice) != 1) {
            clearInputBuffer();
            printError("无效输入，请输入数字");
            continue;
        }
        clearInputBuffer();

        switch (choice) {
            case 1: listCities(); break;
            case 2: listRoutes(); break;
            case 3: menuAddCity(); break;
            case 4: menuAddRoute(); break;
            case 5: menuQueryRoute(); break;
            case 0:
                saveAllData();
                printSuccess("数据已保存，再见！");
                break;
            default:
                printError("无效选项，请重新选择");
        }
    } while (choice != 0);
}

/*====================================================================================================
 *
 *  内置自测框架
 *
 *====================================================================================================*/

static int g_testPassed = 0;
static int g_testFailed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (cond) { \
        printf(COLOR_SUCCESS "  [PASS] " ANSI_RESET "%s\n", msg); \
        g_testPassed++; \
    } else { \
        printf(COLOR_ERROR "  [FAIL] " ANSI_RESET "%s (line %d)\n", msg, __LINE__); \
        g_testFailed++; \
    } \
} while(0)

#define TEST_STATUS(st, expected, msg) TEST_ASSERT((st) == (expected), msg)

static void runSelfTest(void)
{
    printf(COLOR_HEADER "\n╔══════════════════════════════════════════╗\n");
    printf("║          运行内置自测 V2.0                ║\n");
    printf("╚══════════════════════════════════════════╝\n\n" ANSI_RESET);

    cityArrayFree(&g_cities);
    edgePoolFree(&g_edges);
    cityArrayInit(&g_cities);
    edgePoolInit(&g_edges);

    remove(DATA_FILE);

    TraffStatus st;

    /* TC01: 添加城市 - 正常 */
    st = addCity("北京");
    TEST_STATUS(st, TRAFF_OK, "TC01: 添加城市北京成功");

    /* TC02: 添加城市 - 重复 */
    st = addCity("北京");
    TEST_STATUS(st, TRAFF_ERR_DUPLICATE, "TC02: 重复城市返回错误");

    /* TC03: 添加城市 - 空名称 */
    st = addCity("");
    TEST_STATUS(st, TRAFF_ERR_INVALID_INPUT, "TC03: 空城市名返回错误");

    /* TC04: 添加多个城市 */
    addCity("上海"); addCity("广州"); addCity("武汉");
    addCity("西安"); addCity("南京"); addCity("杭州"); addCity("成都");
    TEST_ASSERT(g_cities.count == 8, "TC04: 添加8个城市后计数正确");

    /* TC05: 查询城市 */
    int idx = findCity("上海");
    TEST_ASSERT(idx == 1, "TC05: 按名称查询城市成功");

    /* TC06: 查询不存在的城市 */
    idx = findCity("深圳");
    TEST_ASSERT(idx == -1, "TC06: 查询不存在城市返回-1");

    /* TC07: 添加火车线路 */
    st = addRoute(TRAIN, "G101", "北京", "上海", "08:00", "12:30", 553);
    TEST_STATUS(st, TRAFF_OK, "TC07: 添加火车线路G101成功");

    /* TC08: 添加飞机线路 */
    st = addRoute(PLANE, "CA101", "北京", "上海", "07:00", "09:10", 800);
    TEST_STATUS(st, TRAFF_OK, "TC08: 添加飞机线路CA101成功");

    /* TC09: 添加线路 - 城市不存在 */
    st = addRoute(TRAIN, "G999", "北京", "深圳", "08:00", "12:00", 500);
    TEST_STATUS(st, TRAFF_ERR_NOT_FOUND, "TC09: 不存在城市添加线路返回错误");

    /* TC10: 构建测试路网 */
    addRoute(TRAIN, "G201", "北京", "广州", "07:00", "14:30", 862);
    addRoute(TRAIN, "G301", "北京", "武汉", "08:30", "12:00", 515);
    addRoute(TRAIN, "G401", "上海", "广州", "08:00", "13:20", 793);
    addRoute(TRAIN, "G403", "上海", "南京", "09:00", "10:30", 135);
    addRoute(TRAIN, "G701", "武汉", "西安", "09:00", "13:00", 458);
    addRoute(TRAIN, "G901", "南京", "杭州", "08:00", "09:00",  89);
    addRoute(PLANE, "CA201", "北京", "广州", "08:00", "11:00", 1200);
    addRoute(PLANE, "CZ201", "广州", "杭州", "10:00", "12:00", 780);
    TEST_ASSERT(g_edges.count == 10, "TC10: 添加10条线路后边数正确");

    /* TC11: 最短路径查询 - 北京到上海(最快) */
    int tt, tc, pl;
    int *p = NULL, *pe = NULL;
    st = dijkstraQuery(0, 1, 0, &tt, &tc, &pl, &p, &pe);
    TEST_STATUS(st, TRAFF_OK, "TC11a: 北京到上海最快查询成功");
    if (st == TRAFF_OK) {
        TEST_ASSERT(tt <= 130, "TC11b: 北京到上海最快时间约2小时10分(飞机)");
        free(p); free(pe);
    }

    /* TC12: 最短路径查询 - 北京到上海(最省钱) */
    st = dijkstraQuery(0, 1, 1, &tt, &tc, &pl, &p, &pe);
    TEST_STATUS(st, TRAFF_OK, "TC12a: 北京到上海最省钱查询成功");
    if (st == TRAFF_OK) {
        TEST_ASSERT(tc == 553, "TC12b: 北京到上海最省钱为火车553元");
        free(p); free(pe);
    }

    /* TC13: 中转路径查询 - 北京到杭州(需中转) */
    int hz = findCity("杭州");
    st = dijkstraQuery(0, hz, 0, &tt, &tc, &pl, &p, &pe);
    TEST_STATUS(st, TRAFF_OK, "TC13a: 北京到杭州中转查询成功");
    if (st == TRAFF_OK) {
        TEST_ASSERT(pl >= 3, "TC13b: 北京到杭州需要中转(路径长度>=3)");
        free(p); free(pe);
    }

    /* TC14: 不可达路径测试 */
    addCity("海口");
    int hk = findCity("海口");
    st = dijkstraQuery(0, hk, 0, &tt, &tc, &pl, &p, &pe);
    TEST_STATUS(st, TRAFF_ERR_NO_PATH, "TC14: 不可达城市返回无路径错误");

    /* TC15: 保存文件 */
    st = saveAllData();
    TEST_STATUS(st, TRAFF_OK, "TC15: 保存数据到文件成功");

    /* TC16: Magic Header验证 */
    FILE *fp = fopen(DATA_FILE, "rb");
    TEST_ASSERT(fp != NULL, "TC16a: 数据文件存在");
    if (fp) {
        char mg[MAGIC_LEN + 1] = {0};
        fread(mg, 1, MAGIC_LEN, fp);
        fclose(fp);
        TEST_ASSERT(strcmp(mg, MAGIC_HEADER) == 0, "TC16b: Magic Header验证正确(TRAFV2)");
    }

    /* TC17: 清空并重新加载 */
    size_t cityBefore = g_cities.count;
    size_t edgeBefore = g_edges.count;
    cityArrayFree(&g_cities);
    edgePoolFree(&g_edges);
    cityArrayInit(&g_cities);
    edgePoolInit(&g_edges);
    st = loadAllData();
    TEST_STATUS(st, TRAFF_OK, "TC17a: 从文件加载数据成功");
    TEST_ASSERT(g_cities.count == cityBefore, "TC17b: 加载后城市数量正确");
    TEST_ASSERT(g_edges.count == edgeBefore, "TC17c: 加载后线路数量正确");

    /* TC18: 动态数组扩容测试 */
    for (int i = 0; i < 30; i++) {
        char name[32];
        sprintf(name, "测试城市%d", i);
        addCity(name);
    }
    TEST_ASSERT(g_cities.count == cityBefore + 30, "TC18: 动态数组扩容添加30个城市成功");

    /* TC19: 跨夜航班计算 */
    addRoute(PLANE, "HU999", "北京", "海口", "23:00", "06:00", 600);
    Edge *last = &g_edges.items[g_edges.count - 1];
    TEST_ASSERT(last->duration == 420, "TC19: 跨夜航班(23:00-06:00)时间计算为420分钟");

    /* TC20: 加载后数据完整性 */
    int bj2 = findCity("北京");
    TEST_ASSERT(bj2 == 0, "TC20: 重新加载后北京索引正确");

    /* 清理测试数据 */
    cityArrayFree(&g_cities);
    edgePoolFree(&g_edges);
    cityArrayInit(&g_cities);
    edgePoolInit(&g_edges);
    remove(DATA_FILE);

    printSeparator();
    printf(COLOR_INFO "\n  测试完成: " ANSI_RESET);
    printf(COLOR_SUCCESS "%d 通过" ANSI_RESET ", " COLOR_ERROR "%d 失败" ANSI_RESET "\n\n",
           g_testPassed, g_testFailed);

    if (g_testFailed == 0) {
        printf(COLOR_SUCCESS "  所有测试通过！✓\n\n" ANSI_RESET);
    } else {
        printf(COLOR_ERROR "  有 %d 个测试失败！\n\n" ANSI_RESET, g_testFailed);
    }
}

/*====================================================================================================
 *
 *  主函数 - CLI参数解析
 *
 *====================================================================================================*/

int main(int argc, char *argv[])
{
    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            SetConsoleOutputCP(65001);
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD mode = 0;
            GetConsoleMode(hOut, &mode);
            SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
            showHelp();
            return 0;
        }
        if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0) {
            SetConsoleOutputCP(65001);
            SetConsoleCP(65001);
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD mode = 0;
            GetConsoleMode(hOut, &mode);
            SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
            runSelfTest();
            return g_testFailed > 0 ? 1 : 0;
        }
    }

    if (initSystem() != TRAFF_OK) {
        fprintf(stderr, "系统初始化失败\n");
        return 1;
    }

    mainMenu();
    cleanupSystem();
    return 0;
}
