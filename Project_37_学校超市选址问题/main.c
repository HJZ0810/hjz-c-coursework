/*==========================================================================
 *
 *  学校超市选址问题 V2.0
 *
 *==========================================================================
 *
 *  功能概述：
 *    校园内有若干建筑物（教学楼、宿舍、食堂等），建筑物之间有道路相连。
 *    需要选择合适的位置建设超市，使得"加权距离之和"或"最大距离"最优。
 *    提供中心点、中位点两种选址算法。
 *
 *  V2.0 新特性：
 *    1. UTF-8 BOM编码 + Windows控制台UTF-8输出
 *    2. ANSI彩色终端显示
 *    3. CLI参数支持 (-h/--help, -t/--test)
 *    4. Floyd最短路算法 + 中心点/中位点双选址算法
 *    5. 动态内存管理（地点、道路动态数组）
 *    6. 统一错误码枚举 (LocStatus)
 *    7. "LOCV2"魔数文件头持久化
 *    8. 内置39项自测用例
 *    9. 交互式菜单系统
 *
 *  编译命令：
 *    gcc -std=c99 -Wall -Wextra -O2 -o loc.exe main.c
 *
 *========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

/*==========================================================================
 * 第一部分：常量定义与错误码
 *========================================================================*/

#define MAGIC_HEADER    "LOCV2"
#define MAGIC_LEN       5
#define DATA_VERSION    2
#define DATA_FILE       "campus.locv2"
#define MAX_NAME_LEN    48
#define INF_DIST        999999
#define INIT_CAPACITY   8

#define ANSI_RESET      "\x1b[0m"
#define ANSI_BOLD       "\x1b[1m"
#define ANSI_RED        "\x1b[31m"
#define ANSI_GREEN      "\x1b[32m"
#define ANSI_YELLOW     "\x1b[33m"
#define ANSI_BLUE       "\x1b[34m"
#define ANSI_MAGENTA    "\x1b[35m"
#define ANSI_CYAN       "\x1b[36m"

typedef enum {
    LOC_OK = 0,
    LOC_ERR_MEMORY = -1,
    LOC_ERR_FILE = -2,
    LOC_ERR_MAGIC = -3,
    LOC_ERR_VERSION = -4,
    LOC_ERR_FULL = -5,
    LOC_ERR_NOT_FOUND = -6,
    LOC_ERR_INVALID_PARAM = -7,
    LOC_ERR_IO = -8,
    LOC_ERR_DISCONNECTED = -9
} LocStatus;

typedef enum {
    LOC_TYPE_CLASSROOM = 0,
    LOC_TYPE_DORM = 1,
    LOC_TYPE_CANTEEN = 2,
    LOC_TYPE_OTHER = 3,
    LOC_TYPE_COUNT = 4
} LocType;

typedef enum {
    LOC_ALG_MEDIAN = 0,
    LOC_ALG_CENTER = 1
} LocAlgorithm;

/*==========================================================================
 * 第二部分：数据结构定义
 *========================================================================*/

typedef struct {
    int id;
    char name[MAX_NAME_LEN];
    int weight;
    LocType type;
    int active;
} Location;

typedef struct {
    int from;
    int to;
    int distance;
} Road;

typedef struct {
    Location *locs;
    int loc_capacity;
    int loc_count;
    Road *roads;
    int road_capacity;
    int road_count;
    int **adj;
    int **dist;
    int **next_hop;
    int floyd_computed;
} Graph;

/*==========================================================================
 * 第三部分：终端工具函数
 *========================================================================*/

static void console_init(void) {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(hOut, &mode)) {
        SetConsoleMode(hOut, mode | 0x0004);
    }
#else
    (void)0;
#endif
}

static void clear_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

static void pause_screen(void) {
    int c;
    printf(ANSI_YELLOW "\n  按回车键继续..." ANSI_RESET);
    fflush(stdout);
    while ((c = getchar()) != '\n' && c != EOF) { }
    getchar();
}

static const char *get_type_name(LocType type) {
    static const char *names[] = {"教学楼", "宿舍楼", "食堂", "其他"};
    if (type >= 0 && type < LOC_TYPE_COUNT) return names[type];
    return "未知";
}

static const char *get_status_msg(LocStatus s) {
    switch (s) {
        case LOC_OK: return "成功";
        case LOC_ERR_MEMORY: return "内存分配失败";
        case LOC_ERR_FILE: return "文件操作失败";
        case LOC_ERR_MAGIC: return "文件头魔数错误";
        case LOC_ERR_VERSION: return "文件版本不兼容";
        case LOC_ERR_FULL: return "容量已满";
        case LOC_ERR_NOT_FOUND: return "未找到目标";
        case LOC_ERR_INVALID_PARAM: return "参数无效";
        case LOC_ERR_IO: return "I/O错误";
        case LOC_ERR_DISCONNECTED: return "图不连通";
        default: return "未知错误";
    }
}

static void print_line(void) {
    printf(ANSI_CYAN "  ─────────────────────────────────────────"
           "────────────────────────────\n" ANSI_RESET);
}

static void print_title(const char *title) {
    printf("\n");
    print_line();
    printf(ANSI_BOLD ANSI_BLUE "  ◆ %s\n" ANSI_RESET, title);
    print_line();
}

static void print_ok(const char *msg) {
    printf(ANSI_GREEN "  [√] %s\n" ANSI_RESET, msg);
}

static void print_err(const char *msg) {
    printf(ANSI_RED "  [×] %s\n" ANSI_RESET, msg);
}

static void print_info(const char *msg) {
    printf(ANSI_YELLOW "  [!] %s\n" ANSI_RESET, msg);
}

