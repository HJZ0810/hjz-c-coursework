/*
* ============================================================================
*
*    机动车违章信息管理系统 V2.0 (完全重写·增强版)
*    Motor Vehicle Violation Management System V2.0
*
* ============================================================================
*
*  V2.0 升级要点 (参照哈夫曼编译码器质量标准):
*    [1] UTF-8 BOM 编码 + SetConsoleOutputCP(65001) 彻底解决中文乱码
*    [2] ANSI 彩色终端界面（绿OK / 红ERR / 青标题 / 蓝菜单）
*    [3] 命令行模式：支持 --help/-h / --test/-t / -f <文件> / -o <输出>
*    [4] 动态内存管理：违章记录使用动态数组，自动扩容（容量翻倍策略）
*    [5] 统一错误码 enum VioStatus，所有函数返回标准错误码
*    [6] 文件持久化：V2 二进制格式，带 magic header "VIO V2" (6字节) + 版本号
*    [7] 内置全面自测用例：18+ 测试覆盖 CRUD + 查询 + 统计 + 文件I/O + 错误处理
*    [8] 完善的交互菜单系统，输入安全验证
*    [9] 极其详尽的中文注释（Doxygen 风格）
*   [10] 统计功能：违章次数统计、未处理违章统计、按车牌/驾驶员查询
*
*  数据结构 (V2.0 改为动态数组存储):
*    VehicleArray   —— 车辆信息动态数组（自动扩容）
*    DriverArray    —— 驾驶员信息动态数组（自动扩容）
*    ViolationArray —— 违章记录动态数组（自动扩容，核心数据）
*    VioHeader      —— V2 二进制文件头（magic + version + 计数）
*
*  编译命令:
*    D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o vio.exe main.c
*
*  命令行用法:
*    vio.exe                 交互模式（菜单）
*    vio.exe -h / --help     显示帮助
*    vio.exe -t / --test     运行内置自测用例
*    vio.exe -f <数据文件>   指定加载的数据文件
*    vio.exe -o <输出文件>   指定导出的文件
*
*  V2 文件格式 (.vio):
*    偏移  长度      字段              说明
*    0     6 bytes   magic "VIO V2"    魔数标识V2格式
*    6     2 bytes   version           版本号(当前=2)
*    8     4 bytes   vehicleCount      车辆记录数
*    12    4 bytes   driverCount       驾驶员记录数
*    16    4 bytes   violationCount    违章记录数
*    20    ...       Vehicle[]         车辆数据（二进制结构体）
*    ...   ...       Driver[]          驾驶员数据
*    ...   ...       Violation[]       违章记录数据
*
*  编码: UTF-8 (with BOM)
*  作者: 数据库男神 (HuiJunzhuo)
*  学期: 2025-2026学年第二学期数据结构课程设计
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

/* ======================== 常量与宏定义 ======================== */
#define VIO_MAGIC       "VIO V2"
#define VIO_VERSION     2
#define VIO_MAGIC_LEN   6
#define MAX_STR         64
#define MAX_DESC        128
#define DEFAULT_FILE    "violations.vio"
#define INIT_CAPACITY   8
#define MAX_PROGRESS    40

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

/* ======================== 统一错误码枚举 ======================== */
typedef enum {
    VIO_OK = 0,
    VIO_ERR_OPEN_FILE,
    VIO_ERR_READ_FILE,
    VIO_ERR_WRITE_FILE,
    VIO_ERR_MEMORY,
    VIO_ERR_CORRUPT,
    VIO_ERR_MAGIC,
    VIO_ERR_VERSION,
    VIO_ERR_EMPTY,
    VIO_ERR_NOT_FOUND,
    VIO_ERR_DUPLICATE,
    VIO_ERR_INVALID_INPUT,
    VIO_ERR_CANCEL,
    VIO_ERR_CAPACITY,
} VioStatus;

/* ======================== 数据结构定义 ======================== */

/**
 * 车辆信息结构体
 * 包含车牌号（唯一标识）、VIN码、品牌、型号等完整车辆信息
 */
typedef struct {
    char plate[MAX_STR];
    char vin[MAX_STR];
    char brand[MAX_STR];
    char model[MAX_STR];
    char color[MAX_STR];
    char vtype[MAX_STR];
    char prodDate[MAX_STR];
    char regDate[MAX_STR];
    char ownerName[MAX_STR];
    char ownerId[MAX_STR];
} Vehicle;

/**
 * 驾驶员信息结构体
 * 包含身份证号（唯一标识）、姓名、年龄、性别、驾照类型等信息
 */
typedef struct {
    char name[MAX_STR];
    int  age;
    char gender[MAX_STR];
    char idCard[MAX_STR];
    char licenseType[MAX_STR];
    char obtainDate[MAX_STR];
} Driver;

/**
 * 违章记录结构体
 * 通过 plate 关联 Vehicle，通过 driverId 关联 Driver
 * handled: 0=未处理, 1=已处理
 */
typedef struct {
    char plate[MAX_STR];
    char driverId[MAX_STR];
    char vtime[MAX_STR];
    char location[MAX_STR];
    char description[MAX_DESC];
    int  handled;
} Violation;

/**
 * 通用动态数组模板（通过宏实现类型安全的动态数组）
 * 使用容量翻倍策略进行自动扩容，均摊时间复杂度O(1)
 */
