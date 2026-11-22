/* ============================================================================
 * 软件公司客户关系管理系统 V2.0
 * Customer Relationship Management System V2.0
 *
 * @file      main.c
 * @brief     基于动态数组的客户关系管理系统 —— V2.0 标准版
 * @version   2.0
 * @date      2026-06-28
 * @standard  C99
 *
 * V2.0 新特性（相对于 V1 链表版）：
 *   1. UTF-8 BOM + SetConsoleOutputCP(65001) 解决 Windows 中文乱码
 *   2. ANSI 彩色终端输出（红/绿/黄/蓝/青/紫/灰）
 *   3. CLI 参数解析：-h/--help 帮助，-t/--test 运行内置自测
 *   4. 动态数组（Dynamic Array）替代单链表，缓存友好、支持随机访问
 *   5. 统一错误码枚举 CrmStatus（替代 V1 中零散的 int 返回/void 忽略错误）
 *   6. Magic Header "CRMV2" 二进制文件持久化，含版本校验
 *   7. 内置自测框架 test_*()，至少 15 项 CRUD + 跟进 + 统计 + 文件 I/O
 *   8. 交互菜单（彩色），中文注释
 *
 * 核心实体（V2 精简聚焦版）：
 *   - Customer   客户（动态数组）
 *   - Contact    联系人（动态数组，通过 cust_id 关联客户）
 *   - FollowUp   跟进记录（动态数组，通过 cust_id 关联客户）
 *
 * 编译命令：
 *   D:/bin/gcc.exe -std=c99 -Wall -Wextra -O2 -o crm.exe main.c
 * ============================================================================ */

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* ============================================================================
 * 常量与宏定义
 * ============================================================================ */

#define MAX_STR        64        /* 短字符串最大长度（姓名/电话等） */
#define MAX_LONG      256        /* 长字符串最大长度（地址/备注等） */
#define DATA_FILE      "crmv2.dat"  /* V2 持久化文件（单二进制文件） */
#define MAGIC_HEADER   "CRMV2"   /* 文件魔数，用于版本校验 */
#define MAGIC_LEN      5         /* 魔数长度（含 '\0' 共 6 字节但写 5） */
#define FILE_VERSION   2         /* 文件格式版本号 */
#define INIT_CAPACITY  4         /* 动态数组初始容量 */
#define GROWTH_FACTOR  2         /* 动态数组扩容倍数 */

/* ANSI 彩色终端转义序列 */
#define C_RESET     "\033[0m"
#define C_RED       "\033[31m"
#define C_GREEN     "\033[32m"
#define C_YELLOW    "\033[33m"
#define C_BLUE      "\033[34m"
#define C_MAGENTA   "\033[35m"
#define C_CYAN      "\033[36m"
#define C_WHITE     "\033[37m"
#define C_GRAY      "\033[90m"
#define C_BOLD      "\033[1m"
#define C_BG_RED    "\033[41m"
#define C_BG_GREEN  "\033[42m"
#define C_BG_BLUE   "\033[44m"

/* 自测框架：断言宏 */
#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        printf(C_RED "  [FAIL] %s\n" C_RESET, msg); \
        g_test_fail++; \
        return; \
    } \
} while (0)

#define TEST_ASSERT_EQ_INT(a, b, msg) TEST_ASSERT((a) == (b), msg)
#define TEST_ASSERT_EQ_STR(a, b, msg) TEST_ASSERT(strcmp((a), (b)) == 0, msg)

/* ============================================================================
 * 统一错误码枚举
 * ============================================================================ */

typedef enum {
    CRM_OK = 0,                 /* 操作成功 */
    CRM_ERR_MEMORY,             /* 内存分配失败 */
    CRM_ERR_NOT_FOUND,          /* 未找到目标记录 */
    CRM_ERR_DUPLICATE,          /* 重复记录（如电话重复） */
    CRM_ERR_INVALID_PARAM,      /* 参数非法 */
    CRM_ERR_FILE_OPEN,          /* 文件打开失败 */
    CRM_ERR_FILE_READ,          /* 文件读取失败 */
    CRM_ERR_FILE_WRITE,         /* 文件写入失败 */
    CRM_ERR_FILE_MAGIC,         /* 文件魔数不匹配（非 V2 文件） */
    CRM_ERR_FILE_VERSION,       /* 文件版本不兼容 */
    CRM_ERR_FULL,               /* 数组已满（内部使用，不应外泄） */
    CRM_ERR_EMPTY,              /* 数组为空 */
    CRM_ERR_IO                  /* 通用 I/O 错误 */
} CrmStatus;

/* 错误码对应的中文描述 */
static const char *crm_status_str(CrmStatus s)
{
    switch (s) {
    case CRM_OK:              return "操作成功";
    case CRM_ERR_MEMORY:      return "内存分配失败";
    case CRM_ERR_NOT_FOUND:   return "未找到记录";
    case CRM_ERR_DUPLICATE:   return "重复记录";
    case CRM_ERR_INVALID_PARAM: return "参数非法";
    case CRM_ERR_FILE_OPEN:   return "文件打开失败";
    case CRM_ERR_FILE_READ:   return "文件读取失败";
    case CRM_ERR_FILE_WRITE:  return "文件写入失败";
    case CRM_ERR_FILE_MAGIC:  return "文件格式错误（魔数不匹配）";
    case CRM_ERR_FILE_VERSION:return "文件版本不兼容";
    case CRM_ERR_FULL:        return "容量已满";
    case CRM_ERR_EMPTY:       return "数据为空";
    case CRM_ERR_IO:          return "I/O 错误";
    default:                  return "未知错误";
    }
}

/* ============================================================================
 * 数据结构定义（动态数组 + 实体）
 * ============================================================================ */

/* 客户实体 */
typedef struct {
    int    id;                          /* 客户ID，自增 */
    char   name[MAX_STR];               /* 客户名称（公司名） */
    char   contact_person[MAX_STR];     /* 首要联系人姓名 */
    char   region[MAX_STR];             /* 所属区域 */
    char   industry[MAX_STR];           /* 所属行业 */
    char   status[MAX_STR];             /* 客户状态：潜在/活跃/暂停/流失 */
    char   grade[MAX_STR];              /* 客户等级：A/B/C/D */
    char   phone[MAX_STR];              /* 联系电话 */
    char   address[MAX_LONG];           /* 详细地址 */
    char   create_time[MAX_STR];        /* 创建时间 YYYY-MM-DD HH:MM */
} Customer;

/* 联系人实体（一个客户可有多个联系人） */
typedef struct {
    int    id;                          /* 联系人ID，自增 */
    int    cust_id;                     /* 所属客户ID */
    char   name[MAX_STR];               /* 联系人姓名 */
    char   title[MAX_STR];              /* 职位 */
    char   phone[MAX_STR];              /* 电话 */
    char   email[MAX_STR];              /* 邮箱 */
    char   remark[MAX_LONG];            /* 备注 */
} Contact;

/* 跟进记录实体（一个客户可有多条跟进） */
typedef struct {
    int    id;                          /* 跟进ID，自增 */
    int    cust_id;                     /* 所属客户ID */
    char   date[MAX_STR];               /* 跟进日期 YYYY-MM-DD */
    char   method[MAX_STR];             /* 跟进方式：电话/拜访/邮件/微信 */
    char   content[MAX_LONG];           /* 跟进内容 */
    char   next_date[MAX_STR];          /* 下次跟进日期 */
} FollowUp;