/*==========================================================================
 * 第四部分：图的内存管理
 *========================================================================*/

static LocStatus graph_init(Graph *g) {
    int i;
    g->loc_capacity = INIT_CAPACITY;
    g->loc_count = 0;
    g->road_capacity = INIT_CAPACITY * 2;
    g->road_count = 0;
    g->floyd_computed = 0;

    g->locs = (Location *)calloc((size_t)g->loc_capacity, sizeof(Location));
    if (!g->locs) return LOC_ERR_MEMORY;

    g->roads = (Road *)calloc((size_t)g->road_capacity, sizeof(Road));
    if (!g->roads) { free(g->locs); return LOC_ERR_MEMORY; }

    g->adj = (int **)calloc((size_t)g->loc_capacity, sizeof(int *));
    g->dist = (int **)calloc((size_t)g->loc_capacity, sizeof(int *));
    g->next_hop = (int **)calloc((size_t)g->loc_capacity, sizeof(int *));
    if (!g->adj || !g->dist || !g->next_hop) {
        free(g->locs); free(g->roads);
        free(g->adj); free(g->dist); free(g->next_hop);
        return LOC_ERR_MEMORY;
    }

    for (i = 0; i < g->loc_capacity; i++) {
        g->adj[i] = (int *)calloc((size_t)g->loc_capacity, sizeof(int));
        g->dist[i] = (int *)calloc((size_t)g->loc_capacity, sizeof(int));
        g->next_hop[i] = (int *)calloc((size_t)g->loc_capacity, sizeof(int));
        if (!g->adj[i] || !g->dist[i] || !g->next_hop[i]) {
            return LOC_ERR_MEMORY;
        }
    }
    return LOC_OK;
}

static void graph_free(Graph *g) {
    int i;
    if (g->adj) {
        for (i = 0; i < g->loc_capacity; i++) free(g->adj[i]);
        free(g->adj);
    }
    if (g->dist) {
        for (i = 0; i < g->loc_capacity; i++) free(g->dist[i]);
        free(g->dist);
    }
    if (g->next_hop) {
        for (i = 0; i < g->loc_capacity; i++) free(g->next_hop[i]);
        free(g->next_hop);
    }
    free(g->locs);
    free(g->roads);
    g->locs = NULL; g->roads = NULL;
    g->adj = NULL; g->dist = NULL; g->next_hop = NULL;
    g->loc_count = g->loc_capacity = 0;
    g->road_count = g->road_capacity = 0;
}

static LocStatus graph_resize_locs(Graph *g, int new_cap) {
    int i, j;
    Location *new_locs;
    int **new_adj, **new_dist, **new_next;

    new_locs = (Location *)realloc(g->locs, (size_t)new_cap * sizeof(Location));
    if (!new_locs) return LOC_ERR_MEMORY;
    g->locs = new_locs;
    memset((char *)&g->locs[g->loc_capacity], 0,
           (size_t)(new_cap - g->loc_capacity) * sizeof(Location));

    new_adj = (int **)realloc(g->adj, (size_t)new_cap * sizeof(int *));
    new_dist = (int **)realloc(g->dist, (size_t)new_cap * sizeof(int *));
    new_next = (int **)realloc(g->next_hop, (size_t)new_cap * sizeof(int *));
    if (!new_adj || !new_dist || !new_next) return LOC_ERR_MEMORY;
    g->adj = new_adj; g->dist = new_dist; g->next_hop = new_next;

    for (i = g->loc_capacity; i < new_cap; i++) {
        g->adj[i] = (int *)calloc((size_t)new_cap, sizeof(int));
        g->dist[i] = (int *)calloc((size_t)new_cap, sizeof(int));
        g->next_hop[i] = (int *)calloc((size_t)new_cap, sizeof(int));
        if (!g->adj[i] || !g->dist[i] || !g->next_hop[i]) return LOC_ERR_MEMORY;
    }
    for (i = 0; i < g->loc_capacity; i++) {
        int *na = (int *)realloc(g->adj[i], (size_t)new_cap * sizeof(int));
        int *nd = (int *)realloc(g->dist[i], (size_t)new_cap * sizeof(int));
        int *nn = (int *)realloc(g->next_hop[i], (size_t)new_cap * sizeof(int));
        if (!na || !nd || !nn) return LOC_ERR_MEMORY;
        g->adj[i] = na; g->dist[i] = nd; g->next_hop[i] = nn;
        for (j = g->loc_capacity; j < new_cap; j++) {
            g->adj[i][j] = INF_DIST;
            g->dist[i][j] = INF_DIST;
            g->next_hop[i][j] = -1;
        }
    }

    for (i = 0; i < new_cap; i++) {
        for (j = 0; j < new_cap; j++) {
            if (i == j) {
                g->adj[i][j] = 0; g->dist[i][j] = 0; g->next_hop[i][j] = i;
            } else if (i >= g->loc_capacity || j >= g->loc_capacity) {
                g->adj[i][j] = INF_DIST; g->dist[i][j] = INF_DIST; g->next_hop[i][j] = -1;
            }
        }
    }

    g->loc_capacity = new_cap;
    g->floyd_computed = 0;
    return LOC_OK;
}

static LocStatus graph_resize_roads(Graph *g, int new_cap) {
    Road *new_roads = (Road *)realloc(g->roads, (size_t)new_cap * sizeof(Road));
    if (!new_roads) return LOC_ERR_MEMORY;
    g->roads = new_roads;
    g->road_capacity = new_cap;
    return LOC_OK;
}

