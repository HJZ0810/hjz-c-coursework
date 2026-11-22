/*================================================================================
 *                                                                              *
 *   ██████╗ ██████╗  █████╗ ██████╗ ██╗   ██╗██████╗                           *
 *  ██╔════╝ ██╔══██╗██╔══██╗██╔══██╗██║   ██║╚════██╗                          *
 *  ██║  ███╗██████╔╝███████║██║  ██║██║   ██║ █████╔╝                          *
 *  ██║   ██║██╔══██╗██╔══██║██║  ██║╚██╗ ██╔╝██╔═══╝                           *
 *  ╚██████╔╝██║  ██║██║  ██║██████╔╝ ╚████╔╝ ███████╗                          *
 *   ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚═════╝   ╚═══╝  ╚══════╝                          *
 *                        研究生管理信息系统 V2.0                                *
 *                                                                              *
 *  项目名称: 研究生管理信息系统 (Graduate Student Management System V2.0)       *
 *  源文件名: main.c                                                            *
 *  编译标准: C99 (-std=c99 -Wall -Wextra -O2)                                  *
 *  数据结构: 动态数组 (Dynamic Array) — 替代V1单链表                           *
 *  文件持久化: 二进制格式 + Magic Header "GRADV2"                              *
 *  终端特性: ANSI彩色输出 + UTF-8编码支持 (SetConsoleOutputCP 65001)           *
 *  CLI参数:  -h 显示帮助 / -t 或 --test 运行内置自测                           *
 *================================================================================*/


/* ======================== 头文件包含 ======================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif


/*================================================================================
 *                          常量与宏定义
 *================================================================================
 *
 * 设计说明:
 *   - 所有字符串字段以 '\0' 结尾防止越界访问
 *   - 使用 enum 错误码替代V1的零散printf提示，统一错误处理
 *   - 动态数组初始容量与增长因子平衡内存使用与扩容频率
 *   - 文件magic header "GRADV2" 用于校验文件完整性与版本
 *
 *================================================================================*/

#define MAX_NAME        50          /* 姓名字段最大长度(含'\0') */
#define MAX_ID          20          /* ID字段最大长度(含'\0') */
#define MAX_TITLE       30          /* 职称字段最大长度(含'\0') */
#define MAX_DEPT        50          /* 院系字段最大长度(含'\0') */
#define MAX_TOPIC       100         /* 研究方向字段最大长度(含'\0') */
#define MAX_COURSE      50          /* 课程名称最大长度(含'\0') */
#define MAX_INPUT       256         /* 输入缓冲区大小 */

#define ARRAY_INIT_CAP  8           /* 动态数组初始容量 */
#define ARRAY_GROW_FACTOR 2         /* 动态数组扩容倍数 */

#define FILE_GRADV2     "gradv2.dat" /* V2持久化数据文件(binary) */
#define MAGIC_HEADER    "GRADV2"    /* 文件魔数标识 */
#define MAGIC_LEN       6           /* 魔数长度(不含'\0') */
#define FILE_VERSION    2           /* 文件格式版本号 */


/*================================================================================
 *                          ANSI 彩色终端
 *================================================================================
 *
 * Windows 10+ 已原生支持 ANSI 转义序列(VT100)
 * 在 init_console() 中通过 SetConsoleMode 启用 ENABLE_VIRTUAL_TERMINAL_PROCESSING
 * 颜色方案:
 *   RESET  - 重置所有属性
 *   BOLD   - 粗体高亮
 *   RED    - 错误/失败提示
 *   GREEN  - 成功/通过提示
 *   YELLOW - 警告/菜单标题
 *   BLUE   - 信息/表头
 *   CYAN   - 交互提示
 *   MAGENTA- 测试结果
 *
 *================================================================================*/

#define COLOR_RESET     "\033[0m"
#define COLOR_BOLD      "\033[1m"
#define COLOR_RED       "\033[31m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_BLUE      "\033[34m"
#define COLOR_MAGENTA   "\033[35m"
#define COLOR_CYAN      "\033[36m"
#define COLOR_WHITE     "\033[37m"
#define COLOR_BG_BLUE    "\033[44m"
#define COLOR_BG_RED     "\033[41m"
#define COLOR_BG_GREEN   "\033[42m"


/*================================================================================
 *                          统一错误码枚举
 *================================================================================
 *
 * 设计说明:
 *   V1中错误处理分散在各函数的printf中，无法被调用方程序化判断
 *   V2引入 GradStatus 枚举，所有业务函数返回状态码:
 *     - GRAD_OK (0) 表示成功
 *     - 负值表示各类错误
 *   print_status() 可将状态码转为人类可读的彩色提示
 *
 *================================================================================*/

typedef enum {
    GRAD_OK                     =  0,   /* 操作成功 */
    GRAD_ERR_NULL_PTR           = -1,   /* 空指针错误 */
    GRAD_ERR_MEMORY             = -2,   /* 内存分配失败 */
    GRAD_ERR_NOT_FOUND          = -3,   /* 目标未找到 */
    GRAD_ERR_DUPLICATE          = -4,   /* ID重复冲突 */
    GRAD_ERR_INVALID_INPUT      = -5,   /* 输入参数无效 */
    GRAD_ERR_FILE_IO            = -6,   /* 文件I/O错误 */
    GRAD_ERR_INVALID_SCORE      = -7,   /* 成绩不在0-100范围 */
    GRAD_ERR_ADVISOR_NOT_FOUND  = -8,   /* 导师不存在 */
    GRAD_ERR_STUDENT_NOT_FOUND  = -9,   /* 研究生不存在 */
    GRAD_ERR_ALREADY_ASSIGNED   = -10,  /* 已有导师(重复分配) */
    GRAD_ERR_NOT_ASSIGNED       = -11,  /* 未分配导师(无法解除) */
    GRAD_ERR_FILE_CORRUPT       = -12,  /* 文件数据损坏 */
    GRAD_ERR_FILE_VERSION       = -13,  /* 文件版本不兼容 */
    GRAD_ERR_FILE_MAGIC         = -14   /* 文件魔数不匹配 */
} GradStatus;


/*================================================================================
 *                          数据结构定义
 *================================================================================
 *
 * V2 核心实体:
 *   1. Advisor  (导师)     — 职称、院系，一个导师可指导多名研究生
 *   2. Graduate (研究生)   — 性别、入学年份、导师ID(外键)、研究方向
 *   3. Grade    (成绩)     — 关联研究生ID+课程名，0.0~100.0浮点数
 *
 * 关系模型:
 *   Advisor  1 ──── N  Graduate (通过 Graduate.advisor_id 关联)
 *   Graduate 1 ──── N  Grade    (通过 Grade.student_id 关联)
 *
 * 存储容器:
 *   使用动态数组(Dynamic Array)替代V1的单链表
 *   优势: 随机访问O(1)、缓存友好(内存连续)、计数方便
 *   劣势: 中间插入/删除O(n)需移动元素 (本系统以追加/查询为主，影响可忽略)
 *
 *================================================================================*/

/**
 * @struct Advisor
 * @brief  导师信息结构体
 */
typedef struct {
    char id[MAX_ID];              /* 导师工号, 如 "T2021001" */
    char name[MAX_NAME];          /* 导师姓名 */
    char title[MAX_TITLE];        /* 职称, 如 "教授"/"副教授"/"讲师" */
    char department[MAX_DEPT];    /* 所属院系, 如 "计算机学院" */
} Advisor;

/**
 * @struct Graduate
 * @brief  研究生信息结构体
 */
typedef struct {
    char id[MAX_ID];              /* 学号, 如 "20242001001" */
    char name[MAX_NAME];          /* 姓名 */
    char gender[10];              /* 性别: "男"/"女" */
    int  enrollment_year;         /* 入学年份, 如 2024 */
    char advisor_id[MAX_ID];      /* 导师工号(外键), 空串表示未分配 */
    char research_topic[MAX_TOPIC];/* 研究方向 */
} Graduate;

/**
 * @struct Grade
 * @brief  成绩记录结构体
 */
typedef struct {
    char student_id[MAX_ID];      /* 学号(外键) */
    char course_name[MAX_COURSE]; /* 课程名称 */
    float score;                  /* 成绩, 范围 0.0 ~ 100.0 */
} Grade;

/**
 * @struct DynArray
 * @brief  泛型动态数组容器 (通用实现)
 *
 * @param data      指向元素数组的指针
 * @param size      当前元素个数
 * @param capacity  当前分配的容量(元素数)
 * @param elem_size 单个元素的大小(sizeof)
 */