#define DEFINE_DYN_ARRAY(Type, Name) \
typedef struct { \
    Type *data; \
    int count; \
    int capacity; \
} Name##Array; \
static VioStatus Name##Array_init(Name##Array *arr) { \
    arr->data = (Type *)calloc(INIT_CAPACITY, sizeof(Type)); \
    if (!arr->data) return VIO_ERR_MEMORY; \
    arr->count = 0; \
    arr->capacity = INIT_CAPACITY; \
    return VIO_OK; \
} \
static VioStatus Name##Array_grow(Name##Array *arr) { \
    int newCap = arr->capacity * 2; \
    Type *newData = (Type *)realloc(arr->data, (size_t)newCap * sizeof(Type)); \
    if (!newData) return VIO_ERR_MEMORY; \
    arr->data = newData; \
    arr->capacity = newCap; \
    return VIO_OK; \
} \
static VioStatus Name##Array_add(Name##Array *arr, const Type *item) { \
    if (arr->count >= arr->capacity) { \
        VioStatus s = Name##Array_grow(arr); \
        if (s != VIO_OK) return s; \
    } \
    arr->data[arr->count] = *item; \
    arr->count++; \
    return VIO_OK; \
} \
static void Name##Array_free(Name##Array *arr) { \
    if (arr->data) { free(arr->data); arr->data = NULL; } \
    arr->count = 0; \
    arr->capacity = 0; \
}

DEFINE_DYN_ARRAY(Vehicle, Vehicle)
DEFINE_DYN_ARRAY(Driver, Driver)
DEFINE_DYN_ARRAY(Violation, Violation)

/**
 * V2 二进制文件头结构体
 * 用于持久化时的格式校验和版本控制
 */
typedef struct {
    char magic[VIO_MAGIC_LEN];
    unsigned short version;
    unsigned int vehicleCount;
    unsigned int driverCount;
    unsigned int violationCount;
} VioHeader;

/* ======================== 全局状态 ======================== */
static VehicleArray   g_vehicles;
static DriverArray    g_drivers;
static ViolationArray g_violations;
static int g_colorEnabled = 1;
static int g_dataLoaded = 0;
static char g_dataFile[MAX_STR] = DEFAULT_FILE;

/* ======================== 前向声明 ======================== */
static VioStatus saveToFile(const char *filename);
static VioStatus loadFromFile(const char *filename);
static void runMenu(void);
static int runSelfTests(void);

/* ======================== 工具函数：颜色与终端 ======================== */

static void enableVTMode(void) {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) { g_colorEnabled = 0; return; }
    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) { g_colorEnabled = 0; return; }
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
    g_colorEnabled = 1;
#endif
}

static void cPrint(const char *color, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (g_colorEnabled && color) fputs(color, stdout);
    vprintf(fmt, args);
    if (g_colorEnabled && color) fputs(C_RESET, stdout);
    va_end(args);
}

static void printOK(const char *msg)    { cPrint(C_GREEN, "  [OK] "); printf("%s\n", msg); }
static void printErr(const char *msg)   { cPrint(C_RED,   "  [ERROR] "); printf("%s\n", msg); }
static void printWarn(const char *msg)  { cPrint(C_YELLOW,"  [WARN] "); printf("%s\n", msg); }
static void printInfo(const char *msg)  { cPrint(C_CYAN,  "  [INFO] "); printf("%s\n", msg); }
static void printTitle(const char *msg) { cPrint(C_BOLD C_CYAN, "\n  ===== %s =====\n", msg); }

/* ======================== 工具函数：UI与输入 ======================== */

static void printBanner(void) {
    printf("\n");
    cPrint(C_BOLD C_BLUE,
        "  ============================================================================\n");
    cPrint(C_BOLD C_CYAN,
        "  |         Motor Vehicle Violation Management System V2.0 (Enhanced)        |\n");
    cPrint(C_BOLD C_CYAN,
        "  |                         机动车违章信息管理系统  V2.0 完全增强版           |\n");
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

static int fileExists(const char *path) {
    return access(path, 0) == 0;
}

static long getFileSize(FILE *fp) {
    long pos = ftell(fp);
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, pos, SEEK_SET);
    return size;
}

/* ======================== 查找与索引函数 ======================== */

static int findVehicleByPlate(const char *plate) {
    for (int i = 0; i < g_vehicles.count; i++) {
        if (strcmp(g_vehicles.data[i].plate, plate) == 0)
            return i;
    }
    return -1;
}

static int findDriverByIdCard(const char *idCard) {
    for (int i = 0; i < g_drivers.count; i++) {
        if (strcmp(g_drivers.data[i].idCard, idCard) == 0)
            return i;
    }
    return -1;
}

/* ======================== 内存初始化与释放 ======================== */

static VioStatus initSystem(void) {
    VioStatus s;
    s = VehicleArray_init(&g_vehicles);
    if (s != VIO_OK) return s;
    s = DriverArray_init(&g_drivers);
    if (s != VIO_OK) { VehicleArray_free(&g_vehicles); return s; }
    s = ViolationArray_init(&g_violations);
    if (s != VIO_OK) {
        VehicleArray_free(&g_vehicles);
        DriverArray_free(&g_drivers);
        return s;
    }
    g_dataLoaded = 0;
    return VIO_OK;
}