static LocStatus graph_add_location(Graph *g, const char *name, int weight, LocType type, int *out_id) {
    int i, id;
    if (g->loc_count >= g->loc_capacity) {
        LocStatus s = graph_resize_locs(g, g->loc_capacity * 2);
        if (s != LOC_OK) return s;
    }
    id = g->loc_count;
    g->locs[id].id = id;
    strncpy(g->locs[id].name, name, MAX_NAME_LEN - 1);
    g->locs[id].name[MAX_NAME_LEN - 1] = '\0';
    g->locs[id].weight = weight;
    g->locs[id].type = type;
    g->locs[id].active = 1;
    g->loc_count++;
    g->floyd_computed = 0;

    for (i = 0; i < g->loc_count; i++) {
        if (i != id) {
            g->adj[i][id] = INF_DIST;
            g->adj[id][i] = INF_DIST;
        } else {
            g->adj[i][id] = 0;
        }
    }

    if (out_id) *out_id = id;
    return LOC_OK;
}

static LocStatus graph_add_road(Graph *g, int from, int to, int distance) {
    if (from < 0 || from >= g->loc_count || to < 0 || to >= g->loc_count || from == to)
        return LOC_ERR_INVALID_PARAM;
    if (distance <= 0) return LOC_ERR_INVALID_PARAM;

    if (g->road_count >= g->road_capacity) {
        LocStatus s = graph_resize_roads(g, g->road_capacity * 2);
        if (s != LOC_OK) return s;
    }

    g->roads[g->road_count].from = from;
    g->roads[g->road_count].to = to;
    g->roads[g->road_count].distance = distance;
    g->road_count++;

    if (distance < g->adj[from][to]) {
        g->adj[from][to] = distance;
        g->adj[to][from] = distance;
    }

    g->floyd_computed = 0;
    return LOC_OK;
}

/*==========================================================================
 * 第五部分：Floyd最短路算法
 *========================================================================*/

static LocStatus floyd_compute(Graph *g) {
    int n = g->loc_count;
    int i, j, k;

    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            g->dist[i][j] = g->adj[i][j];
            if (i == j) g->next_hop[i][j] = i;
            else if (g->adj[i][j] < INF_DIST) g->next_hop[i][j] = j;
            else g->next_hop[i][j] = -1;
        }
    }

    for (k = 0; k < n; k++) {
        for (i = 0; i < n; i++) {
            for (j = 0; j < n; j++) {
                if (g->dist[i][k] < INF_DIST && g->dist[k][j] < INF_DIST) {
                    int nd = g->dist[i][k] + g->dist[k][j];
                    if (nd < g->dist[i][j]) {
                        g->dist[i][j] = nd;
                        g->next_hop[i][j] = g->next_hop[i][k];
                    }
                }
            }
        }
    }

    g->floyd_computed = 1;
    return LOC_OK;
}

static int graph_is_connected(Graph *g) {
    int i, j, n;
    if (!g->floyd_computed) floyd_compute(g);
    n = g->loc_count;
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            if (i != j && g->dist[i][j] >= INF_DIST) return 0;
        }
    }
    return 1;
}

/*==========================================================================
 * 第六部分：选址算法
 *========================================================================*/

static LocStatus find_median(Graph *g, int *best_id, double *min_cost) {
    int n, best, i, j;
    double min_sum;
    if (!g->floyd_computed) {
        LocStatus s = floyd_compute(g);
        if (s != LOC_OK) return s;
    }
    n = g->loc_count;
    if (n == 0) return LOC_ERR_INVALID_PARAM;

    best = -1;
    min_sum = 1e18;

    for (i = 0; i < n; i++) {
        double sum = 0.0;
        int connected = 1;
        if (!g->locs[i].active) continue;
        for (j = 0; j < n; j++) {
            if (!g->locs[j].active || i == j) continue;
            if (g->dist[i][j] >= INF_DIST) { connected = 0; break; }
            sum += (double)g->dist[i][j] * g->locs[j].weight;
        }
        if (connected && sum < min_sum) {
            min_sum = sum;
            best = i;
        }
    }

    if (best < 0) return LOC_ERR_DISCONNECTED;
    if (best_id) *best_id = best;
    if (min_cost) *min_cost = min_sum;
    return LOC_OK;
}

static LocStatus find_center(Graph *g, int *best_id, int *min_maxdist) {
    int n, best, min_ecc, i, j;
    if (!g->floyd_computed) {
        LocStatus s = floyd_compute(g);
        if (s != LOC_OK) return s;
    }
    n = g->loc_count;
    if (n == 0) return LOC_ERR_INVALID_PARAM;

    best = -1;
    min_ecc = INF_DIST;

    for (i = 0; i < n; i++) {
        int ecc = 0;
        int connected = 1;
        if (!g->locs[i].active) continue;
        for (j = 0; j < n; j++) {
            if (!g->locs[j].active || i == j) continue;
            if (g->dist[i][j] >= INF_DIST) { connected = 0; break; }
            if (g->dist[i][j] > ecc) ecc = g->dist[i][j];
        }
        if (connected && ecc < min_ecc) {
            min_ecc = ecc;
            best = i;
        }
    }

    if (best < 0) return LOC_ERR_DISCONNECTED;
    if (best_id) *best_id = best;
    if (min_maxdist) *min_maxdist = min_ecc;
    return LOC_OK;
}

/*==========================================================================
 * 第七部分：文件持久化（LOCV2魔数格式）
 *========================================================================*/