typedef struct {
    void  *data;
    int    size;
    int    capacity;
    size_t elem_size;
} DynArray;


/*================================================================================
 *                          全局数据容器
 *================================================================================
 *
 * 使用 static 限制在本文件内，封装性优于V1的链表头指针
 * 三个动态数组分别管理导师、研究生、成绩数据
 *
 * g_ 前缀标识全局变量，避免与局部变量混淆
 *
 *================================================================================*/

static DynArray g_advisors;       /* 导师动态数组 */
static DynArray g_graduates;      /* 研究生动态数组 */
static DynArray g_grades;         /* 成绩动态数组 */


/*================================================================================
 *                          函数前置声明
 *================================================================================*/

/* ---- 控制台与工具 ---- */
static void        init_console(void);
static void        print_line(char ch, int len);
static void        print_centered(const char *title, int width);
static void        clear_input_buffer(void);
static void        safe_gets(char *buf, int size, const char *prompt);
static int         read_int(const char *prompt);
static float       read_float(const char *prompt);
static const char *status_msg(GradStatus s);
static void        print_status(GradStatus s);
static int         str_is_empty(const char *s);

/* ---- 动态数组核心 ---- */
static GradStatus  darray_init(DynArray *arr, size_t elem_size);
static GradStatus  darray_reserve(DynArray *arr, int new_cap);
static GradStatus  darray_push(DynArray *arr, const void *elem);
static void*       darray_get(const DynArray *arr, int index);
static GradStatus  darray_remove_at(DynArray *arr, int index);
static void        darray_free(DynArray *arr);

/* ---- 导师管理 CRUD ---- */
static int         find_advisor_by_id(const char *id);
static GradStatus  advisor_add(const char *id, const char *name,
                                const char *title, const char *dept);
static GradStatus  advisor_delete(const char *id);
static GradStatus  advisor_update(const char *id, const char *name,
                                   const char *title, const char *dept);
static void        advisor_list_all(void);
static int         advisor_count(void);

/* ---- 研究生管理 CRUD ---- */
static int         find_graduate_by_id(const char *id);
static GradStatus  graduate_add(const char *id, const char *name,
                                 const char *gender, int year,
                                 const char *topic);
static GradStatus  graduate_delete(const char *id);
static GradStatus  graduate_update(const char *id, const char *name,
                                    const char *gender, int year,
                                    const char *topic);
static void        graduate_list_all(void);
static int         graduate_count(void);

/* ---- 导师分配 ---- */
static GradStatus  assign_advisor(const char *stu_id, const char *adv_id);
static GradStatus  unassign_advisor(const char *stu_id);
static void        list_students_by_advisor(const char *adv_id);

/* ---- 成绩管理 CRUD ---- */
static int         find_grade(const char *stu_id, const char *course);
static GradStatus  grade_add(const char *stu_id, const char *course, float score);
static GradStatus  grade_update(const char *stu_id, const char *course, float score);
static GradStatus  grade_delete(const char *stu_id, const char *course);
static void        grade_list_by_student(const char *stu_id);
static float       grade_avg_by_student(const char *stu_id);
static int         grade_count(void);

/* ---- 文件持久化 ---- */
static GradStatus  save_to_file(const char *filename);
static GradStatus  load_from_file(const char *filename);

/* ---- 内存释放 ---- */
static void        free_all_data(void);

/* ---- 菜单系统 ---- */
static void        show_main_menu(void);
static void        advisor_menu(void);
static void        graduate_menu(void);
static void        grade_menu(void);
static void        advisor_assignment_menu(void);
static void        statistics_menu(void);
static void        run_interactive(void);

/* ---- 内置自测 ---- */
static int         run_self_tests(void);


/*================================================================================
 *                          控制台初始化与工具函数
 *================================================================================
 *
 * init_console() 关键说明:
 *   1. Windows下设置控制台输入/输出代码页为UTF-8(65001)
 *   2. 启用VT100(ANSI转义序列)处理，使得彩色输出正常工作
 *   3. Linux/macOS下无需特殊设置(默认UTF-8+ANSI)
 *
 *================================================================================*/

static void init_console(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(hOut, &mode)) {
            mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            mode |= ENABLE_PROCESSED_OUTPUT;
            SetConsoleMode(hOut, mode);
        }
    }
#endif
}

static void print_line(char ch, int len)
{
    for (int i = 0; i < len; i++) {
        putchar(ch);
    }
    putchar('\n');
}

static void print_centered(const char *title, int width)
{
    int len = 0;
    const char *p = title;
    while (*p) {
        if ((*p & 0x80) == 0) {
            len++;
            p++;
        } else if ((*p & 0xE0) == 0xC0) {
            len++;
            p += 2;
        } else if ((*p & 0xF0) == 0xE0) {
            len += 2;
            p += 3;
        } else {
            len += 2;
            p += 4;
        }
    }
    int pad = (width - len) / 2;
    if (pad < 0) pad = 0;
    for (int i = 0; i < pad; i++) {
        putchar(' ');
    }
    printf("%s\n", title);
}

static void clear_input_buffer(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

static void safe_gets(char *buf, int size, const char *prompt)
{
    if (prompt != NULL) {
        printf("%s", prompt);
    }
    fflush(stdout);
    if (fgets(buf, size, stdin) != NULL) {
        int len = (int)strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
        } else if (len > 0) {
            clear_input_buffer();
        }
    } else {
        buf[0] = '\0';
    }
}

static int read_int(const char *prompt)
{
    char buf[MAX_INPUT];
    safe_gets(buf, sizeof(buf), prompt);
    return atoi(buf);
}

static float read_float(const char *prompt)
{
    char buf[MAX_INPUT];
    safe_gets(buf, sizeof(buf), prompt);
    return (float)atof(buf);
}

static int str_is_empty(const char *s)
{
    return (s == NULL || s[0] == '\0');
}


/*================================================================================
 *                          状态码映射与打印
 *================================================================================*/

static const char *status_msg(GradStatus s)
{
    switch (s) {
        case GRAD_OK:                    return "操作成功";
        case GRAD_ERR_NULL_PTR:          return "空指针错误";
        case GRAD_ERR_MEMORY:            return "内存分配失败";
        case GRAD_ERR_NOT_FOUND:         return "目标未找到";
        case GRAD_ERR_DUPLICATE:         return "ID重复, 记录已存在";
        case GRAD_ERR_INVALID_INPUT:     return "输入参数无效";
        case GRAD_ERR_FILE_IO:           return "文件读写错误";
        case GRAD_ERR_INVALID_SCORE:     return "成绩无效(须在0~100之间)";
        case GRAD_ERR_ADVISOR_NOT_FOUND: return "导师不存在";
        case GRAD_ERR_STUDENT_NOT_FOUND: return "研究生不存在";
        case GRAD_ERR_ALREADY_ASSIGNED:  return "该研究生已分配导师";
        case GRAD_ERR_NOT_ASSIGNED:      return "该研究生尚未分配导师";
        case GRAD_ERR_FILE_CORRUPT:      return "文件数据损坏";
        case GRAD_ERR_FILE_VERSION:      return "文件版本不兼容";
        case GRAD_ERR_FILE_MAGIC:        return "文件格式错误(非GRADV2数据文件)";
        default:                         return "未知错误";
    }
}

static void print_status(GradStatus s)
{
    if (s == GRAD_OK) {
        printf("  %s[√] %s%s\n", COLOR_GREEN, status_msg(s), COLOR_RESET);
    } else {
        printf("  %s[×] %s (错误码: %d)%s\n", COLOR_RED, status_msg(s), (int)s, COLOR_RESET);
    }
}


/*================================================================================
 *                          动态数组核心实现
 *================================================================================
 *
 * 通用动态数组, 使用 void* 存储任意类型元素
 * 通过 elem_size 记录每个元素的字节大小, 用 memcpy 进行拷贝
 *
 * 内存布局:
 *   [elem0][elem1]...[elemN-1][未使用空间...]
 *   |<- size 个元素 ->|<- (cap-size)*elem_size 字节空闲 ->|
 *
 * 扩容策略: 容量不足时按 ARRAY_GROW_FACTOR(2) 倍增长
 * 初始容量: ARRAY_INIT_CAP(8)
 *
 *================================================================================*/

