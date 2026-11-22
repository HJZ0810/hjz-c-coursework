/*
 * 模拟小商店管理系统 V2.0
 * 功能：商品CRUD、销售管理、统计分析、文件持久化、UTF-8 BOM支持
 * 编译：D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o shop.exe main.c
 * 作者：HJZ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

#define SHOP_MAGIC    "SHOPV2"
#define MAGIC_LEN     6
#define SHOP_VERSION  2
#define MAX_NAME_LEN  50
#define INIT_CAPACITY 16
#define DATA_FILE     "goods.dat"

typedef enum {
    SHOP_OK = 0,
    SHOP_ERR_OPEN_FILE,
    SHOP_ERR_READ_FILE,
    SHOP_ERR_WRITE_FILE,
    SHOP_ERR_MAGIC,
    SHOP_ERR_VERSION,
    SHOP_ERR_MEMORY,
    SHOP_ERR_NOT_FOUND,
    SHOP_ERR_DUPLICATE,
    SHOP_ERR_INVALID_INPUT
} ShopStatus;

typedef enum {
    C_RESET = 0,
    C_RED,
    C_GREEN,
    C_YELLOW,
    C_CYAN,
    C_WHITE
} Color;

typedef struct {
    int   id;
    char  name[MAX_NAME_LEN];
    float price;
    int   stock;
    int   sold;
} Goods;

typedef struct {
    Goods  *items;
    int     count;
    int     capacity;
    char    filepath[256];
} ShopDB;

static const char *statusMsg[] = {
    "操作成功",
    "无法打开文件",
    "读取文件失败",
    "写入文件失败",
    "无效的文件格式（magic不匹配）",
    "不支持的文件版本",
    "内存分配失败",
    "商品未找到",
    "商品ID重复",
    "输入无效"
};

static ShopDB g_db;
static int g_testPassed = 0;
static int g_testFailed = 0;

#ifdef _WIN32
static void enableVT100(void) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) return;
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, mode);
}
#endif

static void cPrint(Color color, const char *text) {
    static const char *codes[] = {
        "\033[0m", "\033[31m", "\033[32m", "\033[33m", "\033[36m", "\033[37m"
    };
    printf("%s%s%s", codes[color], text, codes[C_RESET]);
}

static void printOK(const char *msg) {
    cPrint(C_GREEN, "[OK] ");
    printf("%s\n", msg);
}

static void printERR(const char *msg) {
    cPrint(C_RED, "[ERR] ");
    printf("%s\n", msg);
}

static void printTitle(const char *title) {
    cPrint(C_CYAN, "\n=== ");
    cPrint(C_CYAN, title);
    cPrint(C_CYAN, " ===\n");
}

static const char *statusStr(ShopStatus s) {
    if (s >= 0 && s <= SHOP_ERR_INVALID_INPUT)
        return statusMsg[s];
    return "未知错误";
}

static void dbInit(ShopDB *db, const char *filepath) {
    db->items = NULL;
    db->count = 0;
    db->capacity = 0;
    strncpy(db->filepath, filepath ? filepath : DATA_FILE, sizeof(db->filepath) - 1);
    db->filepath[sizeof(db->filepath) - 1] = '\0';
}

static ShopStatus dbEnsureCapacity(ShopDB *db, int needed) {
    if (db->count + needed <= db->capacity) return SHOP_OK;
    int newCap = db->capacity == 0 ? INIT_CAPACITY : db->capacity;
    while (newCap < db->count + needed) newCap *= 2;
    Goods *p = (Goods*)realloc(db->items, (size_t)newCap * sizeof(Goods));
    if (!p) return SHOP_ERR_MEMORY;
    db->items = p;
    db->capacity = newCap;
    return SHOP_OK;
}

static void dbFree(ShopDB *db) {
    free(db->items);
    db->items = NULL;
    db->count = 0;
    db->capacity = 0;
}

static int dbFindById(ShopDB *db, int id) {
    for (int i = 0; i < db->count; i++)
        if (db->items[i].id == id) return i;
    return -1;
}

static int dbNextId(ShopDB *db) {
    int max = 0;
    for (int i = 0; i < db->count; i++)
        if (db->items[i].id > max) max = db->items[i].id;
    return max + 1;
}

static ShopStatus dbAdd(ShopDB *db, const char *name, float price, int stock) {
    if (!name || strlen(name) == 0 || price < 0 || stock < 0)
        return SHOP_ERR_INVALID_INPUT;
    ShopStatus st = dbEnsureCapacity(db, 1);
    if (st != SHOP_OK) return st;
    Goods *g = &db->items[db->count];
    g->id = dbNextId(db);
    strncpy(g->name, name, MAX_NAME_LEN - 1);
    g->name[MAX_NAME_LEN - 1] = '\0';
    g->price = price;
    g->stock = stock;
    g->sold = 0;
    db->count++;
    return SHOP_OK;
}

static ShopStatus dbDelete(ShopDB *db, int id) {
    int idx = dbFindById(db, id);
    if (idx < 0) return SHOP_ERR_NOT_FOUND;
    for (int i = idx; i < db->count - 1; i++)
        db->items[i] = db->items[i + 1];
    db->count--;
    return SHOP_OK;
}

static ShopStatus dbSell(ShopDB *db, int id, int qty) {
    if (qty <= 0) return SHOP_ERR_INVALID_INPUT;
    int idx = dbFindById(db, id);
    if (idx < 0) return SHOP_ERR_NOT_FOUND;
    if (db->items[idx].stock < qty) return SHOP_ERR_INVALID_INPUT;
    db->items[idx].stock -= qty;
    db->items[idx].sold += qty;
    return SHOP_OK;
}

static ShopStatus dbRestock(ShopDB *db, int id, int qty) {
    if (qty <= 0) return SHOP_ERR_INVALID_INPUT;
    int idx = dbFindById(db, id);
    if (idx < 0) return SHOP_ERR_NOT_FOUND;
    db->items[idx].stock += qty;
    return SHOP_OK;
}

static int hasUtf8BOM(FILE *fp) {
    unsigned char bom[3];
    long pos = ftell(fp);
    if (fread(bom, 1, 3, fp) != 3) { fseek(fp, pos, SEEK_SET); return 0; }
    if (bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF) return 1;
    fseek(fp, pos, SEEK_SET);
    return 0;
}

static int isTextFormat(FILE *fp) {
    long pos = ftell(fp);
    char buf[MAGIC_LEN];
    int n = (int)fread(buf, 1, MAGIC_LEN, fp);
    fseek(fp, pos, SEEK_SET);
    if (n < MAGIC_LEN) return 0;
    return memcmp(buf, SHOP_MAGIC, MAGIC_LEN) != 0;
}

static ShopStatus dbLoad(ShopDB *db, const char *filepath) {
    if (filepath) strncpy(db->filepath, filepath, sizeof(db->filepath) - 1);
    FILE *fp = fopen(db->filepath, "rb");
    if (!fp) return SHOP_ERR_OPEN_FILE;

    if (isTextFormat(fp)) {
        fclose(fp);
        fp = fopen(db->filepath, "r");
        if (!fp) return SHOP_ERR_OPEN_FILE;
        hasUtf8BOM(fp);
        dbFree(db);
        char name[MAX_NAME_LEN];
        float price;
        int stock, sold;
        while (fscanf(fp, "%49[^,],%f,%d,%d\n", name, &price, &stock, &sold) == 4) {
            ShopStatus st = dbAdd(db, name, price, stock);
            if (st != SHOP_OK) { fclose(fp); return st; }
            db->items[db->count - 1].sold = sold;
        }
        fclose(fp);
        return SHOP_OK;
    }

    char magic[MAGIC_LEN];
    int version;
    if (fread(magic, 1, MAGIC_LEN, fp) != MAGIC_LEN) { fclose(fp); return SHOP_ERR_READ_FILE; }
    if (memcmp(magic, SHOP_MAGIC, MAGIC_LEN) != 0) { fclose(fp); return SHOP_ERR_MAGIC; }
    if (fread(&version, sizeof(int), 1, fp) != 1) { fclose(fp); return SHOP_ERR_READ_FILE; }
    if (version != SHOP_VERSION) { fclose(fp); return SHOP_ERR_VERSION; }
    int count;
    if (fread(&count, sizeof(int), 1, fp) != 1) { fclose(fp); return SHOP_ERR_READ_FILE; }
    dbFree(db);
    ShopStatus st = dbEnsureCapacity(db, count);
    if (st != SHOP_OK) { fclose(fp); return st; }
    if (fread(db->items, sizeof(Goods), (size_t)count, fp) != (size_t)count) {
        fclose(fp); return SHOP_ERR_READ_FILE;
    }
    db->count = count;
    fclose(fp);
    return SHOP_OK;
}

static ShopStatus dbSave(ShopDB *db) {
    FILE *fp = fopen(db->filepath, "wb");
    if (!fp) return SHOP_ERR_OPEN_FILE;
    if (fwrite(SHOP_MAGIC, 1, MAGIC_LEN, fp) != MAGIC_LEN) { fclose(fp); return SHOP_ERR_WRITE_FILE; }
    int version = SHOP_VERSION;
    if (fwrite(&version, sizeof(int), 1, fp) != 1) { fclose(fp); return SHOP_ERR_WRITE_FILE; }
    if (fwrite(&db->count, sizeof(int), 1, fp) != 1) { fclose(fp); return SHOP_ERR_WRITE_FILE; }
    if (db->count > 0) {
        if (fwrite(db->items, sizeof(Goods), (size_t)db->count, fp) != (size_t)db->count) {
            fclose(fp); return SHOP_ERR_WRITE_FILE;
        }
    }
    fclose(fp);
    return SHOP_OK;
}

static void dbList(ShopDB *db) {
    printTitle("商品列表");
    if (db->count == 0) {
        printf("  （暂无商品）\n");
        return;
    }
    printf("  %-4s %-20s %-10s %-8s %-8s\n", "ID", "名称", "单价", "库存", "销量");
    printf("  --------------------------------------------------------\n");
    for (int i = 0; i < db->count; i++) {
        Goods *g = &db->items[i];
        printf("  %-4d %-20s %-10.2f %-8d %-8d\n", g->id, g->name, g->price, g->stock, g->sold);
    }
    printf("  共 %d 件商品\n", db->count);
}

static void readLine(char *buf, int size) {
    if (!fgets(buf, size, stdin)) { buf[0] = '\0'; return; }
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
}

static char *trim(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    char *end = s + strlen(s) - 1;
    while (end > s && (*end == ' ' || *end == '\t')) *end-- = '\0';
    return s;
}

static void doAdd(void) {
    char name[MAX_NAME_LEN];
    float price;
    int stock;
    printTitle("添加商品");
    printf("  商品名称："); readLine(name, MAX_NAME_LEN); trim(name);
    printf("  单价：");
    char buf[64]; readLine(buf, sizeof(buf));
    if (sscanf(buf, "%f", &price) != 1 || price < 0) { printERR("单价无效"); return; }
    printf("  库存数量："); readLine(buf, sizeof(buf));
    if (sscanf(buf, "%d", &stock) != 1 || stock < 0) { printERR("库存无效"); return; }
    ShopStatus st = dbAdd(&g_db, name, price, stock);
    if (st == SHOP_OK) {
        printOK("商品添加成功");
        dbSave(&g_db);
    } else {
        printERR(statusStr(st));
    }
}

static void doDelete(void) {
    char buf[64]; int id;
    printTitle("删除商品");
    printf("  商品ID："); readLine(buf, sizeof(buf));
    if (sscanf(buf, "%d", &id) != 1) { printERR("ID无效"); return; }
    ShopStatus st = dbDelete(&g_db, id);
    if (st == SHOP_OK) { printOK("商品删除成功"); dbSave(&g_db); }
    else printERR(statusStr(st));
}

static void doSell(void) {
    char buf[64]; int id, qty;
    printTitle("商品销售");
    dbList(&g_db);
    printf("  商品ID："); readLine(buf, sizeof(buf));
    if (sscanf(buf, "%d", &id) != 1) { printERR("ID无效"); return; }
    printf("  销售数量："); readLine(buf, sizeof(buf));
    if (sscanf(buf, "%d", &qty) != 1) { printERR("数量无效"); return; }
    ShopStatus st = dbSell(&g_db, id, qty);
    if (st == SHOP_OK) { printOK("销售成功"); dbSave(&g_db); }
    else printERR(statusStr(st));
}

static void doRestock(void) {
    char buf[64]; int id, qty;
    printTitle("商品进货");
    dbList(&g_db);
    printf("  商品ID："); readLine(buf, sizeof(buf));
    if (sscanf(buf, "%d", &id) != 1) { printERR("ID无效"); return; }
    printf("  进货数量："); readLine(buf, sizeof(buf));
    if (sscanf(buf, "%d", &qty) != 1) { printERR("数量无效"); return; }
    ShopStatus st = dbRestock(&g_db, id, qty);
    if (st == SHOP_OK) { printOK("进货成功"); dbSave(&g_db); }
    else printERR(statusStr(st));
}

static void doQuery(void) {
    char buf[64]; int id;
    printTitle("查询商品");
    printf("  商品ID："); readLine(buf, sizeof(buf));
    if (sscanf(buf, "%d", &id) != 1) { printERR("ID无效"); return; }
    int idx = dbFindById(&g_db, id);
    if (idx < 0) { printERR(statusStr(SHOP_ERR_NOT_FOUND)); return; }
    Goods *g = &g_db.items[idx];
    printf("  ID:     %d\n", g->id);
    printf("  名称:   %s\n", g->name);
    printf("  单价:   %.2f\n", g->price);
    printf("  库存:   %d\n", g->stock);
    printf("  销量:   %d\n", g->sold);
    printf("  销售额: %.2f\n", g->price * g->sold);
}

static void doStat(void) {
    printTitle("销售统计");
    if (g_db.count == 0) { printf("  （无数据）\n"); return; }
    float totalRevenue = 0;
    int totalSold = 0, totalStock = 0;
    for (int i = 0; i < g_db.count; i++) {
        totalRevenue += g_db.items[i].price * g_db.items[i].sold;
        totalSold += g_db.items[i].sold;
        totalStock += g_db.items[i].stock;
    }
    printf("  商品总数：%d\n", g_db.count);
    printf("  总库存：  %d\n", totalStock);
    printf("  总销量：  %d\n", totalSold);
    printf("  总销售额："); cPrint(C_GREEN, ""); printf("%.2f\n", totalRevenue); cPrint(C_RESET, "");
}

static void doStatBySalesRate(void) {
    printTitle("按销售率排序统计");
    if (g_db.count == 0) { printf("  （无数据）\n"); return; }
    int *indices = (int*)malloc((size_t)g_db.count * sizeof(int));
    if (!indices) { printERR(statusStr(SHOP_ERR_MEMORY)); return; }
    for (int i = 0; i < g_db.count; i++) indices[i] = i;
    for (int i = 0; i < g_db.count - 1; i++)
        for (int j = 0; j < g_db.count - 1 - i; j++) {
            Goods *a = &g_db.items[indices[j]];
            Goods *b = &g_db.items[indices[j + 1]];
            float ra = (a->sold + a->stock) > 0 ? (float)a->sold / (float)(a->sold + a->stock) : 0;
            float rb = (b->sold + b->stock) > 0 ? (float)b->sold / (float)(b->sold + b->stock) : 0;
            if (ra < rb) { int t = indices[j]; indices[j] = indices[j + 1]; indices[j + 1] = t; }
        }
    printf("  %-4s %-20s %-8s %-8s %-8s\n", "排名", "名称", "销量", "总量", "销售率");
    printf("  --------------------------------------------------------\n");
    for (int i = 0; i < g_db.count; i++) {
        Goods *g = &g_db.items[indices[i]];
        int total = g->sold + g->stock;
        float rate = total > 0 ? (float)g->sold / (float)total * 100.0f : 0;
        printf("  %-4d %-20s %-8d %-8d %-7.1f%%\n", i + 1, g->name, g->sold, total, rate);
    }
    free(indices);
}

static void printUsage(const char *prog) {
    cPrint(C_CYAN, "模拟小商店管理系统 V2.0\n");
    printf("用法：\n");
    printf("  %s              启动交互模式\n", prog);
    printf("  %s -h, --help   显示帮助\n", prog);
    printf("  %s -t, --test   运行内置自测\n", prog);
    printf("  %s -f <file>    指定数据文件（默认 %s）\n", prog, DATA_FILE);
    printf("\n交互模式菜单：\n");
    printf("  1.添加商品  2.删除商品  3.商品销售  4.商品进货\n");
    printf("  5.查询商品  6.商品列表  7.销售统计  8.按销售率排序\n");
    printf("  0.退出\n");
}

static void interactiveLoop(void) {
    char buf[64];
    int choice;
    while (1) {
        printTitle("主菜单");
        printf("  1.添加商品  2.删除商品  3.商品销售  4.商品进货\n");
        printf("  5.查询商品  6.商品列表  7.销售统计  8.按销售率排序\n");
        printf("  0.退出\n");
        printf("  请选择：");
        readLine(buf, sizeof(buf));
        if (sscanf(buf, "%d", &choice) != 1) { printERR("请输入数字"); continue; }
        switch (choice) {
            case 1: doAdd(); break;
            case 2: doDelete(); break;
            case 3: doSell(); break;
            case 4: doRestock(); break;
            case 5: doQuery(); break;
            case 6: dbList(&g_db); break;
            case 7: doStat(); break;
            case 8: doStatBySalesRate(); break;
            case 0: printOK("感谢使用，再见！"); return;
            default: printERR("无效选项");
        }
        printf("\n按回车键继续...");
        readLine(buf, sizeof(buf));
    }
}

#define TEST_ASSERT(cond, msg) do { \
    if (cond) { g_testPassed++; cPrint(C_GREEN, "  [PASS] "); printf("%s\n", msg); } \
    else { g_testFailed++; cPrint(C_RED, "  [FAIL] "); printf("%s\n", msg); } \
} while(0)

static void runSelfTest(void) {
    printTitle("内置自测用例");
    ShopDB testDb;
    dbInit(&testDb, "test_shop.dat");
    remove("test_shop.dat");

    ShopStatus st = dbAdd(&testDb, "可乐", 3.5f, 100);
    TEST_ASSERT(st == SHOP_OK, "添加商品-可乐");
    TEST_ASSERT(testDb.count == 1, "商品数量=1");

    st = dbAdd(&testDb, "面包", 5.0f, 50);
    TEST_ASSERT(st == SHOP_OK, "添加商品-面包");
    TEST_ASSERT(testDb.count == 2, "商品数量=2");

    st = dbAdd(&testDb, "矿泉水", 2.0f, 200);
    TEST_ASSERT(st == SHOP_OK, "添加商品-矿泉水");
    TEST_ASSERT(testDb.count == 3, "商品数量=3");

    TEST_ASSERT(testDb.items[0].id == 1, "可乐ID=1");
    TEST_ASSERT(testDb.items[1].id == 2, "面包ID=2");
    TEST_ASSERT(testDb.items[2].id == 3, "矿泉水ID=3");

    st = dbSell(&testDb, 1, 30);
    TEST_ASSERT(st == SHOP_OK, "销售可乐30瓶");
    TEST_ASSERT(testDb.items[0].stock == 70, "可乐库存=70");
    TEST_ASSERT(testDb.items[0].sold == 30, "可乐销量=30");

    st = dbSell(&testDb, 1, 100);
    TEST_ASSERT(st == SHOP_ERR_INVALID_INPUT, "销售超过库存应失败");

    st = dbRestock(&testDb, 2, 20);
    TEST_ASSERT(st == SHOP_OK, "进货面包20个");
    TEST_ASSERT(testDb.items[1].stock == 70, "面包库存=70");

    int idx = dbFindById(&testDb, 2);
    TEST_ASSERT(idx == 1, "查找面包ID=2在索引1");

    idx = dbFindById(&testDb, 999);
    TEST_ASSERT(idx == -1, "查找不存在ID返回-1");

    st = dbDelete(&testDb, 3);
    TEST_ASSERT(st == SHOP_OK, "删除矿泉水");
    TEST_ASSERT(testDb.count == 2, "删除后数量=2");
    TEST_ASSERT(testDb.items[1].id == 2, "删除后面包在索引1");

    st = dbDelete(&testDb, 999);
    TEST_ASSERT(st == SHOP_ERR_NOT_FOUND, "删除不存在商品失败");

    st = dbSave(&testDb);
    TEST_ASSERT(st == SHOP_OK, "保存到文件");

    ShopDB loadDb;
    dbInit(&loadDb, "test_shop.dat");
    st = dbLoad(&loadDb, NULL);
    TEST_ASSERT(st == SHOP_OK, "从文件加载");
    TEST_ASSERT(loadDb.count == 2, "加载后商品数=2");
    TEST_ASSERT(loadDb.items[0].id == 1, "加载后可乐ID=1");
    TEST_ASSERT(loadDb.items[0].sold == 30, "加载后可乐销量=30");
    TEST_ASSERT(loadDb.items[0].stock == 70, "加载后可乐库存=70");
    dbFree(&loadDb);

    st = dbAdd(&testDb, "", 1.0f, 10);
    TEST_ASSERT(st == SHOP_ERR_INVALID_INPUT, "空名称添加失败");
    st = dbAdd(&testDb, "测试", -1.0f, 10);
    TEST_ASSERT(st == SHOP_ERR_INVALID_INPUT, "负价格添加失败");
    st = dbAdd(&testDb, "测试", 1.0f, -5);
    TEST_ASSERT(st == SHOP_ERR_INVALID_INPUT, "负库存添加失败");

    for (int i = 0; i < 20; i++) {
        char name[32];
        sprintf(name, "测试商品%d", i);
        dbAdd(&testDb, name, (float)(i + 1), i * 10);
    }
    TEST_ASSERT(testDb.count == 22, "批量添加后数量=22（动态扩容测试）");
    TEST_ASSERT(testDb.capacity >= 22, "容量足够");

    dbFree(&testDb);
    remove("test_shop.dat");

    printTitle("测试结果");
    cPrint(C_GREEN, "  通过："); printf("%d\n", g_testPassed);
    cPrint(C_RED, "  失败："); printf("%d\n", g_testFailed);
    if (g_testFailed == 0) {
        cPrint(C_GREEN, "\n  所有测试通过！\n\n");
    } else {
        cPrint(C_RED, "\n  有测试失败！\n\n");
    }
}

int main(int argc, char *argv[]) {
#ifdef _WIN32
    enableVT100();
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    dbInit(&g_db, DATA_FILE);
    const char *filepath = DATA_FILE;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            dbFree(&g_db);
            return 0;
        }
        if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--test") == 0) {
            runSelfTest();
            dbFree(&g_db);
            return g_testFailed > 0 ? 1 : 0;
        }
        if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            filepath = argv[++i];
            strncpy(g_db.filepath, filepath, sizeof(g_db.filepath) - 1);
            g_db.filepath[sizeof(g_db.filepath) - 1] = '\0';
        }
    }

    cPrint(C_CYAN, "========================================\n");
    cPrint(C_CYAN, "    模拟小商店管理系统 V2.0\n");
    cPrint(C_CYAN, "========================================\n");

    ShopStatus st = dbLoad(&g_db, filepath);
    if (st == SHOP_OK) {
        printOK("数据加载成功");
    } else if (st == SHOP_ERR_OPEN_FILE) {
        printOK("新建数据文件");
    } else {
        printERR(statusStr(st));
        printf("  将使用空数据库启动\n");
    }

    interactiveLoop();
    dbSave(&g_db);
    dbFree(&g_db);
    return 0;
}
