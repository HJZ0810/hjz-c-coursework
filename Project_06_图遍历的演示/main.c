/* ﻿ */
/* ========================================
 * 图遍历演示系统 V2.0 (Graph Traversal V2.0)
 * C99标准实现 | 动态内存管理 | 彩色终端输出
 * 核心功能：邻接表存储 | DFS深度优先 | BFS广度优先 | 生成树可视化 | 文件持久化
 * 编译器：D:\bin\gcc.exe -std=c99 -Wall -Wextra -O2
 * ======================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#ifdef _WIN32
#include <windows.h>
#endif

/* ======================== ANSI彩色输出 ======================== */

#define C_RESET   "\x1b[0m"
#define C_RED     "\x1b[31m"
#define C_GREEN   "\x1b[32m"
#define C_YELLOW  "\x1b[33m"
#define C_BLUE    "\x1b[34m"
#define C_MAGENTA "\x1b[35m"
#define C_CYAN    "\x1b[36m"
#define C_WHITE   "\x1b[37m"
#define C_BOLD    "\x1b[1m"
#define C_DIM     "\x1b[2m"

static void cPrint(const char *color, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    printf("%s", color);
    vprintf(fmt, ap);
    printf("%s", C_RESET);
    va_end(ap);
}

static void printOk(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    printf("  %s[OK]%s ", C_GREEN, C_RESET);
    vprintf(fmt, ap); printf("\n");
    va_end(ap);
}

static void printErr(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    printf("  %s[ERR]%s ", C_RED, C_RESET);
    vprintf(fmt, ap); printf("\n");
    va_end(ap);
}

static void printTitle(const char *title) {
    int len = (int)strlen(title) + 8;
    printf("\n  ");
    for (int i = 0; i < len; i++) printf("=");
    printf("\n  %s>> %s <<%s\n", C_CYAN, title, C_RESET);
    printf("  ");
    for (int i = 0; i < len; i++) printf("=");
    printf("\n");
}

static void printDivider(void) {
    printf("  %s", C_DIM);
    for (int i = 0; i < 58; i++) printf("-");
    printf("%s\n", C_RESET);
}

/* ======================== 错误码枚举 ======================== */

typedef enum {
    GRAPH_OK = 0,
    GRAPH_ERR_OPEN_FILE,
    GRAPH_ERR_READ_FILE,
    GRAPH_ERR_WRITE_FILE,
    GRAPH_ERR_MEMORY,
    GRAPH_ERR_MAGIC,
    GRAPH_ERR_VERSION,
    GRAPH_ERR_EMPTY,
    GRAPH_ERR_INVALID_PARAM,
    GRAPH_ERR_VERTEX_EXISTS,
    GRAPH_ERR_VERTEX_NOT_FOUND,
    GRAPH_ERR_INVALID_VEX_IDX,
    GRAPH_ERR_STACK_FULL,
    GRAPH_ERR_STACK_EMPTY,
    GRAPH_ERR_QUEUE_FULL,
    GRAPH_ERR_QUEUE_EMPTY,
    GRAPH_ERR_CORRUPT,
    GRAPH_ERR_CANCEL,
    GRAPH_ERR_TEST_FAIL,
} GraphStatus;

static const char* graphStatusStr(GraphStatus s) {
    static const char *msgs[] = {
        "成功", "无法打开文件", "文件读取失败", "文件写入失败", "内存分配失败",
        "文件魔数错误", "文件版本不兼容", "图为空", "参数无效",
        "顶点已存在", "顶点不存在", "顶点索引无效",
        "栈满", "栈空", "队满", "队空",
        "数据损坏", "操作取消", "测试失败"
    };
    if (s >= 0 && s < (int)(sizeof(msgs)/sizeof(msgs[0]))) return msgs[s];
    return "未知错误";
}

/* ======================== 数据结构定义 ======================== */

#define MAX_VERTEX_NAME 31
#define GRAPH_MAGIC "GRAPHV2"
#define GRAPH_MAGIC_LEN 7
#define GRAPH_VERSION 2
#define INITIAL_CAPACITY 16
#define INITIAL_STACK_CAP 16
#define INITIAL_QUEUE_CAP 16

typedef struct ArcNode {
    int adjvex;
    struct ArcNode *nextarc;
    int weight;
} ArcNode;

typedef struct VNode {
    char name[MAX_VERTEX_NAME + 1];
    ArcNode *firstarc;
} VNode;

typedef struct {
    VNode *vertices;
    int vexNum;
    int arcNum;
    int capacity;
} ALGraph;

typedef struct {
    int *parent;
    int *child;
    int *sibling;
    int *order;
    int orderLen;
    int count;
    int capacity;
} SpanTree;

typedef struct {
    int *data;
    int top;
    int capacity;
} Stack;

typedef struct {
    int *data;
    int front;
    int rear;
    int size;
    int capacity;
} Queue;

typedef struct {
    char magic[GRAPH_MAGIC_LEN + 1];
    int version;
    int vexNum;
    int arcNum;
} GraphHeader;

#define DEFAULT_GRAPH_FILE "graph_v2.bin"

/* ======================== 栈操作（动态扩容） ======================== */

static GraphStatus stackInit(Stack *s, int initialCap) {
    if (!s || initialCap <= 0) return GRAPH_ERR_INVALID_PARAM;
    s->data = (int*)malloc(sizeof(int) * initialCap);
    if (!s->data) return GRAPH_ERR_MEMORY;
    s->top = 0; s->capacity = initialCap;
    return GRAPH_OK;
}

static void stackDestroy(Stack *s) {
    if (s) { free(s->data); s->data = NULL; s->top = s->capacity = 0; }
}

static int stackEmpty(Stack *s) { return s->top == 0; }

static int stackFull(Stack *s) { return s->top >= s->capacity; }

static GraphStatus stackPush(Stack *s, int val) {
    if (!s) return GRAPH_ERR_INVALID_PARAM;
    if (stackFull(s)) {
        int newCap = s->capacity * 2;
        int *newData = (int*)realloc(s->data, sizeof(int) * newCap);
        if (!newData) return GRAPH_ERR_MEMORY;
        s->data = newData; s->capacity = newCap;
    }
    s->data[s->top++] = val;
    return GRAPH_OK;
}