static GradStatus darray_init(DynArray *arr, size_t elem_size)
{
    if (arr == NULL || elem_size == 0) {
        return GRAD_ERR_NULL_PTR;
    }
    arr->elem_size = elem_size;
    arr->size      = 0;
    arr->capacity  = ARRAY_INIT_CAP;
    arr->data      = malloc(elem_size * (size_t)arr->capacity);
    if (arr->data == NULL) {
        arr->capacity = 0;
        return GRAD_ERR_MEMORY;
    }
    return GRAD_OK;
}

static GradStatus darray_reserve(DynArray *arr, int new_cap)
{
    if (arr == NULL || new_cap <= arr->capacity) {
        return GRAD_OK;
    }
    void *new_data = realloc(arr->data, arr->elem_size * (size_t)new_cap);
    if (new_data == NULL) {
        return GRAD_ERR_MEMORY;
    }
    arr->data     = new_data;
    arr->capacity = new_cap;
    return GRAD_OK;
}

static GradStatus darray_push(DynArray *arr, const void *elem)
{
    if (arr == NULL || elem == NULL) {
        return GRAD_ERR_NULL_PTR;
    }
    if (arr->size >= arr->capacity) {
        int new_cap = arr->capacity * ARRAY_GROW_FACTOR;
        if (new_cap <= arr->capacity) new_cap = arr->capacity + 8;
        GradStatus st = darray_reserve(arr, new_cap);
        if (st != GRAD_OK) return st;
    }
    char *dest = (char *)arr->data + arr->elem_size * (size_t)arr->size;
    memcpy(dest, elem, arr->elem_size);
    arr->size++;
    return GRAD_OK;
}

static void *darray_get(const DynArray *arr, int index)
{
    if (arr == NULL || index < 0 || index >= arr->size) {
        return NULL;
    }
    return (char *)arr->data + arr->elem_size * (size_t)index;
}

static GradStatus darray_remove_at(DynArray *arr, int index)
{
    if (arr == NULL) {
        return GRAD_ERR_NULL_PTR;
    }
    if (index < 0 || index >= arr->size) {
        return GRAD_ERR_NOT_FOUND;
    }
    if (index < arr->size - 1) {
        char *dst = (char *)arr->data + arr->elem_size * (size_t)index;
        char *src = dst + arr->elem_size;
        size_t bytes = arr->elem_size * (size_t)(arr->size - index - 1);
        memmove(dst, src, bytes);
    }
    arr->size--;
    return GRAD_OK;
}

static void darray_free(DynArray *arr)
{
    if (arr == NULL) return;
    free(arr->data);
    arr->data     = NULL;
    arr->size     = 0;
    arr->capacity = 0;
}


/*================================================================================
 *                          导师管理 CRUD
 *================================================================================*/

static int find_advisor_by_id(const char *id)
{
    if (str_is_empty(id)) return -1;
    for (int i = 0; i < g_advisors.size; i++) {
        Advisor *a = (Advisor *)darray_get(&g_advisors, i);
        if (a != NULL && strcmp(a->id, id) == 0) {
            return i;
        }
    }
    return -1;
}

static GradStatus advisor_add(const char *id, const char *name,
                               const char *title, const char *dept)
{
    if (str_is_empty(id) || str_is_empty(name)) {
        return GRAD_ERR_INVALID_INPUT;
    }
    if (find_advisor_by_id(id) >= 0) {
        return GRAD_ERR_DUPLICATE;
    }
    Advisor a;
    memset(&a, 0, sizeof(a));
    strncpy(a.id, id, MAX_ID - 1);
    strncpy(a.name, name, MAX_NAME - 1);
    strncpy(a.title, str_is_empty(title) ? "未知" : title, MAX_TITLE - 1);
    strncpy(a.department, str_is_empty(dept) ? "未知" : dept, MAX_DEPT - 1);
    return darray_push(&g_advisors, &a);
}

static GradStatus advisor_delete(const char *id)
{
    int idx = find_advisor_by_id(id);
    if (idx < 0) return GRAD_ERR_ADVISOR_NOT_FOUND;
    for (int i = 0; i < g_graduates.size; i++) {
        Graduate *g = (Graduate *)darray_get(&g_graduates, i);
        if (g != NULL && strcmp(g->advisor_id, id) == 0) {
            g->advisor_id[0] = '\0';
        }
    }
    return darray_remove_at(&g_advisors, idx);
}

static GradStatus advisor_update(const char *id, const char *name,
                                  const char *title, const char *dept)
{
    int idx = find_advisor_by_id(id);
    if (idx < 0) return GRAD_ERR_ADVISOR_NOT_FOUND;
    Advisor *a = (Advisor *)darray_get(&g_advisors, idx);
    if (a == NULL) return GRAD_ERR_NULL_PTR;
    if (!str_is_empty(name)) {
        strncpy(a->name, name, MAX_NAME - 1);
        a->name[MAX_NAME - 1] = '\0';
    }
    if (!str_is_empty(title)) {
        strncpy(a->title, title, MAX_TITLE - 1);
        a->title[MAX_TITLE - 1] = '\0';
    }
    if (!str_is_empty(dept)) {
        strncpy(a->department, dept, MAX_DEPT - 1);
        a->department[MAX_DEPT - 1] = '\0';
    }
    return GRAD_OK;
}

static void advisor_list_all(void)
{
    print_line('-', 70);
    printf("  %s%-12s %-12s %-14s %-20s%s\n",
           COLOR_BOLD, "工号", "姓名", "职称", "院系", COLOR_RESET);
    print_line('-', 70);
    for (int i = 0; i < g_advisors.size; i++) {
        Advisor *a = (Advisor *)darray_get(&g_advisors, i);
        if (a != NULL) {
            int stu_count = 0;
            for (int j = 0; j < g_graduates.size; j++) {
                Graduate *g = (Graduate *)darray_get(&g_graduates, j);
                if (g != NULL && strcmp(g->advisor_id, a->id) == 0) stu_count++;
            }
            printf("  %-12s %-12s %-14s %-20s (指导%d人)\n",
                   a->id, a->name, a->title, a->department, stu_count);
        }
    }
    print_line('-', 70);
    printf("  共 %d 位导师\n", g_advisors.size);
}

static int advisor_count(void)
{
    return g_advisors.size;
}


/*================================================================================
 *                          研究生管理 CRUD
 *================================================================================*/

static int find_graduate_by_id(const char *id)
{
    if (str_is_empty(id)) return -1;
    for (int i = 0; i < g_graduates.size; i++) {
        Graduate *g = (Graduate *)darray_get(&g_graduates, i);
        if (g != NULL && strcmp(g->id, id) == 0) {
            return i;
        }
    }
    return -1;
}

static GradStatus graduate_add(const char *id, const char *name,
                                const char *gender, int year,
                                const char *topic)
{
    if (str_is_empty(id) || str_is_empty(name) || year < 1900 || year > 2100) {
        return GRAD_ERR_INVALID_INPUT;
    }
    if (find_graduate_by_id(id) >= 0) {
        return GRAD_ERR_DUPLICATE;
    }
    Graduate g;
    memset(&g, 0, sizeof(g));
    strncpy(g.id, id, MAX_ID - 1);
    strncpy(g.name, name, MAX_NAME - 1);
    strncpy(g.gender, str_is_empty(gender) ? "未知" : gender, sizeof(g.gender) - 1);
    g.enrollment_year = year;
    g.advisor_id[0] = '\0';
    strncpy(g.research_topic, str_is_empty(topic) ? "未设置" : topic, MAX_TOPIC - 1);
    return darray_push(&g_graduates, &g);
}

static GradStatus graduate_delete(const char *id)
{
    int idx = find_graduate_by_id(id);
    if (idx < 0) return GRAD_ERR_STUDENT_NOT_FOUND;
    for (int i = g_grades.size - 1; i >= 0; i--) {
        Grade *gr = (Grade *)darray_get(&g_grades, i);
        if (gr != NULL && strcmp(gr->student_id, id) == 0) {
            darray_remove_at(&g_grades, i);
        }
    }
    return darray_remove_at(&g_graduates, idx);
}

static GradStatus graduate_update(const char *id, const char *name,
                                   const char *gender, int year,
                                   const char *topic)
{
    int idx = find_graduate_by_id(id);
    if (idx < 0) return GRAD_ERR_STUDENT_NOT_FOUND;
    Graduate *g = (Graduate *)darray_get(&g_graduates, idx);
    if (g == NULL) return GRAD_ERR_NULL_PTR;
    if (!str_is_empty(name)) {
        strncpy(g->name, name, MAX_NAME - 1);
        g->name[MAX_NAME - 1] = '\0';
    }
    if (!str_is_empty(gender)) {
        strncpy(g->gender, gender, sizeof(g->gender) - 1);
        g->gender[sizeof(g->gender) - 1] = '\0';
    }
    if (year >= 1900 && year <= 2100) g->enrollment_year = year;
    if (!str_is_empty(topic)) {
        strncpy(g->research_topic, topic, MAX_TOPIC - 1);
        g->research_topic[MAX_TOPIC - 1] = '\0';
    }
    return GRAD_OK;
}