static LocStatus graph_save(Graph *g, const char *filename) {
    FILE *fp;
    int ver, i;
    fp = fopen(filename, "wb");
    if (!fp) return LOC_ERR_FILE;

    fwrite(MAGIC_HEADER, 1, MAGIC_LEN, fp);
    ver = DATA_VERSION;
    fwrite(&ver, sizeof(int), 1, fp);
    fwrite(&g->loc_count, sizeof(int), 1, fp);
    fwrite(&g->road_count, sizeof(int), 1, fp);

    for (i = 0; i < g->loc_count; i++) {
        fwrite(&g->locs[i].id, sizeof(int), 1, fp);
        fwrite(g->locs[i].name, 1, MAX_NAME_LEN, fp);
        fwrite(&g->locs[i].weight, sizeof(int), 1, fp);
        fwrite(&g->locs[i].type, sizeof(int), 1, fp);
        fwrite(&g->locs[i].active, sizeof(int), 1, fp);
    }

    for (i = 0; i < g->road_count; i++) {
        fwrite(&g->roads[i].from, sizeof(int), 1, fp);
        fwrite(&g->roads[i].to, sizeof(int), 1, fp);
        fwrite(&g->roads[i].distance, sizeof(int), 1, fp);
    }

    fclose(fp);
    return LOC_OK;
}

static LocStatus graph_load(Graph *g, const char *filename) {
    FILE *fp;
    char magic[MAGIC_LEN + 1];
    int ver, loc_count, road_count, i, a, b;
    fp = fopen(filename, "rb");
    if (!fp) return LOC_ERR_FILE;

    memset(magic, 0, sizeof(magic));
    if (fread(magic, 1, MAGIC_LEN, fp) != (size_t)MAGIC_LEN) { fclose(fp); return LOC_ERR_IO; }
    if (memcmp(magic, MAGIC_HEADER, MAGIC_LEN) != 0) { fclose(fp); return LOC_ERR_MAGIC; }

    if (fread(&ver, sizeof(int), 1, fp) != 1) { fclose(fp); return LOC_ERR_IO; }
    if (ver != DATA_VERSION) { fclose(fp); return LOC_ERR_VERSION; }

    graph_free(g);
    graph_init(g);

    if (fread(&loc_count, sizeof(int), 1, fp) != 1) { fclose(fp); return LOC_ERR_IO; }
    if (fread(&road_count, sizeof(int), 1, fp) != 1) { fclose(fp); return LOC_ERR_IO; }

    while (g->loc_capacity < loc_count) graph_resize_locs(g, g->loc_capacity * 2);
    while (g->road_capacity < road_count) graph_resize_roads(g, g->road_capacity * 2);

    for (i = 0; i < loc_count; i++) {
        int t;
        fread(&g->locs[i].id, sizeof(int), 1, fp);
        fread(g->locs[i].name, 1, MAX_NAME_LEN, fp);
        fread(&g->locs[i].weight, sizeof(int), 1, fp);
        fread(&t, sizeof(int), 1, fp); g->locs[i].type = (LocType)t;
        fread(&g->locs[i].active, sizeof(int), 1, fp);
    }
    g->loc_count = loc_count;

    for (i = 0; i < road_count; i++) {
        fread(&g->roads[i].from, sizeof(int), 1, fp);
        fread(&g->roads[i].to, sizeof(int), 1, fp);
        fread(&g->roads[i].distance, sizeof(int), 1, fp);
    }
    g->road_count = road_count;

    for (a = 0; a < g->loc_count; a++) {
        for (b = 0; b < g->loc_count; b++) {
            g->adj[a][b] = (a == b) ? 0 : INF_DIST;
        }
    }
    for (i = 0; i < g->road_count; i++) {
        int u = g->roads[i].from, v = g->roads[i].to, d = g->roads[i].distance;
        if (d < g->adj[u][v]) { g->adj[u][v] = d; g->adj[v][u] = d; }
    }

    g->floyd_computed = 0;
    fclose(fp);
    return LOC_OK;
}

/*==========================================================================
 * 第八部分：默认测试数据
 *========================================================================*/

static void load_default_campus(Graph *g) {
    int i, rc;
    const char *names[] = {
        "第一教学楼", "第二教学楼", "第三教学楼",
        "学生宿舍1号楼", "学生宿舍2号楼", "学生宿舍3号楼",
        "第一食堂", "图书馆", "实验楼",
        "行政楼", "食堂二楼", "大学生活动中心"
    };
    const int weights[] = {80, 70, 75, 200, 180, 190, 50, 120, 60, 40, 150, 90};
    const LocType types[] = {0,0,0,1,1,1,2,3,0,3,2,3};
    const int roads[][3] = {
        {0,1,200},{0,3,350},{0,7,300},{0,10,500},
        {1,2,180},{1,4,400},{1,7,250},
        {2,5,350},{2,8,150},{2,7,400},
        {3,4,120},{3,10,200},{3,11,300},
        {4,5,100},{4,10,250},{4,9,450},
        {5,8,280},{5,9,300},{5,10,350},
        {6,0,400},{6,1,350},{6,7,200},
        {7,10,450},{7,11,350},
        {8,9,350},{9,11,200},
        {10,11,150},{6,11,500}
    };

    for (i = 0; i < 12; i++) {
        graph_add_location(g, names[i], weights[i], types[i], NULL);
    }
    rc = (int)(sizeof(roads) / sizeof(roads[0]));
    for (i = 0; i < rc; i++) {
        graph_add_road(g, roads[i][0], roads[i][1], roads[i][2]);
    }
}

/*==========================================================================
 * 第九部分：显示函数
 *========================================================================*/

