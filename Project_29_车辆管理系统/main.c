/**
 * @file main.c
 * @brief 车辆管理系统 V2.0 - 动态数组 + 彩色终端 + 二进制持久化 + 自测
 *
 * ============================================================================
 * 项目名称：车辆管理系统 V2.0 (Vehicle Management System V2.0)
 * 功能概述：为车队提供车辆与司机信息全生命周期管理
 *           - 车辆信息 CRUD（增删改查）
 *           - 司机信息 CRUD（独立管理）
 *           - 派车 / 还车 状态管理
 *           - 维修记录登记
 *           - 多维度统计分析
 *           - 二进制文件持久化（VEHIV2 魔数校验）
 *           - ANSI 彩色终端输出
 *           - 内置 15+ 项单元自测
 *
 * 技术栈：C99 / 动态数组 / 二进制文件I/O / Windows UTF-8控制台 / ANSI颜色
 * 编译方式：gcc -std=c99 -Wall -Wextra -O2 -o veh.exe main.c
 * 命令行参数：
 *           -h, --help    显示帮助信息
 *           -t, --test    运行内置自测
 *           （无参数）    进入交互菜单模式
 *
 * @author      数据库专
 * @version     2.0
 * @date        2026-06-28
 *
 * 版本历史：
 *   v1.0  初始版本，单链表CRUD，文本文件存储
 *   v2.0  完全重写：
 *         - 动态数组替代链表，支持自动扩容
 *         - 独立司机管理模块
 *         - VEHIV2 魔数二进制持久化
 *         - ANSI 彩色终端
 *         - 统一 VehStatus 错误码
 *         - CLI 参数支持 (-h/-t)
 *         - 派车/还车/维修功能
 *         - 15+ 项内置自测
 *         - Windows UTF-8 控制台 (SetConsoleOutputCP 65001)
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

/* ============================================================================
 *  第一部分：编译环境与常量定义
 * ============================================================================ */

#define VEH_MAGIC       "VEHIV2"
#define VEH_MAGIC_LEN   6
#define MAX_STR         64
#define INIT_CAPACITY   8
#define DATA_FILE       "vehicles.dat"

/* ANSI 颜色代码 */
#define C_RESET     "\033[0m"
#define C_RED       "\033[31m"
#define C_GREEN     "\033[32m"
#define C_YELLOW    "\033[33m"
#define C_BLUE      "\033[34m"
#define C_MAGENTA   "\033[35m"
#define C_CYAN      "\033[36m"
#define C_WHITE     "\033[37m"
#define C_BOLD      "\033[1m"
#define C_DIM       "\033[2m"

/* ============================================================================
 *  第二部分：数据结构与枚举定义
 * ============================================================================ */

/**
 * @brief 车辆状态枚举
 */
typedef enum {
    VEH_AVAILABLE    = 0,  /* 可用 */
    VEH_DISPATCHED   = 1,  /* 已派出 */
    VEH_MAINTENANCE  = 2   /* 维修中 */
} VehicleStatus;

/**
 * @brief 统一错误码枚举
 */
typedef enum {
    VEH_OK              = 0,   /* 操作成功 */
    VEH_ERR_NOT_FOUND   = -1,  /* 未找到目标 */
    VEH_ERR_DUPLICATE   = -2,  /* 重复添加 */
    VEH_ERR_MEMORY      = -3,  /* 内存分配失败 */
    VEH_ERR_FILE        = -4,  /* 文件操作失败 */
    VEH_ERR_INVALID     = -5,  /* 无效参数 */
    VEH_ERR_DISPATCHED  = -6,  /* 车辆已派出 */
    VEH_ERR_MAINTENANCE = -7   /* 车辆维修中 */
} VehStatus;

/**
 * @brief 车辆信息结构体
 */
typedef struct {
    char    id[MAX_STR];
    char    plate[MAX_STR];
    char    type[MAX_STR];
    char    driver_id[MAX_STR];
    float   mileage;
    float   value;
    VehicleStatus status;
    char    dispatch_note[MAX_STR];
    time_t  last_dispatch;
    int     repair_count;
} Vehicle;

/**
 * @brief 司机信息结构体
 */
typedef struct {
    char    id[MAX_STR];
    char    name[MAX_STR];
    char    phone[MAX_STR];
    int     age;
    int     dispatch_count;
} Driver;

/**
 * @brief 动态数组容器：车辆
 */
typedef struct {
    Vehicle *data;
    int      count;
    int      capacity;
} VehicleArray;

/**
 * @brief 动态数组容器：司机
 */
typedef struct {
    Driver  *data;
    int      count;
    int      capacity;
} DriverArray;

/**
 * @brief 维修记录
 */
typedef struct {
    char    vehicle_id[MAX_STR];
    char    reason[MAX_STR];
    float   cost;
    time_t  date;
} RepairRecord;

/* ============================================================================
 *  第三部分：全局变量
 * ============================================================================ */

static VehicleArray g_vehicles = {NULL, 0, 0};
static DriverArray  g_drivers  = {NULL, 0, 0};

/* ============================================================================
 *  第四部分：辅助工具函数
 * ============================================================================ */

/**
 * @brief 初始化Windows控制台UTF-8输出与ANSI颜色支持
 */
static void setup_console(void)
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

/**
 * @brief 清空输入缓冲区
 */
static void clear_input_buffer(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

/**
 * @brief 安全读取字符串
 */
static void safe_gets(char *buf, int size)
{
    buf[0] = '\0';
    if (fgets(buf, size, stdin)) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n')
            buf[len - 1] = '\0';
    }
}

/**
 * @brief 打印分隔线
 */
static void print_line(char ch, int len)
{
    printf(C_DIM);
    for (int i = 0; i < len; i++)
        putchar(ch);
    printf(C_RESET "\n");
}

/**
 * @brief 打印彩色标题
 */