static GraphStatus stackPop(Stack *s, int *out) {
    if (!s || !out) return GRAPH_ERR_INVALID_PARAM;
    if (stackEmpty(s)) return GRAPH_ERR_STACK_EMPTY;
    *out = s->data[--s->top];
    return GRAPH_OK;
}

static GraphStatus stackTop(Stack *s, int *out) {
    if (!s || !out) return GRAPH_ERR_INVALID_PARAM;
    if (stackEmpty(s)) return GRAPH_ERR_STACK_EMPTY;
    *out = s->data[s->top - 1];
    return GRAPH_OK;
}

/* ======================== 队列操作（动态扩容环形队列） ======================== */

static GraphStatus queueInit(Queue *q, int initialCap) {
    if (!q || initialCap <= 0) return GRAPH_ERR_INVALID_PARAM;
    q->data = (int*)malloc(sizeof(int) * initialCap);
    if (!q->data) return GRAPH_ERR_MEMORY;
    q->front = q->rear = q->size = 0; q->capacity = initialCap;
    return GRAPH_OK;
}

static void queueDestroy(Queue *q) {
    if (q) { free(q->data); q->data = NULL; q->front = q->rear = q->size = q->capacity = 0; }
}

static int queueEmpty(Queue *q) { return q->size == 0; }

static int queueFull(Queue *q) { return q->size >= q->capacity; }

static GraphStatus queueEnqueue(Queue *q, int val) {
    if (!q) return GRAPH_ERR_INVALID_PARAM;
    if (queueFull(q)) {
        int newCap = q->capacity * 2;
        int *newData = (int*)malloc(sizeof(int) * newCap);
        if (!newData) return GRAPH_ERR_MEMORY;
        for (int i = 0; i < q->size; i++)
            newData[i] = q->data[(q->front + i) % q->capacity];
        free(q->data);
        q->data = newData; q->front = 0; q->rear = q->size; q->capacity = newCap;
    }
    q->data[q->rear] = val; q->rear = (q->rear + 1) % q->capacity; q->size++;
    return GRAPH_OK;
}

static GraphStatus queueDequeue(Queue *q, int *out) {
    if (!q || !out) return GRAPH_ERR_INVALID_PARAM;
    if (queueEmpty(q)) return GRAPH_ERR_QUEUE_EMPTY;
    *out = q->data[q->front]; q->front = (q->front + 1) % q->capacity; q->size--;
    return GRAPH_OK;
}

/* ======================== 生成树操作 ======================== */

static GraphStatus treeInit(SpanTree *t, int capacity) {
    if (!t || capacity <= 0) return GRAPH_ERR_INVALID_PARAM;
    t->parent = (int*)malloc(sizeof(int) * capacity);
    t->child = (int*)malloc(sizeof(int) * capacity);
    t->sibling = (int*)malloc(sizeof(int) * capacity);
    t->order = (int*)malloc(sizeof(int) * capacity);
    if (!t->parent || !t->child || !t->sibling || !t->order) {
        free(t->parent); free(t->child); free(t->sibling); free(t->order);
        return GRAPH_ERR_MEMORY;
    }
    for (int i = 0; i < capacity; i++) {
        t->parent[i] = t->child[i] = t->sibling[i] = -1;
    }
    t->orderLen = 0; t->count = 0; t->capacity = capacity;
    return GRAPH_OK;
}

static void treeDestroy(SpanTree *t) {
    if (t) {
        free(t->parent); free(t->child); free(t->sibling); free(t->order);
        t->parent = t->child = t->sibling = t->order = NULL;
        t->orderLen = t->count = t->capacity = 0;
    }
}

static void treeSetRoot(SpanTree *t, int root) {
    if (t && root >= 0 && root < t->capacity) { t->parent[root] = -1; t->count++; }
}

static void treeAddEdge(SpanTree *t, int u, int v) {
    if (!t || u < 0 || v < 0 || u >= t->capacity || v >= t->capacity) return;
    t->parent[v] = u;
    if (t->child[u] == -1) { t->child[u] = v; }
    else {
        int cur = t->child[u];
        while (t->sibling[cur] != -1) cur = t->sibling[cur];
        t->sibling[cur] = v;
    }
    t->count++;
}

static void treeAddOrder(SpanTree *t, int v) {
    if (t && t->orderLen < t->capacity) t->order[t->orderLen++] = v;
}

/* ======================== 图基本操作（动态邻接表） ======================== */

static GraphStatus graphInit(ALGraph *g, int initialCapacity) {
    if (!g || initialCapacity <= 0) return GRAPH_ERR_INVALID_PARAM;
    g->vertices = (VNode*)calloc(initialCapacity, sizeof(VNode));
    if (!g->vertices) return GRAPH_ERR_MEMORY;
    for (int i = 0; i < initialCapacity; i++) g->vertices[i].firstarc = NULL;
    g->vexNum = 0; g->arcNum = 0; g->capacity = initialCapacity;
    return GRAPH_OK;
}

static void graphDestroy(ALGraph *g) {
    if (!g || !g->vertices) return;
    for (int i = 0; i < g->vexNum; i++) {
        ArcNode *p = g->vertices[i].firstarc;
        while (p) { ArcNode *q = p->nextarc; free(p); p = q; }
        g->vertices[i].firstarc = NULL;
    }
    free(g->vertices); g->vertices = NULL; g->vexNum = g->arcNum = g->capacity = 0;
}

static int graphLocateVex(ALGraph *g, const char *name) {
    if (!g || !name) return -1;
    for (int i = 0; i < g->vexNum; i++)
        if (strcmp(g->vertices[i].name, name) == 0) return i;
    return -1;
}