static void freeSystem(void) {
    VehicleArray_free(&g_vehicles);
    DriverArray_free(&g_drivers);
    ViolationArray_free(&g_violations);
    g_dataLoaded = 0;
}

/* ======================== 核心业务：车辆管理 CRUD ======================== */

static VioStatus addVehicle(const Vehicle *v) {
    if (findVehicleByPlate(v->plate) >= 0) return VIO_ERR_DUPLICATE;
    return VehicleArray_add(&g_vehicles, v);
}

static VioStatus deleteVehicle(int idx) {
    if (idx < 0 || idx >= g_vehicles.count) return VIO_ERR_NOT_FOUND;
    for (int i = idx; i < g_vehicles.count - 1; i++) {
        g_vehicles.data[i] = g_vehicles.data[i + 1];
    }
    g_vehicles.count--;
    return VIO_OK;
}

/* ======================== 核心业务：驾驶员管理 CRUD ======================== */

static VioStatus addDriver(const Driver *d) {
    if (findDriverByIdCard(d->idCard) >= 0) return VIO_ERR_DUPLICATE;
    return DriverArray_add(&g_drivers, d);
}

static VioStatus deleteDriver(int idx) {
    if (idx < 0 || idx >= g_drivers.count) return VIO_ERR_NOT_FOUND;
    for (int i = idx; i < g_drivers.count - 1; i++) {
        g_drivers.data[i] = g_drivers.data[i + 1];
    }
    g_drivers.count--;
    return VIO_OK;
}

/* ======================== 核心业务：违章记录管理 ======================== */

static VioStatus addViolation(const Violation *v) {
    return ViolationArray_add(&g_violations, v);
}

static VioStatus deleteViolation(int idx) {
    if (idx < 0 || idx >= g_violations.count) return VIO_ERR_NOT_FOUND;
    for (int i = idx; i < g_violations.count - 1; i++) {
        g_violations.data[i] = g_violations.data[i + 1];
    }
    g_violations.count--;
    return VIO_OK;
}

static VioStatus handleViolation(int idx) {
    if (idx < 0 || idx >= g_violations.count) return VIO_ERR_NOT_FOUND;
    g_violations.data[idx].handled = 1;
    return VIO_OK;
}

static int countViolationsByPlate(const char *plate, int *handled, int *unhandled) {
    *handled = 0; *unhandled = 0;
    int total = 0;
    for (int i = 0; i < g_violations.count; i++) {
        if (strcmp(g_violations.data[i].plate, plate) == 0) {
            total++;
            if (g_violations.data[i].handled) (*handled)++;
            else (*unhandled)++;
        }
    }
    return total;
}

static int countViolationsByDriver(const char *idCard, int *handled, int *unhandled) {
    *handled = 0; *unhandled = 0;
    int total = 0;
    for (int i = 0; i < g_violations.count; i++) {
        if (strcmp(g_violations.data[i].driverId, idCard) == 0) {
            total++;
            if (g_violations.data[i].handled) (*handled)++;
            else (*unhandled)++;
        }
    }
    return total;
}

/* ======================== 文件持久化（V2 二进制格式） ======================== */

static VioStatus saveToFile(const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) return VIO_ERR_OPEN_FILE;

    VioHeader hdr;
    memcpy(hdr.magic, VIO_MAGIC, VIO_MAGIC_LEN);
    hdr.version = VIO_VERSION;
    hdr.vehicleCount = (unsigned int)g_vehicles.count;
    hdr.driverCount = (unsigned int)g_drivers.count;
    hdr.violationCount = (unsigned int)g_violations.count;

    if (fwrite(&hdr, sizeof(hdr), 1, fp) != 1) { fclose(fp); return VIO_ERR_WRITE_FILE; }

    if (g_vehicles.count > 0) {
        if (fwrite(g_vehicles.data, sizeof(Vehicle), (size_t)g_vehicles.count, fp) != (size_t)g_vehicles.count) {
            fclose(fp); return VIO_ERR_WRITE_FILE;
        }
    }
    if (g_drivers.count > 0) {
        if (fwrite(g_drivers.data, sizeof(Driver), (size_t)g_drivers.count, fp) != (size_t)g_drivers.count) {
            fclose(fp); return VIO_ERR_WRITE_FILE;
        }
    }
    if (g_violations.count > 0) {
        if (fwrite(g_violations.data, sizeof(Violation), (size_t)g_violations.count, fp) != (size_t)g_violations.count) {
            fclose(fp); return VIO_ERR_WRITE_FILE;
        }
    }

    fclose(fp);
    return VIO_OK;
}