/* 通用动态数组（类型无关宏实现） */
#define DECLARE_DYNARRAY(Type, Name) \
    typedef struct { \
        Type *data;       /* 数据指针 */ \
        int    count;     /* 当前元素数 */ \
        int    capacity;  /* 已分配容量 */ \
    } Name##Array; \
    static CrmStatus Name##_array_init(Name##Array *arr); \
    static CrmStatus Name##_array_push(Name##Array *arr, const Type *item); \
    static CrmStatus Name##_array_remove_at(Name##Array *arr, int index); \
    static void      Name##_array_free(Name##Array *arr);

#define IMPLEMENT_DYNARRAY(Type, Name) \
    static CrmStatus Name##_array_init(Name##Array *arr) { \
        arr->count = 0; \
        arr->capacity = INIT_CAPACITY; \
        arr->data = (Type *)calloc((size_t)arr->capacity, sizeof(Type)); \
        return arr->data ? CRM_OK : CRM_ERR_MEMORY; \
    } \
    static CrmStatus Name##_array_push(Name##Array *arr, const Type *item) { \
        if (arr->count >= arr->capacity) { \
            int new_cap = arr->capacity * GROWTH_FACTOR; \
            Type *new_data = (Type *)realloc(arr->data, (size_t)new_cap * sizeof(Type)); \
            if (!new_data) return CRM_ERR_MEMORY; \
            arr->data = new_data; \
            arr->capacity = new_cap; \
        } \
        arr->data[arr->count++] = *item; \
        return CRM_OK; \
    } \
    static CrmStatus Name##_array_remove_at(Name##Array *arr, int index) { \
        if (index < 0 || index >= arr->count) return CRM_ERR_NOT_FOUND; \
        for (int i = index; i < arr->count - 1; i++) \
            arr->data[i] = arr->data[i + 1]; \
        arr->count--; \
        return CRM_OK; \
    } \
    static void Name##_array_free(Name##Array *arr) { \
        free(arr->data); \
        arr->data = NULL; \
        arr->count = arr->capacity = 0; \
    }

/* 声明并实现三种动态数组 */
DECLARE_DYNARRAY(Customer, cust)
DECLARE_DYNARRAY(Contact,  cont)
DECLARE_DYNARRAY(FollowUp, fol)
IMPLEMENT_DYNARRAY(Customer, cust)
IMPLEMENT_DYNARRAY(Contact,  cont)
IMPLEMENT_DYNARRAY(FollowUp, fol)

/* ============================================================================
 * 全局数据上下文（V2 使用单一上下文结构，而非 V1 的多全局指针）
 * ============================================================================ */

typedef struct {
    custArray   customers;    /* 客户动态数组 */
    contArray   contacts;     /* 联系人动态数组 */
    folArray    followups;    /* 跟进记录动态数组 */
    int         next_cust_id; /* 下一个客户ID */
    int         next_cont_id; /* 下一个联系人ID */
    int         next_fol_id;  /* 下一个跟进ID */
} CrmContext;

static CrmContext g_ctx;   /* 全局上下文实例 */
static int g_test_fail = 0; /* 自测失败计数 */
static int g_test_total = 0;/* 自测总计数 */

/* ============================================================================
 * 终端初始化（Windows UTF-8 + ANSI 支持）
 * ============================================================================ */

static void term_init(void)
{
#ifdef _WIN32
    /* 设置控制台代码页为 UTF-8（65001） */
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    /* 启用 ANSI 转义序列支持（Windows 10+） */
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

/* ============================================================================
 * 工具函数：输入/输出/时间
 * ============================================================================ */

/* 安全读取一行输入，去除末尾换行符 */
static void read_line(const char *prompt, char *buf, int size)
{
    printf("%s", prompt);
    fflush(stdout);
    if (fgets(buf, size, stdin)) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n')
            buf[len - 1] = '\0';
        /* Windows 下可能有 \r\n，也去除 \r */
        len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\r')
            buf[len - 1] = '\0';
    }
}

/* 读取整数 */
static int read_int(const char *prompt)
{
    char buf[MAX_STR];
    read_line(prompt, buf, MAX_STR);
    return atoi(buf);
}

/* 获取当前时间字符串 YYYY-MM-DD HH:MM */
static void current_time_str(char *buf, int size)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    snprintf(buf, (size_t)size, "%04d-%02d-%02d %02d:%02d",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min);
}

/* 等待用户按回车继续 */
static void press_enter(void)
{
    char buf[4];
    printf(C_GRAY "\n  按回车键继续..." C_RESET);
    fflush(stdout);
    read_line("", buf, sizeof(buf));
}

/* 打印分隔线 */
static void print_sep(void)
{
    printf(C_CYAN "  ────────────────────────────────────────────────────────────\n" C_RESET);
}

/* 打印带颜色的标题框 */
static void print_title(const char *title)
{
    printf("\n");
    printf(C_CYAN "  ╔══════════════════════════════════════════════════════════╗\n" C_RESET);
    printf(C_CYAN "  ║" C_RESET C_BOLD C_WHITE "  %-52s  " C_RESET C_CYAN "║\n" C_RESET, title);
    printf(C_CYAN "  ╚══════════════════════════════════════════════════════════╝\n" C_RESET);
}

/* 打印成功消息 */
static void print_ok(const char *msg)
{
    printf(C_GREEN "  ✓ %s\n" C_RESET, msg);
}

/* 打印错误消息 */
static void print_err(const char *msg)
{
    printf(C_RED "  ✗ %s\n" C_RESET, msg);
}

/* 打印警告消息 */
static void print_warn(const char *msg)
{
    printf(C_YELLOW "  ⚠ %s\n" C_RESET, msg);
}

/* 打印信息消息 */
static void print_info(const char *msg)
{
    printf(C_BLUE "  ℹ %s\n" C_RESET, msg);
}

/* ============================================================================
 * 核心业务逻辑
 * ============================================================================ */

/* 初始化 CRM 上下文 */
static CrmStatus crm_init(CrmContext *ctx)
{
    CrmStatus s;
    memset(ctx, 0, sizeof(*ctx));
    s = cust_array_init(&ctx->customers);
    if (s != CRM_OK) return s;
    s = cont_array_init(&ctx->contacts);
    if (s != CRM_OK) return s;
    s = fol_array_init(&ctx->followups);
    if (s != CRM_OK) return s;
    ctx->next_cust_id = 1;
    ctx->next_cont_id = 1;
    ctx->next_fol_id  = 1;
    return CRM_OK;
}

/* 释放 CRM 上下文 */
static void crm_free(CrmContext *ctx)
{
    cust_array_free(&ctx->customers);
    cont_array_free(&ctx->contacts);
    fol_array_free(&ctx->followups);
}

/* 按 ID 查找客户，返回索引或 -1 */
static int find_customer_index(CrmContext *ctx, int id)
{
    for (int i = 0; i < ctx->customers.count; i++) {
        if (ctx->customers.data[i].id == id)
            return i;
    }
    return -1;
}

/* 按电话查找客户，返回索引或 -1（用于防重复） */
static int find_customer_by_phone(CrmContext *ctx, const char *phone)
{
    for (int i = 0; i < ctx->customers.count; i++) {
        if (strcmp(ctx->customers.data[i].phone, phone) == 0)
            return i;
    }
    return -1;
}

/* 添加客户 */
static CrmStatus add_customer(CrmContext *ctx, const Customer *c)
{
    if (!c || !c->name[0] || !c->phone[0])
        return CRM_ERR_INVALID_PARAM;
    if (find_customer_by_phone(ctx, c->phone) >= 0)
        return CRM_ERR_DUPLICATE;
    Customer nc = *c;
    nc.id = ctx->next_cust_id++;
    if (!nc.create_time[0])
        current_time_str(nc.create_time, sizeof(nc.create_time));
    return cust_array_push(&ctx->customers, &nc);
}