static void graduate_list_all(void)
{
    print_line('-', 90);
    printf("  %s%-14s %-10s %-6s %-8s %-12s %-25s%s\n",
           COLOR_BOLD, "学号", "姓名", "性别", "入学年", "导师工号", "研究方向", COLOR_RESET);
    print_line('-', 90);
    for (int i = 0; i < g_graduates.size; i++) {
        Graduate *g = (Graduate *)darray_get(&g_graduates, i);
        if (g != NULL) {
            const char *adv = str_is_empty(g->advisor_id) ? "(未分配)" : g->advisor_id;
            printf("  %-14s %-10s %-6s %-8d %-12s %-25s\n",
                   g->id, g->name, g->gender, g->enrollment_year,
                   adv, g->research_topic);
        }
    }
    print_line('-', 90);
    printf("  共 %d 名研究生\n", g_graduates.size);
}

static int graduate_count(void)
{
    return g_graduates.size;
}


/*================================================================================
 *                          导师分配业务
 *================================================================================*/

static GradStatus assign_advisor(const char *stu_id, const char *adv_id)
{
    int si = find_graduate_by_id(stu_id);
    if (si < 0) return GRAD_ERR_STUDENT_NOT_FOUND;
    if (find_advisor_by_id(adv_id) < 0) return GRAD_ERR_ADVISOR_NOT_FOUND;
    Graduate *g = (Graduate *)darray_get(&g_graduates, si);
    if (g == NULL) return GRAD_ERR_NULL_PTR;
    if (!str_is_empty(g->advisor_id)) {
        return GRAD_ERR_ALREADY_ASSIGNED;
    }
    strncpy(g->advisor_id, adv_id, MAX_ID - 1);
    g->advisor_id[MAX_ID - 1] = '\0';
    return GRAD_OK;
}

static GradStatus unassign_advisor(const char *stu_id)
{
    int si = find_graduate_by_id(stu_id);
    if (si < 0) return GRAD_ERR_STUDENT_NOT_FOUND;
    Graduate *g = (Graduate *)darray_get(&g_graduates, si);
    if (g == NULL) return GRAD_ERR_NULL_PTR;
    if (str_is_empty(g->advisor_id)) {
        return GRAD_ERR_NOT_ASSIGNED;
    }
    g->advisor_id[0] = '\0';
    return GRAD_OK;
}

static void list_students_by_advisor(const char *adv_id)
{
    Advisor *a = NULL;
    int ai = find_advisor_by_id(adv_id);
    if (ai >= 0) a = (Advisor *)darray_get(&g_advisors, ai);
    printf("\n");
    print_line('-', 70);
    if (a != NULL) {
        printf("  %s导师: %s (%s)%s\n", COLOR_CYAN, a->name, a->title, COLOR_RESET);
    } else {
        printf("  %s导师工号: %s%s\n", COLOR_CYAN, adv_id, COLOR_RESET);
    }
    print_line('-', 70);
    int cnt = 0;
    for (int i = 0; i < g_graduates.size; i++) {
        Graduate *g = (Graduate *)darray_get(&g_graduates, i);
        if (g != NULL && strcmp(g->advisor_id, adv_id) == 0) {
            printf("  %-14s %-10s %s\n", g->id, g->name, g->research_topic);
            cnt++;
        }
    }
    if (cnt == 0) {
        printf("  (暂无指导研究生)\n");
    }
    print_line('-', 70);
    printf("  共指导 %d 名研究生\n", cnt);
}


/*================================================================================
 *                          成绩管理 CRUD
 *================================================================================*/

static int find_grade(const char *stu_id, const char *course)
{
    if (str_is_empty(stu_id) || str_is_empty(course)) return -1;
    for (int i = 0; i < g_grades.size; i++) {
        Grade *gr = (Grade *)darray_get(&g_grades, i);
        if (gr != NULL && strcmp(gr->student_id, stu_id) == 0
                       && strcmp(gr->course_name, course) == 0) {
            return i;
        }
    }
    return -1;
}

static GradStatus grade_add(const char *stu_id, const char *course, float score)
{
    if (str_is_empty(stu_id) || str_is_empty(course)) {
        return GRAD_ERR_INVALID_INPUT;
    }
    if (score < 0.0f || score > 100.0f) {
        return GRAD_ERR_INVALID_SCORE;
    }
    if (find_graduate_by_id(stu_id) < 0) {
        return GRAD_ERR_STUDENT_NOT_FOUND;
    }
    if (find_grade(stu_id, course) >= 0) {
        return GRAD_ERR_DUPLICATE;
    }
    Grade gr;
    memset(&gr, 0, sizeof(gr));
    strncpy(gr.student_id, stu_id, MAX_ID - 1);
    strncpy(gr.course_name, course, MAX_COURSE - 1);
    gr.score = score;
    return darray_push(&g_grades, &gr);
}

static GradStatus grade_update(const char *stu_id, const char *course, float score)
{
    if (score < 0.0f || score > 100.0f) {
        return GRAD_ERR_INVALID_SCORE;
    }
    int idx = find_grade(stu_id, course);
    if (idx < 0) return GRAD_ERR_NOT_FOUND;
    Grade *gr = (Grade *)darray_get(&g_grades, idx);
    if (gr == NULL) return GRAD_ERR_NULL_PTR;
    gr->score = score;
    return GRAD_OK;
}

static GradStatus grade_delete(const char *stu_id, const char *course)
{
    int idx = find_grade(stu_id, course);
    if (idx < 0) return GRAD_ERR_NOT_FOUND;
    return darray_remove_at(&g_grades, idx);
}

static void grade_list_by_student(const char *stu_id)
{
    int gi = find_graduate_by_id(stu_id);
    const char *sname = "未知";
    if (gi >= 0) {
        Graduate *g = (Graduate *)darray_get(&g_graduates, gi);
        if (g != NULL) sname = g->name;
    }
    printf("\n");
    print_line('-', 60);
    printf("  %s学生: %s (%s)%s\n", COLOR_CYAN, sname, stu_id, COLOR_RESET);
    print_line('-', 60);
    printf("  %s%-30s %-10s%s\n", COLOR_BOLD, "课程名称", "成绩", COLOR_RESET);
    print_line('-', 60);
    int cnt = 0;
    float sum = 0.0f;
    for (int i = 0; i < g_grades.size; i++) {
        Grade *gr = (Grade *)darray_get(&g_grades, i);
        if (gr != NULL && strcmp(gr->student_id, stu_id) == 0) {
            const char *color = COLOR_RESET;
            if (gr->score < 60.0f)      color = COLOR_RED;
            else if (gr->score >= 90.0f) color = COLOR_GREEN;
            else if (gr->score >= 80.0f) color = COLOR_CYAN;
            printf("  %-30s %s%6.1f%s\n", gr->course_name, color, gr->score, COLOR_RESET);
            sum += gr->score;
            cnt++;
        }
    }
    if (cnt == 0) {
        printf("  (暂无成绩记录)\n");
    } else {
        print_line('-', 60);
        float avg = sum / (float)cnt;
        const char *ac = COLOR_RESET;
        if (avg < 60.0f)      ac = COLOR_RED;
        else if (avg >= 90.0f) ac = COLOR_GREEN;
        printf("  %s平均分: %s%.2f%s (共%d门课程)\n",
               COLOR_BOLD, ac, avg, COLOR_RESET, cnt);
    }
    print_line('-', 60);
}

static float grade_avg_by_student(const char *stu_id)
{
    float sum = 0.0f;
    int cnt = 0;
    for (int i = 0; i < g_grades.size; i++) {
        Grade *gr = (Grade *)darray_get(&g_grades, i);
        if (gr != NULL && strcmp(gr->student_id, stu_id) == 0) {
            sum += gr->score;
            cnt++;
        }
    }
    return cnt > 0 ? sum / (float)cnt : -1.0f;
}

static int grade_count(void)
{
    return g_grades.size;
}