static GraphStatus graphAddVex(ALGraph *g, const char *name, int *outIdx) {
    if (!g || !name || !*name) return GRAPH_ERR_INVALID_PARAM;
    if (graphLocateVex(g, name) >= 0) return GRAPH_ERR_VERTEX_EXISTS;
    if (g->vexNum >= g->capacity) {
        int newCap = g->capacity * 2;
        VNode *newV = (VNode*)realloc(g->vertices, sizeof(VNode) * newCap);
        if (!newV) return GRAPH_ERR_MEMORY;
        for (int i = g->capacity; i < newCap; i++) { newV[i].name[0] = '\0'; newV[i].firstarc = NULL; }
        g->vertices = newV; g->capacity = newCap;
    }
    int idx = g->vexNum;
    strncpy(g->vertices[idx].name, name, MAX_VERTEX_NAME);
    g->vertices[idx].name[MAX_VERTEX_NAME] = '\0';
    g->vertices[idx].firstarc = NULL;
    g->vexNum++;
    if (outIdx) *outIdx = idx;
    return GRAPH_OK;
}

static GraphStatus graphAddArc(ALGraph *g, int vi, int vj, int weight) {
    if (!g) return GRAPH_ERR_INVALID_PARAM;
    if (vi < 0 || vi >= g->vexNum || vj < 0 || vj >= g->vexNum) return GRAPH_ERR_INVALID_VEX_IDX;
    ArcNode *p = g->vertices[vi].firstarc;
    while (p) { if (p->adjvex == vj) return GRAPH_OK; p = p->nextarc; }
    ArcNode *newArc = (ArcNode*)malloc(sizeof(ArcNode));
    if (!newArc) return GRAPH_ERR_MEMORY;
    newArc->adjvex = vj; newArc->weight = weight;
    newArc->nextarc = g->vertices[vi].firstarc;
    g->vertices[vi].firstarc = newArc;
    g->arcNum++;
    return GRAPH_OK;
}

static GraphStatus graphAddEdge(ALGraph *g, int vi, int vj, int weight) {
    GraphStatus s;
    s = graphAddArc(g, vi, vj, weight); if (s != GRAPH_OK) return s;
    s = graphAddArc(g, vj, vi, weight); return s;
}

static void graphPrint(ALGraph *g) {
    if (!g || g->vexNum == 0) { printErr("图为空，无法显示"); return; }
    printTitle("邻接表表示");
    for (int i = 0; i < g->vexNum; i++) {
        cPrint(C_BOLD C_CYAN, "  [%d] %s", i, g->vertices[i].name);
        ArcNode *p = g->vertices[i].firstarc;
        int cnt = 0;
        while (p) {
            printf(" -> %s(%d)", g->vertices[p->adjvex].name, p->adjvex);
            if (p->weight != 1) cPrint(C_YELLOW, "[w=%d]", p->weight);
            p = p->nextarc; cnt++;
        }
        printf("  %s(度:%d)%s\n", C_DIM, cnt, C_RESET);
    }
    printf("\n  %s共 %d 个顶点, %d 条边%s\n", C_GREEN, g->vexNum, g->arcNum, C_RESET);
}

/* ======================== DFS 深度优先遍历（非递归实现） ======================== */

static GraphStatus dfsTraverse(ALGraph *g, int start, SpanTree *treeOut, int *orderOut, int *orderLen) {
    if (!g || g->vexNum == 0) return GRAPH_ERR_EMPTY;
    if (start < 0 || start >= g->vexNum) return GRAPH_ERR_INVALID_VEX_IDX;

    char *visited = (char*)calloc(g->vexNum, sizeof(char));
    if (!visited) return GRAPH_ERR_MEMORY;

    Stack s;
    GraphStatus st = stackInit(&s, INITIAL_STACK_CAP);
    if (st != GRAPH_OK) { free(visited); return st; }

    if (treeOut) { treeInit(treeOut, g->vexNum); treeSetRoot(treeOut, start); }
    int orderCount = 0;

    int *iterPos = (int*)calloc(g->vexNum, sizeof(int));
    if (!iterPos) { free(visited); stackDestroy(&s); return GRAPH_ERR_MEMORY; }

    stackPush(&s, start);
    visited[start] = 1;
    if (orderOut) orderOut[orderCount] = start;
    if (treeOut) treeAddOrder(treeOut, start);
    orderCount++;
    iterPos[start] = 0;

    printf("  DFS遍历路径: ");
    cPrint(C_BOLD C_GREEN, "%s", g->vertices[start].name);

    while (!stackEmpty(&s)) {
        int u; stackTop(&s, &u);
        ArcNode *arc = g->vertices[u].firstarc;
        int step = 0; int found = 0;
        while (arc && step < iterPos[u]) { arc = arc->nextarc; step++; }

        while (arc) {
            int v = arc->adjvex;
            if (!visited[v]) {
                visited[v] = 1;
                stackPush(&s, v);
                iterPos[u]++;
                if (treeOut) treeAddEdge(treeOut, u, v);
                if (orderOut) orderOut[orderCount] = v;
                if (treeOut) treeAddOrder(treeOut, v);
                orderCount++;
                printf(" ->%s", C_CYAN); cPrint(C_BOLD C_GREEN, "%s", g->vertices[v].name); printf("%s", C_RESET);
                iterPos[v] = 0; found = 1;
                break;
            }
            arc = arc->nextarc; iterPos[u]++;
        }

        if (!found) { int tmp; stackPop(&s, &tmp); }
    }

    for (int vi = 0; vi < g->vexNum; vi++) {
        if (!visited[vi]) {
            stackPush(&s, vi); visited[vi] = 1;
            if (orderOut) orderOut[orderCount] = vi;
            if (treeOut) treeAddOrder(treeOut, vi);
            orderCount++;
            iterPos[vi] = 0;
            printf(" |%s", C_MAGENTA); cPrint(C_BOLD C_GREEN, "%s", g->vertices[vi].name); printf("%s", C_RESET);
            while (!stackEmpty(&s)) {
                int u; stackTop(&s, &u);
                ArcNode *arc = g->vertices[u].firstarc;
                int step = 0; int found = 0;
                while (arc && step < iterPos[u]) { arc = arc->nextarc; step++; }
                while (arc) {
                    int v = arc->adjvex;
                    if (!visited[v]) {
                        visited[v] = 1; stackPush(&s, v); iterPos[u]++;
                        if (treeOut) treeAddEdge(treeOut, u, v);
                        if (orderOut) orderOut[orderCount] = v;
                        if (treeOut) treeAddOrder(treeOut, v);
                        orderCount++;
                        printf(" ->%s", C_CYAN); cPrint(C_BOLD C_GREEN, "%s", g->vertices[v].name); printf("%s", C_RESET);
                        iterPos[v] = 0; found = 1; break;
                    }
                    arc = arc->nextarc; iterPos[u]++;
                }
                if (!found) { int tmp; stackPop(&s, &tmp); }
            }
        }
    }

    printf("\n");
    if (orderLen) *orderLen = orderCount;
    free(iterPos); free(visited); stackDestroy(&s);
    return GRAPH_OK;
}