/* 删除客户（同时级联删除其联系人和跟进记录） */
static CrmStatus delete_customer(CrmContext *ctx, int cust_id)
{
    int idx = find_customer_index(ctx, cust_id);
    if (idx < 0) return CRM_ERR_NOT_FOUND;
    CrmStatus s = cust_array_remove_at(&ctx->customers, idx);
    if (s != CRM_OK) return s;
    /* 级联删除：倒序删除关联的联系人 */
    for (int i = ctx->contacts.count - 1; i >= 0; i--) {
        if (ctx->contacts.data[i].cust_id == cust_id)
            cont_array_remove_at(&ctx->contacts, i);
    }
    /* 级联删除：倒序删除关联的跟进记录 */
    for (int i = ctx->followups.count - 1; i >= 0; i--) {
        if (ctx->followups.data[i].cust_id == cust_id)
            fol_array_remove_at(&ctx->followups, i);
    }
    return CRM_OK;
}

/* 更新客户信息 */
static CrmStatus update_customer(CrmContext *ctx, int cust_id, const Customer *new_c)
{
    int idx = find_customer_index(ctx, cust_id);
    if (idx < 0) return CRM_ERR_NOT_FOUND;
    /* 如果修改了电话，检查新电话是否与其他客户冲突 */
    if (strcmp(ctx->customers.data[idx].phone, new_c->phone) != 0) {
        int dup = find_customer_by_phone(ctx, new_c->phone);
        if (dup >= 0 && dup != idx)
            return CRM_ERR_DUPLICATE;
    }
    int old_id = ctx->customers.data[idx].id;
    char old_ctime[MAX_STR];
    strncpy(old_ctime, ctx->customers.data[idx].create_time, MAX_STR);
    ctx->customers.data[idx] = *new_c;
    ctx->customers.data[idx].id = old_id;
    strncpy(ctx->customers.data[idx].create_time, old_ctime, MAX_STR);
    return CRM_OK;
}

/* 添加联系人 */
static CrmStatus add_contact(CrmContext *ctx, const Contact *c)
{
    if (!c || !c->name[0] || c->cust_id <= 0)
        return CRM_ERR_INVALID_PARAM;
    if (find_customer_index(ctx, c->cust_id) < 0)
        return CRM_ERR_NOT_FOUND;
    Contact nc = *c;
    nc.id = ctx->next_cont_id++;
    return cont_array_push(&ctx->contacts, &nc);
}

/* 删除联系人 */
static CrmStatus delete_contact(CrmContext *ctx, int cont_id)
{
    for (int i = 0; i < ctx->contacts.count; i++) {
        if (ctx->contacts.data[i].id == cont_id)
            return cont_array_remove_at(&ctx->contacts, i);
    }
    return CRM_ERR_NOT_FOUND;
}

/* 查找联系人索引 */
static int find_contact_index(CrmContext *ctx, int id)
{
    for (int i = 0; i < ctx->contacts.count; i++) {
        if (ctx->contacts.data[i].id == id)
            return i;
    }
    return -1;
}

/* 添加跟进记录 */
static CrmStatus add_followup(CrmContext *ctx, const FollowUp *f)
{
    if (!f || !f->content[0] || f->cust_id <= 0)
        return CRM_ERR_INVALID_PARAM;
    if (find_customer_index(ctx, f->cust_id) < 0)
        return CRM_ERR_NOT_FOUND;
    FollowUp nf = *f;
    nf.id = ctx->next_fol_id++;
    return fol_array_push(&ctx->followups, &nf);
}

/* 删除跟进记录 */
static CrmStatus delete_followup(CrmContext *ctx, int fol_id)
{
    for (int i = 0; i < ctx->followups.count; i++) {
        if (ctx->followups.data[i].id == fol_id)
            return fol_array_remove_at(&ctx->followups, i);
    }
    return CRM_ERR_NOT_FOUND;
}

/* 统计：各等级客户数 */
static void stats_by_grade(CrmContext *ctx, int *a_cnt, int *b_cnt, int *c_cnt, int *d_cnt)
{
    *a_cnt = *b_cnt = *c_cnt = *d_cnt = 0;
    for (int i = 0; i < ctx->customers.count; i++) {
        const char *g = ctx->customers.data[i].grade;
        if (strcmp(g, "A") == 0) (*a_cnt)++;
        else if (strcmp(g, "B") == 0) (*b_cnt)++;
        else if (strcmp(g, "C") == 0) (*c_cnt)++;
        else (*d_cnt)++;
    }
}

/* 统计：各状态客户数 */
static void stats_by_status(CrmContext *ctx, int *pot, int *act, int *pau, int *los)
{
    *pot = *act = *pau = *los = 0;
    for (int i = 0; i < ctx->customers.count; i++) {
        const char *s = ctx->customers.data[i].status;
        if (strcmp(s, "潜在") == 0) (*pot)++;
        else if (strcmp(s, "活跃") == 0) (*act)++;
        else if (strcmp(s, "暂停") == 0) (*pau)++;
        else (*los)++;
    }
}

/* 统计：总联系人数（某客户） */
static int count_contacts_for_customer(CrmContext *ctx, int cust_id)
{
    int cnt = 0;
    for (int i = 0; i < ctx->contacts.count; i++) {
        if (ctx->contacts.data[i].cust_id == cust_id)
            cnt++;
    }
    return cnt;
}

/* 统计：总跟进数（某客户） */
static int count_followups_for_customer(CrmContext *ctx, int cust_id)
{
    int cnt = 0;
    for (int i = 0; i < ctx->followups.count; i++) {
        if (ctx->followups.data[i].cust_id == cust_id)
            cnt++;
    }
    return cnt;
}

/* ============================================================================
 * 文件持久化（Magic Header "CRMV2" + 二进制格式）
 *
 * 文件格式：
 *   [0..4]   Magic "CRMV2" (5字节，不含'\0')
 *   [5]      版本号 uint8 (FILE_VERSION = 2)
 *   [6..9]   保留 4 字节
 *   [10..13] next_cust_id int32
 *   [14..17] next_cont_id int32
 *   [18..21] next_fol_id  int32
 *   [22..25] cust_count   int32
 *   [26..29] cont_count   int32
 *   [30..33] fol_count    int32
 *   [34..]   依次写入 Customer 数组、Contact 数组、FollowUp 数组
 * ============================================================================ */