/*================================================================================
 *                          文件持久化 (二进制 + Magic Header)
 *================================================================================
 *
 * 文件格式 (GRADV2 Binary Format):
 *   ┌────────────────────────────────────────────────┐
 *   │ Offset 0:  Magic[6]  = "GRADV2"               │
 *   │ Offset 6:  Version   = 2 (uint16)             │
 *   │ Offset 8:  Reserved  = 0 (uint16, 对齐)       │
 *   │ Offset 10: AdvisorCount  (int32)              │
 *   │ Offset 14: GraduateCount (int32)              │
 *   │ Offset 18: GradeCount    (int32)              │
 *   │ ───────────────────────────────────────────── │
 *   │ Advisor[0..N-1]  每条 = sizeof(Advisor)字节   │
 *   │ Graduate[0..M-1] 每条 = sizeof(Graduate)字节  │
 *   │ Grade[0..K-1]    每条 = sizeof(Grade)字节     │
 *   └────────────────────────────────────────────────┘
 *
 * 说明:
 *   - 使用固定大小结构体直接fwrite/fread,简单高效
 *   - Magic header校验防止误读非V2文件
 *   - Version字段支持未来格式升级的兼容性检查
 *   - 采用小端序(x86/x64默认),文件可在同架构间直接共享
 *
 *================================================================================*/

static GradStatus save_to_file(const char *filename)
{
    if (str_is_empty(filename)) filename = FILE_GRADV2;
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) return GRAD_ERR_FILE_IO;

    char magic[MAGIC_LEN] = MAGIC_HEADER;
    unsigned short version = FILE_VERSION;
    unsigned short reserved = 0;
    int adv_cnt = g_advisors.size;
    int grad_cnt = g_graduates.size;
    int grade_cnt = g_grades.size;

    if (fwrite(magic, 1, MAGIC_LEN, fp) != MAGIC_LEN)  { fclose(fp); return GRAD_ERR_FILE_IO; }
    if (fwrite(&version, sizeof(version), 1, fp) != 1) { fclose(fp); return GRAD_ERR_FILE_IO; }
    if (fwrite(&reserved, sizeof(reserved), 1, fp) != 1){ fclose(fp); return GRAD_ERR_FILE_IO; }
    if (fwrite(&adv_cnt, sizeof(int), 1, fp) != 1)     { fclose(fp); return GRAD_ERR_FILE_IO; }
    if (fwrite(&grad_cnt, sizeof(int), 1, fp) != 1)    { fclose(fp); return GRAD_ERR_FILE_IO; }
    if (fwrite(&grade_cnt, sizeof(int), 1, fp) != 1)   { fclose(fp); return GRAD_ERR_FILE_IO; }

    for (int i = 0; i < g_advisors.size; i++) {
        Advisor *a = (Advisor *)darray_get(&g_advisors, i);
        if (a != NULL) {
            if (fwrite(a, sizeof(Advisor), 1, fp) != 1) { fclose(fp); return GRAD_ERR_FILE_IO; }
        }
    }
    for (int i = 0; i < g_graduates.size; i++) {
        Graduate *g = (Graduate *)darray_get(&g_graduates, i);
        if (g != NULL) {
            if (fwrite(g, sizeof(Graduate), 1, fp) != 1) { fclose(fp); return GRAD_ERR_FILE_IO; }
        }
    }
    for (int i = 0; i < g_grades.size; i++) {
        Grade *gr = (Grade *)darray_get(&g_grades, i);
        if (gr != NULL) {
            if (fwrite(gr, sizeof(Grade), 1, fp) != 1) { fclose(fp); return GRAD_ERR_FILE_IO; }
        }
    }

    fclose(fp);
    return GRAD_OK;
}

static GradStatus load_from_file(const char *filename)
{
    if (str_is_empty(filename)) filename = FILE_GRADV2;
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) return GRAD_ERR_FILE_IO;

    char magic[MAGIC_LEN];
    unsigned short version, reserved;
    int adv_cnt, grad_cnt, grade_cnt;

    if (fread(magic, 1, MAGIC_LEN, fp) != MAGIC_LEN) { fclose(fp); return GRAD_ERR_FILE_CORRUPT; }
    if (memcmp(magic, MAGIC_HEADER, MAGIC_LEN) != 0)  { fclose(fp); return GRAD_ERR_FILE_MAGIC; }

    if (fread(&version, sizeof(version), 1, fp) != 1) { fclose(fp); return GRAD_ERR_FILE_CORRUPT; }
    if (version != FILE_VERSION)                       { fclose(fp); return GRAD_ERR_FILE_VERSION; }

    if (fread(&reserved, sizeof(reserved), 1, fp) != 1) { fclose(fp); return GRAD_ERR_FILE_CORRUPT; }
    if (fread(&adv_cnt, sizeof(int), 1, fp) != 1)      { fclose(fp); return GRAD_ERR_FILE_CORRUPT; }
    if (fread(&grad_cnt, sizeof(int), 1, fp) != 1)     { fclose(fp); return GRAD_ERR_FILE_CORRUPT; }
    if (fread(&grade_cnt, sizeof(int), 1, fp) != 1)    { fclose(fp); return GRAD_ERR_FILE_CORRUPT; }

    if (adv_cnt < 0 || grad_cnt < 0 || grade_cnt < 0
        || adv_cnt > 100000 || grad_cnt > 100000 || grade_cnt > 500000) {
        fclose(fp); return GRAD_ERR_FILE_CORRUPT;
    }

    free_all_data();
    darray_init(&g_advisors, sizeof(Advisor));
    darray_init(&g_graduates, sizeof(Graduate));
    darray_init(&g_grades, sizeof(Grade));

    darray_reserve(&g_advisors, (adv_cnt > ARRAY_INIT_CAP) ? adv_cnt : ARRAY_INIT_CAP);
    darray_reserve(&g_graduates, (grad_cnt > ARRAY_INIT_CAP) ? grad_cnt : ARRAY_INIT_CAP);
    darray_reserve(&g_grades, (grade_cnt > ARRAY_INIT_CAP) ? grade_cnt : ARRAY_INIT_CAP);

    for (int i = 0; i < adv_cnt; i++) {
        Advisor a;
        memset(&a, 0, sizeof(a));
        if (fread(&a, sizeof(Advisor), 1, fp) != 1) { fclose(fp); return GRAD_ERR_FILE_CORRUPT; }
        darray_push(&g_advisors, &a);
    }
    for (int i = 0; i < grad_cnt; i++) {
        Graduate g;
        memset(&g, 0, sizeof(g));
        if (fread(&g, sizeof(Graduate), 1, fp) != 1) { fclose(fp); return GRAD_ERR_FILE_CORRUPT; }
        darray_push(&g_graduates, &g);
    }
    for (int i = 0; i < grade_cnt; i++) {
        Grade gr;
        memset(&gr, 0, sizeof(gr));
        if (fread(&gr, sizeof(Grade), 1, fp) != 1) { fclose(fp); return GRAD_ERR_FILE_CORRUPT; }
        darray_push(&g_grades, &gr);
    }

    fclose(fp);
    return GRAD_OK;
}


/*================================================================================
 *                          内存释放
 *================================================================================*/

static void free_all_data(void)
{
    darray_free(&g_advisors);
    darray_free(&g_graduates);
    darray_free(&g_grades);
}


/*================================================================================
 *                          交互式菜单系统
 *================================================================================*/

static void show_main_menu(void)
{
    printf("\n");
    print_line('=', 60);
    print_centered("研究生管理信息系统 V2.0", 60);
    print_line('=', 60);
    printf("  %s[1]%s 导师管理\n", COLOR_YELLOW, COLOR_RESET);
    printf("  %s[2]%s 研究生管理\n", COLOR_YELLOW, COLOR_RESET);
    printf("  %s[3]%s 导师分配\n", COLOR_YELLOW, COLOR_RESET);
    printf("  %s[4]%s 成绩管理\n", COLOR_YELLOW, COLOR_RESET);
    printf("  %s[5]%s 统计信息\n", COLOR_YELLOW, COLOR_RESET);
    printf("  %s[6]%s 保存数据到文件\n", COLOR_BLUE, COLOR_RESET);
    printf("  %s[7]%s 从文件加载数据\n", COLOR_BLUE, COLOR_RESET);
    printf("  %s[0]%s 退出系统\n", COLOR_RED, COLOR_RESET);
    print_line('=', 60);
}