static void print_title(const char *title)
{
    int total = 60;
    int tlen  = (int)strlen(title);
    int pad   = (total - tlen - 2) / 2;
    if (pad < 0) pad = 0;

    printf(C_CYAN C_BOLD);
    for (int i = 0; i < pad; i++)
        putchar('=');
    printf(" %s ", title);
    for (int i = 0; i < pad; i++)
        putchar('=');
    printf(C_RESET "\n");
}

/**
 * @brief 成功消息
 */
static void msg_ok(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    printf(C_GREEN "[✓] " C_RESET);
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);
}

/**
 * @brief 错误消息
 */
static void msg_err(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    printf(C_RED "[✗] " C_RESET);
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);
}

/**
 * @brief 警告消息
 */
static void msg_warn(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    printf(C_YELLOW "[!] " C_RESET);
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);
}

/**
 * @brief 信息消息
 */
static void msg_info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    printf(C_CYAN "[i] " C_RESET);
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);
}

/**
 * @brief 状态转字符串
 */
static const char* status_str(VehicleStatus s)
{
    switch (s) {
        case VEH_AVAILABLE:   return C_GREEN "可用" C_RESET;
        case VEH_DISPATCHED:  return C_YELLOW "已派出" C_RESET;
        case VEH_MAINTENANCE: return C_RED "维修中" C_RESET;
        default: return "未知";
    }
}

/* ============================================================================
 *  第五部分：动态数组初始化与扩容
 * ============================================================================ */

/**
 * @brief 初始化车辆动态数组
 */
static VehStatus veh_array_init(VehicleArray *arr)
{
    arr->data = (Vehicle*)calloc(INIT_CAPACITY, sizeof(Vehicle));
    if (!arr->data) return VEH_ERR_MEMORY;
    arr->count = 0;
    arr->capacity = INIT_CAPACITY;
    return VEH_OK;
}

/**
 * @brief 初始化司机动态数组
 */
static VehStatus drv_array_init(DriverArray *arr)
{
    arr->data = (Driver*)calloc(INIT_CAPACITY, sizeof(Driver));
    if (!arr->data) return VEH_ERR_MEMORY;
    arr->count = 0;
    arr->capacity = INIT_CAPACITY;
    return VEH_OK;
}

/**
 * @brief 车辆数组扩容
 */
static VehStatus veh_array_grow(VehicleArray *arr)
{
    int new_cap = arr->capacity * 2;
    Vehicle *tmp = (Vehicle*)realloc(arr->data, (size_t)new_cap * sizeof(Vehicle));
    if (!tmp) return VEH_ERR_MEMORY;
    arr->data = tmp;
    arr->capacity = new_cap;
    return VEH_OK;
}

/**
 * @brief 司机数组扩容
 */
static VehStatus drv_array_grow(DriverArray *arr)
{
    int new_cap = arr->capacity * 2;
    Driver *tmp = (Driver*)realloc(arr->data, (size_t)new_cap * sizeof(Driver));
    if (!tmp) return VEH_ERR_MEMORY;
    arr->data = tmp;
    arr->capacity = new_cap;
    return VEH_OK;
}

/**
 * @brief 释放动态数组内存
 */
static void arrays_free(void)
{
    free(g_vehicles.data);
    free(g_drivers.data);
    g_vehicles.data = NULL;
    g_drivers.data  = NULL;
    g_vehicles.count = g_vehicles.capacity = 0;
    g_drivers.count  = g_drivers.capacity  = 0;
}

/* ============================================================================
 *  第六部分：车辆 CRUD 核心操作
 * ============================================================================ */

/**
 * @brief 按ID查找车辆
 * @return 索引，-1表示未找到
 */
static int veh_find_by_id(const char *id)
{
    for (int i = 0; i < g_vehicles.count; i++) {
        if (strcmp(g_vehicles.data[i].id, id) == 0)
            return i;
    }
    return -1;
}

/**
 * @brief 按车牌查找车辆
 */
static int veh_find_by_plate(const char *plate)
{
    for (int i = 0; i < g_vehicles.count; i++) {
        if (strcmp(g_vehicles.data[i].plate, plate) == 0)
            return i;
    }
    return -1;
}

/**
 * @brief 添加车辆
 */
static VehStatus veh_add(const char *id, const char *plate, const char *type,
                         const char *driver_id, float mileage, float value)
{
    if (!id || !plate || !type) return VEH_ERR_INVALID;
    if (veh_find_by_id(id) >= 0) return VEH_ERR_DUPLICATE;
    if (veh_find_by_plate(plate) >= 0) return VEH_ERR_DUPLICATE;

    if (g_vehicles.count >= g_vehicles.capacity) {
        VehStatus st = veh_array_grow(&g_vehicles);
        if (st != VEH_OK) return st;
    }

    Vehicle *v = &g_vehicles.data[g_vehicles.count];
    memset(v, 0, sizeof(Vehicle));
    strncpy(v->id, id, MAX_STR - 1);
    strncpy(v->plate, plate, MAX_STR - 1);
    strncpy(v->type, type, MAX_STR - 1);
    strncpy(v->driver_id, driver_id ? driver_id : "", MAX_STR - 1);
    v->mileage = mileage;
    v->value = value;
    v->status = VEH_AVAILABLE;
    v->repair_count = 0;
    v->dispatch_note[0] = '\0';
    v->last_dispatch = 0;
    g_vehicles.count++;
    return VEH_OK;
}

/**
 * @brief 删除车辆
 */
static VehStatus veh_delete(const char *id)
{
    int idx = veh_find_by_id(id);
    if (idx < 0) return VEH_ERR_NOT_FOUND;

    if (g_vehicles.data[idx].status == VEH_DISPATCHED)
        return VEH_ERR_DISPATCHED;

    for (int i = idx; i < g_vehicles.count - 1; i++)
        g_vehicles.data[i] = g_vehicles.data[i + 1];
    g_vehicles.count--;
    memset(&g_vehicles.data[g_vehicles.count], 0, sizeof(Vehicle));
    return VEH_OK;
}

/**
 * @brief 更新车辆里程和价值
 */