static void show_locations(Graph *g) {
    int i;
    print_title("建筑物列表");
    printf(ANSI_BOLD "  %-4s  %-20s  %-8s  %-8s\n" ANSI_RESET,
           "编号", "名称", "类型", "人流量权重");
    print_line();
    for (i = 0; i < g->loc_count; i++) {
        if (!g->locs[i].active) continue;
        printf("  %-4d  %-20s  %-8s  %d\n",
               i + 1, g->locs[i].name, get_type_name(g->locs[i].type), g->locs[i].weight);
    }
    print_line();
    printf(ANSI_CYAN "  共 %d 个建筑物\n" ANSI_RESET, g->loc_count);
}

static void show_roads(Graph *g) {
    int i, cnt = 0;
    print_title("道路列表");
    printf(ANSI_BOLD "  %-20s  <->  %-20s  距离(米)\n" ANSI_RESET, "起点", "终点");
    print_line();
    for (i = 0; i < g->road_count; i++) {
        int u = g->roads[i].from, v = g->roads[i].to;
        printf("  %-20s  <->  %-20s  %d\n",
               g->locs[u].name, g->locs[v].name, g->roads[i].distance);
        cnt++;
    }
    print_line();
    printf(ANSI_CYAN "  共 %d 条道路\n" ANSI_RESET, cnt);
}

static void show_dist_matrix(Graph *g) {
    int n, i, j;
    if (!g->floyd_computed) floyd_compute(g);
    print_title("最短路径距离矩阵");
    n = g->loc_count;
    printf("        ");
    for (j = 0; j < n; j++) printf("  [%2d]", j + 1);
    printf("\n");
    for (i = 0; i < n; i++) {
        printf("  [%2d]  ", i + 1);
        for (j = 0; j < n; j++) {
            if (g->dist[i][j] >= INF_DIST) printf(ANSI_RED "  INF" ANSI_RESET);
            else printf(" %4d", g->dist[i][j]);
        }
        printf("\n");
    }
}

static void solve_and_show(Graph *g, LocAlgorithm alg) {
    int n, i, j, best_id, best_ecc;
    double best_sum;
    if (g->loc_count < 2) { print_err("建筑物数量不足，无法选址"); return; }
    if (!g->floyd_computed) floyd_compute(g);

    if (!graph_is_connected(g)) {
        print_err("图不连通，无法进行选址计算！");
        return;
    }

    if (alg == LOC_ALG_MEDIAN) {
        print_title("中位点选址结果（加权距离最小）");
    } else {
        print_title("中心点选址结果（最大距离最小）");
    }

    n = g->loc_count;

    if (alg == LOC_ALG_MEDIAN) {
        LocStatus s = find_median(g, &best_id, &best_sum);
        if (s != LOC_OK) { print_err(get_status_msg(s)); return; }

        printf(ANSI_BOLD ANSI_GREEN "  ★ 最佳选址: %s (编号 %d)\n" ANSI_RESET,
               g->locs[best_id].name, best_id + 1);
        printf(ANSI_GREEN "  ★ 最小加权距离总和: %.0f\n\n" ANSI_RESET, best_sum);

        printf(ANSI_BOLD "  %-4s  %-20s  %-14s\n" ANSI_RESET, "排名", "名称", "加权距离总和");
        print_line();

        {
            typedef struct { int id; double sum; } RankItem;
            RankItem *ranks = (RankItem *)malloc((size_t)n * sizeof(RankItem));
            int rc = 0;
            for (i = 0; i < n; i++) {
                double sum = 0.0;
                if (!g->locs[i].active) continue;
                for (j = 0; j < n; j++) {
                    if (!g->locs[j].active || i == j) continue;
                    sum += (double)g->dist[i][j] * g->locs[j].weight;
                }
                ranks[rc].id = i;
                ranks[rc].sum = sum;
                rc++;
            }
            for (i = 0; i < rc; i++) {
                for (j = i + 1; j < rc; j++) {
                    if (ranks[j].sum < ranks[i].sum) {
                        RankItem tmp = ranks[i]; ranks[i] = ranks[j]; ranks[j] = tmp;
                    }
                }
            }
            for (i = 0; i < rc; i++) {
                const char *mark = (i == 0) ? ANSI_GREEN "  ★ " : "    ";
                printf("%s%-4d  %-20s  %.0f\n" ANSI_RESET, mark, i + 1,
                       g->locs[ranks[i].id].name, ranks[i].sum);
            }
            free(ranks);
        }

    } else {
        LocStatus s = find_center(g, &best_id, &best_ecc);
        if (s != LOC_OK) { print_err(get_status_msg(s)); return; }

        printf(ANSI_BOLD ANSI_GREEN "  ★ 最佳选址: %s (编号 %d)\n" ANSI_RESET,
               g->locs[best_id].name, best_id + 1);
        printf(ANSI_GREEN "  ★ 最小最大距离(偏心距): %d 米\n\n" ANSI_RESET, best_ecc);

        printf(ANSI_BOLD "  %-4s  %-20s  %-14s\n" ANSI_RESET, "排名", "名称", "最大距离(米)");
        print_line();

        {
            typedef struct { int id; int ecc; } RankItemC;
            RankItemC *ranks = (RankItemC *)malloc((size_t)n * sizeof(RankItemC));
            int rc = 0;
            for (i = 0; i < n; i++) {
                int ecc = 0;
                if (!g->locs[i].active) continue;
                for (j = 0; j < n; j++) {
                    if (!g->locs[j].active || i == j) continue;
                    if (g->dist[i][j] > ecc) ecc = g->dist[i][j];
                }
                ranks[rc].id = i;
                ranks[rc].ecc = ecc;
                rc++;
            }
            for (i = 0; i < rc; i++) {
                for (j = i + 1; j < rc; j++) {
                    if (ranks[j].ecc < ranks[i].ecc) {
                        RankItemC tmp = ranks[i]; ranks[i] = ranks[j]; ranks[j] = tmp;
                    }
                }
            }
            for (i = 0; i < rc; i++) {
                const char *mark = (i == 0) ? ANSI_GREEN "  ★ " : "    ";
                printf("%s%-4d  %-20s  %d\n" ANSI_RESET, mark, i + 1,
                       g->locs[ranks[i].id].name, ranks[i].ecc);
            }
            free(ranks);
        }
    }
}