static void advisor_menu(void)
{
    while (1) {
        printf("\n");
        print_line('*', 50);
        print_centered("导师管理", 50);
        print_line('*', 50);
        printf("  1. 查看所有导师\n");
        printf("  2. 添加导师\n");
        printf("  3. 修改导师信息\n");
        printf("  4. 删除导师\n");
        printf("  5. 查看某导师的研究生\n");
        printf("  0. 返回主菜单\n");
        print_line('-', 50);
        int choice = read_int("  请选择: ");
        char id[MAX_ID], name[MAX_NAME], title[MAX_TITLE], dept[MAX_DEPT];

        switch (choice) {
            case 1:
                advisor_list_all();
                break;
            case 2: {
                safe_gets(id, sizeof(id), "  导师工号: ");
                safe_gets(name, sizeof(name), "  导师姓名: ");
                safe_gets(title, sizeof(title), "  职称(教授/副教授/讲师): ");
                safe_gets(dept, sizeof(dept), "  所属院系: ");
                print_status(advisor_add(id, name, title, dept));
                break;
            }
            case 3: {
                safe_gets(id, sizeof(id), "  要修改的导师工号: ");
                safe_gets(name, sizeof(name), "  新姓名(回车不修改): ");
                safe_gets(title, sizeof(title), "  新职称(回车不修改): ");
                safe_gets(dept, sizeof(dept), "  新院系(回车不修改): ");
                print_status(advisor_update(id, name, title, dept));
                break;
            }
            case 4: {
                safe_gets(id, sizeof(id), "  要删除的导师工号: ");
                print_status(advisor_delete(id));
                break;
            }
            case 5: {
                safe_gets(id, sizeof(id), "  导师工号: ");
                list_students_by_advisor(id);
                break;
            }
            case 0:
                return;
            default:
                printf("  %s[!] 无效选择,请重新输入%s\n", COLOR_RED, COLOR_RESET);
        }
    }
}

static void graduate_menu(void)
{
    while (1) {
        printf("\n");
        print_line('*', 50);
        print_centered("研究生管理", 50);
        print_line('*', 50);
        printf("  1. 查看所有研究生\n");
        printf("  2. 添加研究生\n");
        printf("  3. 修改研究生信息\n");
        printf("  4. 删除研究生\n");
        printf("  0. 返回主菜单\n");
        print_line('-', 50);
        int choice = read_int("  请选择: ");
        char id[MAX_ID], name[MAX_NAME], gender[10], topic[MAX_TOPIC];
        int year;

        switch (choice) {
            case 1:
                graduate_list_all();
                break;
            case 2: {
                safe_gets(id, sizeof(id), "  学号: ");
                safe_gets(name, sizeof(name), "  姓名: ");
                safe_gets(gender, sizeof(gender), "  性别(男/女): ");
                year = read_int("  入学年份(如2024): ");
                safe_gets(topic, sizeof(topic), "  研究方向: ");
                print_status(graduate_add(id, name, gender, year, topic));
                break;
            }
            case 3: {
                safe_gets(id, sizeof(id), "  要修改的学号: ");
                safe_gets(name, sizeof(name), "  新姓名(回车不修改): ");
                safe_gets(gender, sizeof(gender), "  新性别(回车不修改): ");
                printf("  新入学年份(0不修改): "); fflush(stdout);
                year = read_int("");
                safe_gets(topic, sizeof(topic), "  新研究方向(回车不修改): ");
                print_status(graduate_update(id, name, gender, year, topic));
                break;
            }
            case 4: {
                safe_gets(id, sizeof(id), "  要删除的学号: ");
                print_status(graduate_delete(id));
                break;
            }
            case 0:
                return;
            default:
                printf("  %s[!] 无效选择%s\n", COLOR_RED, COLOR_RESET);
        }
    }
}

static void advisor_assignment_menu(void)
{
    while (1) {
        printf("\n");
        print_line('*', 50);
        print_centered("导师分配", 50);
        print_line('*', 50);
        printf("  1. 为研究生分配导师\n");
        printf("  2. 解除研究生导师分配\n");
        printf("  3. 查看某导师指导的研究生\n");
        printf("  4. 查看未分配导师的研究生\n");
        printf("  0. 返回主菜单\n");
        print_line('-', 50);
        int choice = read_int("  请选择: ");
        char sid[MAX_ID], aid[MAX_ID];

        switch (choice) {
            case 1:
                safe_gets(sid, sizeof(sid), "  研究生学号: ");
                safe_gets(aid, sizeof(aid), "  导师工号: ");
                print_status(assign_advisor(sid, aid));
                break;
            case 2:
                safe_gets(sid, sizeof(sid), "  研究生学号: ");
                print_status(unassign_advisor(sid));
                break;
            case 3:
                safe_gets(aid, sizeof(aid), "  导师工号: ");
                list_students_by_advisor(aid);
                break;
            case 4: {
                printf("\n  %s--- 未分配导师的研究生 ---%s\n", COLOR_YELLOW, COLOR_RESET);
                int cnt = 0;
                for (int i = 0; i < g_graduates.size; i++) {
                    Graduate *g = (Graduate *)darray_get(&g_graduates, i);
                    if (g != NULL && str_is_empty(g->advisor_id)) {
                        printf("  %-14s %-10s %s\n", g->id, g->name, g->research_topic);
                        cnt++;
                    }
                }
                if (cnt == 0) printf("  (所有研究生均已分配导师)\n");
                else printf("  共 %d 名研究生未分配导师\n", cnt);
                break;
            }
            case 0:
                return;
            default:
                printf("  %s[!] 无效选择%s\n", COLOR_RED, COLOR_RESET);
        }
    }
}

static void grade_menu(void)
{
    while (1) {
        printf("\n");
        print_line('*', 50);
        print_centered("成绩管理", 50);
        print_line('*', 50);
        printf("  1. 录入/添加成绩\n");
        printf("  2. 修改成绩\n");
        printf("  3. 删除成绩\n");
        printf("  4. 查询学生所有成绩\n");
        printf("  0. 返回主菜单\n");
        print_line('-', 50);
        int choice = read_int("  请选择: ");
        char sid[MAX_ID], course[MAX_COURSE];
        float score;

        switch (choice) {
            case 1:
                safe_gets(sid, sizeof(sid), "  学号: ");
                safe_gets(course, sizeof(course), "  课程名称: ");
                score = read_float("  成绩(0~100): ");
                print_status(grade_add(sid, course, score));
                break;
            case 2:
                safe_gets(sid, sizeof(sid), "  学号: ");
                safe_gets(course, sizeof(course), "  课程名称: ");
                score = read_float("  新成绩(0~100): ");
                print_status(grade_update(sid, course, score));
                break;
            case 3:
                safe_gets(sid, sizeof(sid), "  学号: ");
                safe_gets(course, sizeof(course), "  课程名称: ");
                print_status(grade_delete(sid, course));
                break;
            case 4:
                safe_gets(sid, sizeof(sid), "  学号: ");
                grade_list_by_student(sid);
                break;
            case 0:
                return;
            default:
                printf("  %s[!] 无效选择%s\n", COLOR_RED, COLOR_RESET);
        }
    }
}

static void statistics_menu(void)
{
    printf("\n");
    print_line('=', 60);
    print_centered("统计信息", 60);
    print_line('=', 60);
    printf("  导师总数:       %s%d%s\n", COLOR_GREEN, advisor_count(), COLOR_RESET);
    printf("  研究生总数:     %s%d%s\n", COLOR_GREEN, graduate_count(), COLOR_RESET);
    printf("  成绩记录总数:   %s%d%s\n", COLOR_GREEN, grade_count(), COLOR_RESET);

    int assigned = 0;
    for (int i = 0; i < g_graduates.size; i++) {
        Graduate *g = (Graduate *)darray_get(&g_graduates, i);
        if (g != NULL && !str_is_empty(g->advisor_id)) assigned++;
    }
    printf("  已分配导师:     %s%d%s / %d\n",
           COLOR_CYAN, assigned, COLOR_RESET, graduate_count());

    if (grade_count() > 0) {
        float total_sum = 0.0f;
        int total_cnt = 0;
        int pass_cnt = 0;
        float highest = -1.0f, lowest = 101.0f;
        for (int i = 0; i < g_grades.size; i++) {
            Grade *gr = (Grade *)darray_get(&g_grades, i);
            if (gr != NULL) {
                total_sum += gr->score;
                total_cnt++;
                if (gr->score >= 60.0f) pass_cnt++;
                if (gr->score > highest) highest = gr->score;
                if (gr->score < lowest)  lowest  = gr->score;
            }
        }
        if (total_cnt > 0) {
            print_line('-', 60);
            printf("  全局平均分:     %s%.2f%s\n", COLOR_CYAN, total_sum / (float)total_cnt, COLOR_RESET);
            printf("  最高分:         %s%.1f%s\n", COLOR_GREEN, highest, COLOR_RESET);
            printf("  最低分:         %s%.1f%s\n", COLOR_RED, lowest, COLOR_RESET);
            printf("  及格率:         %s%.1f%%%s (%d/%d)\n",
                   COLOR_YELLOW,
                   100.0f * (float)pass_cnt / (float)total_cnt,
                   COLOR_RESET, pass_cnt, total_cnt);
        }
    }
    print_line('=', 60);
}