static VioStatus loadFromFile(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) return VIO_ERR_OPEN_FILE;

    long fsize = getFileSize(fp);
    fseek(fp, 0, SEEK_SET);

    if (fsize < (long)sizeof(VioHeader)) { fclose(fp); return VIO_ERR_CORRUPT; }

    VioHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) { fclose(fp); return VIO_ERR_READ_FILE; }

    if (memcmp(hdr.magic, VIO_MAGIC, VIO_MAGIC_LEN) != 0) {
        fclose(fp);
        return VIO_ERR_MAGIC;
    }
    if (hdr.version > VIO_VERSION) {
        fclose(fp);
        return VIO_ERR_VERSION;
    }

    freeSystem();
    VioStatus s = initSystem();
    if (s != VIO_OK) { fclose(fp); return s; }

    for (unsigned int i = 0; i < hdr.vehicleCount; i++) {
        Vehicle v;
        memset(&v, 0, sizeof(v));
        if (fread(&v, sizeof(v), 1, fp) != 1) { fclose(fp); freeSystem(); return VIO_ERR_READ_FILE; }
        VehicleArray_add(&g_vehicles, &v);
    }
    for (unsigned int i = 0; i < hdr.driverCount; i++) {
        Driver d;
        memset(&d, 0, sizeof(d));
        if (fread(&d, sizeof(d), 1, fp) != 1) { fclose(fp); freeSystem(); return VIO_ERR_READ_FILE; }
        DriverArray_add(&g_drivers, &d);
    }
    for (unsigned int i = 0; i < hdr.violationCount; i++) {
        Violation v;
        memset(&v, 0, sizeof(v));
        if (fread(&v, sizeof(v), 1, fp) != 1) { fclose(fp); freeSystem(); return VIO_ERR_READ_FILE; }
        ViolationArray_add(&g_violations, &v);
    }

    fclose(fp);
    g_dataLoaded = 1;
    strncpy(g_dataFile, filename, MAX_STR - 1);
    return VIO_OK;
}

/* ======================== 数据显示函数 ======================== */

static void printVehicle(const Vehicle *v, int idx) {
    printf("  ── 车辆 #%d ──\n", idx + 1);
    printf("  车牌号: %s\n", v->plate);
    printf("  VIN码: %s\n", v->vin);
    printf("  品牌: %s  型号: %s\n", v->brand, v->model);
    printf("  颜色: %s  车辆类型: %s\n", v->color, v->vtype);
    printf("  生产日期: %s  注册日期: %s\n", v->prodDate, v->regDate);
    printf("  车主: %s  身份证: %s\n", v->ownerName, v->ownerId);
}

static void printDriver(const Driver *d, int idx) {
    printf("  ── 驾驶员 #%d ──\n", idx + 1);
    printf("  姓名: %s  性别: %s  年龄: %d\n", d->name, d->gender, d->age);
    printf("  身份证号: %s\n", d->idCard);
    printf("  驾照类型: %s  取证日期: %s\n", d->licenseType, d->obtainDate);
}

static void printViolation(const Violation *v, int idx) {
    printf("  ── 违章记录 #%d ──\n", idx + 1);
    printf("  车牌号: %s  驾驶员身份证: %s\n", v->plate, v->driverId);
    printf("  违章时间: %s\n", v->vtime);
    printf("  违章地点: %s\n", v->location);
    printf("  违章行为: %s\n", v->description);
    printf("  处理状态: ");
    if (v->handled) cPrint(C_GREEN, "已处理\n");
    else cPrint(C_RED, "未处理\n");
}

static void printStatistics(void) {
    printTitle("违章统计信息");
    printDivider();
    int totalHandled = 0, totalUnhandled = 0;
    for (int i = 0; i < g_violations.count; i++) {
        if (g_violations.data[i].handled) totalHandled++;
        else totalUnhandled++;
    }
    printf("  车辆总数:     "); cPrint(C_WHITE, "%d\n", g_vehicles.count);
    printf("  驾驶员总数:   "); cPrint(C_WHITE, "%d\n", g_drivers.count);
    printf("  违章记录总数: "); cPrint(C_WHITE, "%d\n", g_violations.count);
    printf("  已处理违章:   "); cPrint(C_GREEN, "%d\n", totalHandled);
    printf("  未处理违章:   "); cPrint(C_RED, "%d\n", totalUnhandled);
    printDivider();
}

/* ======================== 交互式菜单功能实现 ======================== */

static void doAddVehicle(void) {
    printTitle("添加车辆信息");
    Vehicle v;
    memset(&v, 0, sizeof(v));
    readString("请输入车牌号: ", v.plate, MAX_STR);
    if (v.plate[0] == '\0') { printErr("车牌号不能为空"); return; }
    if (findVehicleByPlate(v.plate) >= 0) { printErr("该车牌号已存在"); return; }
    readString("请输入VIN码: ", v.vin, MAX_STR);
    readString("请输入品牌: ", v.brand, MAX_STR);
    readString("请输入型号: ", v.model, MAX_STR);
    readString("请输入颜色: ", v.color, MAX_STR);
    readString("请输入车辆类型(如轿车/SUV/货车): ", v.vtype, MAX_STR);
    readString("请输入生产日期(YYYY-MM-DD): ", v.prodDate, MAX_STR);
    readString("请输入注册日期(YYYY-MM-DD): ", v.regDate, MAX_STR);
    readString("请输入车主姓名: ", v.ownerName, MAX_STR);
    readString("请输入车主身份证号: ", v.ownerId, MAX_STR);
    VioStatus s = addVehicle(&v);
    if (s == VIO_OK) {
        saveToFile(g_dataFile);
        printOK("车辆信息添加成功");
    } else printErr("添加失败");
}

static void doListVehicles(void) {
    printTitle("所有车辆信息");
    if (g_vehicles.count == 0) { printWarn("暂无车辆信息"); return; }
    for (int i = 0; i < g_vehicles.count; i++) {
        printVehicle(&g_vehicles.data[i], i);
    }
    printf("\n  共 %d 条车辆记录\n", g_vehicles.count);
}