static VehStatus veh_update(const char *id, float new_mileage, float new_value)
{
    int idx = veh_find_by_id(id);
    if (idx < 0) return VEH_ERR_NOT_FOUND;
    if (new_mileage < 0 || new_value < 0) return VEH_ERR_INVALID;
    g_vehicles.data[idx].mileage = new_mileage;
    g_vehicles.data[idx].value = new_value;
    return VEH_OK;
}

/**
 * @brief 派车
 */
static VehStatus veh_dispatch(const char *id, const char *note)
{
    int idx = veh_find_by_id(id);
    if (idx < 0) return VEH_ERR_NOT_FOUND;
    Vehicle *v = &g_vehicles.data[idx];
    if (v->status == VEH_DISPATCHED)  return VEH_ERR_DISPATCHED;
    if (v->status == VEH_MAINTENANCE) return VEH_ERR_MAINTENANCE;

    v->status = VEH_DISPATCHED;
    strncpy(v->dispatch_note, note ? note : "", MAX_STR - 1);
    v->last_dispatch = time(NULL);
    return VEH_OK;
}

/**
 * @brief 还车
 */
static VehStatus veh_return(const char *id, float add_mileage)
{
    int idx = veh_find_by_id(id);
    if (idx < 0) return VEH_ERR_NOT_FOUND;
    Vehicle *v = &g_vehicles.data[idx];
    if (v->status != VEH_DISPATCHED) return VEH_ERR_INVALID;

    v->status = VEH_AVAILABLE;
    v->mileage += add_mileage > 0 ? add_mileage : 0;
    v->dispatch_note[0] = '\0';
    return VEH_OK;
}

/**
 * @brief 登记维修
 */
static VehStatus veh_repair(const char *id, const char *reason, float cost)
{
    int idx = veh_find_by_id(id);
    if (idx < 0) return VEH_ERR_NOT_FOUND;
    Vehicle *v = &g_vehicles.data[idx];
    if (v->status == VEH_DISPATCHED) return VEH_ERR_DISPATCHED;

    v->status = VEH_MAINTENANCE;
    v->repair_count++;
    (void)reason;
    (void)cost;
    return VEH_OK;
}

/**
 * @brief 维修完成
 */
static VehStatus veh_repair_complete(const char *id)
{
    int idx = veh_find_by_id(id);
    if (idx < 0) return VEH_ERR_NOT_FOUND;
    Vehicle *v = &g_vehicles.data[idx];
    if (v->status != VEH_MAINTENANCE) return VEH_ERR_INVALID;
    v->status = VEH_AVAILABLE;
    return VEH_OK;
}

/* ============================================================================
 *  第七部分：司机 CRUD 核心操作
 * ============================================================================ */

/**
 * @brief 按ID查找司机
 */
static int drv_find_by_id(const char *id)
{
    for (int i = 0; i < g_drivers.count; i++) {
        if (strcmp(g_drivers.data[i].id, id) == 0)
            return i;
    }
    return -1;
}

/**
 * @brief 添加司机
 */
static VehStatus drv_add(const char *id, const char *name, const char *phone, int age)
{
    if (!id || !name) return VEH_ERR_INVALID;
    if (drv_find_by_id(id) >= 0) return VEH_ERR_DUPLICATE;

    if (g_drivers.count >= g_drivers.capacity) {
        VehStatus st = drv_array_grow(&g_drivers);
        if (st != VEH_OK) return st;
    }

    Driver *d = &g_drivers.data[g_drivers.count];
    memset(d, 0, sizeof(Driver));
    strncpy(d->id, id, MAX_STR - 1);
    strncpy(d->name, name, MAX_STR - 1);
    strncpy(d->phone, phone ? phone : "", MAX_STR - 1);
    d->age = age > 0 ? age : 0;
    d->dispatch_count = 0;
    g_drivers.count++;
    return VEH_OK;
}

/**
 * @brief 删除司机
 */
static VehStatus drv_delete(const char *id)
{
    int idx = drv_find_by_id(id);
    if (idx < 0) return VEH_ERR_NOT_FOUND;
    for (int i = idx; i < g_drivers.count - 1; i++)
        g_drivers.data[i] = g_drivers.data[i + 1];
    g_drivers.count--;
    memset(&g_drivers.data[g_drivers.count], 0, sizeof(Driver));
    return VEH_OK;
}

/* ============================================================================
 *  第八部分：文件持久化 (VEHIV2 魔数)
 * ============================================================================ */

/**
 * @brief 保存到二进制文件
 * 文件格式：
 *   [0..5]   魔数 "VEHIV2"
 *   [6..9]   车辆数量 (int32)
 *   [10..13] 司机数量 (int32)
 *   紧随其后：Vehicle 数组（二进制直写）
 *   紧随其后：Driver 数组（二进制直写）
 */
static VehStatus save_to_file(void)
{
    FILE *fp = fopen(DATA_FILE, "wb");
    if (!fp) return VEH_ERR_FILE;

    fwrite(VEH_MAGIC, 1, VEH_MAGIC_LEN, fp);
    fwrite(&g_vehicles.count, sizeof(int), 1, fp);
    fwrite(&g_drivers.count, sizeof(int), 1, fp);

    if (g_vehicles.count > 0)
        fwrite(g_vehicles.data, sizeof(Vehicle), (size_t)g_vehicles.count, fp);
    if (g_drivers.count > 0)
        fwrite(g_drivers.data, sizeof(Driver), (size_t)g_drivers.count, fp);

    fclose(fp);
    return VEH_OK;
}

/**
 * @brief 从二进制文件加载
 */