/* ======================== BFS 广度优先遍历 ======================== */

static GraphStatus bfsTraverse(ALGraph *g, int start, SpanTree *treeOut, int *orderOut, int *orderLen) {
    if (!g || g->vexNum == 0) return GRAPH_ERR_EMPTY;
    if (start < 0 || start >= g->vexNum) return GRAPH_ERR_INVALID_VEX_IDX;

    char *visited = (char*)calloc(g->vexNum, sizeof(char));
    if (!visited) return GRAPH_ERR_MEMORY;
    Queue q;
    GraphStatus st = queueInit(&q, INITIAL_QUEUE_CAP);
    if (st != GRAPH_OK) { free(visited); return st; }

    if (treeOut) treeInit(treeOut, g->vexNum);
    int orderCount = 0;
    int *level = (int*)calloc(g->vexNum, sizeof(int));
    if (!level) { free(visited); queueDestroy(&q); return GRAPH_ERR_MEMORY; }

    queueEnqueue(&q, start); visited[start] = 1; level[start] = 0;
    if (treeOut) treeSetRoot(treeOut, start);
    if (orderOut) orderOut[orderCount] = start;
    if (treeOut) treeAddOrder(treeOut, start);
    orderCount++;

    printf("  BFS遍历路径: ");
    cPrint(C_BOLD C_GREEN, "%s", g->vertices[start].name);
    printf("%s(L0)%s", C_DIM, C_RESET);

    int lastLevel = 0;
    while (!queueEmpty(&q)) {
        int u; queueDequeue(&q, &u);
        if (level[u] > lastLevel) { lastLevel = level[u]; }
        for (ArcNode *p = g->vertices[u].firstarc; p; p = p->nextarc) {
            int v = p->adjvex;
            if (!visited[v]) {
                visited[v] = 1; level[v] = level[u] + 1;
                queueEnqueue(&q, v);
                if (treeOut) treeAddEdge(treeOut, u, v);
                if (orderOut) orderOut[orderCount] = v;
                if (treeOut) treeAddOrder(treeOut, v);
                orderCount++;
                if (level[v] > lastLevel) { cPrint(C_YELLOW, " |L%d", level[v]); lastLevel = level[v]; }
                printf(" ->%s", C_CYAN); cPrint(C_BOLD C_GREEN, "%s", g->vertices[v].name); printf("%s(L%d)%s", C_DIM, level[v], C_RESET);
            }
        }
    }

    for (int vi = 0; vi < g->vexNum; vi++) {
        if (!visited[vi]) {
            queueEnqueue(&q, vi); visited[vi] = 1; level[vi] = 0;
            if (orderOut) orderOut[orderCount] = vi;
            if (treeOut) treeAddOrder(treeOut, vi);
            orderCount++;
            cPrint(C_MAGENTA, " |CC:"); cPrint(C_BOLD C_GREEN, "%s", g->vertices[vi].name); printf("%s(L0)%s", C_DIM, C_RESET);
            while (!queueEmpty(&q)) {
                int u; queueDequeue(&q, &u);
                for (ArcNode *p = g->vertices[u].firstarc; p; p = p->nextarc) {
                    int v = p->adjvex;
                    if (!visited[v]) {
                        visited[v] = 1; level[v] = level[u] + 1; queueEnqueue(&q, v);
                        if (treeOut) treeAddEdge(treeOut, u, v);
                        if (orderOut) orderOut[orderCount] = v;
                        if (treeOut) treeAddOrder(treeOut, v);
                        orderCount++;
                        printf(" ->%s", C_CYAN); cPrint(C_BOLD C_GREEN, "%s", g->vertices[v].name); printf("%s(L%d)%s", C_DIM, level[v], C_RESET);
                    }
                }
            }
        }
    }

    printf("\n");
    if (orderLen) *orderLen = orderCount;
    free(level); free(visited); queueDestroy(&q);
    return GRAPH_OK;
}

/* ======================== 生成树树形打印 ======================== */

static int treeGetChildren(SpanTree *t, int parent, int *children, int maxChildren) {
    int count = 0;
    int cur = t->child[parent];
    while (cur != -1 && count < maxChildren) {
        children[count++] = cur;
        cur = t->sibling[cur];
    }
    return count;
}

static void treePrintHelper(SpanTree *t, ALGraph *g, int node, int depth, int *isLastArr) {
    static const char *T_RIGHT  = "+-- ";
    static const char *T_BRANCH = "+-- ";
    static const char *T_VERT   = "|   ";
    static const char *T_SPACE  = "    ";
    for (int i = 0; i < depth; i++) {
        if (i == depth - 1) {
            printf("%s", isLastArr[i] ? T_RIGHT : T_BRANCH);
        } else {
            printf("%s", isLastArr[i] ? T_SPACE : T_VERT);
        }
    }
    cPrint(C_CYAN, "%s(%d)\n", g->vertices[node].name, node);

    int children[256];
    int childCount = treeGetChildren(t, node, children, 256);

    for (int i = 0; i < childCount; i++) {
        int newIsLast[256];
        for (int j = 0; j < depth; j++) newIsLast[j] = isLastArr[j];
        newIsLast[depth] = (i == childCount - 1);
        treePrintHelper(t, g, children[i], depth + 1, newIsLast);
    }
}