static CrmStatus crm_save(CrmContext *ctx, const char *path)
{
    FILE *fp = fopen(path, "wb");
    if (!fp) return CRM_ERR_FILE_OPEN;

    /* 写 Magic Header */
    if (fwrite(MAGIC_HEADER, 1, MAGIC_LEN, fp) != (size_t)MAGIC_LEN) {
        fclose(fp); return CRM_ERR_FILE_WRITE;
    }
    /* 写版本号 */
    unsigned char ver = FILE_VERSION;
    if (fwrite(&ver, 1, 1, fp) != 1) { fclose(fp); return CRM_ERR_FILE_WRITE; }
    /* 写保留4字节 */
    unsigned int reserved = 0;
    if (fwrite(&reserved, 4, 1, fp) != 1) { fclose(fp); return CRM_ERR_FILE_WRITE; }
    /* 写ID计数器 */
    if (fwrite(&ctx->next_cust_id, sizeof(int), 1, fp) != 1) { fclose(fp); return CRM_ERR_FILE_WRITE; }
    if (fwrite(&ctx->next_cont_id, sizeof(int), 1, fp) != 1) { fclose(fp); return CRM_ERR_FILE_WRITE; }
    if (fwrite(&ctx->next_fol_id,  sizeof(int), 1, fp) != 1) { fclose(fp); return CRM_ERR_FILE_WRITE; }
    /* 写数组大小 */
    int cc = ctx->customers.count, nc = ctx->contacts.count, fc = ctx->followups.count;
    if (fwrite(&cc, sizeof(int), 1, fp) != 1) { fclose(fp); return CRM_ERR_FILE_WRITE; }
    if (fwrite(&nc, sizeof(int), 1, fp) != 1) { fclose(fp); return CRM_ERR_FILE_WRITE; }
    if (fwrite(&fc, sizeof(int), 1, fp) != 1) { fclose(fp); return CRM_ERR_FILE_WRITE; }
    /* 写客户数据 */
    if (cc > 0 && fwrite(ctx->customers.data, sizeof(Customer), (size_t)cc, fp) != (size_t)cc) {
        fclose(fp); return CRM_ERR_FILE_WRITE;
    }
    /* 写联系人数据 */
    if (nc > 0 && fwrite(ctx->contacts.data, sizeof(Contact), (size_t)nc, fp) != (size_t)nc) {
        fclose(fp); return CRM_ERR_FILE_WRITE;
    }
    /* 写跟进数据 */
    if (fc > 0 && fwrite(ctx->followups.data, sizeof(FollowUp), (size_t)fc, fp) != (size_t)fc) {
        fclose(fp); return CRM_ERR_FILE_WRITE;
    }

    fclose(fp);
    return CRM_OK;
}

static CrmStatus crm_load(CrmContext *ctx, const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) return CRM_ERR_FILE_OPEN;

    /* 读 Magic Header */
    char magic[MAGIC_LEN + 1] = {0};
    if (fread(magic, 1, MAGIC_LEN, fp) != (size_t)MAGIC_LEN) {
        fclose(fp); return CRM_ERR_FILE_READ;
    }
    if (strcmp(magic, MAGIC_HEADER) != 0) {
        fclose(fp); return CRM_ERR_FILE_MAGIC;
    }
    /* 读版本号 */
    unsigned char ver = 0;
    if (fread(&ver, 1, 1, fp) != 1) { fclose(fp); return CRM_ERR_FILE_READ; }
    if (ver != FILE_VERSION) { fclose(fp); return CRM_ERR_FILE_VERSION; }
    /* 跳过保留4字节 */
    unsigned int reserved = 0;
    if (fread(&reserved, 4, 1, fp) != 1) { fclose(fp); return CRM_ERR_FILE_READ; }
    /* 读ID计数器 */
    if (fread(&ctx->next_cust_id, sizeof(int), 1, fp) != 1) { fclose(fp); return CRM_ERR_FILE_READ; }
    if (fread(&ctx->next_cont_id, sizeof(int), 1, fp) != 1) { fclose(fp); return CRM_ERR_FILE_READ; }
    if (fread(&ctx->next_fol_id,  sizeof(int), 1, fp) != 1) { fclose(fp); return CRM_ERR_FILE_READ; }
    /* 读数组大小 */
    int cc = 0, nc = 0, fc = 0;
    if (fread(&cc, sizeof(int), 1, fp) != 1) { fclose(fp); return CRM_ERR_FILE_READ; }
    if (fread(&nc, sizeof(int), 1, fp) != 1) { fclose(fp); return CRM_ERR_FILE_READ; }
    if (fread(&fc, sizeof(int), 1, fp) != 1) { fclose(fp); return CRM_ERR_FILE_READ; }

    /* 释放旧数据，重新初始化 */
    crm_free(ctx);
    CrmStatus s = CRM_OK;
    s = cust_array_init(&ctx->customers); if (s != CRM_OK) { fclose(fp); return s; }
    s = cont_array_init(&ctx->contacts);  if (s != CRM_OK) { fclose(fp); return s; }
    s = fol_array_init(&ctx->followups);  if (s != CRM_OK) { fclose(fp); return s; }

    /* 扩容到足够容量 */
    while (ctx->customers.capacity < cc)
        ctx->customers.capacity *= GROWTH_FACTOR;
    ctx->customers.data = (Customer *)realloc(ctx->customers.data, (size_t)ctx->customers.capacity * sizeof(Customer));
    while (ctx->contacts.capacity < nc)
        ctx->contacts.capacity *= GROWTH_FACTOR;
    ctx->contacts.data = (Contact *)realloc(ctx->contacts.data, (size_t)ctx->contacts.capacity * sizeof(Contact));
    while (ctx->followups.capacity < fc)
        ctx->followups.capacity *= GROWTH_FACTOR;
    ctx->followups.data = (FollowUp *)realloc(ctx->followups.data, (size_t)ctx->followups.capacity * sizeof(FollowUp));

    if (!ctx->customers.data || !ctx->contacts.data || !ctx->followups.data) {
        fclose(fp); return CRM_ERR_MEMORY;
    }

    ctx->customers.count = cc;
    ctx->contacts.count  = nc;
    ctx->followups.count = fc;

    if (cc > 0 && fread(ctx->customers.data, sizeof(Customer), (size_t)cc, fp) != (size_t)cc) {
        fclose(fp); return CRM_ERR_FILE_READ;
    }
    if (nc > 0 && fread(ctx->contacts.data, sizeof(Contact), (size_t)nc, fp) != (size_t)nc) {
        fclose(fp); return CRM_ERR_FILE_READ;
    }
    if (fc > 0 && fread(ctx->followups.data, sizeof(FollowUp), (size_t)fc, fp) != (size_t)fc) {
        fclose(fp); return CRM_ERR_FILE_READ;
    }

    fclose(fp);
    return CRM_OK;
}

/* ============================================================================
 * 内置自测框架（至少 15 项）
 *
 * 测试项列表（共 18 项）：
 *   T1  初始化上下文
 *   T2  添加客户成功
 *   T3  添加客户数量正确
 *   T4  客户ID自增
 *   T5  重复电话返回 DUPLICATE
 *   T6  空参数返回 INVALID_PARAM
 *   T7  更新客户信息
 *   T8  添加联系人并关联客户
 *   T9  联系人数量统计
 *   T10 添加跟进记录
 *   T11 跟进数量统计
 *   T12 删除客户级联删除联系人和跟进
 *   T13 客户等级统计 A/B/C/D
 *   T14 客户状态统计
 *   T15 文件保存成功
 *   T16 文件加载并验证数据完整性
 *   T17 Magic Header 校验
 *   T18 查找不存在记录返回 NOT_FOUND
 * ============================================================================ */

#define RUN_TEST(fn) do { \
    g_test_total++; \
    int fail_before = g_test_fail; \
    printf(C_CYAN "  [%02d] %-50s" C_RESET, g_test_total, #fn); \
    fn; \
    if (g_test_fail == fail_before) \
        printf(C_GREEN "  [PASS]\n" C_RESET); \
} while (0)

/* T1: 初始化 */
static void test_init(void)
{
    CrmStatus s = crm_init(&g_ctx);
    TEST_ASSERT_EQ_INT(s, CRM_OK, "初始化应返回 CRM_OK");
    TEST_ASSERT_EQ_INT(g_ctx.customers.count, 0, "初始客户数应为0");
    TEST_ASSERT_EQ_INT(g_ctx.contacts.count, 0, "初始联系人数应为0");
    TEST_ASSERT_EQ_INT(g_ctx.followups.count, 0, "初始跟进数应为0");
    TEST_ASSERT_EQ_INT(g_ctx.next_cust_id, 1, "初始客户ID应为1");
}