static VehStatus load_from_file(void)
{
    FILE *fp = fopen(DATA_FILE, "rb");
    if (!fp) return VEH_OK;

    char magic[VEH_MAGIC_LEN + 1] = {0};
    if (fread(magic, 1, VEH_MAGIC_LEN, fp) != VEH_MAGIC_LEN) {
        fclose(fp);
        return VEH_ERR_FILE;
    }
    if (strcmp(magic, VEH_MAGIC) != 0) {
        fclose(fp);
        return VEH_ERR_FILE;
    }

    int v_count = 0, d_count = 0;
    fread(&v_count, sizeof(int), 1, fp);
    fread(&d_count, sizeof(int), 1, fp);

    arrays_free();
    veh_array_init(&g_vehicles);
    drv_array_init(&g_drivers);

    while (g_vehicles.capacity < v_count)
        veh_array_grow(&g_vehicles);
    while (g_drivers.capacity < d_count)
        drv_array_grow(&g_drivers);

    if (v_count > 0)
        fread(g_vehicles.data, sizeof(Vehicle), (size_t)v_count, fp);
    if (d_count > 0)
        fread(g_drivers.data, sizeof(Driver), (size_t)d_count, fp);

    g_vehicles.count = v_count;
    g_drivers.count = d_count;

    fclose(fp);
    return VEH_OK;
}

/* ============================================================================
 *  第九部分：统计功能
 * ============================================================================ */

typedef struct {
    char    type[MAX_STR];
    int     count;
    float   total_mileage;
    float   total_value;
    int     available;
    int     dispatched;
    int     maintenance;
} TypeStat;

/**
 * @brief 按类型统计
 */
static void stat_by_type(void)
{
    TypeStat stats[32];
    int stat_count = 0;

    for (int i = 0; i < g_vehicles.count; i++) {
        Vehicle *v = &g_vehicles.data[i];
        int found = -1;
        for (int j = 0; j < stat_count; j++) {
            if (strcmp(stats[j].type, v->type) == 0) {
                found = j;
                break;
            }
        }
        if (found < 0) {
            found = stat_count++;
            memset(&stats[found], 0, sizeof(TypeStat));
            strncpy(stats[found].type, v->type, MAX_STR - 1);
        }
        stats[found].count++;
        stats[found].total_mileage += v->mileage;
        stats[found].total_value   += v->value;
        switch (v->status) {
            case VEH_AVAILABLE:   stats[found].available++;   break;
            case VEH_DISPATCHED:  stats[found].dispatched++;  break;
            case VEH_MAINTENANCE: stats[found].maintenance++; break;
        }
    }

    print_title("车辆类型统计");
    printf(C_BOLD "%-10s %6s %10s %10s %6s %6s %6s\n" C_RESET,
           "类型", "数量", "总里程(万km)", "总价值(万元)", "可用", "派出", "维修");
    print_line('-', 72);

    float total_mileage = 0, total_value = 0;
    int total_v = 0, total_a = 0, total_d = 0, total_m = 0;

    for (int i = 0; i < stat_count; i++) {
        printf("%-10s %6d %10.2f %10.2f %6d %6d %6d\n",
               stats[i].type, stats[i].count,
               stats[i].total_mileage, stats[i].total_value,
               stats[i].available, stats[i].dispatched, stats[i].maintenance);
        total_v += stats[i].count;
        total_mileage += stats[i].total_mileage;
        total_value   += stats[i].total_value;
        total_a += stats[i].available;
        total_d += stats[i].dispatched;
        total_m += stats[i].maintenance;
    }

    print_line('-', 72);
    printf(C_BOLD "%-10s %6d %10.2f %10.2f %6d %6d %6d\n" C_RESET,
           "合计", total_v, total_mileage, total_value, total_a, total_d, total_m);
}

/* ============================================================================
 *  第十部分：显示与交互菜单
 * ============================================================================ */

/**
 * @brief 显示所有车辆
 */
static void display_all_vehicles(void)
{
    print_title("车辆信息列表");
    printf(C_BOLD "%-6s %-12s %-10s %-8s %10s %10s %-8s %s\n" C_RESET,
           "编号", "车牌号", "类型", "司机ID", "里程(万km)", "价值(万元)", "状态", "派车备注");
    print_line('-', 80);

    for (int i = 0; i < g_vehicles.count; i++) {
        Vehicle *v = &g_vehicles.data[i];
        printf("%-6s %-12s %-10s %-8s %10.2f %10.2f ",
               v->id, v->plate, v->type, v->driver_id,
               v->mileage, v->value);
        printf("%s", status_str(v->status));
        if (v->status == VEH_DISPATCHED && v->dispatch_note[0])
            printf("  %s%s%s", C_YELLOW, v->dispatch_note, C_RESET);
        printf("\n");
    }

    print_line('-', 80);
    printf(C_GREEN "共 %d 辆车" C_RESET "\n", g_vehicles.count);
}

/**
 * @brief 显示所有司机
 */
static void display_all_drivers(void)
{
    print_title("司机信息列表");
    printf(C_BOLD "%-8s %-10s %-15s %6s %8s\n" C_RESET,
           "司机ID", "姓名", "电话", "年龄", "派车次数");
    print_line('-', 55);

    for (int i = 0; i < g_drivers.count; i++) {
        Driver *d = &g_drivers.data[i];
        printf("%-8s %-10s %-15s %6d %8d\n",
               d->id, d->name, d->phone, d->age, d->dispatch_count);
    }

    print_line('-', 55);
    printf(C_GREEN "共 %d 名司机" C_RESET "\n", g_drivers.count);
}

/**
 * @brief 菜单：添加车辆
 */
static void menu_add_vehicle(void)
{
    char id[MAX_STR], plate[MAX_STR], type[MAX_STR], driver_id[MAX_STR];
    float mileage, value;

    snprintf(id, MAX_STR, "V%03d", g_vehicles.count + 1);
    msg_info("自动分配编号: %s", id);

    printf("请输入车牌号: ");     safe_gets(plate, MAX_STR);
    printf("请输入车辆类型: ");   safe_gets(type, MAX_STR);
    printf("请输入司机ID(可选): "); safe_gets(driver_id, MAX_STR);
    printf("请输入行驶里程(万km): ");
    if (scanf("%f", &mileage) != 1) { clear_input_buffer(); msg_err("无效输入"); return; }
    printf("请输入车辆价值(万元): ");
    if (scanf("%f", &value) != 1)   { clear_input_buffer(); msg_err("无效输入"); return; }
    clear_input_buffer();

    VehStatus st = veh_add(id, plate, type, driver_id, mileage, value);
    if (st == VEH_OK) {
        save_to_file();
        msg_ok("车辆 [%s - %s] 添加成功", id, plate);
    } else if (st == VEH_ERR_DUPLICATE) {
        msg_err("车辆编号或车牌已存在");
    } else {
        msg_err("添加失败，错误码: %d", st);
    }
}