static void treePrint(SpanTree *t, ALGraph *g, int root, const char *title) {
    printTitle(title);
    printf("  "); cPrint(C_BOLD C_GREEN, "%s(%d)  [ROOT]\n", g->vertices[root].name, root);
    int arr[1] = {1};
    int children[256];
    int cc = treeGetChildren(t, root, children, 256);
    for (int i = 0; i < cc; i++) {
        arr[0] = (i == cc - 1);
        treePrintHelper(t, g, children[i], 1, arr);
    }
    printf("\n");
}

/* ======================== 文件持久化（带GRAPHV2魔数） ======================== */

static GraphStatus graphSaveToFile(ALGraph *g, const char *filename) {
    if (!g || g->vexNum == 0) return GRAPH_ERR_EMPTY;
    FILE *fp = fopen(filename, "wb");
    if (!fp) return GRAPH_ERR_OPEN_FILE;

    GraphHeader hdr;
    memcpy(hdr.magic, GRAPH_MAGIC, GRAPH_MAGIC_LEN);
    hdr.magic[GRAPH_MAGIC_LEN] = '\0';
    hdr.version = GRAPH_VERSION;
    hdr.vexNum = g->vexNum;
    hdr.arcNum = g->arcNum;

    if (fwrite(&hdr, sizeof(hdr), 1, fp) != 1) { fclose(fp); return GRAPH_ERR_WRITE_FILE; }

    for (int i = 0; i < g->vexNum; i++) {
        char nameBuf[MAX_VERTEX_NAME + 1] = {0};
        memcpy(nameBuf, g->vertices[i].name, MAX_VERTEX_NAME);
        nameBuf[MAX_VERTEX_NAME] = '\0';
        if (fwrite(nameBuf, 1, MAX_VERTEX_NAME + 1, fp) != MAX_VERTEX_NAME + 1) {
            fclose(fp); return GRAPH_ERR_WRITE_FILE;
        }
    }

    for (int i = 0; i < g->vexNum; i++) {
        int edgeCount = 0;
        for (ArcNode *p = g->vertices[i].firstarc; p; p = p->nextarc) edgeCount++;
        if (fwrite(&edgeCount, sizeof(int), 1, fp) != 1) { fclose(fp); return GRAPH_ERR_WRITE_FILE; }
        for (ArcNode *p = g->vertices[i].firstarc; p; p = p->nextarc) {
            if (fwrite(&p->adjvex, sizeof(int), 1, fp) != 1) { fclose(fp); return GRAPH_ERR_WRITE_FILE; }
            if (fwrite(&p->weight, sizeof(int), 1, fp) != 1) { fclose(fp); return GRAPH_ERR_WRITE_FILE; }
        }
    }

    fclose(fp);
    return GRAPH_OK;
}

static GraphStatus graphLoadFromFile(ALGraph *g, const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) return GRAPH_ERR_OPEN_FILE;

    GraphHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) { fclose(fp); return GRAPH_ERR_READ_FILE; }

    if (memcmp(hdr.magic, GRAPH_MAGIC, GRAPH_MAGIC_LEN) != 0) { fclose(fp); return GRAPH_ERR_MAGIC; }
    if (hdr.version != GRAPH_VERSION) { fclose(fp); return GRAPH_ERR_VERSION; }
    if (hdr.vexNum <= 0 || hdr.vexNum > 100000) { fclose(fp); return GRAPH_ERR_CORRUPT; }

    graphDestroy(g);
    GraphStatus st = graphInit(g, hdr.vexNum > INITIAL_CAPACITY ? hdr.vexNum : INITIAL_CAPACITY);
    if (st != GRAPH_OK) { fclose(fp); return st; }

    for (int i = 0; i < hdr.vexNum; i++) {
        char nameBuf[MAX_VERTEX_NAME + 1] = {0};
        if (fread(nameBuf, 1, MAX_VERTEX_NAME + 1, fp) != MAX_VERTEX_NAME + 1) {
            fclose(fp); graphDestroy(g); return GRAPH_ERR_READ_FILE;
        }
        int outIdx;
        st = graphAddVex(g, nameBuf, &outIdx);
        if (st != GRAPH_OK) { fclose(fp); graphDestroy(g); return st; }
    }

    for (int i = 0; i < hdr.vexNum; i++) {
        int edgeCount;
        if (fread(&edgeCount, sizeof(int), 1, fp) != 1) { fclose(fp); graphDestroy(g); return GRAPH_ERR_READ_FILE; }
        for (int j = 0; j < edgeCount; j++) {
            int adj, w;
            if (fread(&adj, sizeof(int), 1, fp) != 1) { fclose(fp); graphDestroy(g); return GRAPH_ERR_READ_FILE; }
            if (fread(&w, sizeof(int), 1, fp) != 1) { fclose(fp); graphDestroy(g); return GRAPH_ERR_READ_FILE; }
            ArcNode *newArc = (ArcNode*)malloc(sizeof(ArcNode));
            if (!newArc) { fclose(fp); graphDestroy(g); return GRAPH_ERR_MEMORY; }
            newArc->adjvex = adj; newArc->weight = w;
            newArc->nextarc = g->vertices[i].firstarc;
            g->vertices[i].firstarc = newArc;
        }
    }
    g->arcNum = hdr.arcNum;

    fclose(fp);
    return GRAPH_OK;
}

/* ======================== 输入工具函数 ======================== */

static int readInt(const char *prompt, int min, int max) {
    char buf[64]; int val;
    while (1) {
        printf("%s", prompt); fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) return -1;
        if (sscanf(buf, "%d", &val) == 1 && val >= min && val <= max) return val;
        cPrint(C_YELLOW, "  请输入 %d~%d 之间的整数\n", min, max);
    }
}

static void readString(const char *prompt, char *buf, int bufSize) {
    printf("%s", prompt); fflush(stdout);
    if (!fgets(buf, bufSize, stdin)) { buf[0] = '\0'; return; }
    char *nl = strchr(buf, '\n'); if (nl) *nl = '\0';
}

static int fileExists(const char *path) {
    FILE *fp = fopen(path, "r");
    if (fp) { fclose(fp); return 1; }
    return 0;
}

/* ======================== 创建示例图（8个城市） ======================== */