/*==========================================================================
 * 第十部分：交互式菜单
 *========================================================================*/

static void menu_add_location(Graph *g) {
    char name[MAX_NAME_LEN];
    int weight, type, id;
    LocStatus s;
    print_title("添加建筑物");

    printf("  请输入建筑物名称: ");
    scanf("%47s", name);
    printf("  请输入人流量权重(正整数): ");
    if (scanf("%d", &weight) != 1 || weight <= 0) {
        print_err("权重无效"); return;
    }
    printf("  请选择类型 (0-教学楼 1-宿舍楼 2-食堂 3-其他): ");
    if (scanf("%d", &type) != 1 || type < 0 || type >= LOC_TYPE_COUNT) {
        print_err("类型无效"); return;
    }

    s = graph_add_location(g, name, weight, (LocType)type, &id);
    if (s == LOC_OK) {
        print_ok("添加成功");
        graph_save(g, DATA_FILE);
    } else {
        print_err(get_status_msg(s));
    }
}

static void menu_add_road(Graph *g) {
    int from, to, dist;
    LocStatus s;
    show_locations(g);
    print_title("添加道路");
    printf("  起点编号: ");
    if (scanf("%d", &from) != 1 || from < 1 || from > g->loc_count) {
        print_err("起点编号无效"); return;
    }
    printf("  终点编号: ");
    if (scanf("%d", &to) != 1 || to < 1 || to > g->loc_count || from == to) {
        print_err("终点编号无效"); return;
    }
    printf("  距离(米): ");
    if (scanf("%d", &dist) != 1 || dist <= 0) {
        print_err("距离无效"); return;
    }

    s = graph_add_road(g, from - 1, to - 1, dist);
    if (s == LOC_OK) {
        print_ok("道路添加成功");
        graph_save(g, DATA_FILE);
    } else {
        print_err(get_status_msg(s));
    }
}

static void show_main_menu(Graph *g) {
    int choice;
    do {
        clear_screen();
        printf(ANSI_BOLD ANSI_MAGENTA "\n");
        printf("  ╔══════════════════════════════════════════════════════╗\n");
        printf("  ║          学校超市选址系统 V2.0                       ║\n");
        printf("  ║       School Supermarket Location System             ║\n");
        printf("  ╚══════════════════════════════════════════════════════╝\n" ANSI_RESET);
        printf(ANSI_CYAN "  数据状态: %d 个建筑物, %d 条道路\n" ANSI_RESET,
               g->loc_count, g->road_count);
        printf("\n");
        printf(ANSI_BOLD "  [1]" ANSI_RESET " 查看建筑物列表\n");
        printf(ANSI_BOLD "  [2]" ANSI_RESET " 查看道路列表\n");
        printf(ANSI_BOLD "  [3]" ANSI_RESET " 添加建筑物\n");
        printf(ANSI_BOLD "  [4]" ANSI_RESET " 添加道路\n");
        printf(ANSI_BOLD "  [5]" ANSI_RESET " Floyd计算 + 距离矩阵\n");
        printf(ANSI_BOLD "  [6]" ANSI_RESET " 中位点选址（最小加权距离）\n");
        printf(ANSI_BOLD "  [7]" ANSI_RESET " 中心点选址（最小最大距离）\n");
        printf(ANSI_BOLD "  [8]" ANSI_RESET " 重新加载默认校园数据\n");
        printf(ANSI_BOLD "  [9]" ANSI_RESET " 保存数据到文件\n");
        printf(ANSI_BOLD "  [0]" ANSI_RESET " 退出系统\n");
        printf("\n" ANSI_BOLD "  请选择操作: " ANSI_RESET);

        if (scanf("%d", &choice) != 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) { }
            choice = -1;
        }

        switch (choice) {
            case 1: show_locations(g); pause_screen(); break;
            case 2: show_roads(g); pause_screen(); break;
            case 3: menu_add_location(g); pause_screen(); break;
            case 4: menu_add_road(g); pause_screen(); break;
            case 5: show_dist_matrix(g); pause_screen(); break;
            case 6: solve_and_show(g, LOC_ALG_MEDIAN); pause_screen(); break;
            case 7: solve_and_show(g, LOC_ALG_CENTER); pause_screen(); break;
            case 8:
                graph_free(g);
                graph_init(g);
                load_default_campus(g);
                graph_save(g, DATA_FILE);
                print_ok("默认校园数据已加载");
                pause_screen();
                break;
            case 9: {
                LocStatus s = graph_save(g, DATA_FILE);
                if (s == LOC_OK) print_ok("数据保存成功");
                else print_err(get_status_msg(s));
                pause_screen();
                break;
            }
            case 0:
                graph_save(g, DATA_FILE);
                printf(ANSI_GREEN "\n  感谢使用，再见！\n" ANSI_RESET);
                break;
            default:
                print_err("无效选择，请重试");
                pause_screen();
        }
    } while (choice != 0);
}

/*==========================================================================
 * 第十一部分：内置自测（39项）
 *========================================================================*/

typedef struct { int passed; int total; } TestResult;

static void test_report(TestResult *r, int cond, const char *msg) {
    r->total++;
    if (cond) {
        r->passed++;
        printf(ANSI_GREEN "    [PASS] %s\n" ANSI_RESET, msg);
    } else {
        printf(ANSI_RED "    [FAIL] %s\n" ANSI_RESET, msg);
    }
}