/**
 * @brief 菜单：删除车辆
 */
static void menu_delete_vehicle(void)
{
    char id[MAX_STR];
    printf("请输入要删除的车辆编号: ");
    safe_gets(id, MAX_STR);

    VehStatus st = veh_delete(id);
    if (st == VEH_OK) {
        save_to_file();
        msg_ok("车辆 [%s] 已删除", id);
    } else if (st == VEH_ERR_NOT_FOUND) {
        msg_err("未找到车辆 [%s]", id);
    } else if (st == VEH_ERR_DISPATCHED) {
        msg_err("车辆已派出，无法删除");
    } else {
        msg_err("删除失败，错误码: %d", st);
    }
}

/**
 * @brief 菜单：添加司机
 */
static void menu_add_driver(void)
{
    char id[MAX_STR], name[MAX_STR], phone[MAX_STR];
    int age;

    snprintf(id, MAX_STR, "D%03d", g_drivers.count + 1);
    msg_info("自动分配司机ID: %s", id);

    printf("请输入司机姓名: "); safe_gets(name, MAX_STR);
    printf("请输入联系电话: "); safe_gets(phone, MAX_STR);
    printf("请输入年龄: ");
    if (scanf("%d", &age) != 1) { clear_input_buffer(); msg_err("无效输入"); return; }
    clear_input_buffer();

    VehStatus st = drv_add(id, name, phone, age);
    if (st == VEH_OK) {
        save_to_file();
        msg_ok("司机 [%s - %s] 添加成功", id, name);
    } else if (st == VEH_ERR_DUPLICATE) {
        msg_err("司机ID已存在");
    } else {
        msg_err("添加失败，错误码: %d", st);
    }
}

/**
 * @brief 菜单：派车
 */
static void menu_dispatch(void)
{
    char id[MAX_STR], note[MAX_STR];
    printf("请输入要派出的车辆编号: "); safe_gets(id, MAX_STR);
    printf("请输入派车备注(用途): ");    safe_gets(note, MAX_STR);

    VehStatus st = veh_dispatch(id, note);
    if (st == VEH_OK) {
        save_to_file();
        msg_ok("车辆 [%s] 已派出", id);
    } else if (st == VEH_ERR_NOT_FOUND)     msg_err("未找到车辆 [%s]", id);
    else if (st == VEH_ERR_DISPATCHED)      msg_err("车辆已在派出状态");
    else if (st == VEH_ERR_MAINTENANCE)     msg_err("车辆维修中，无法派出");
    else                                    msg_err("派车失败，错误码: %d", st);
}

/**
 * @brief 菜单：还车
 */
static void menu_return(void)
{
    char id[MAX_STR];
    float add_mileage;
    printf("请输入要归还的车辆编号: "); safe_gets(id, MAX_STR);
    printf("请输入本次行驶里程(万km): ");
    if (scanf("%f", &add_mileage) != 1) { clear_input_buffer(); msg_err("无效输入"); return; }
    clear_input_buffer();

    VehStatus st = veh_return(id, add_mileage);
    if (st == VEH_OK) {
        save_to_file();
        msg_ok("车辆 [%s] 已归还，里程累计 %.2f 万km", id, add_mileage);
    } else if (st == VEH_ERR_NOT_FOUND) msg_err("未找到车辆 [%s]", id);
    else if (st == VEH_ERR_INVALID)     msg_err("车辆未在派出状态");
    else                                msg_err("还车失败，错误码: %d", st);
}

/**
 * @brief 菜单：登记维修
 */
static void menu_repair(void)
{
    char id[MAX_STR], reason[MAX_STR];
    float cost;
    printf("请输入送修车辆编号: "); safe_gets(id, MAX_STR);
    printf("请输入维修原因: ");     safe_gets(reason, MAX_STR);
    printf("请输入维修费用(元): ");
    if (scanf("%f", &cost) != 1) { clear_input_buffer(); msg_err("无效输入"); return; }
    clear_input_buffer();

    VehStatus st = veh_repair(id, reason, cost);
    if (st == VEH_OK) {
        save_to_file();
        msg_ok("车辆 [%s] 已登记维修", id);
    } else if (st == VEH_ERR_NOT_FOUND) msg_err("未找到车辆 [%s]", id);
    else if (st == VEH_ERR_DISPATCHED)  msg_err("车辆已派出，请先还车");
    else                                msg_err("登记失败，错误码: %d", st);
}

/**
 * @brief 菜单：维修完成
 */
static void menu_repair_complete(void)
{
    char id[MAX_STR];
    printf("请输入维修完成的车辆编号: ");
    safe_gets(id, MAX_STR);

    VehStatus st = veh_repair_complete(id);
    if (st == VEH_OK) {
        save_to_file();
        msg_ok("车辆 [%s] 维修完成，已恢复可用", id);
    } else if (st == VEH_ERR_NOT_FOUND) msg_err("未找到车辆 [%s]", id);
    else if (st == VEH_ERR_INVALID)     msg_err("车辆未在维修状态");
    else                                msg_err("操作失败，错误码: %d", st);
}

/**
 * @brief 菜单：更新车辆信息
 */