static void run_interactive(void)
{
    GradStatus st = load_from_file(FILE_GRADV2);
    if (st == GRAD_OK) {
        printf("  %s[√] 已自动加载数据文件 (%s)%s\n",
               COLOR_GREEN, FILE_GRADV2, COLOR_RESET);
    } else if (st == GRAD_ERR_FILE_IO) {
        printf("  %s[!] 未找到数据文件,以空数据启动%s\n", COLOR_YELLOW, COLOR_RESET);
    } else {
        printf("  %s[!] 数据文件加载异常: %s%s\n", COLOR_RED, status_msg(st), COLOR_RESET);
    }

    while (1) {
        show_main_menu();
        int choice = read_int("  请选择操作: ");
        switch (choice) {
            case 1: advisor_menu(); break;
            case 2: graduate_menu(); break;
            case 3: advisor_assignment_menu(); break;
            case 4: grade_menu(); break;
            case 5: statistics_menu(); break;
            case 6: print_status(save_to_file(FILE_GRADV2)); break;
            case 7: print_status(load_from_file(FILE_GRADV2)); break;
            case 0:
                printf("\n  %s正在保存数据...%s\n", COLOR_CYAN, COLOR_RESET);
                print_status(save_to_file(FILE_GRADV2));
                printf("  %s感谢使用,再见!%s\n\n", COLOR_GREEN, COLOR_RESET);
                free_all_data();
                return;
            default:
                printf("  %s[!] 无效选择,请重新输入%s\n", COLOR_RED, COLOR_RESET);
        }
    }
}


/*================================================================================
 *                          内置自测框架与15+测试用例
 *================================================================================
 *
 * 测试框架设计:
 *   - test_assert(cond, name) 宏: 条件成立标记PASS,否则FAIL并计数
 *   - 测试涵盖: CRUD(导师/研究生/成绩)、导师分配、边界条件、文件I/O
 *   - 至少15项测试,覆盖V2全部核心功能
 *
 *================================================================================*/

static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(cond, name) do {                                  \
    if (cond) {                                                       \
        printf("  %s[PASS]%s %s\n", COLOR_GREEN, COLOR_RESET, name);  \
        g_tests_passed++;                                             \
    } else {                                                          \
        printf("  %s[FAIL]%s %s (line %d)\n",                         \
               COLOR_RED, COLOR_RESET, name, __LINE__);               \
        g_tests_failed++;                                             \
    }                                                                 \
} while (0)