static int run_self_test(void) {
    TestResult tr = {0, 0};
    printf(ANSI_BOLD ANSI_CYAN "\n  === 学校超市选址系统 V2.0 自检程序 ===\n\n" ANSI_RESET);

    printf(ANSI_BOLD "  [1] 图初始化测试\n" ANSI_RESET);
    {
        Graph g;
        test_report(&tr, graph_init(&g) == LOC_OK, "graph_init 初始化成功");
        test_report(&tr, g.loc_count == 0, "初始建筑数量为0");
        test_report(&tr, g.loc_capacity == INIT_CAPACITY, "初始容量正确");
        graph_free(&g);
    }

    printf(ANSI_BOLD "\n  [2] 添加地点测试\n" ANSI_RESET);
    {
        Graph g; int id0, id1, id2;
        graph_init(&g);
        test_report(&tr, graph_add_location(&g, "A栋", 10, LOC_TYPE_CLASSROOM, &id0) == LOC_OK, "添加A栋成功");
        test_report(&tr, graph_add_location(&g, "B栋", 20, LOC_TYPE_DORM, &id1) == LOC_OK, "添加B栋成功");
        test_report(&tr, graph_add_location(&g, "C栋", 30, LOC_TYPE_CANTEEN, &id2) == LOC_OK, "添加C栋成功");
        test_report(&tr, g.loc_count == 3, "建筑数量为3");
        test_report(&tr, id0 == 0 && id1 == 1 && id2 == 2, "ID分配正确");
        graph_free(&g);
    }

    printf(ANSI_BOLD "\n  [3] 添加道路测试\n" ANSI_RESET);
    {
        Graph g;
        graph_init(&g);
        graph_add_location(&g, "A", 10, LOC_TYPE_OTHER, NULL);
        graph_add_location(&g, "B", 20, LOC_TYPE_OTHER, NULL);
        graph_add_location(&g, "C", 30, LOC_TYPE_OTHER, NULL);
        test_report(&tr, graph_add_road(&g, 0, 1, 100) == LOC_OK, "添加A-B道路(100m)");
        test_report(&tr, graph_add_road(&g, 1, 2, 200) == LOC_OK, "添加B-C道路(200m)");
        test_report(&tr, graph_add_road(&g, 0, 2, 500) == LOC_OK, "添加A-C道路(500m)");
        test_report(&tr, g.road_count == 3, "道路数量为3");
        graph_free(&g);
    }

    printf(ANSI_BOLD "\n  [4] Floyd最短路测试\n" ANSI_RESET);
    {
        Graph g;
        graph_init(&g);
        graph_add_location(&g, "A", 10, LOC_TYPE_OTHER, NULL);
        graph_add_location(&g, "B", 20, LOC_TYPE_OTHER, NULL);
        graph_add_location(&g, "C", 30, LOC_TYPE_OTHER, NULL);
        graph_add_road(&g, 0, 1, 100);
        graph_add_road(&g, 1, 2, 200);
        graph_add_road(&g, 0, 2, 500);
        test_report(&tr, floyd_compute(&g) == LOC_OK, "Floyd计算成功");
        test_report(&tr, g.dist[0][0] == 0, "节点到自身距离为0");
        test_report(&tr, g.dist[0][1] == 100, "A-B直接距离100");
        test_report(&tr, g.dist[0][2] == 300, "A-C最短路为100+200=300(非500)");
        test_report(&tr, g.dist[1][2] == 200, "B-C直接距离200");
        test_report(&tr, graph_is_connected(&g) == 1, "三节点完全连通");
        graph_free(&g);
    }

    printf(ANSI_BOLD "\n  [5] 中位点(加权距离)测试\n" ANSI_RESET);
    {
        Graph g; int mid_id; double mid_cost, costB;
        graph_init(&g);
        graph_add_location(&g, "A", 10, LOC_TYPE_OTHER, NULL);
        graph_add_location(&g, "B", 20, LOC_TYPE_OTHER, NULL);
        graph_add_location(&g, "C", 30, LOC_TYPE_OTHER, NULL);
        graph_add_road(&g, 0, 1, 100);
        graph_add_road(&g, 1, 2, 200);
        graph_add_road(&g, 0, 2, 500);
        floyd_compute(&g);
        test_report(&tr, find_median(&g, &mid_id, &mid_cost) == LOC_OK, "中位点计算成功");
        costB = (double)g.dist[1][0]*10 + (double)g.dist[1][2]*30;
        test_report(&tr, mid_id == 1, "B栋是中位点(位置中心+权重高)");
        test_report(&tr, mid_cost == costB, "加权距离总和计算正确");
        test_report(&tr, mid_cost > 0, "加权距离为正");
        graph_free(&g);
    }

    printf(ANSI_BOLD "\n  [6] 中心点(偏心距)测试\n" ANSI_RESET);
    {
        Graph g; int cen_id, cen_ecc;
        graph_init(&g);
        graph_add_location(&g, "A", 10, LOC_TYPE_OTHER, NULL);
        graph_add_location(&g, "B", 20, LOC_TYPE_OTHER, NULL);
        graph_add_location(&g, "C", 30, LOC_TYPE_OTHER, NULL);
        graph_add_road(&g, 0, 1, 100);
        graph_add_road(&g, 1, 2, 200);
        graph_add_road(&g, 0, 2, 500);
        floyd_compute(&g);
        test_report(&tr, find_center(&g, &cen_id, &cen_ecc) == LOC_OK, "中心点计算成功");
        test_report(&tr, cen_ecc == 200, "中心点偏心距为200");
        test_report(&tr, cen_id == 1, "B栋为中心点(偏心距最小)");
        graph_free(&g);
    }

    printf(ANSI_BOLD "\n  [7] 文件持久化测试\n" ANSI_RESET);
    {
        Graph g, g2;
        graph_init(&g);
        graph_add_location(&g, "X1", 50, LOC_TYPE_OTHER, NULL);
        graph_add_location(&g, "X2", 60, LOC_TYPE_OTHER, NULL);
        graph_add_location(&g, "X3", 70, LOC_TYPE_OTHER, NULL);
        graph_add_road(&g, 0, 1, 150);
        graph_add_road(&g, 1, 2, 250);
        graph_add_road(&g, 0, 2, 500);
        test_report(&tr, graph_save(&g, "_test_locv2.tmp") == LOC_OK, "保存到文件成功");
        graph_init(&g2);
        test_report(&tr, graph_load(&g2, "_test_locv2.tmp") == LOC_OK, "从文件加载成功");
        test_report(&tr, g2.loc_count == g.loc_count, "加载后建筑数量一致");
        test_report(&tr, g2.road_count == g.road_count, "加载后道路数量一致");
        floyd_compute(&g2);
        test_report(&tr, g2.dist[0][2] == 400, "加载后最短路计算正确(150+250=400)");
        graph_free(&g);
        graph_free(&g2);
        remove("_test_locv2.tmp");
    }

    printf(ANSI_BOLD "\n  [8] 默认校园数据测试\n" ANSI_RESET);
    {
        Graph g3;
        graph_init(&g3);
        load_default_campus(&g3);
        test_report(&tr, g3.loc_count == 12, "默认校园12个建筑");
        test_report(&tr, g3.road_count == 28, "默认校园28条道路");
        test_report(&tr, floyd_compute(&g3) == LOC_OK, "默认数据Floyd计算成功");
        test_report(&tr, graph_is_connected(&g3) == 1, "默认校园图连通");
        graph_free(&g3);
    }

    printf(ANSI_BOLD "\n  [9] 动态扩容测试\n" ANSI_RESET);
    {
        Graph g3;
        int i, large_cap_ok = 1;
        graph_init(&g3);
        load_default_campus(&g3);
        for (i = 0; i < 50; i++) {
            char buf[32];
            sprintf(buf, "测试楼%d", i);
            if (graph_add_location(&g3, buf, 50 + i, LOC_TYPE_OTHER, NULL) != LOC_OK) {
                large_cap_ok = 0; break;
            }
        }
        test_report(&tr, large_cap_ok, "动态添加50个地点(扩容)成功");
        test_report(&tr, g3.loc_count == 62, "扩容后地点总数62");
        test_report(&tr, g3.loc_capacity >= 62, "容量自动增长");
        graph_free(&g3);
    }

    printf(ANSI_BOLD "\n  [10] 边界条件测试\n" ANSI_RESET);
    {
        Graph g4;
        graph_init(&g4);
        graph_add_location(&g4, "X", 10, LOC_TYPE_OTHER, NULL);
        graph_add_location(&g4, "Y", 20, LOC_TYPE_OTHER, NULL);
        floyd_compute(&g4);
        test_report(&tr, graph_is_connected(&g4) == 0, "无边双节点图检测为不连通");
        graph_free(&g4);
    }

    printf("\n");
    print_line();
    printf(ANSI_BOLD "  自检结果: %d / %d 通过\n" ANSI_RESET, tr.passed, tr.total);
    if (tr.passed == tr.total) {
        printf(ANSI_BOLD ANSI_GREEN "  ✓ 全部测试通过！\n" ANSI_RESET);
        return 0;
    } else {
        printf(ANSI_BOLD ANSI_RED "  ✗ %d 项测试失败！\n" ANSI_RESET, tr.total - tr.passed);
        return 1;
    }
}