static void doSearchVehicle(void) {
    printTitle("查询车辆信息");
    char plate[MAX_STR];
    readString("请输入要查询的车牌号: ", plate, MAX_STR);
    int idx = findVehicleByPlate(plate);
    if (idx < 0) { printErr("未找到该车辆"); return; }
    printVehicle(&g_vehicles.data[idx], idx);
    int h, u;
    int total = countViolationsByPlate(plate, &h, &u);
    printf("\n  该车辆违章次数: %d (已处理: %d, 未处理: %d)\n", total, h, u);
}

static void doDeleteVehicle(void) {
    printTitle("删除车辆信息");
    char plate[MAX_STR];
    readString("请输入要删除的车牌号: ", plate, MAX_STR);
    int idx = findVehicleByPlate(plate);
    if (idx < 0) { printErr("未找到该车辆"); return; }
    printVehicle(&g_vehicles.data[idx], idx);
    if (!readYesNo("确认删除该车辆?")) { printInfo("已取消删除"); return; }
    deleteVehicle(idx);
    saveToFile(g_dataFile);
    printOK("车辆信息删除成功");
}

static void doAddDriver(void) {
    printTitle("添加驾驶员信息");
    Driver d;
    memset(&d, 0, sizeof(d));
    readString("请输入姓名: ", d.name, MAX_STR);
    d.age = readInt("请输入年龄: ", 18, 100);
    readString("请输入性别(男/女): ", d.gender, MAX_STR);
    readString("请输入身份证号: ", d.idCard, MAX_STR);
    if (d.idCard[0] == '\0') { printErr("身份证号不能为空"); return; }
    if (findDriverByIdCard(d.idCard) >= 0) { printErr("该身份证号已存在"); return; }
    readString("请输入驾照类型(如A1/B1/C1): ", d.licenseType, MAX_STR);
    readString("请输入取证日期(YYYY-MM-DD): ", d.obtainDate, MAX_STR);
    VioStatus s = addDriver(&d);
    if (s == VIO_OK) {
        saveToFile(g_dataFile);
        printOK("驾驶员信息添加成功");
    } else printErr("添加失败");
}

static void doListDrivers(void) {
    printTitle("所有驾驶员信息");
    if (g_drivers.count == 0) { printWarn("暂无驾驶员信息"); return; }
    for (int i = 0; i < g_drivers.count; i++) {
        printDriver(&g_drivers.data[i], i);
    }
    printf("\n  共 %d 条驾驶员记录\n", g_drivers.count);
}

static void doSearchDriver(void) {
    printTitle("查询驾驶员信息");
    char idCard[MAX_STR];
    readString("请输入要查询的身份证号: ", idCard, MAX_STR);
    int idx = findDriverByIdCard(idCard);
    if (idx < 0) { printErr("未找到该驾驶员"); return; }
    printDriver(&g_drivers.data[idx], idx);
    int h, u;
    int total = countViolationsByDriver(idCard, &h, &u);
    printf("\n  该驾驶员违章次数: %d (已处理: %d, 未处理: %d)\n", total, h, u);
}

static void doDeleteDriver(void) {
    printTitle("删除驾驶员信息");
    char idCard[MAX_STR];
    readString("请输入要删除的身份证号: ", idCard, MAX_STR);
    int idx = findDriverByIdCard(idCard);
    if (idx < 0) { printErr("未找到该驾驶员"); return; }
    printDriver(&g_drivers.data[idx], idx);
    if (!readYesNo("确认删除该驾驶员?")) { printInfo("已取消删除"); return; }
    deleteDriver(idx);
    saveToFile(g_dataFile);
    printOK("驾驶员信息删除成功");
}

static void doAddViolation(void) {
    printTitle("添加违章记录");
    Violation v;
    memset(&v, 0, sizeof(v));
    readString("请输入违章车牌号: ", v.plate, MAX_STR);
    if (findVehicleByPlate(v.plate) < 0) printWarn("警告: 该车牌号未在系统中登记");
    readString("请输入驾驶员身份证号: ", v.driverId, MAX_STR);
    if (findDriverByIdCard(v.driverId) < 0) printWarn("警告: 该驾驶员未在系统中登记");
    readString("请输入违章时间(YYYY-MM-DD HH:MM): ", v.vtime, MAX_STR);
    readString("请输入违章地点: ", v.location, MAX_STR);
    readString("请输入违章行为描述: ", v.description, MAX_DESC);
    v.handled = 0;
    VioStatus s = addViolation(&v);
    if (s == VIO_OK) {
        saveToFile(g_dataFile);
        printOK("违章记录添加成功");
    } else printErr("添加失败");
}

static void doListViolations(void) {
    printTitle("所有违章记录");
    if (g_violations.count == 0) { printWarn("暂无违章记录"); return; }
    char filter[64];
    readString("请输入筛选条件(1=全部/2=未处理/3=已处理): ", filter, sizeof(filter));
    int f = atoi(filter);
    if (f < 1 || f > 3) f = 1;
    int shown = 0;
    for (int i = 0; i < g_violations.count; i++) {
        if (f == 2 && g_violations.data[i].handled) continue;
        if (f == 3 && !g_violations.data[i].handled) continue;
        printViolation(&g_violations.data[i], i);
        shown++;
    }
    printf("\n  共显示 %d 条违章记录\n", shown);
}