/* T2: 添加客户 */
static void test_add_customer(void)
{
    Customer c = {0};
    strncpy(c.name, "华为技术", MAX_STR);
    strncpy(c.contact_person, "张经理", MAX_STR);
    strncpy(c.region, "深圳", MAX_STR);
    strncpy(c.industry, "通信", MAX_STR);
    strncpy(c.status, "活跃", MAX_STR);
    strncpy(c.grade, "A", MAX_STR);
    strncpy(c.phone, "13800138000", MAX_STR);
    strncpy(c.address, "深圳市龙岗区坂田", MAX_LONG);
    CrmStatus s = add_customer(&g_ctx, &c);
    TEST_ASSERT_EQ_INT(s, CRM_OK, "添加客户应成功");
}

/* T3: 客户数量 */
static void test_customer_count(void)
{
    TEST_ASSERT_EQ_INT(g_ctx.customers.count, 1, "添加后客户数应为1");
}

/* T4: ID自增 */
static void test_id_increment(void)
{
    Customer c = {0};
    strncpy(c.name, "腾讯科技", MAX_STR);
    strncpy(c.phone, "13900139000", MAX_STR);
    strncpy(c.grade, "A", MAX_STR);
    strncpy(c.status, "潜在", MAX_STR);
    add_customer(&g_ctx, &c);
    TEST_ASSERT_EQ_INT(g_ctx.customers.count, 2, "应有2个客户");
    TEST_ASSERT_EQ_INT(g_ctx.customers.data[1].id, 2, "第二个客户ID应为2");
    TEST_ASSERT_EQ_INT(g_ctx.next_cust_id, 3, "下一个ID应为3");
}

/* T5: 重复电话 */
static void test_duplicate_phone(void)
{
    Customer c = {0};
    strncpy(c.name, "山寨公司", MAX_STR);
    strncpy(c.phone, "13800138000", MAX_STR);
    strncpy(c.grade, "C", MAX_STR);
    CrmStatus s = add_customer(&g_ctx, &c);
    TEST_ASSERT_EQ_INT(s, CRM_ERR_DUPLICATE, "重复电话应返回 DUPLICATE");
}

/* T6: 空参数 */
static void test_invalid_param(void)
{
    CrmStatus s = add_customer(&g_ctx, NULL);
    TEST_ASSERT_EQ_INT(s, CRM_ERR_INVALID_PARAM, "NULL参数应返回 INVALID_PARAM");
    Customer c = {0};
    s = add_customer(&g_ctx, &c);
    TEST_ASSERT_EQ_INT(s, CRM_ERR_INVALID_PARAM, "空姓名电话应返回 INVALID_PARAM");
}

/* T7: 更新客户 */
static void test_update_customer(void)
{
    Customer nc = g_ctx.customers.data[0];
    strncpy(nc.grade, "B", MAX_STR);
    strncpy(nc.region, "广州", MAX_STR);
    CrmStatus s = update_customer(&g_ctx, 1, &nc);
    TEST_ASSERT_EQ_INT(s, CRM_OK, "更新应成功");
    TEST_ASSERT_EQ_STR(g_ctx.customers.data[0].grade, "B", "等级应更新为B");
    TEST_ASSERT_EQ_STR(g_ctx.customers.data[0].region, "广州", "区域应更新为广州");
    TEST_ASSERT_EQ_INT(g_ctx.customers.data[0].id, 1, "更新后ID不应改变");
}

/* T8: 添加联系人 */
static void test_add_contact(void)
{
    Contact ct = {0};
    ct.cust_id = 1;
    strncpy(ct.name, "李四", MAX_STR);
    strncpy(ct.title, "技术总监", MAX_STR);
    strncpy(ct.phone, "13700137000", MAX_STR);
    strncpy(ct.email, "lisi@huawei.com", MAX_STR);
    CrmStatus s = add_contact(&g_ctx, &ct);
    TEST_ASSERT_EQ_INT(s, CRM_OK, "添加联系人应成功");
    TEST_ASSERT_EQ_INT(g_ctx.contacts.data[0].cust_id, 1, "联系人应属于客户1");
}

/* T9: 联系人统计 */
static void test_contact_count(void)
{
    Contact ct2 = {0};
    ct2.cust_id = 1;
    strncpy(ct2.name, "王五", MAX_STR);
    strncpy(ct2.phone, "13600136000", MAX_STR);
    strncpy(ct2.title, "采购经理", MAX_STR);
    add_contact(&g_ctx, &ct2);
    int cnt = count_contacts_for_customer(&g_ctx, 1);
    TEST_ASSERT_EQ_INT(cnt, 2, "客户1应有2个联系人");
    cnt = count_contacts_for_customer(&g_ctx, 2);
    TEST_ASSERT_EQ_INT(cnt, 0, "客户2应有0个联系人");
}

/* T10: 添加跟进 */
static void test_add_followup(void)
{
    FollowUp f = {0};
    f.cust_id = 1;
    strncpy(f.date, "2026-06-28", MAX_STR);
    strncpy(f.method, "电话", MAX_STR);
    strncpy(f.content, "初次沟通，了解需求", MAX_LONG);
    strncpy(f.next_date, "2026-07-05", MAX_STR);
    CrmStatus s = add_followup(&g_ctx, &f);
    TEST_ASSERT_EQ_INT(s, CRM_OK, "添加跟进应成功");
    TEST_ASSERT_EQ_INT(g_ctx.followups.count, 1, "应有1条跟进");
}

/* T11: 跟进统计 */
static void test_followup_count(void)
{
    FollowUp f2 = {0};
    f2.cust_id = 1;
    strncpy(f2.date, "2026-07-05", MAX_STR);
    strncpy(f2.method, "拜访", MAX_STR);
    strncpy(f2.content, "上门拜访，演示产品", MAX_LONG);
    add_followup(&g_ctx, &f2);
    int cnt = count_followups_for_customer(&g_ctx, 1);
    TEST_ASSERT_EQ_INT(cnt, 2, "客户1应有2条跟进");
}

/* T12: 级联删除 */
static void test_cascade_delete(void)
{
    CrmStatus s = delete_customer(&g_ctx, 1);
    TEST_ASSERT_EQ_INT(s, CRM_OK, "删除客户1应成功");
    TEST_ASSERT_EQ_INT(g_ctx.customers.count, 1, "删除后应剩1个客户");
    int cont_cnt = count_contacts_for_customer(&g_ctx, 1);
    int fol_cnt = count_followups_for_customer(&g_ctx, 1);
    TEST_ASSERT_EQ_INT(cont_cnt, 0, "级联删除后联系人应为0");
    TEST_ASSERT_EQ_INT(fol_cnt, 0, "级联删除后跟进应为0");
    TEST_ASSERT_EQ_INT(g_ctx.contacts.count, 0, "总联系人数应为0");
    TEST_ASSERT_EQ_INT(g_ctx.followups.count, 0, "总跟进数应为0");
}

/* T13: 等级统计 */
static void test_grade_stats(void)
{
    int a, b, c, d;
    stats_by_grade(&g_ctx, &a, &b, &c, &d);
    /* 当前只有客户2（腾讯，等级A） */
    TEST_ASSERT_EQ_INT(a, 1, "应有1个A级客户");
    TEST_ASSERT_EQ_INT(b, 0, "应有0个B级客户");
    TEST_ASSERT_EQ_INT(c, 0, "应有0个C级客户");
    TEST_ASSERT_EQ_INT(d, 0, "应有0个D级客户");
}