/*==========================================================================
 * 第十二部分：帮助信息与main函数
 *========================================================================*/

static void print_help(const char *prog) {
    printf(ANSI_BOLD ANSI_CYAN "\n  学校超市选址系统 V2.0\n" ANSI_RESET);
    printf(ANSI_CYAN "  ─────────────────────────────\n" ANSI_RESET);
    printf("  用法: %s [选项]\n\n", prog);
    printf(ANSI_BOLD "  选项:\n" ANSI_RESET);
    printf("    -h, --help      显示帮助信息\n");
    printf("    -t, --test      运行内置自检程序\n");
    printf("    (无参数)         启动交互式菜单\n\n");
    printf(ANSI_BOLD "  选址算法:\n" ANSI_RESET);
    printf("    · 中位点 (Median): 最小化加权距离总和\n");
    printf("    · 中心点 (Center): 最小化最大距离(偏心距)\n\n");
    printf(ANSI_BOLD "  数据文件: %s (LOCV2格式)\n" ANSI_RESET, DATA_FILE);
}

int main(int argc, char *argv[]) {
    Graph g;
    LocStatus load_status;
    console_init();
    srand((unsigned int)time(NULL));

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        }
        if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0) {
            return run_self_test();
        }
        print_err("未知参数，使用 -h 查看帮助");
        return 1;
    }

    if (graph_init(&g) != LOC_OK) {
        print_err("内存初始化失败");
        return 1;
    }

    load_status = graph_load(&g, DATA_FILE);
    if (load_status != LOC_OK) {
        print_info("未找到有效数据文件，加载默认校园数据...");
        graph_free(&g);
        graph_init(&g);
        load_default_campus(&g);
        graph_save(&g, DATA_FILE);
    }

    show_main_menu(&g);
    graph_free(&g);
    return 0;
}