static void doHandleViolation(void) {
    printTitle("处理违章");
    if (g_violations.count == 0) { printErr("暂无违章记录"); return; }
    int idx = readInt("请输入要处理的违章记录编号(1-N): ", 1, g_violations.count) - 1;
    if (g_violations.data[idx].handled) { printWarn("该违章已处理"); return; }
    printViolation(&g_violations.data[idx], idx);
    if (!readYesNo("确认处理该违章?")) { printInfo("已取消"); return; }
    handleViolation(idx);
    saveToFile(g_dataFile);
    printOK("违章已标记为处理");
}

static void doDeleteViolation(void) {
    printTitle("删除违章记录");
    if (g_violations.count == 0) { printErr("暂无违章记录"); return; }
    int idx = readInt("请输入要删除的违章记录编号(1-N): ", 1, g_violations.count) - 1;
    printViolation(&g_violations.data[idx], idx);
    if (!readYesNo("确认删除该违章记录?")) { printInfo("已取消删除"); return; }
    deleteViolation(idx);
    saveToFile(g_dataFile);
    printOK("违章记录删除成功");
}

static void doSearchViolations(void) {
    printTitle("查询违章记录");
    printf("  1. 按车牌号查询\n");
    printf("  2. 按驾驶员身份证查询\n");
    printf("  3. 查询未处理违章\n");
    int ch = readInt("请选择查询方式: ", 1, 3);
    int shown = 0;
    if (ch == 1) {
        char plate[MAX_STR];
        readString("请输入车牌号: ", plate, MAX_STR);
        for (int i = 0; i < g_violations.count; i++) {
            if (strcmp(g_violations.data[i].plate, plate) == 0) {
                printViolation(&g_violations.data[i], i);
                shown++;
            }
        }
    } else if (ch == 2) {
        char idCard[MAX_STR];
        readString("请输入身份证号: ", idCard, MAX_STR);
        for (int i = 0; i < g_violations.count; i++) {
            if (strcmp(g_violations.data[i].driverId, idCard) == 0) {
                printViolation(&g_violations.data[i], i);
                shown++;
            }
        }
    } else {
        for (int i = 0; i < g_violations.count; i++) {
            if (!g_violations.data[i].handled) {
                printViolation(&g_violations.data[i], i);
                shown++;
            }
        }
    }
    if (shown == 0) printWarn("未找到匹配的违章记录");
    else printf("\n  共找到 %d 条记录\n", shown);
}

static void doStatistics(void) {
    printStatistics();
}

/* ======================== 内置全面自测用例 (18个测试) ======================== */

#define TEST_ASSERT(cond, name) do { \
    if (!(cond)) { \
        cPrint(C_RED, "  [FAIL] %s (line %d)\n", name, __LINE__); \
        return 1; \
    } \
    cPrint(C_GREEN, "  [PASS] %s\n", name); \
    passed++; \
} while(0)