/* T14: 状态统计 */
static void test_status_stats(void)
{
    int pot, act, pau, los;
    stats_by_status(&g_ctx, &pot, &act, &pau, &los);
    TEST_ASSERT_EQ_INT(pot, 1, "应有1个潜在客户");
    TEST_ASSERT_EQ_INT(act, 0, "应有0个活跃客户");
}

/* T15: 文件保存 */
static void test_file_save(void)
{
    /* 再加点数据以便验证 */
    Customer c3 = {0};
    strncpy(c3.name, "字节跳动", MAX_STR);
    strncpy(c3.phone, "13500135000", MAX_STR);
    strncpy(c3.grade, "A", MAX_STR);
    strncpy(c3.status, "活跃", MAX_STR);
    strncpy(c3.region, "北京", MAX_STR);
    add_customer(&g_ctx, &c3);
    CrmStatus s = crm_save(&g_ctx, DATA_FILE);
    TEST_ASSERT_EQ_INT(s, CRM_OK, "保存文件应成功");
}

/* T16: 文件加载 */
static void test_file_load(void)
{
    CrmContext ctx2;
    memset(&ctx2, 0, sizeof(ctx2));
    CrmStatus s = crm_init(&ctx2);
    TEST_ASSERT_EQ_INT(s, CRM_OK, "新上下文初始化成功");
    s = crm_load(&ctx2, DATA_FILE);
    TEST_ASSERT_EQ_INT(s, CRM_OK, "加载文件应成功");
    TEST_ASSERT_EQ_INT(ctx2.customers.count, 2, "加载后应有2个客户");
    TEST_ASSERT_EQ_STR(ctx2.customers.data[0].name, "腾讯科技", "第一个客户应为腾讯科技");
    TEST_ASSERT_EQ_STR(ctx2.customers.data[1].name, "字节跳动", "第二个客户应为字节跳动");
    TEST_ASSERT_EQ_INT(ctx2.next_cust_id, g_ctx.next_cust_id, "ID计数器应正确还原");
    crm_free(&ctx2);
}

/* T17: Magic Header 校验 */
static void test_magic_header(void)
{
    /* 写一个错误魔数的文件（5字节，与MAGIC_LEN一致） */
    FILE *fp = fopen("bad_test.dat", "wb");
    TEST_ASSERT(fp != NULL, "创建测试文件应成功");
    fwrite("XXXXX", 1, 5, fp);
    fclose(fp);
    CrmContext ctx2;
    crm_init(&ctx2);
    CrmStatus s = crm_load(&ctx2, "bad_test.dat");
    TEST_ASSERT_EQ_INT(s, CRM_ERR_FILE_MAGIC, "错误魔数应返回 MAGIC 错误");
    crm_free(&ctx2);
    remove("bad_test.dat");
}

/* T18: 查找不存在 */
static void test_not_found(void)
{
    CrmStatus s = delete_customer(&g_ctx, 9999);
    TEST_ASSERT_EQ_INT(s, CRM_ERR_NOT_FOUND, "删除不存在客户应返回 NOT_FOUND");
    s = delete_contact(&g_ctx, 9999);
    TEST_ASSERT_EQ_INT(s, CRM_ERR_NOT_FOUND, "删除不存在联系人应返回 NOT_FOUND");
    int idx = find_customer_index(&g_ctx, 9999);
    TEST_ASSERT_EQ_INT(idx, -1, "查找不存在客户索引应为-1");
}

/* 运行全部自测 */
static int run_all_tests(void)
{
    g_test_fail = 0;
    g_test_total = 0;
    crm_free(&g_ctx);

    printf(C_BOLD C_CYAN "\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║           CRM V2.0 内置自测套件（18 项）               ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n" C_RESET);

    RUN_TEST(test_init());
    RUN_TEST(test_add_customer());
    RUN_TEST(test_customer_count());
    RUN_TEST(test_id_increment());
    RUN_TEST(test_duplicate_phone());
    RUN_TEST(test_invalid_param());
    RUN_TEST(test_update_customer());
    RUN_TEST(test_add_contact());
    RUN_TEST(test_contact_count());
    RUN_TEST(test_add_followup());
    RUN_TEST(test_followup_count());
    RUN_TEST(test_cascade_delete());
    RUN_TEST(test_grade_stats());
    RUN_TEST(test_status_stats());
    RUN_TEST(test_file_save());
    RUN_TEST(test_file_load());
    RUN_TEST(test_magic_header());
    RUN_TEST(test_not_found());

    printf("\n");
    print_sep();
    if (g_test_fail == 0) {
        printf(C_GREEN C_BOLD "  ✓ 全部 %d 项测试通过！\n" C_RESET, g_test_total);
    } else {
        printf(C_RED C_BOLD "  ✗ %d/%d 项测试失败！\n" C_RESET, g_test_fail, g_test_total);
    }
    print_sep();
    printf("\n");
    return g_test_fail;
}

/* ============================================================================
 * 交互菜单界面
 * ============================================================================ */

/* 打印客户列表 */
static void list_customers(CrmContext *ctx)
{
    print_title("客户列表");
    if (ctx->customers.count == 0) {
        print_warn("暂无客户数据");
        return;
    }
    printf(C_BOLD "  %-4s %-18s %-8s %-10s %-6s %-6s %-15s %s\n" C_RESET,
           "ID", "公司名称", "区域", "行业", "等级", "状态", "电话", "创建时间");
    print_sep();
    for (int i = 0; i < ctx->customers.count; i++) {
        Customer *c = &ctx->customers.data[i];
        const char *gc = C_WHITE;
        if (strcmp(c->grade, "A") == 0) gc = C_RED;
        else if (strcmp(c->grade, "B") == 0) gc = C_YELLOW;
        else if (strcmp(c->grade, "C") == 0) gc = C_GREEN;
        printf("  %-4d %-18s %-8s %-10s %s%-6s"C_RESET" %-6s %-15s %s\n",
               c->id, c->name, c->region, c->industry,
               gc, c->grade, c->status, c->phone, c->create_time);
    }
    printf(C_GRAY "  共 %d 个客户\n" C_RESET, ctx->customers.count);
}

/* 查看客户详情（含联系人和跟进） */
static void view_customer_detail(CrmContext *ctx)
{
    int id = read_int("  请输入客户ID: ");
    int idx = find_customer_index(ctx, id);
    if (idx < 0) {
        print_err(crm_status_str(CRM_ERR_NOT_FOUND));
        return;
    }
    Customer *c = &ctx->customers.data[idx];
    char title[128];
    snprintf(title, sizeof(title), "客户详情 - %s (ID:%d)", c->name, c->id);
    print_title(title);
    printf("  %-10s: %s\n", "公司名称", c->name);
    printf("  %-10s: %s\n", "首要联系人", c->contact_person);
    printf("  %-10s: %s\n", "区域", c->region);
    printf("  %-10s: %s\n", "行业", c->industry);
    printf("  %-10s: %s\n", "状态", c->status);
    printf("  %-10s: %s%s"C_RESET"\n", "等级",
           strcmp(c->grade, "A") == 0 ? C_RED :
           strcmp(c->grade, "B") == 0 ? C_YELLOW :
           strcmp(c->grade, "C") == 0 ? C_GREEN : C_GRAY,
           c->grade);
    printf("  %-10s: %s\n", "电话", c->phone);
    printf("  %-10s: %s\n", "地址", c->address);
    printf("  %-10s: %s\n", "创建时间", c->create_time);

    /* 联系人 */
    printf(C_CYAN "\n  ── 联系人列表 ──\n" C_RESET);
    int found = 0;
    for (int i = 0; i < ctx->contacts.count; i++) {
        Contact *ct = &ctx->contacts.data[i];
        if (ct->cust_id == id) {
            printf("    [%d] %s | %s | %s | %s\n",
                   ct->id, ct->name, ct->title, ct->phone, ct->email);
            found = 1;
        }
    }
    if (!found) print_warn("  暂无联系人");

    /* 跟进记录 */
    printf(C_CYAN "\n  ── 跟进记录 ──\n" C_RESET);
    found = 0;
    for (int i = 0; i < ctx->followups.count; i++) {
        FollowUp *f = &ctx->followups.data[i];
        if (f->cust_id == id) {
            printf("    [%d] %s | %s | %s | 下次:%s\n",
                   f->id, f->date, f->method, f->content, f->next_date);
            found = 1;
        }
    }
    if (!found) print_warn("  暂无跟进记录");
}