static GraphStatus graphCreateTest(ALGraph *g) {
    graphDestroy(g);
    graphInit(g, INITIAL_CAPACITY);
    const char *cities[] = {"北京", "上海", "广州", "深圳", "成都", "武汉", "西安", "南京"};
    int edges[][3] = {
        {0,1,1318},{0,5,1229},{0,6,1085},{1,3,1480},{1,7,301},
        {2,3,137},{2,4,1660},{3,4,1800},{4,5,1180},{5,7,520},{6,4,720},{6,5,740}
    };
    for (int i = 0; i < 8; i++) {
        GraphStatus st = graphAddVex(g, cities[i], NULL);
        if (st != GRAPH_OK) return st;
    }
    for (int i = 0; i < (int)(sizeof(edges)/sizeof(edges[0])); i++) {
        GraphStatus st = graphAddEdge(g, edges[i][0], edges[i][1], edges[i][2]);
        if (st != GRAPH_OK) return st;
    }
    return GRAPH_OK;
}

/* ======================== 内置自测用例 ======================== */

static int runTests(void) {
    int passed = 0, failed = 0;
    const int TOTAL = 14;
    printTitle("内置自测 - Self Test V2.0");
    printf("  共 %d 个测试用例\n\n", TOTAL);

#define TEST(name) do { printf("  [T%02d] %-45s ", passed+failed+1, name); } while(0)
#define PASS() do { printOk("PASS"); passed++; } while(0)
#define FAIL(msg) do { printErr("FAIL: %s", msg); failed++; } while(0)
#define CHECK(cond, msg) do { if(cond){PASS();}else{FAIL(msg);} } while(0)

    /* T01 */
    TEST("初始化空图");
    ALGraph g;
    GraphStatus st = graphInit(&g, INITIAL_CAPACITY);
    CHECK(st == GRAPH_OK && g.vexNum == 0 && g.arcNum == 0, "初始化失败");

    /* T02 */
    TEST("添加顶点A,B,C,D");
    int ia, ib, ic, id;
    st = graphAddVex(&g, "A", &ia) == GRAPH_OK;
    st &= graphAddVex(&g, "B", &ib) == GRAPH_OK;
    st &= graphAddVex(&g, "C", &ic) == GRAPH_OK;
    st &= graphAddVex(&g, "D", &id) == GRAPH_OK;
    CHECK(st && g.vexNum == 4, "添加顶点失败");

    /* T03 */
    TEST("重复添加顶点A应失败");
    (void)ia; (void)ib; (void)ic; (void)id;
    st = graphAddVex(&g, "A", NULL);
    CHECK(st == GRAPH_ERR_VERTEX_EXISTS, "重复检查失效");

    /* T04 */
    TEST("添加无向边A-B,B-C,C-D,D-A");
    st = graphAddEdge(&g, 0, 1, 1);
    st &= graphAddEdge(&g, 1, 2, 1);
    st &= graphAddEdge(&g, 2, 3, 1);
    st &= graphAddEdge(&g, 3, 0, 1);
    CHECK(st == GRAPH_OK && g.arcNum == 8, "边添加失败");

    /* T05 */
    TEST("查找顶点B位置");
    int bi = graphLocateVex(&g, "B");
    CHECK(bi == 1, "查找位置错误");

    /* T06 */
    TEST("栈操作 push/pop");
    Stack stk; stackInit(&stk, 4);
    stackPush(&stk, 10); stackPush(&stk, 20); stackPush(&stk, 30);
    int v = 0; stackPop(&stk, &v);
    int ok = (v == 30);
    stackPop(&stk, &v); ok &= (v == 20);
    stackPop(&stk, &v); ok &= (v == 10);
    stackDestroy(&stk);
    CHECK(ok, "栈顺序错误");

    /* T07 */
    TEST("队列操作 enqueue/dequeue");
    Queue q; queueInit(&q, 4);
    queueEnqueue(&q, 10); queueEnqueue(&q, 20); queueEnqueue(&q, 30);
    queueDequeue(&q, &v); ok = (v == 10);
    queueDequeue(&q, &v); ok &= (v == 20);
    queueDequeue(&q, &v); ok &= (v == 30);
    queueDestroy(&q);
    CHECK(ok, "队列FIFO错误");

    /* T08 */
    TEST("栈动态扩容(>=64元素)");
    stackInit(&stk, 4);
    ok = 1;
    for (int i = 0; i < 64; i++) if (stackPush(&stk, i) != GRAPH_OK) { ok = 0; break; }
    for (int i = 63; i >= 0; i--) {
        if (stackPop(&stk, &v) != GRAPH_OK || v != i) { ok = 0; break; }
    }
    stackDestroy(&stk);
    CHECK(ok, "扩容失败");

    /* T09 */
    TEST("DFS遍历顺序（从A开始）");
    SpanTree dtree;
    int dfsOrder[16]; int dfsLen;
    st = dfsTraverse(&g, 0, &dtree, dfsOrder, &dfsLen);
    int dfsOk = (st == GRAPH_OK && dfsLen == 4 && dfsOrder[0] == 0);
    treeDestroy(&dtree);
    CHECK(dfsOk, "DFS遍历失败");

    /* T10 */
    TEST("BFS遍历顺序（从A开始）");
    SpanTree btree;
    int bfsOrder[16]; int bfsLen;
    st = bfsTraverse(&g, 0, &btree, bfsOrder, &bfsLen);
    int bfsOk = (st == GRAPH_OK && bfsLen == 4 && bfsOrder[0] == 0);
    treeDestroy(&btree);
    CHECK(bfsOk, "BFS遍历失败");

    /* T11 */
    TEST("文件保存 (GRAPHV2魔数)");
    st = graphSaveToFile(&g, "graph_test_v2.bin");
    CHECK(st == GRAPH_OK && fileExists("graph_test_v2.bin"), "保存失败");

    /* T12 */
    TEST("文件加载并验证");
    ALGraph g2; graphInit(&g2, 16);
    st = graphLoadFromFile(&g2, "graph_test_v2.bin");
    int loadOk = (st == GRAPH_OK && g2.vexNum == 4 && g2.arcNum == 8);
    loadOk &= (graphLocateVex(&g2, "C") == 2);
    graphDestroy(&g2);
    CHECK(loadOk, "加载验证失败");

    /* T13 */
    TEST("错误文件魔数检测");
    {
        FILE *fp = fopen("graph_bad.bin", "wb");
        if (fp) {
            char bad[64]; memset(bad, 'X', sizeof(bad));
            fwrite(bad, 1, sizeof(bad), fp); fclose(fp);
        }
        ALGraph g2; graphInit(&g2, 16);
        st = graphLoadFromFile(&g2, "graph_bad.bin");
        graphDestroy(&g2);
        CHECK(st == GRAPH_ERR_MAGIC, "魔数检测失效");
        remove("graph_bad.bin");
    }

    /* T14 */
    TEST("空图DFS应返回错误");
    ALGraph empty; graphInit(&empty, 4);
    (void)id;
    st = dfsTraverse(&empty, 0, NULL, NULL, NULL);
    graphDestroy(&empty);
    CHECK(st == GRAPH_ERR_EMPTY, "空图检查失效");

    graphDestroy(&g);
    remove("graph_test_v2.bin");

    printDivider();
    cPrint(C_BOLD, "  测试结果: ");
    cPrint(C_GREEN, "%d 通过  ", passed);
    cPrint(failed ? C_RED : C_GREEN, "%d 失败", failed);
    cPrint(C_DIM, "  (共 %d 项)", TOTAL);
    printf("\n");
    printDivider();
    return failed == 0 ? 0 : 1;
}