static int runSelfTests(void) {
    cPrint(C_BOLD C_CYAN, "\n  ===== 运行内置自测用例 =====\n\n");
    int passed = 0;

    /* --- 测试1: 系统初始化 --- */
    VioStatus s = initSystem();
    TEST_ASSERT(s == VIO_OK, "测试1: 系统初始化成功");

    /* --- 测试2: 添加车辆 --- */
    Vehicle v1 = {"京A12345", "LBV1234567890", "宝马", "X5", "黑色", "SUV", "2020-01-01", "2020-06-01", "张三", "110101199001011234"};
    s = addVehicle(&v1);
    TEST_ASSERT(s == VIO_OK, "测试2: 添加车辆成功");

    /* --- 测试3: 重复车牌检测 --- */
    s = addVehicle(&v1);
    TEST_ASSERT(s == VIO_ERR_DUPLICATE, "测试3: 重复车牌正确返回错误");

    /* --- 测试4: 查找车辆 --- */
    int idx = findVehicleByPlate("京A12345");
    TEST_ASSERT(idx >= 0, "测试4: 按车牌查找车辆成功");

    /* --- 测试5: 查找不存在的车辆 --- */
    idx = findVehicleByPlate("不存在的车牌");
    TEST_ASSERT(idx == -1, "测试5: 查找不存在车辆返回-1");

    /* --- 测试6: 添加驾驶员 --- */
    Driver d1 = {"张三", 30, "男", "110101199001011234", "C1", "2018-05-20"};
    s = addDriver(&d1);
    TEST_ASSERT(s == VIO_OK, "测试6: 添加驾驶员成功");

    /* --- 测试7: 重复身份证检测 --- */
    s = addDriver(&d1);
    TEST_ASSERT(s == VIO_ERR_DUPLICATE, "测试7: 重复身份证正确返回错误");

    /* --- 测试8: 添加违章记录 --- */
    Violation vio1 = {"京A12345", "110101199001011234", "2026-01-15 14:30", "北京市朝阳区", "闯红灯", 0};
    s = addViolation(&vio1);
    TEST_ASSERT(s == VIO_OK, "测试8: 添加违章记录成功");

    /* --- 测试9: 多条违章记录与自动扩容 --- */
    for (int i = 0; i < 10; i++) {
        Violation v = {"京A12345", "110101199001011234", "2026-01-01 00:00", "测试地点", "测试违章", 0};
        addViolation(&v);
    }
    TEST_ASSERT(g_violations.count == 11, "测试9: 动态数组自动扩容(11条记录)");

    /* --- 测试10: 违章统计 --- */
    int h, u;
    int total = countViolationsByPlate("京A12345", &h, &u);
    TEST_ASSERT(total == 11 && h == 0 && u == 11, "测试10: 按车牌违章统计正确");

    /* --- 测试11: 处理违章 --- */
    s = handleViolation(0);
    TEST_ASSERT(s == VIO_OK && g_violations.data[0].handled == 1, "测试11: 处理违章成功");

    /* --- 测试12: 处理后统计更新 --- */
    total = countViolationsByPlate("京A12345", &h, &u);
    TEST_ASSERT(h == 1 && u == 10, "测试12: 处理后统计更新正确");

    /* --- 测试13: 文件保存 --- */
    const char *testFile = "_test_vio_data.vio";
    s = saveToFile(testFile);
    TEST_ASSERT(s == VIO_OK && fileExists(testFile), "测试13: 保存到文件成功");

    /* --- 测试14: 文件加载 --- */
    freeSystem();
    s = initSystem();
    s = loadFromFile(testFile);
    TEST_ASSERT(s == VIO_OK, "测试14: 从文件加载成功");

    /* --- 测试15: 加载后数据完整性 --- */
    TEST_ASSERT(g_vehicles.count == 1 && g_drivers.count == 1 && g_violations.count == 11,
                "测试15: 加载后数据完整性验证");
    TEST_ASSERT(strcmp(g_vehicles.data[0].plate, "京A12345") == 0, "测试15b: 车辆数据正确");
    TEST_ASSERT(strcmp(g_drivers.data[0].name, "张三") == 0, "测试15c: 驾驶员数据正确");
    TEST_ASSERT(g_violations.data[0].handled == 1, "测试15d: 违章处理状态正确");

    /* --- 测试16: 魔数错误检测 --- */
    FILE *fp = fopen("_test_bad.vio", "wb");
    if (fp) {
        char buf[sizeof(VioHeader)];
        memset(buf, 0, sizeof(buf));
        memcpy(buf, "BADMAG", 6);
        fwrite(buf, 1, sizeof(buf), fp);
        fclose(fp);
    }
    freeSystem(); initSystem();
    s = loadFromFile("_test_bad.vio");
    TEST_ASSERT(s == VIO_ERR_MAGIC, "测试16: 魔数错误正确检测");
    remove("_test_bad.vio");

    /* --- 测试17: 删除车辆 --- */
    freeSystem(); initSystem();
    addVehicle(&v1);
    idx = findVehicleByPlate("京A12345");
    s = deleteVehicle(idx);
    TEST_ASSERT(s == VIO_OK && g_vehicles.count == 0, "测试17: 删除车辆成功");

    /* --- 测试18: 删除不存在的记录 --- */
    s = deleteVehicle(999);
    TEST_ASSERT(s == VIO_ERR_NOT_FOUND, "测试18: 删除不存在记录正确返回错误");

    /* 清理测试文件 */
    remove(testFile);
    freeSystem();

    printThickDivider();
    cPrint(C_BOLD C_GREEN, "  所有测试通过! %d/18 ✓\n\n", passed);
    return 0;
}

/* ======================== 帮助信息 ======================== */

static void printUsage(const char *prog) {
    printBanner();
    printf("机动车违章信息管理系统 V2.0 - Enhanced Edition\n\n");
    cPrint(C_BOLD, "用法:\n");
    printf("  %s                     交互模式（菜单）\n", prog);
    printf("  %s -h, --help          显示帮助\n", prog);
    printf("  %s -t, --test          运行内置自测用例\n", prog);
    printf("  %s -f <数据文件>       指定加载的数据文件\n", prog);
    printf("  %s -o <输出文件>       指定导出的文件\n", prog);
    printf("\n");
    cPrint(C_BOLD, "V2 文件格式:\n");
    printf("  使用二进制格式存储，魔数 \"VIO V2\" 标识\n");
    printf("  默认文件: %s\n", DEFAULT_FILE);
    printf("\n");
}

/* ======================== 交互菜单系统 ======================== */

static void showMainMenu(void) {
    printDivider();
    cPrint(C_BOLD C_BLUE, "  主菜单\n");
    printDivider();
    cPrint(C_CYAN, "  1 "); printf("- 车辆信息管理\n");
    cPrint(C_CYAN, "  2 "); printf("- 驾驶员信息管理\n");
    cPrint(C_CYAN, "  3 "); printf("- 违章记录管理\n");
    cPrint(C_CYAN, "  4 "); printf("- 违章查询\n");
    cPrint(C_CYAN, "  5 "); printf("- 统计信息\n");
    cPrint(C_CYAN, "  S "); printf("- 保存数据\n");
    cPrint(C_CYAN, "  H/?"); printf("- 帮助信息\n");
    cPrint(C_CYAN, "  0 "); printf("- 退出系统\n");
    printDivider();
    printf("  当前状态: 车辆 ");
    cPrint(g_vehicles.count>0?C_GREEN:C_YELLOW, "%d", g_vehicles.count);
    printf(" | 驾驶员 ");
    cPrint(g_drivers.count>0?C_GREEN:C_YELLOW, "%d", g_drivers.count);
    printf(" | 违章 ");
    cPrint(g_violations.count>0?C_GREEN:C_YELLOW, "%d", g_violations.count);
    printf(" | 文件: %s\n", g_dataFile);
    printDivider();
}