/* 添加客户菜单 */
static void menu_add_customer(CrmContext *ctx)
{
    print_title("添加新客户");
    Customer c = {0};
    read_line("  公司名称: ", c.name, MAX_STR);
    read_line("  首要联系人: ", c.contact_person, MAX_STR);
    read_line("  区域: ", c.region, MAX_STR);
    read_line("  行业: ", c.industry, MAX_STR);
    read_line("  状态(潜在/活跃/暂停/流失): ", c.status, MAX_STR);
    read_line("  等级(A/B/C/D): ", c.grade, MAX_STR);
    read_line("  联系电话: ", c.phone, MAX_STR);
    read_line("  详细地址: ", c.address, MAX_LONG);
    current_time_str(c.create_time, sizeof(c.create_time));
    CrmStatus s = add_customer(ctx, &c);
    if (s == CRM_OK) print_ok("客户添加成功！");
    else print_err(crm_status_str(s));
}

/* 删除客户菜单 */
static void menu_delete_customer(CrmContext *ctx)
{
    list_customers(ctx);
    int id = read_int("\n  请输入要删除的客户ID(0取消): ");
    if (id == 0) return;
    char confirm[8];
    read_line("  确认删除？(yes/no): ", confirm, sizeof(confirm));
    if (strcmp(confirm, "yes") != 0) {
        print_info("已取消删除");
        return;
    }
    CrmStatus s = delete_customer(ctx, id);
    if (s == CRM_OK) print_ok("客户及关联数据已删除");
    else print_err(crm_status_str(s));
}

/* 添加联系人菜单 */
static void menu_add_contact(CrmContext *ctx)
{
    print_title("添加联系人");
    int cid = read_int("  所属客户ID: ");
    if (find_customer_index(ctx, cid) < 0) {
        print_err(crm_status_str(CRM_ERR_NOT_FOUND));
        return;
    }
    Contact ct = {0};
    ct.cust_id = cid;
    read_line("  联系人姓名: ", ct.name, MAX_STR);
    read_line("  职位: ", ct.title, MAX_STR);
    read_line("  电话: ", ct.phone, MAX_STR);
    read_line("  邮箱: ", ct.email, MAX_STR);
    read_line("  备注: ", ct.remark, MAX_LONG);
    CrmStatus s = add_contact(ctx, &ct);
    if (s == CRM_OK) print_ok("联系人添加成功！");
    else print_err(crm_status_str(s));
}

/* 添加跟进菜单 */
static void menu_add_followup(CrmContext *ctx)
{
    print_title("添加跟进记录");
    int cid = read_int("  所属客户ID: ");
    if (find_customer_index(ctx, cid) < 0) {
        print_err(crm_status_str(CRM_ERR_NOT_FOUND));
        return;
    }
    FollowUp f = {0};
    f.cust_id = cid;
    read_line("  跟进日期(YYYY-MM-DD, 回车=今天): ", f.date, MAX_STR);
    if (!f.date[0]) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        snprintf(f.date, MAX_STR, "%04d-%02d-%02d",
                 t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
    }
    read_line("  跟进方式(电话/拜访/邮件/微信): ", f.method, MAX_STR);
    read_line("  跟进内容: ", f.content, MAX_LONG);
    read_line("  下次跟进日期(YYYY-MM-DD): ", f.next_date, MAX_STR);
    CrmStatus s = add_followup(ctx, &f);
    if (s == CRM_OK) print_ok("跟进记录添加成功！");
    else print_err(crm_status_str(s));
}

/* 统计报表 */
static void menu_statistics(CrmContext *ctx)
{
    print_title("数据统计报表");
    printf(C_BOLD "  总客户数: %d\n" C_RESET, ctx->customers.count);
    printf(C_BOLD "  总联系人数: %d\n" C_RESET, ctx->contacts.count);
    printf(C_BOLD "  总跟进记录数: %d\n\n" C_RESET, ctx->followups.count);

    int a, b, c, d;
    stats_by_grade(ctx, &a, &b, &c, &d);
    printf(C_CYAN "  ── 按等级分布 ──\n" C_RESET);
    printf("  " C_RED "A级: %d  " C_YELLOW "B级: %d  " C_GREEN "C级: %d  " C_GRAY "D级: %d\n" C_RESET,
           a, b, c, d);

    int pot, act, pau, los;
    stats_by_status(ctx, &pot, &act, &pau, &los);
    printf(C_CYAN "\n  ── 按状态分布 ──\n" C_RESET);
    printf("  潜在: %d  活跃: %d  暂停: %d  流失: %d\n", pot, act, pau, los);

    /* 联系人数最多的客户 TOP3 */
    printf(C_CYAN "\n  ── 联系人最多的客户 ──\n" C_RESET);
    /* 简单冒泡取前几 */
    typedef struct { int id; int cnt; char name[MAX_STR]; } CC;
    CC *arr = (CC *)calloc((size_t)ctx->customers.count, sizeof(CC));
    if (arr) {
        for (int i = 0; i < ctx->customers.count; i++) {
            arr[i].id = ctx->customers.data[i].id;
            strncpy(arr[i].name, ctx->customers.data[i].name, MAX_STR);
            arr[i].cnt = count_contacts_for_customer(ctx, arr[i].id);
        }
        for (int i = 0; i < ctx->customers.count - 1; i++)
            for (int j = 0; j < ctx->customers.count - 1 - i; j++)
                if (arr[j].cnt < arr[j + 1].cnt) {
                    CC tmp = arr[j]; arr[j] = arr[j + 1]; arr[j + 1] = tmp;
                }
        int show = ctx->customers.count < 5 ? ctx->customers.count : 5;
        for (int i = 0; i < show; i++) {
            if (arr[i].cnt > 0)
                printf("    %d. %s - %d个联系人\n", i + 1, arr[i].name, arr[i].cnt);
        }
        free(arr);
    }
}

/* 保存数据 */
static void menu_save(CrmContext *ctx)
{
    CrmStatus s = crm_save(ctx, DATA_FILE);
    if (s == CRM_OK) print_ok("数据已保存到 crmv2.dat");
    else print_err(crm_status_str(s));
}