/* ======================== 菜单系统 ======================== */

static void showMenu(ALGraph *g) {
    printDivider();
    cPrint(C_BOLD C_BLUE, "  主菜单\n");
    printDivider();
    cPrint(C_CYAN, "  1 "); printf("- 手动创建图\n");
    cPrint(C_CYAN, "  2 "); printf("- 从文件加载图\n");
    cPrint(C_CYAN, "  3 "); printf("- 保存图到文件\n");
    cPrint(C_CYAN, "  4 "); printf("- 显示邻接表\n");
    cPrint(C_CYAN, "  5 "); printf("- DFS 深度优先遍历\n");
    cPrint(C_CYAN, "  6 "); printf("- BFS 广度优先遍历\n");
    cPrint(C_CYAN, "  7 "); printf("- DFS + BFS 对比演示\n");
    cPrint(C_CYAN, "  8 "); printf("- 加载内置示例图(8城市)\n");
    cPrint(C_CYAN, "  H/?"); printf("- 帮助信息\n");
    cPrint(C_CYAN, "  0 "); printf("- 退出系统\n");
    printDivider();
    printf("  当前状态: 顶点 ");
    cPrint(g->vexNum > 0 ? C_GREEN : C_YELLOW, "%d", g->vexNum);
    printf(" | 边 ");
    cPrint(g->arcNum > 0 ? C_GREEN : C_YELLOW, "%d", g->arcNum);
    printf("\n");
    printDivider();
}

static void createGraphManual(ALGraph *g) {
    graphDestroy(g);
    graphInit(g, 16);
    int n = readInt("请输入顶点数(1~32): ", 1, 32);
    char name[64];
    for (int i = 0; i < n; i++) {
        sprintf(name, "请输入第 %d 个顶点名称: ", i+1);
        char vname[MAX_VERTEX_NAME + 1];
        readString(name, vname, sizeof(vname));
        if (vname[0] == '\0') {
            sprintf(vname, "V%d", i+1);
        }
        GraphStatus s = graphAddVex(g, vname, NULL);
        if (s != GRAPH_OK) printErr("%s", graphStatusStr(s));
    }
    int m = readInt("请输入边数: ", 0, n*(n-1)/2);
    int added = 0;
    for (int i = 0; i < m; ) {
        char buf[64];
        int u, v;
        char prompt[64];
        sprintf(prompt, "  边 %d: ", i+1);
        readString(prompt, buf, sizeof(buf));
        if (sscanf(buf, "%d %d", &u, &v) == 2) {
            u--; v--;
            if (u >= 0 && u < g->vexNum && v >= 0 && v < g->vexNum && u != v) {
                graphAddEdge(g, u, v, 1);
                i++; added++;
            } else {
                cPrint(C_YELLOW, "  顶点编号范围 1~%d，请重新输入\n", g->vexNum);
            }
        }
    }
    printOk("创建完成！");
    printf("  顶点: %d, 边: %d\n\n", g->vexNum, added);
}

static int getStartVex(ALGraph *g) {
    if (g->vexNum == 0) { printErr("图为空"); return -1; }
    printf("\n  可选顶点:\n");
    for (int i = 0; i < g->vexNum; i++) {
        cPrint(C_CYAN, "    [%d] %s", i+1, g->vertices[i].name);
        if ((i+1) % 4 == 0) printf("\n");
    }
    printf("\n");
    return readInt("请输入起始顶点编号: ", 1, g->vexNum) - 1;
}

static void showHelp(void) {
    printf("\n");
    cPrint(C_BOLD C_CYAN, "  Graph Traversal V2.0 - 使用帮助\n");
    printDivider();
    printf("  命令行参数:\n");
    cPrint(C_CYAN, "    --help, -h    "); printf("显示帮助信息\n");
    cPrint(C_CYAN, "    --test, -t    "); printf("运行内置自测用例\n");
    cPrint(C_CYAN, "    (无参数)      "); printf("启动交互式菜单\n\n");
    printf("  功能说明:\n");
    cPrint(C_CYAN, "    DFS "); printf("- 深度优先搜索：一条路走到黑再回溯\n");
    cPrint(C_CYAN, "    BFS "); printf("- 广度优先搜索：按层遍历，齐头并进\n");
    cPrint(C_CYAN, "    文件"); printf(" - 二进制持久化，带 GRAPHV2 魔数校验\n");
    printf("\n");
}