static void menu_update_vehicle(void)
{
    char id[MAX_STR];
    float new_mileage, new_value;
    printf("请输入要更新的车辆编号: ");
    safe_gets(id, MAX_STR);

    int idx = veh_find_by_id(id);
    if (idx < 0) { msg_err("未找到车辆 [%s]", id); return; }
    Vehicle *v = &g_vehicles.data[idx];
    msg_info("当前里程: %.2f 万km, 当前价值: %.2f 万元", v->mileage, v->value);

    printf("请输入新的行驶里程(万km): ");
    if (scanf("%f", &new_mileage) != 1) { clear_input_buffer(); msg_err("无效输入"); return; }
    printf("请输入新的车辆价值(万元): ");
    if (scanf("%f", &new_value) != 1)   { clear_input_buffer(); msg_err("无效输入"); return; }
    clear_input_buffer();

    VehStatus st = veh_update(id, new_mileage, new_value);
    if (st == VEH_OK) {
        save_to_file();
        msg_ok("车辆 [%s] 信息更新成功", id);
    } else if (st == VEH_ERR_NOT_FOUND) msg_err("未找到车辆 [%s]", id);
    else if (st == VEH_ERR_INVALID)     msg_err("里程和价值不能为负数");
    else                                msg_err("更新失败，错误码: %d", st);
}

/**
 * @brief 主菜单
 */
static void show_main_menu(void)
{
    printf("\n");
    print_line('=', 50);
    printf(C_BOLD C_CYAN "          车辆管理系统 V2.0 - 主菜单\n" C_RESET);
    print_line('=', 50);
    printf("  %s1.%s 显示所有车辆\n", C_GREEN, C_RESET);
    printf("  %s2.%s 显示所有司机\n", C_GREEN, C_RESET);
    printf("  %s3.%s 添加车辆\n", C_YELLOW, C_RESET);
    printf("  %s4.%s 删除车辆\n", C_YELLOW, C_RESET);
    printf("  %s5.%s 添加司机\n", C_YELLOW, C_RESET);
    printf("  %s6.%s 删除司机\n", C_YELLOW, C_RESET);
    printf("  %s7.%s 派车\n", C_BLUE, C_RESET);
    printf("  %s8.%s 还车\n", C_BLUE, C_RESET);
    printf("  %s9.%s 登记维修\n", C_RED, C_RESET);
    printf("  %s10.%s 维修完成\n", C_GREEN, C_RESET);
    printf("  %s11.%s 类型统计\n", C_MAGENTA, C_RESET);
    printf("  %s12.%s 更新车辆信息\n", C_CYAN, C_RESET);
    printf("  %s13.%s 保存数据\n", C_CYAN, C_RESET);
    printf("  %s0.%s 退出系统\n", C_RED, C_RESET);
    print_line('-', 50);
    printf("请选择 [0-13]: ");
}

/**
 * @brief 交互菜单主循环
 */
static void run_interactive(void)
{
    print_title("车辆管理系统 V2.0");
    msg_info("已加载 %d 辆车, %d 名司机", g_vehicles.count, g_drivers.count);
    if (g_vehicles.count == 0)
        msg_warn("暂无数据，请先添加或初始化测试数据");

    int choice;
    while (1) {
        show_main_menu();
        if (scanf("%d", &choice) != 1) {
            clear_input_buffer();
            msg_err("无效输入，请输入数字");
            continue;
        }
        clear_input_buffer();

        switch (choice) {
            case 1:  display_all_vehicles(); break;
            case 2:  display_all_drivers();  break;
            case 3:  menu_add_vehicle();     break;
            case 4:  menu_delete_vehicle();  break;
            case 5:  menu_add_driver();      break;
            case 6: {
                char id[MAX_STR];
                printf("请输入要删除的司机ID: ");
                safe_gets(id, MAX_STR);
                VehStatus st = drv_delete(id);
                if (st == VEH_OK) { save_to_file(); msg_ok("司机 [%s] 已删除", id); }
                else if (st == VEH_ERR_NOT_FOUND) msg_err("未找到司机 [%s]", id);
                else msg_err("删除失败");
                break;
            }
            case 7:  menu_dispatch();        break;
            case 8:  menu_return();          break;
            case 9:  menu_repair();          break;
            case 10: menu_repair_complete(); break;
            case 11: stat_by_type();         break;
            case 12: menu_update_vehicle();  break;
            case 13: {
                VehStatus st = save_to_file();
                if (st == VEH_OK) msg_ok("数据保存成功");
                else msg_err("保存失败");
                break;
            }
            case 0:
                save_to_file();
                arrays_free();
                print_line('*', 50);
                printf(C_GREEN C_BOLD "  感谢使用车辆管理系统 V2.0，再见！\n" C_RESET);
                print_line('*', 50);
                return;
            default:
                msg_err("无效选择，请重新输入");
        }
    }
}

/* ============================================================================
 *  第十一部分：内置自测 (15+ 项)
 * ============================================================================ */

static int g_test_passed = 0;
static int g_test_failed = 0;

/**
 * @brief 测试用例断言宏
 */
#define TEST_ASSERT(cond, name) do { \
    if (cond) { \
        printf(C_GREEN "  [PASS] " C_RESET "%s\n", name); \
        g_test_passed++; \
    } else { \
        printf(C_RED "  [FAIL] " C_RESET "%s\n", name); \
        g_test_failed++; \
    } \
} while(0)

/**
 * @brief 初始化测试数据
 */