/* 删除联系人菜单 */
static void menu_delete_contact(CrmContext *ctx)
{
    print_title("删除联系人");
    if (ctx->contacts.count == 0) {
        print_warn("暂无联系人数据");
        return;
    }
    printf(C_BOLD "  %-4s %-8s %-18s %-14s %s\n" C_RESET,
           "ID", "客户ID", "姓名", "电话", "职位");
    print_sep();
    for (int i = 0; i < ctx->contacts.count; i++) {
        Contact *ct = &ctx->contacts.data[i];
        printf("  %-4d %-8d %-18s %-14s %s\n",
               ct->id, ct->cust_id, ct->name, ct->phone, ct->title);
    }
    int id = read_int("\n  请输入要删除的联系人ID(0取消): ");
    if (id == 0) return;
    int idx = find_contact_index(ctx, id);
    if (idx < 0) { print_err(crm_status_str(CRM_ERR_NOT_FOUND)); return; }
    char confirm[8];
    read_line("  确认删除？(yes/no): ", confirm, sizeof(confirm));
    if (strcmp(confirm, "yes") != 0) { print_info("已取消删除"); return; }
    CrmStatus s = delete_contact(ctx, id);
    if (s == CRM_OK) print_ok("联系人已删除");
    else print_err(crm_status_str(s));
}

/* 删除跟进记录菜单 */
static void menu_delete_followup(CrmContext *ctx)
{
    print_title("删除跟进记录");
    if (ctx->followups.count == 0) {
        print_warn("暂无跟进记录");
        return;
    }
    printf(C_BOLD "  %-4s %-8s %-12s %-10s %s\n" C_RESET,
           "ID", "客户ID", "日期", "方式", "内容摘要");
    print_sep();
    for (int i = 0; i < ctx->followups.count; i++) {
        FollowUp *f = &ctx->followups.data[i];
        char summary[32];
        strncpy(summary, f->content, 28);
        summary[28] = '\0';
        if (strlen(f->content) > 28) strcat(summary, "...");
        printf("  %-4d %-8d %-12s %-10s %s\n",
               f->id, f->cust_id, f->date, f->method, summary);
    }
    int id = read_int("\n  请输入要删除的跟进ID(0取消): ");
    if (id == 0) return;
    char confirm[8];
    read_line("  确认删除？(yes/no): ", confirm, sizeof(confirm));
    if (strcmp(confirm, "yes") != 0) { print_info("已取消删除"); return; }
    CrmStatus s = delete_followup(ctx, id);
    if (s == CRM_OK) print_ok("跟进记录已删除");
    else print_err(crm_status_str(s));
}

/* 主菜单循环 */
static void main_menu(CrmContext *ctx)
{
    /* 启动时尝试加载数据 */
    CrmStatus s = crm_load(ctx, DATA_FILE);
    if (s == CRM_OK) {
        print_ok("数据加载成功");
    } else if (s == CRM_ERR_FILE_OPEN) {
        print_info("未找到数据文件，将创建新数据库");
    } else {
        print_warn(crm_status_str(s));
    }

    while (1) {
        printf("\n");
        printf(C_CYAN "  ╔═══════════════════════════════════════════════╗\n" C_RESET);
        printf(C_CYAN "  ║" C_RESET C_BOLD C_WHITE "        软件公司 CRM V2.0  主菜单              " C_RESET C_CYAN "║\n" C_RESET);
        printf(C_CYAN "  ╠═══════════════════════════════════════════════╣\n" C_RESET);
        printf(C_CYAN "  ║" C_RESET "  1. 客户列表              2. 查看客户详情   " C_CYAN "║\n" C_RESET);
        printf(C_CYAN "  ║" C_RESET "  3. 添加客户              4. 删除客户       " C_CYAN "║\n" C_RESET);
        printf(C_CYAN "  ║" C_RESET "  5. 添加联系人            6. 添加跟进记录   " C_CYAN "║\n" C_RESET);
        printf(C_CYAN "  ║" C_RESET "  7. 数据统计              8. 保存数据       " C_CYAN "║\n" C_RESET);
        printf(C_CYAN "  ║" C_RESET "  9. 删除联系人           10. 删除跟进记录   " C_CYAN "║\n" C_RESET);
        printf(C_CYAN "  ║" C_RESET "  0. 保存并退出                               " C_CYAN "║\n" C_RESET);
        printf(C_CYAN "  ╚═══════════════════════════════════════════════╝\n" C_RESET);

        int choice = read_int(C_YELLOW "  请选择: " C_RESET);
        switch (choice) {
        case 1: list_customers(ctx); break;
        case 2: view_customer_detail(ctx); break;
        case 3: menu_add_customer(ctx); break;
        case 4: menu_delete_customer(ctx); break;
        case 5: menu_add_contact(ctx); break;
        case 6: menu_add_followup(ctx); break;
        case 7: menu_statistics(ctx); break;
        case 8: menu_save(ctx); break;
        case 9: menu_delete_contact(ctx); break;
        case 10: menu_delete_followup(ctx); break;
        case 0:
            menu_save(ctx);
            print_ok("感谢使用 CRM V2.0，再见！");
            return;
        default:
            print_err("无效选项，请重新输入");
        }
        if (choice != 0) press_enter();
    }
}

/* ============================================================================
 * CLI 帮助信息
 * ============================================================================ */

static void print_help(const char *prog)
{
    printf(C_BOLD C_CYAN "\n  软件公司客户关系管理系统 V2.0\n" C_RESET);
    printf("  Customer Relationship Management V2.0\n\n");
    printf(C_BOLD "  用法:" C_RESET "\n");
    printf("    %s              启动交互菜单\n", prog);
    printf("    %s -h/--help    显示帮助信息\n", prog);
    printf("    %s -t/--test    运行内置自测套件\n", prog);
    printf("\n");
    printf(C_BOLD "  特性:" C_RESET "\n");
    printf("    - UTF-8 + ANSI 彩色终端\n");
    printf("    - 客户/联系人/跟进记录 动态数组存储\n");
    printf("    - 统一错误码枚举 CrmStatus\n");
    printf("    - Magic Header \"CRMV2\" 二进制持久化\n");
    printf("    - 18 项内置自测覆盖 CRUD/统计/文件I/O\n");
    printf("    - 级联删除、数据统计、交互菜单\n\n");
}

/* ============================================================================
 * 入口函数
 * ============================================================================ */

int main(int argc, char *argv[])
{
    term_init();

    /* 解析 CLI 参数 */
    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        }
        if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0) {
            if (crm_init(&g_ctx) != CRM_OK) {
                fprintf(stderr, "初始化失败\n");
                return 1;
            }
            int fail = run_all_tests();
            crm_free(&g_ctx);
            /* 清理测试产生的数据文件 */
            remove(DATA_FILE);
            return fail;
        }
        /* 未知参数，显示帮助 */
        printf(C_RED "  未知参数: %s\n" C_RESET, argv[1]);
        print_help(argv[0]);
        return 1;
    }

    /* 正常启动：初始化 + 进入主菜单 */
    if (crm_init(&g_ctx) != CRM_OK) {
        fprintf(stderr, C_RED "CRM 初始化失败（内存不足）\n" C_RESET);
        return 1;
    }

    printf(C_BOLD C_MAGENTA "\n");
    printf("  ╔══════════════════════════════════════════════════════════╗\n");
    printf("  ║                                                          ║\n");
    printf("  ║" C_RESET C_WHITE "       软件公司客户关系管理系统  " C_GREEN "V2.0" C_RESET C_WHITE "               " C_RESET C_BOLD C_MAGENTA "║\n");
    printf("  ║" C_RESET C_GRAY "       Customer Relationship Management V2.0        " C_RESET C_BOLD C_MAGENTA "║\n");
    printf("  ║                                                          ║\n");
    printf("  ╚══════════════════════════════════════════════════════════╝\n" C_RESET);

    main_menu(&g_ctx);
    crm_free(&g_ctx);
    return 0;
}