static void runMenu(void) {
    ALGraph g;
    graphInit(&g, 16);

    cPrint(C_BOLD C_CYAN, "\n");
    printf("  ╔══════════════════════════════════════════════╗\n");
    printf("  ║     图遍历演示系统  Graph Traversal V2.0     ║\n");
    printf("  ║     DFS / BFS / 邻接表 / 文件持久化          ║\n");
    printf("  ╚══════════════════════════════════════════════╝\n");

    if (fileExists(DEFAULT_GRAPH_FILE)) {
        GraphStatus s = graphLoadFromFile(&g, DEFAULT_GRAPH_FILE);
        if (s == GRAPH_OK) {
            printOk("已从 %s 加载图", DEFAULT_GRAPH_FILE);
        }
    }

    char choice[16];
    while (1) {
        showMenu(&g);
        printf("  请选择操作: "); fflush(stdout);
        if (fgets(choice, sizeof(choice), stdin) == NULL) break;
        char *nl = strchr(choice, '\n'); if (nl) *nl = '\0';

        if (strcmp(choice, "1") == 0) {
            createGraphManual(&g);
        } else if (strcmp(choice, "2") == 0) {
            char fname[256] = {0};
            readString("  文件名(回车默认 graph_v2.bin): ", fname, sizeof(fname));
            if (fname[0] == '\0') strcpy(fname, DEFAULT_GRAPH_FILE);
            GraphStatus s = graphLoadFromFile(&g, fname);
            if (s == GRAPH_OK) printOk("加载成功 - %d顶点 %d边", g.vexNum, g.arcNum);
            else printErr("%s", graphStatusStr(s));
        } else if (strcmp(choice, "3") == 0) {
            if (g.vexNum == 0) { printErr("图为空"); continue; }
            char fname[256] = {0};
            readString("  文件名(回车默认 graph_v2.bin): ", fname, sizeof(fname));
            if (fname[0] == '\0') strcpy(fname, DEFAULT_GRAPH_FILE);
            GraphStatus s = graphSaveToFile(&g, fname);
            if (s == GRAPH_OK) printOk("保存成功 -> %s", fname);
            else printErr("%s", graphStatusStr(s));
        } else if (strcmp(choice, "4") == 0) {
            graphPrint(&g);
        } else if (strcmp(choice, "5") == 0) {
            int start = getStartVex(&g);
            if (start >= 0) {
                printTitle("DFS 深度优先遍历");
                SpanTree tree;
                GraphStatus s = dfsTraverse(&g, start, &tree, NULL, NULL);
                if (s == GRAPH_OK) {
                    treePrint(&tree, &g, start, "DFS生成树");
                    treeDestroy(&tree);
                } else printErr("%s", graphStatusStr(s));
            }
        } else if (strcmp(choice, "6") == 0) {
            int start = getStartVex(&g);
            if (start >= 0) {
                printTitle("BFS 广度优先遍历");
                SpanTree tree;
                GraphStatus s = bfsTraverse(&g, start, &tree, NULL, NULL);
                if (s == GRAPH_OK) {
                    treePrint(&tree, &g, start, "BFS生成树");
                    treeDestroy(&tree);
                } else printErr("%s", graphStatusStr(s));
            }
        } else if (strcmp(choice, "7") == 0) {
            int start = getStartVex(&g);
            if (start >= 0) {
                printTitle("DFS vs BFS 对比演示");
                SpanTree dfsTree, bfsTree;
                GraphStatus s1 = dfsTraverse(&g, start, &dfsTree, NULL, NULL);
                printf("\n");
                GraphStatus s2 = bfsTraverse(&g, start, &bfsTree, NULL, NULL);
                if (s1 == GRAPH_OK) treePrint(&dfsTree, &g, start, "DFS生成树");
                if (s2 == GRAPH_OK) treePrint(&bfsTree, &g, start, "BFS生成树");
                if (s1 == GRAPH_OK) treeDestroy(&dfsTree);
                if (s2 == GRAPH_OK) treeDestroy(&bfsTree);
            }
        } else if (strcmp(choice, "8") == 0) {
            GraphStatus s = graphCreateTest(&g);
            if (s == GRAPH_OK) {
                printOk("已加载8城市交通示例图");
                graphPrint(&g);
                graphSaveToFile(&g, DEFAULT_GRAPH_FILE);
                printTitle("DFS演示 (从北京)");
                SpanTree dfsTree, bfsTree;
                dfsTraverse(&g, 0, &dfsTree, NULL, NULL);
                printf("\n");
                printTitle("BFS演示 (从北京)");
                bfsTraverse(&g, 0, &bfsTree, NULL, NULL);
                treePrint(&dfsTree, &g, 0, "DFS生成树");
                treePrint(&bfsTree, &g, 0, "BFS生成树");
                treeDestroy(&dfsTree);
                treeDestroy(&bfsTree);
            } else printErr("%s", graphStatusStr(s));
        } else if (strcmp(choice, "h") == 0 || strcmp(choice, "H") == 0 || strcmp(choice, "?") == 0) {
            showHelp();
        } else if (strcmp(choice, "0") == 0) {
            break;
        } else {
            cPrint(C_YELLOW, "  无效选择，请输入 0-8 或 h\n");
        }
    }

    if (g.vexNum > 0) {
        graphSaveToFile(&g, DEFAULT_GRAPH_FILE);
    }
    graphDestroy(&g);
    cPrint(C_GREEN, "\n  感谢使用，再见！\n\n");
}

/* ======================== CLI命令行参数处理 ======================== */

static void printUsage(void) {
    printf("Graph Traversal Demonstration System V2.0\n");
    printf("Usage: graph_traversal.exe [option]\n\n");
    printf("Options:\n");
    printf("  --help, -h      Show this help message\n");
    printf("  --test, -t      Run built-in self tests\n");
    printf("  (no args)       Start interactive menu mode\n");
}

static int runCommandLine(int argc, char *argv[]) {
    if (argc < 2) return 0;
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        printUsage(); return 0;
    }
    if (strcmp(argv[1], "--test") == 0 || strcmp(argv[1], "-t") == 0) {
        return runTests();
    }
    printErr("未知参数: %s", argv[1]);
    printUsage();
    return 1;
}

/* ======================== 主函数 ======================== */

int main(int argc, char *argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    if (argc > 1) {
        return runCommandLine(argc, argv);
    }

    runMenu();
    return 0;
}