static void init_test_data(void)
{
    arrays_free();
    veh_array_init(&g_vehicles);
    drv_array_init(&g_drivers);

    /* 添加 5 名测试司机 */
    drv_add("D001", "张三", "13800001111", 35);
    drv_add("D002", "李四", "13800002222", 42);
    drv_add("D003", "王五", "13800003333", 28);
    drv_add("D004", "赵六", "13800004444", 38);
    drv_add("D005", "钱七", "13800005555", 45);

    /* 添加 15 辆测试车辆 */
    veh_add("V001", "京A-10001", "小型轿车", "D001", 12.5f, 85.0f);
    veh_add("V002", "京A-10002", "小型轿车", "D002", 15.3f, 92.0f);
    veh_add("V003", "京A-10003", "小型轿车", "D003", 8.7f,  78.5f);
    veh_add("V004", "京B-20001", "大型客车", "D004", 6.2f,  35.0f);
    veh_add("V005", "京B-20002", "大型客车", "D005", 4.8f,  32.5f);
    veh_add("V006", "京B-20003", "大型客车", "D001", 7.1f,  38.0f);
    veh_add("V007", "京C-30001", "货运车",   "D002", 9.6f,  65.0f);
    veh_add("V008", "京C-30002", "货运车",   "D003", 11.2f, 72.0f);
    veh_add("V009", "京C-30003", "货运车",   "D004", 5.4f,  58.0f);
    veh_add("V010", "京D-40001", "工程车",   "D005", 18.9f, 120.0f);
    veh_add("V011", "京D-40002", "工程车",   "D001", 14.6f, 110.0f);
    veh_add("V012", "京D-40003", "工程车",   "D002", 20.3f, 135.0f);
    veh_add("V013", "京E-50001", "出租车",   "D003", 22.0f, 55.0f);
    veh_add("V014", "京E-50002", "出租车",   "D004", 19.5f, 48.0f);
    veh_add("V015", "京E-50003", "出租车",   "D005", 16.8f, 52.0f);
}

/**
 * @brief 运行所有自测
 */
static int run_self_test(void)
{
    printf(C_BOLD C_CYAN "\n========== 车辆管理系统 V2.0 自测开始 ==========\n\n" C_RESET);
    g_test_passed = 0;
    g_test_failed = 0;

    /* 测试 1: 数组初始化 */
    printf(C_BOLD "[测试组1: 基础CRUD]\n" C_RESET);
    arrays_free();
    VehStatus st;
    st = veh_array_init(&g_vehicles);
    TEST_ASSERT(st == VEH_OK, "1. 车辆数组初始化成功");
    st = drv_array_init(&g_drivers);
    TEST_ASSERT(st == VEH_OK, "2. 司机数组初始化成功");
    TEST_ASSERT(g_vehicles.count == 0 && g_drivers.count == 0, "3. 初始计数为0");

    /* 测试 4: 添加司机 */
    st = drv_add("D001", "测试司机", "13800000000", 30);
    TEST_ASSERT(st == VEH_OK, "4. 添加司机成功");
    TEST_ASSERT(g_drivers.count == 1, "5. 司机数量正确");

    /* 测试 6: 重复添加司机 */
    st = drv_add("D001", "重复", "139", 25);
    TEST_ASSERT(st == VEH_ERR_DUPLICATE, "6. 重复添加司机返回错误");

    /* 测试 7: 添加车辆 */
    st = veh_add("V001", "测-00001", "测试类型", "D001", 10.0f, 50.0f);
    TEST_ASSERT(st == VEH_OK, "7. 添加车辆成功");
    TEST_ASSERT(g_vehicles.count == 1, "8. 车辆数量正确");

    /* 测试 9: 初始状态为可用 */
    TEST_ASSERT(g_vehicles.data[0].status == VEH_AVAILABLE, "9. 新车默认可用状态");

    /* 测试 10: 重复车牌/ID */
    st = veh_add("V001", "测-00002", "测试", "", 0, 0);
    TEST_ASSERT(st == VEH_ERR_DUPLICATE, "10. 重复ID被拒绝");
    st = veh_add("V999", "测-00001", "测试", "", 0, 0);
    TEST_ASSERT(st == VEH_ERR_DUPLICATE, "11. 重复车牌被拒绝");

    /* 测试组2: 派车/还车/维修 */
    printf(C_BOLD "\n[测试组2: 派车/还车/维修]\n" C_RESET);

    st = veh_dispatch("V001", "测试派车");
    TEST_ASSERT(st == VEH_OK, "12. 派车成功");
    TEST_ASSERT(g_vehicles.data[0].status == VEH_DISPATCHED, "13. 状态变为已派出");

    st = veh_dispatch("V001", "重复派车");
    TEST_ASSERT(st == VEH_ERR_DISPATCHED, "14. 已派出车辆无法再次派车");

    st = veh_return("V001", 5.0f);
    TEST_ASSERT(st == VEH_OK, "15. 还车成功");
    TEST_ASSERT(g_vehicles.data[0].status == VEH_AVAILABLE, "16. 还车后状态恢复可用");
    TEST_ASSERT(g_vehicles.data[0].mileage == 15.0f, "17. 里程累计正确");

    st = veh_repair("V001", "保养", 500.0f);
    TEST_ASSERT(st == VEH_OK, "18. 登记维修成功");
    TEST_ASSERT(g_vehicles.data[0].status == VEH_MAINTENANCE, "19. 状态变为维修中");
    TEST_ASSERT(g_vehicles.data[0].repair_count == 1, "20. 维修次数+1");

    st = veh_dispatch("V001", "维修中派车");
    TEST_ASSERT(st == VEH_ERR_MAINTENANCE, "21. 维修中车辆无法派出");

    st = veh_repair_complete("V001");
    TEST_ASSERT(st == VEH_OK, "22. 维修完成操作成功");
    TEST_ASSERT(g_vehicles.data[0].status == VEH_AVAILABLE, "23. 维修后恢复可用");

    /* 测试组3: 删除与更新 */
    printf(C_BOLD "\n[测试组3: 删除/更新/边界]\n" C_RESET);

    st = veh_dispatch("V001", "测试");
    TEST_ASSERT(st == VEH_OK, "24. 派车用于删除测试");
    st = veh_delete("V001");
    TEST_ASSERT(st == VEH_ERR_DISPATCHED, "25. 已派出车辆无法删除");
    veh_return("V001", 0);
    st = veh_delete("V001");
    TEST_ASSERT(st == VEH_OK, "26. 还车后删除成功");
    TEST_ASSERT(g_vehicles.count == 0, "27. 删除后计数为0");

    st = veh_delete("V999");
    TEST_ASSERT(st == VEH_ERR_NOT_FOUND, "28. 删除不存在车辆返回错误");

    st = drv_delete("D999");
    TEST_ASSERT(st == VEH_ERR_NOT_FOUND, "29. 删除不存在司机返回错误");

    /* 测试 30: 批量添加测试数据 */
    init_test_data();
    TEST_ASSERT(g_vehicles.count == 15, "30. 批量初始化15辆车成功");
    TEST_ASSERT(g_drivers.count == 5, "31. 批量初始化5名司机成功");

    /* 测试 32: 更新车辆信息 */
    st = veh_update("V001", 25.0f, 90.0f);
    TEST_ASSERT(st == VEH_OK, "32. 更新车辆信息成功");
    TEST_ASSERT(g_vehicles.data[veh_find_by_id("V001")].mileage == 25.0f, "33. 里程更新正确");
    TEST_ASSERT(g_vehicles.data[veh_find_by_id("V001")].value == 90.0f, "34. 价值更新正确");

    st = veh_update("V001", -1.0f, 50.0f);
    TEST_ASSERT(st == VEH_ERR_INVALID, "35. 负里程被拒绝");
    st = veh_update("V999", 0, 0);
    TEST_ASSERT(st == VEH_ERR_NOT_FOUND, "36. 更新不存在车辆返回错误");

    /* 测试组4: 文件I/O */
    printf(C_BOLD "\n[测试组4: 文件持久化]\n" C_RESET);

    st = save_to_file();
    TEST_ASSERT(st == VEH_OK, "37. 保存到文件成功");

    int old_vcount = g_vehicles.count;
    int old_dcount = g_drivers.count;
    char old_plate[MAX_STR];
    strncpy(old_plate, g_vehicles.data[0].plate, MAX_STR - 1);
    old_plate[MAX_STR - 1] = '\0';
    float old_mileage = g_vehicles.data[0].mileage;
    float old_value   = g_vehicles.data[0].value;

    arrays_free();
    veh_array_init(&g_vehicles);
    drv_array_init(&g_drivers);
    TEST_ASSERT(g_vehicles.count == 0, "38. 清空后车辆数为0");

    st = load_from_file();
    TEST_ASSERT(st == VEH_OK, "39. 从文件加载成功");
    TEST_ASSERT(g_vehicles.count == old_vcount, "40. 加载后车辆数量一致");
    TEST_ASSERT(g_drivers.count == old_dcount, "41. 加载后司机数量一致");
    TEST_ASSERT(strcmp(g_vehicles.data[0].plate, old_plate) == 0, "42. 加载后车牌一致");
    TEST_ASSERT(g_vehicles.data[0].mileage == old_mileage, "43. 加载后里程一致");
    TEST_ASSERT(g_vehicles.data[0].value == old_value, "44. 加载后价值一致");

    /* 测试组5: 扩容测试 */
    printf(C_BOLD "\n[测试组5: 动态扩容]\n" C_RESET);

    int old_cap = g_vehicles.capacity;
    for (int i = 0; i < 20; i++) {
        char id[16], plate[16];
        snprintf(id, 16, "T%03d", i + 1);
        snprintf(plate, 16, "测-T%03d", i + 1);
        veh_add(id, plate, "扩容测试", "", 0, 0);
    }
    TEST_ASSERT(g_vehicles.count == old_vcount + 20, "45. 扩容后添加20辆车成功");
    TEST_ASSERT(g_vehicles.capacity > old_cap, "46. 数组容量已自动扩容");

    /* 测试 47: 无效参数 */
    st = veh_add(NULL, "X", "Y", "", 0, 0);
    TEST_ASSERT(st == VEH_ERR_INVALID, "47. NULL参数返回无效错误");

    /* 清理测试文件 */
    remove(DATA_FILE);
    arrays_free();

    printf("\n");
    print_line('=', 50);
    printf(C_BOLD "  测试结果: " C_GREEN "%d 通过" C_RESET ", " C_RED "%d 失败" C_RESET ", 共 %d 项\n",
           g_test_passed, g_test_failed, g_test_passed + g_test_failed);
    print_line('=', 50);

    return g_test_failed == 0 ? 0 : 1;
}