static int run_self_tests(void)
{
    g_tests_passed = 0;
    g_tests_failed = 0;

    printf("\n%s", COLOR_BOLD);
    print_line('=', 60);
    print_centered("研究生管理系统 V2.0 — 内置自测", 60);
    print_line('=', 60);
    printf("%s\n", COLOR_RESET);

    /* ========== 测试 T01: 添加导师 ========== */
    GradStatus s;
    s = advisor_add("T001", "张教授", "教授", "计算机学院");
    TEST_ASSERT(s == GRAD_OK, "T01 添加导师(T001张教授)");

    /* ========== 测试 T02: 重复添加导师 ========== */
    s = advisor_add("T001", "张教授", "教授", "计算机学院");
    TEST_ASSERT(s == GRAD_ERR_DUPLICATE, "T02 重复添加导师应返回DUPLICATE");

    /* ========== 测试 T03: 添加更多导师 ========== */
    s = advisor_add("T002", "李副教授", "副教授", "软件学院");
    TEST_ASSERT(s == GRAD_OK && advisor_count() == 2, "T03 添加T002后导师数=2");

    s = advisor_add("T003", "王讲师", "讲师", "人工智能学院");
    TEST_ASSERT(s == GRAD_OK && advisor_count() == 3, "T04 添加T003后导师数=3");

    /* ========== 测试 T05: 查找导师 ========== */
    int idx = find_advisor_by_id("T002");
    TEST_ASSERT(idx >= 0, "T05 查找T002应存在");

    idx = find_advisor_by_id("T999");
    TEST_ASSERT(idx == -1, "T06 查找不存在的T999应返回-1");

    /* ========== 测试 T07: 修改导师信息 ========== */
    s = advisor_update("T002", "李副教授(博导)", "教授", "软件学院");
    TEST_ASSERT(s == GRAD_OK, "T07 修改T002信息成功");
    idx = find_advisor_by_id("T002");
    Advisor *a = (Advisor *)darray_get(&g_advisors, idx);
    TEST_ASSERT(a != NULL && strcmp(a->title, "教授") == 0, "T08 修改后T002职称=教授");

    /* ========== 测试 T09: 添加研究生 ========== */
    s = graduate_add("2024001", "小明", "男", 2024, "人工智能");
    TEST_ASSERT(s == GRAD_OK, "T09 添加研究生(2024001小明)");

    s = graduate_add("2024002", "小红", "女", 2024, "软件工程");
    TEST_ASSERT(s == GRAD_OK, "T10 添加研究生(2024002小红)");

    s = graduate_add("2024003", "小刚", "男", 2023, "计算机视觉");
    TEST_ASSERT(s == GRAD_OK && graduate_count() == 3, "T11 添加后研究生总数=3");

    /* ========== 测试 T12: 重复学号 ========== */
    s = graduate_add("2024001", "小明2", "男", 2024, "AI");
    TEST_ASSERT(s == GRAD_ERR_DUPLICATE, "T12 重复学号返回DUPLICATE");

    /* ========== 测试 T13: 导师分配 ========== */
    s = assign_advisor("2024001", "T001");
    TEST_ASSERT(s == GRAD_OK, "T13 为2024001分配导师T001成功");

    s = assign_advisor("2024001", "T002");
    TEST_ASSERT(s == GRAD_ERR_ALREADY_ASSIGNED, "T14 重复分配导师应返回ALREADY_ASSIGNED");

    s = assign_advisor("2024002", "T001");
    TEST_ASSERT(s == GRAD_OK, "T15 为2024002分配导师T001成功");

    s = assign_advisor("2024003", "T002");
    TEST_ASSERT(s == GRAD_OK, "T16 为2024003分配导师T002成功");

    /* ========== 测试 T17: 分配不存在的导师 ========== */
    s = assign_advisor("2024001", "T888");
    TEST_ASSERT(s == GRAD_ERR_ADVISOR_NOT_FOUND, "T17 分配不存在导师返回ADVISOR_NOT_FOUND");

    /* ========== 测试 T18: 解除导师分配 ========== */
    s = unassign_advisor("2024001");
    TEST_ASSERT(s == GRAD_OK, "T18 解除2024001导师分配成功");

    s = unassign_advisor("2024001");
    TEST_ASSERT(s == GRAD_ERR_NOT_ASSIGNED, "T19 重复解除应返回NOT_ASSIGNED");

    /* 重新分配T001给2024001用于后续测试 */
    assign_advisor("2024001", "T001");

    /* ========== 测试 T20: 添加成绩 ========== */
    s = grade_add("2024001", "高级算法", 92.5f);
    TEST_ASSERT(s == GRAD_OK, "T20 录入2024001高级算法92.5分");

    s = grade_add("2024001", "机器学习", 88.0f);
    TEST_ASSERT(s == GRAD_OK, "T21 录入2024001机器学习88分");

    s = grade_add("2024001", "英语", 76.5f);
    TEST_ASSERT(s == GRAD_OK, "T22 录入2024001英语76.5分");

    s = grade_add("2024002", "高级算法", 95.0f);
    TEST_ASSERT(s == GRAD_OK, "T23 录入2024002高级算法95分");

    /* ========== 测试 T24: 无效成绩 ========== */
    s = grade_add("2024001", "非法课程", 105.0f);
    TEST_ASSERT(s == GRAD_ERR_INVALID_SCORE, "T24 成绩>100应返回INVALID_SCORE");

    s = grade_add("2024001", "非法课程", -5.0f);
    TEST_ASSERT(s == GRAD_ERR_INVALID_SCORE, "T25 成绩<0应返回INVALID_SCORE");

    /* ========== 测试 T26: 重复成绩 ========== */
    s = grade_add("2024001", "高级算法", 80.0f);
    TEST_ASSERT(s == GRAD_ERR_DUPLICATE, "T26 重复添加同一课程成绩返回DUPLICATE");

    /* ========== 测试 T27: 修改成绩 ========== */
    s = grade_update("2024001", "英语", 82.0f);
    TEST_ASSERT(s == GRAD_OK, "T27 修改2024001英语成绩为82分");
    int gi = find_grade("2024001", "英语");
    Grade *gr = (Grade *)darray_get(&g_grades, gi);
    TEST_ASSERT(gr != NULL && gr->score > 81.9f && gr->score < 82.1f,
                "T28 修改后英语成绩确为82分");

    /* ========== 测试 T29: 平均分计算 ========== */
    float avg = grade_avg_by_student("2024001");
    TEST_ASSERT(avg > 87.0f && avg < 88.0f, "T29 2024001平均分为(92.5+88+82)/3≈87.5");

    /* ========== 测试 T30: 文件保存 ========== */
    s = save_to_file(FILE_GRADV2);
    TEST_ASSERT(s == GRAD_OK, "T30 保存数据到文件成功");

    /* ========== 测试 T31: 文件加载并验证数据完整性 ========== */
    free_all_data();
    darray_init(&g_advisors, sizeof(Advisor));
    darray_init(&g_graduates, sizeof(Graduate));
    darray_init(&g_grades, sizeof(Grade));

    s = load_from_file(FILE_GRADV2);
    TEST_ASSERT(s == GRAD_OK, "T31 从文件加载数据成功");
    TEST_ASSERT(advisor_count() == 3, "T32 加载后导师数=3");
    TEST_ASSERT(graduate_count() == 3, "T33 加载后研究生数=3");
    TEST_ASSERT(grade_count() == 4, "T34 加载后成绩记录数=4");

    /* ========== 测试 T35: 删除导师(级联清空学生导师ID) ========== */
    s = advisor_delete("T003");
    TEST_ASSERT(s == GRAD_OK && advisor_count() == 2, "T35 删除T003后导师数=2");

    /* ========== 测试 T36: 删除研究生(级联删除成绩) ========== */
    int grades_before = grade_count();
    s = graduate_delete("2024003");
    TEST_ASSERT(s == GRAD_OK && graduate_count() == 2, "T36 删除2024003后研究生数=2");
    TEST_ASSERT(grade_count() == grades_before, "T37 2024003无成绩记录,成绩数不变");

    /* ========== 测试 T38: 删除成绩 ========== */
    grades_before = grade_count();
    s = grade_delete("2024001", "英语");
    TEST_ASSERT(s == GRAD_OK && grade_count() == grades_before - 1,
                "T38 删除2024001英语成绩后记录数-1");

    /* ========== 测试 T39: 空输入校验 ========== */
    s = advisor_add("", "无名", "教授", "某院");
    TEST_ASSERT(s == GRAD_ERR_INVALID_INPUT, "T39 空导师ID返回INVALID_INPUT");

    s = graduate_add("", "无名", "男", 2024, "无");
    TEST_ASSERT(s == GRAD_ERR_INVALID_INPUT, "T40 空学号返回INVALID_INPUT");

    /* ========== 测试 T41: 删除不存在的实体 ========== */
    s = advisor_delete("T999");
    TEST_ASSERT(s == GRAD_ERR_ADVISOR_NOT_FOUND, "T41 删除不存在导师返回ADVISOR_NOT_FOUND");

    s = graduate_delete("9999999");
    TEST_ASSERT(s == GRAD_ERR_STUDENT_NOT_FOUND, "T42 删除不存在研究生返回STUDENT_NOT_FOUND");

    /* ========== 测试 T43: 动态数组扩容(>INIT_CAP) ========== */
    free_all_data();
    darray_init(&g_advisors, sizeof(Advisor));
    darray_init(&g_graduates, sizeof(Graduate));
    darray_init(&g_grades, sizeof(Grade));
    int bulk_ok = 1;
    for (int i = 0; i < 20; i++) {
        char tid[20], tname[50];
        sprintf(tid, "B%03d", i);
        sprintf(tname, "导师%d", i);
        if (advisor_add(tid, tname, "教授", "测试学院") != GRAD_OK) bulk_ok = 0;
    }
    TEST_ASSERT(bulk_ok && advisor_count() == 20, "T43 批量添加20位导师(触发扩容)成功");

    /* ========== 测试 T44: 文件魔数校验 ========== */
    FILE *fpt = fopen("bad_file.dat", "wb");
    if (fpt != NULL) {
        fprintf(fpt, "BADMAG");
        fclose(fpt);
        s = load_from_file("bad_file.dat");
        TEST_ASSERT(s == GRAD_ERR_FILE_MAGIC, "T44 错误魔数文件返回FILE_MAGIC");
        remove("bad_file.dat");
    } else {
        TEST_ASSERT(0, "T44 创建测试文件失败");
    }

    /* 清理测试数据 */
    free_all_data();
    darray_init(&g_advisors, sizeof(Advisor));
    darray_init(&g_graduates, sizeof(Graduate));
    darray_init(&g_grades, sizeof(Grade));

    printf("\n");
    print_line('=', 60);
    printf("  %s测试结果: %s%d 通过%s, %s%d 失败%s, 共 %d 项\n",
           COLOR_BOLD,
           COLOR_GREEN, g_tests_passed, COLOR_RESET,
           g_tests_failed > 0 ? COLOR_RED : COLOR_GREEN,
           g_tests_failed, COLOR_RESET,
           g_tests_passed + g_tests_failed);
    print_line('=', 60);

    int ret = (g_tests_failed == 0) ? 0 : 1;
    if (ret == 0) {
        printf("\n  %s所有测试通过! ✓%s\n\n", COLOR_BG_GREEN COLOR_WHITE, COLOR_RESET);
    } else {
        printf("\n  %s存在失败的测试! ✗%s\n\n", COLOR_BG_RED COLOR_WHITE, COLOR_RESET);
    }
    return ret;
}


/*================================================================================
 *                          CLI帮助信息
 *================================================================================*/

static void print_help(const char *progname)
{
    printf("%s", COLOR_BOLD);
    print_line('=', 60);
    print_centered("研究生管理信息系统 V2.0", 60);
    print_line('=', 60);
    printf("%s", COLOR_RESET);
    printf("  %s用法:%s %s [选项]\n\n", COLOR_CYAN, COLOR_RESET, progname);
    printf("  %s选项:%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("    -h, --help       显示此帮助信息\n");
    printf("    -t, --test       运行内置自测(15+项)\n");
    printf("    (无参数)         启动交互式菜单\n\n");
    printf("  %s数据文件:%s %s\n", COLOR_YELLOW, COLOR_RESET, FILE_GRADV2);
    printf("  %s文件格式:%s 二进制 (Magic: %s, Version: %d)\n\n",
           COLOR_YELLOW, COLOR_RESET, MAGIC_HEADER, FILE_VERSION);
    printf("  %s编译命令:%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("    gcc -std=c99 -Wall -Wextra -O2 -o grad.exe main.c\n");
    print_line('=', 60);
}


/*================================================================================
 *                          主函数入口
 *================================================================================
 *
 * 启动流程:
 *   1. init_console() — 设置UTF-8编码+ANSI彩色
 *   2. 初始化三个动态数组
 *   3. 解析CLI参数:
 *        -h / --help → 显示帮助并退出
 *        -t / --test → 运行自测并退出(返回0/1)
 *        无参数      → 进入交互式菜单
 *   4. 退出前释放所有动态内存
 *
 *================================================================================*/

int main(int argc, char *argv[])
{
    init_console();

    darray_init(&g_advisors, sizeof(Advisor));
    darray_init(&g_graduates, sizeof(Graduate));
    darray_init(&g_grades, sizeof(Grade));

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_help(argv[0]);
            free_all_data();
            return 0;
        }
        if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0) {
            int ret = run_self_tests();
            free_all_data();
            return ret;
        }
        printf("  %s[!] 未知参数: %s%s\n", COLOR_RED, argv[1], COLOR_RESET);
        printf("  使用 -h 或 --help 查看帮助\n");
        free_all_data();
        return 1;
    }

    run_interactive();
    return 0;
}