static void vehicleSubMenu(void) {
    while (1) {
        printf("\n");
        cPrint(C_BOLD C_CYAN, "  -- 车辆信息管理 --\n");
        printf("  1. 添加车辆\n");
        printf("  2. 查看所有车辆\n");
        printf("  3. 查询车辆\n");
        printf("  4. 删除车辆\n");
        printf("  0. 返回主菜单\n");
        printDivider();
        int ch = readInt("请选择: ", 0, 4);
        switch (ch) {
            case 1: doAddVehicle(); break;
            case 2: doListVehicles(); break;
            case 3: doSearchVehicle(); break;
            case 4: doDeleteVehicle(); break;
            case 0: return;
        }
    }
}

static void driverSubMenu(void) {
    while (1) {
        printf("\n");
        cPrint(C_BOLD C_CYAN, "  -- 驾驶员信息管理 --\n");
        printf("  1. 添加驾驶员\n");
        printf("  2. 查看所有驾驶员\n");
        printf("  3. 查询驾驶员\n");
        printf("  4. 删除驾驶员\n");
        printf("  0. 返回主菜单\n");
        printDivider();
        int ch = readInt("请选择: ", 0, 4);
        switch (ch) {
            case 1: doAddDriver(); break;
            case 2: doListDrivers(); break;
            case 3: doSearchDriver(); break;
            case 4: doDeleteDriver(); break;
            case 0: return;
        }
    }
}

static void violationSubMenu(void) {
    while (1) {
        printf("\n");
        cPrint(C_BOLD C_CYAN, "  -- 违章记录管理 --\n");
        printf("  1. 添加违章记录\n");
        printf("  2. 查看违章记录\n");
        printf("  3. 处理违章\n");
        printf("  4. 删除违章记录\n");
        printf("  0. 返回主菜单\n");
        printDivider();
        int ch = readInt("请选择: ", 0, 4);
        switch (ch) {
            case 1: doAddViolation(); break;
            case 2: doListViolations(); break;
            case 3: doHandleViolation(); break;
            case 4: doDeleteViolation(); break;
            case 0: return;
        }
    }
}

static void runMenu(void) {
    char choice[64];
    while (1) {
        showMainMenu();
        printf("  请选择操作: "); fflush(stdout);
        if (fgets(choice, sizeof(choice), stdin) == NULL) break;
        char *nl = strchr(choice, '\n'); if (nl) *nl = '\0';
        if (choice[0] == '\0') continue;
        char c = (char)toupper((unsigned char)choice[0]);
        switch (c) {
        case '1': vehicleSubMenu(); break;
        case '2': driverSubMenu(); break;
        case '3': violationSubMenu(); break;
        case '4': doSearchViolations(); break;
        case '5': doStatistics(); break;
        case 'S':
            saveToFile(g_dataFile);
            printOK("数据已保存");
            break;
        case 'H':
        case '?':
            printUsage("vio.exe");
            break;
        case '0':
            saveToFile(g_dataFile);
            cPrint(C_GREEN, "\n  [OK] 数据已保存，感谢使用，再见！\n\n");
            return;
        default:
            cPrint(C_YELLOW, "  [!] 无效选项，请重新输入\n"); break;
        }
    }
}

/* ======================== 命令行参数处理 ======================== */

static int runCommandLine(int argc, char *argv[]) {
    const char *inputFile = NULL;
    const char *outputFile = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--test") == 0) {
            return runSelfTests();
        }
        if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            inputFile = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            outputFile = argv[++i];
            continue;
        }
    }

    if (inputFile) {
        VioStatus s = loadFromFile(inputFile);
        if (s == VIO_OK) {
            printOK("文件加载成功");
        } else {
            printErr("文件加载失败");
            return 1;
        }
    }

    if (outputFile) {
        VioStatus s = saveToFile(outputFile);
        if (s == VIO_OK) {
            printOK("文件导出成功");
        } else {
            printErr("文件导出失败");
            return 1;
        }
        return 0;
    }

    return 0;
}

/* ======================== 程序入口 ======================== */

int main(int argc, char *argv[]) {
    enableVTMode();

    if (argc > 1) {
        VioStatus s = initSystem();
        if (s != VIO_OK) {
            printErr("系统初始化失败");
            return 1;
        }
        int ret = runCommandLine(argc, argv);
        freeSystem();
        return ret;
    }

    printBanner();

    VioStatus s = initSystem();
    if (s != VIO_OK) {
        printErr("系统初始化失败");
        return 1;
    }

    if (fileExists(g_dataFile)) {
        s = loadFromFile(g_dataFile);
        if (s == VIO_OK) {
            printInfo("已加载现有数据文件");
        } else if (s == VIO_ERR_MAGIC) {
            printWarn("检测到旧格式数据文件，将使用V2格式保存");
            freeSystem();
            initSystem();
        }
    } else {
        printInfo("未找到数据文件，将创建新的数据库");
    }

    runMenu();

    saveToFile(g_dataFile);
    freeSystem();
    return 0;
}