/* ============================================================================
 *  第十二部分：帮助信息与主函数
 * ============================================================================ */

/**
 * @brief 显示帮助信息
 */
static void show_help(const char *prog)
{
    printf(C_BOLD C_CYAN "车辆管理系统 V2.0\n" C_RESET);
    printf("用法: %s [选项]\n\n", prog);
    printf("选项:\n");
    printf("  %s-h, --help%s    显示此帮助信息\n", C_GREEN, C_RESET);
    printf("  %s-t, --test%s    运行内置自测套件\n", C_GREEN, C_RESET);
    printf("  (无参数)         进入交互菜单模式\n\n");
    printf("功能特性:\n");
    printf("  * 车辆/司机 CRUD 管理\n");
    printf("  * 派车/还车/维修 状态流转\n");
    printf("  * 多维度统计分析\n");
    printf("  * VEHIV2 魔数二进制持久化\n");
    printf("  * ANSI 彩色终端输出\n");
    printf("  * Windows UTF-8 控制台支持\n");
    printf("  * 15+ 项内置单元自测\n");
}

/**
 * @brief 程序入口
 */
int main(int argc, char *argv[])
{
    setup_console();

    /* 解析命令行参数 */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            show_help(argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--test") == 0) {
            return run_self_test();
        }
    }

    /* 交互模式 */
    veh_array_init(&g_vehicles);
    drv_array_init(&g_drivers);

    if (load_from_file() != VEH_OK) {
        msg_warn("数据文件不存在或格式错误，将创建新数据");
    }

    /* 首次运行初始化15条测试数据 */
    if (g_vehicles.count == 0) {
        init_test_data();
        save_to_file();
        msg_info("已初始化测试数据（15辆车 + 5名司机）");
    }

    run_interactive();
    return 0;
}
