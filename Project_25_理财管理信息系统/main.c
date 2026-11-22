/* ============================================================================
 *  家庭理财管理信息系统 V2.0
 *  Family Financial Management System V2.0
 *
 *  V2.0 升级特性：
 *    - UTF-8 BOM + SetConsoleOutputCP(65001) 中文支持
 *    - ANSI 彩色终端输出
 *    - CLI 参数 (-h/--help, -t/--test)
 *    - 动态数组内存管理（账户+交易记录）
 *    - 统一错误码 enum FinStatus
 *    - Magic header "FIN V2" 单一文件持久化
 *    - 内置≥15项自测 (CRUD+统计+余额+I/O)
 *    - 交互式菜单 + 中文注释
 *
 *  编译：gcc -std=c99 -Wall -Wextra -O2 -o finance.exe main.c
 * ========================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

/* ======================== 常量定义 ======================== */

#define MAX_STR        64
#define MAX_DESC       128
#define DATA_FILE      "finance.dat"
#define MAGIC_HEADER   "FIN V2"
#define MAGIC_LEN      6
#define INIT_CAPACITY  4
#define EPSILON        0.001

/* 交易类型 */
enum { TX_INCOME = 1, TX_EXPENSE = 0 };

/* ======================== 统一错误码 ======================== */

typedef enum {
    FIN_OK = 0,
    FIN_ERR_MEMORY,
    FIN_ERR_FILE_OPEN,
    FIN_ERR_FILE_READ,
    FIN_ERR_FILE_WRITE,
    FIN_ERR_MAGIC,
    FIN_ERR_NOT_FOUND,
    FIN_ERR_INVALID_PARAM,
    FIN_ERR_DUPLICATE,
    FIN_ERR_IO
} FinStatus;

/* ======================== ANSI 颜色 ======================== */

#define C_RESET   "\033[0m"
#define C_RED     "\033[31m"
#define C_GREEN   "\033[32m"
#define C_YELLOW  "\033[33m"
#define C_BLUE    "\033[34m"
#define C_MAGENTA "\033[35m"
#define C_CYAN    "\033[36m"
#define C_WHITE   "\033[37m"
#define C_BOLD    "\033[1m"
#define C_DIM     "\033[2m"

/* ======================== 数据结构（动态数组） ======================== */

typedef struct {
    int    id;
    char   name[MAX_STR];
    double balance;
} Account;

typedef struct {
    int    id;
    int    account_id;
    int    type;          /* TX_INCOME=1, TX_EXPENSE=0 */
    double amount;
    char   category[MAX_STR];
    char   date[MAX_STR];
    char   desc[MAX_DESC];
} Transaction;

typedef struct {
    Account     *data;
    size_t       count;
    size_t       capacity;
} AccountList;

typedef struct {
    Transaction *data;
    size_t       count;
    size_t       capacity;
} TransactionList;

typedef struct {
    AccountList     accounts;
    TransactionList transactions;
    int             next_acc_id;
    int             next_tx_id;
} Database;

/* ======================== 全局数据库 ======================== */

static Database g_db = {0};

/* ======================== 控制台初始化 ======================== */

static void console_init(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(hOut, &mode)) {
        mode |= 0x0004; /* ENABLE_VIRTUAL_TERMINAL_PROCESSING */
        SetConsoleMode(hOut, mode);
    }
#endif
}

/* ======================== 工具函数 ======================== */

static void safe_strcpy(char *dst, const char *src, size_t size)
{
    if (!dst || !src || size == 0) return;
    strncpy(dst, src, size - 1);
    dst[size - 1] = '\0';
}

static void clear_buf(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF) ;
}

static void db_init(Database *db)
{
    db->accounts.data = NULL;
    db->accounts.count = 0;
    db->accounts.capacity = 0;
    db->transactions.data = NULL;
    db->transactions.count = 0;
    db->transactions.capacity = 0;
    db->next_acc_id = 1;
    db->next_tx_id = 1;
}

static void db_free(Database *db)
{
    free(db->accounts.data);
    free(db->transactions.data);
    db->accounts.data = NULL;
    db->transactions.data = NULL;
    db->accounts.count = db->accounts.capacity = 0;
    db->transactions.count = db->transactions.capacity = 0;
}

static FinStatus account_list_grow(AccountList *list)
{
    size_t new_cap = list->capacity == 0 ? INIT_CAPACITY : list->capacity * 2;
    Account *p = (Account *)realloc(list->data, new_cap * sizeof(Account));
    if (!p) return FIN_ERR_MEMORY;
    list->data = p;
    list->capacity = new_cap;
    return FIN_OK;
}

static FinStatus tx_list_grow(TransactionList *list)
{
    size_t new_cap = list->capacity == 0 ? INIT_CAPACITY : list->capacity * 2;
    Transaction *p = (Transaction *)realloc(list->data, new_cap * sizeof(Transaction));
    if (!p) return FIN_ERR_MEMORY;
    list->data = p;
    list->capacity = new_cap;
    return FIN_OK;
}

static int find_account_idx(const Database *db, int id)
{
    for (size_t i = 0; i < db->accounts.count; i++) {
        if (db->accounts.data[i].id == id) return (int)i;
    }
    return -1;
}

/* ======================== 账户 CRUD ======================== */

static FinStatus account_add(Database *db, const char *name, double initial, int *out_id)
{
    if (!name || !*name) return FIN_ERR_INVALID_PARAM;
    FinStatus st = account_list_grow(&db->accounts);
    if (st != FIN_OK) return st;
    int id = db->next_acc_id++;
    Account *a = &db->accounts.data[db->accounts.count++];
    a->id = id;
    safe_strcpy(a->name, name, MAX_STR);
    a->balance = initial;
    if (out_id) *out_id = id;
    return FIN_OK;
}

static FinStatus account_delete(Database *db, int id)
{
    int idx = find_account_idx(db, id);
    if (idx < 0) return FIN_ERR_NOT_FOUND;
    /* 检查是否有交易引用 */
    for (size_t i = 0; i < db->transactions.count; i++) {
        if (db->transactions.data[i].account_id == id)
            return FIN_ERR_INVALID_PARAM;
    }
    size_t last = db->accounts.count - 1;
    if ((size_t)idx != last) {
        db->accounts.data[idx] = db->accounts.data[last];
    }
    db->accounts.count--;
    return FIN_OK;
}

static FinStatus account_update(Database *db, int id, const char *new_name)
{
    int idx = find_account_idx(db, id);
    if (idx < 0) return FIN_ERR_NOT_FOUND;
    if (new_name && *new_name) {
        safe_strcpy(db->accounts.data[idx].name, new_name, MAX_STR);
    }
    return FIN_OK;
}

static Account *account_get(Database *db, int id)
{
    int idx = find_account_idx(db, id);
    if (idx < 0) return NULL;
    return &db->accounts.data[idx];
}

/* ======================== 交易 CRUD ======================== */

static FinStatus tx_add(Database *db, int account_id, int type, double amount,
                        const char *category, const char *date, const char *desc)
{
    if (amount < 0) return FIN_ERR_INVALID_PARAM;
    Account *a = account_get(db, account_id);
    if (!a) return FIN_ERR_NOT_FOUND;
    FinStatus st = tx_list_grow(&db->transactions);
    if (st != FIN_OK) return st;
    int id = db->next_tx_id++;
    Transaction *t = &db->transactions.data[db->transactions.count++];
    t->id = id;
    t->account_id = account_id;
    t->type = type;
    t->amount = amount;
    safe_strcpy(t->category, category ? category : "", MAX_STR);
    safe_strcpy(t->date, date ? date : "", MAX_STR);
    safe_strcpy(t->desc, desc ? desc : "", MAX_DESC);
    if (type == TX_INCOME) a->balance += amount;
    else                   a->balance -= amount;
    return FIN_OK;
}

static int find_tx_idx(const Database *db, int id)
{
    for (size_t i = 0; i < db->transactions.count; i++) {
        if (db->transactions.data[i].id == id) return (int)i;
    }
    return -1;
}

static FinStatus tx_delete(Database *db, int id)
{
    int idx = find_tx_idx(db, id);
    if (idx < 0) return FIN_ERR_NOT_FOUND;
    Transaction *t = &db->transactions.data[idx];
    Account *a = account_get(db, t->account_id);
    if (a) {
        if (t->type == TX_INCOME) a->balance -= t->amount;
        else                      a->balance += t->amount;
    }
    size_t last = db->transactions.count - 1;
    if ((size_t)idx != last) {
        db->transactions.data[idx] = db->transactions.data[last];
    }
    db->transactions.count--;
    return FIN_OK;
}

/* ======================== 统计功能 ======================== */

static double calc_total_income(const Database *db)
{
    double sum = 0.0;
    for (size_t i = 0; i < db->transactions.count; i++) {
        if (db->transactions.data[i].type == TX_INCOME)
            sum += db->transactions.data[i].amount;
    }
    return sum;
}

static double calc_total_expense(const Database *db)
{
    double sum = 0.0;
    for (size_t i = 0; i < db->transactions.count; i++) {
        if (db->transactions.data[i].type == TX_EXPENSE)
            sum += db->transactions.data[i].amount;
    }
    return sum;
}

static double calc_net_balance(const Database *db)
{
    return calc_total_income(db) - calc_total_expense(db);
}

static int count_tx_by_type(const Database *db, int type)
{
    int cnt = 0;
    for (size_t i = 0; i < db->transactions.count; i++) {
        if (db->transactions.data[i].type == type) cnt++;
    }
    return cnt;
}

/* ======================== 文件持久化（Magic Header） ======================== */

static FinStatus db_save(const Database *db, const char *path)
{
    FILE *fp = fopen(path, "wb");
    if (!fp) return FIN_ERR_FILE_OPEN;
    if (fwrite(MAGIC_HEADER, 1, MAGIC_LEN, fp) != MAGIC_LEN) { fclose(fp); return FIN_ERR_FILE_WRITE; }
    if (fwrite(&db->next_acc_id, sizeof(int), 1, fp) != 1)    { fclose(fp); return FIN_ERR_FILE_WRITE; }
    if (fwrite(&db->next_tx_id,  sizeof(int), 1, fp) != 1)    { fclose(fp); return FIN_ERR_FILE_WRITE; }
    size_t acc_cnt = db->accounts.count;
    size_t tx_cnt  = db->transactions.count;
    if (fwrite(&acc_cnt, sizeof(size_t), 1, fp) != 1)         { fclose(fp); return FIN_ERR_FILE_WRITE; }
    if (fwrite(&tx_cnt,  sizeof(size_t), 1, fp) != 1)         { fclose(fp); return FIN_ERR_FILE_WRITE; }
    if (acc_cnt > 0 && fwrite(db->accounts.data, sizeof(Account), acc_cnt, fp) != acc_cnt) {
        fclose(fp); return FIN_ERR_FILE_WRITE;
    }
    if (tx_cnt > 0 && fwrite(db->transactions.data, sizeof(Transaction), tx_cnt, fp) != tx_cnt) {
        fclose(fp); return FIN_ERR_FILE_WRITE;
    }
    fclose(fp);
    return FIN_OK;
}

static FinStatus db_load(Database *db, const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) return FIN_ERR_FILE_OPEN;
    char magic[MAGIC_LEN] = {0};
    if (fread(magic, 1, MAGIC_LEN, fp) != MAGIC_LEN) { fclose(fp); return FIN_ERR_FILE_READ; }
    if (memcmp(magic, MAGIC_HEADER, MAGIC_LEN) != 0) { fclose(fp); return FIN_ERR_MAGIC; }
    int next_acc = 0, next_tx = 0;
    if (fread(&next_acc, sizeof(int), 1, fp) != 1)    { fclose(fp); return FIN_ERR_FILE_READ; }
    if (fread(&next_tx,  sizeof(int), 1, fp) != 1)    { fclose(fp); return FIN_ERR_FILE_READ; }
    size_t acc_cnt = 0, tx_cnt = 0;
    if (fread(&acc_cnt, sizeof(size_t), 1, fp) != 1)  { fclose(fp); return FIN_ERR_FILE_READ; }
    if (fread(&tx_cnt,  sizeof(size_t), 1, fp) != 1)  { fclose(fp); return FIN_ERR_FILE_READ; }
    db_free(db);
    Account     *accs = NULL;
    Transaction *txs  = NULL;
    if (acc_cnt > 0) {
        accs = (Account *)malloc(acc_cnt * sizeof(Account));
        if (!accs) { fclose(fp); return FIN_ERR_MEMORY; }
        if (fread(accs, sizeof(Account), acc_cnt, fp) != acc_cnt) { free(accs); fclose(fp); return FIN_ERR_FILE_READ; }
    }
    if (tx_cnt > 0) {
        txs = (Transaction *)malloc(tx_cnt * sizeof(Transaction));
        if (!txs) { free(accs); fclose(fp); return FIN_ERR_MEMORY; }
        if (fread(txs, sizeof(Transaction), tx_cnt, fp) != tx_cnt) { free(accs); free(txs); fclose(fp); return FIN_ERR_FILE_READ; }
    }
    fclose(fp);
    db->accounts.data = accs;
    db->accounts.count = acc_cnt;
    db->accounts.capacity = acc_cnt;
    db->transactions.data = txs;
    db->transactions.count = tx_cnt;
    db->transactions.capacity = tx_cnt;
    db->next_acc_id = next_acc;
    db->next_tx_id = next_tx;
    return FIN_OK;
}

/* ======================== 输入辅助 ======================== */

static int read_int(const char *prompt)
{
    int val;
    printf("  %s", prompt);
    while (scanf("%d", &val) != 1) {
        printf("  %s输入无效，请重新输入: %s", C_RED, C_RESET);
        clear_buf();
    }
    clear_buf();
    return val;
}

static double read_double(const char *prompt)
{
    double val;
    printf("  %s", prompt);
    while (scanf("%lf", &val) != 1) {
        printf("  %s输入无效，请重新输入: %s", C_RED, C_RESET);
        clear_buf();
    }
    clear_buf();
    return val;
}

static void read_str(const char *prompt, char *buf, size_t size)
{
    printf("  %s", prompt);
    if (fgets(buf, (int)size, stdin) != NULL) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    } else {
        buf[0] = '\0';
    }
}

static void pause_wait(void)
{
    printf("\n  %s按回车键继续...%s", C_DIM, C_RESET);
    getchar();
}

static void print_sep(void)
{
    printf("  %s──────────────────────────────────────────────%s\n", C_CYAN, C_RESET);
}

static void print_header(const char *title)
{
    printf("\n");
    print_sep();
    printf("  %s%s%s %s%s%s\n", C_BOLD, C_CYAN, "▌", C_YELLOW, title, C_RESET);
    print_sep();
}

/* ======================== 显示功能 ======================== */

static void show_accounts(void)
{
    print_header("账户列表");
    printf("  %s%-6s %-20s %12s%s\n", C_BOLD, "ID", "账户名称", "余额(元)", C_RESET);
    print_sep();
    for (size_t i = 0; i < g_db.accounts.count; i++) {
        Account *a = &g_db.accounts.data[i];
        const char *color = a->balance >= 0 ? C_GREEN : C_RED;
        printf("  %-6d %-20s %s%12.2f%s\n", a->id, a->name, color, a->balance, C_RESET);
    }
    if (g_db.accounts.count == 0) {
        printf("  %s（暂无账户）%s\n", C_DIM, C_RESET);
    }
}

static void show_transactions(void)
{
    print_header("交易记录");
    printf("  %s%-5s %-5s %-6s %10s %-10s %-12s %s%s\n",
           C_BOLD, "ID", "账户", "类型", "金额", "分类", "日期", "备注", C_RESET);
    print_sep();
    for (size_t i = 0; i < g_db.transactions.count; i++) {
        Transaction *t = &g_db.transactions.data[i];
        const char *tn = t->type == TX_INCOME ? "收入" : "支出";
        const char *tc = t->type == TX_INCOME ? C_GREEN : C_RED;
        const char *sign = t->type == TX_INCOME ? "+" : "-";
        printf("  %-5d %-5d %s%-6s%s %s%s%9.2f%s %-10s %-12s %s\n",
               t->id, t->account_id, tc, tn, C_RESET,
               tc, sign, t->amount, C_RESET,
               t->category, t->date, t->desc);
    }
    if (g_db.transactions.count == 0) {
        printf("  %s（暂无交易记录）%s\n", C_DIM, C_RESET);
    }
}

static void show_stats(void)
{
    print_header("收支统计");
    double inc = calc_total_income(&g_db);
    double exp = calc_total_expense(&g_db);
    double net = inc - exp;
    int inc_cnt = count_tx_by_type(&g_db, TX_INCOME);
    int exp_cnt = count_tx_by_type(&g_db, TX_EXPENSE);
    printf("  %s总收入：%s%s%.2f%s 元  (%d 笔)\n", C_BOLD, C_GREEN, C_BOLD, inc, C_RESET, inc_cnt);
    printf("  %s总支出：%s%s%.2f%s 元  (%d 笔)\n", C_BOLD, C_RED, C_BOLD, exp, C_RESET, exp_cnt);
    const char *nc = net >= 0 ? C_GREEN : C_RED;
    printf("  %s净结余：%s%s%.2f%s 元\n", C_BOLD, nc, C_BOLD, net, C_RESET);
    double total_bal = 0;
    for (size_t i = 0; i < g_db.accounts.count; i++) total_bal += g_db.accounts.data[i].balance;
    printf("  %s账户总资产：%s%s%.2f%s 元\n", C_BOLD, C_CYAN, C_BOLD, total_bal, C_RESET);
}

/* ======================== 菜单操作 ======================== */

static void menu_add_account(void)
{
    print_header("添加账户");
    char name[MAX_STR];
    read_str("账户名称：", name, MAX_STR);
    double bal = read_double("初始余额：");
    int id = 0;
    FinStatus st = account_add(&g_db, name, bal, &id);
    if (st == FIN_OK) {
        printf("  %s✓ 账户添加成功，ID=%d%s\n", C_GREEN, id, C_RESET);
        db_save(&g_db, DATA_FILE);
    } else {
        printf("  %s✗ 添加失败 (错误码=%d)%s\n", C_RED, (int)st, C_RESET);
    }
}

static void menu_del_account(void)
{
    show_accounts();
    int id = read_int("请输入要删除的账户ID：");
    FinStatus st = account_delete(&g_db, id);
    if (st == FIN_OK) {
        printf("  %s✓ 账户已删除%s\n", C_GREEN, C_RESET);
        db_save(&g_db, DATA_FILE);
    } else if (st == FIN_ERR_INVALID_PARAM) {
        printf("  %s✗ 该账户下仍有交易记录，无法删除%s\n", C_RED, C_RESET);
    } else {
        printf("  %s✗ 删除失败 (错误码=%d)%s\n", C_RED, (int)st, C_RESET);
    }
}

static void menu_add_tx(void)
{
    print_header("添加交易记录");
    show_accounts();
    int aid = read_int("账户ID：");
    if (account_get(&g_db, aid) == NULL) {
        printf("  %s✗ 账户不存在%s\n", C_RED, C_RESET);
        return;
    }
    int tp = read_int("类型（1=收入，0=支出）：");
    if (tp != 0 && tp != 1) { printf("  %s✗ 类型无效%s\n", C_RED, C_RESET); return; }
    double amt = read_double("金额：");
    char cat[MAX_STR], dt[MAX_STR], dsc[MAX_DESC];
    read_str("分类：", cat, MAX_STR);
    read_str("日期(yyyy-mm-dd)：", dt, MAX_STR);
    read_str("备注：", dsc, MAX_DESC);
    FinStatus st = tx_add(&g_db, aid, tp, amt, cat, dt, dsc);
    if (st == FIN_OK) {
        printf("  %s✓ 交易记录已添加%s\n", C_GREEN, C_RESET);
        db_save(&g_db, DATA_FILE);
    } else {
        printf("  %s✗ 添加失败 (错误码=%d)%s\n", C_RED, (int)st, C_RESET);
    }
}

static void menu_del_tx(void)
{
    show_transactions();
    int id = read_int("请输入要删除的交易ID：");
    FinStatus st = tx_delete(&g_db, id);
    if (st == FIN_OK) {
        printf("  %s✓ 交易已删除，账户余额已回滚%s\n", C_GREEN, C_RESET);
        db_save(&g_db, DATA_FILE);
    } else {
        printf("  %s✗ 删除失败 (错误码=%d)%s\n", C_RED, (int)st, C_RESET);
    }
}

static void main_menu(void)
{
    int choice;
    do {
        printf("\n");
        print_sep();
        printf("  %s%s家庭理财管理系统 V2.0%s\n", C_BOLD, C_YELLOW, C_RESET);
        print_sep();
        printf("  %s1.%s 查看账户列表\n", C_CYAN, C_RESET);
        printf("  %s2.%s 添加账户\n", C_CYAN, C_RESET);
        printf("  %s3.%s 删除账户\n", C_CYAN, C_RESET);
        printf("  %s4.%s 查看交易记录\n", C_CYAN, C_RESET);
        printf("  %s5.%s 添加交易记录\n", C_CYAN, C_RESET);
        printf("  %s6.%s 删除交易记录\n", C_CYAN, C_RESET);
        printf("  %s7.%s 收支统计\n", C_CYAN, C_RESET);
        printf("  %s8.%s 保存数据\n", C_CYAN, C_RESET);
        printf("  %s0.%s 退出\n", C_CYAN, C_RESET);
        print_sep();
        choice = read_int("请选择：");
        switch (choice) {
        case 1: show_accounts(); pause_wait(); break;
        case 2: menu_add_account(); pause_wait(); break;
        case 3: menu_del_account(); pause_wait(); break;
        case 4: show_transactions(); pause_wait(); break;
        case 5: menu_add_tx(); pause_wait(); break;
        case 6: menu_del_tx(); pause_wait(); break;
        case 7: show_stats(); pause_wait(); break;
        case 8:
            if (db_save(&g_db, DATA_FILE) == FIN_OK)
                printf("  %s✓ 数据已保存%s\n", C_GREEN, C_RESET);
            pause_wait(); break;
        case 0:
            db_save(&g_db, DATA_FILE);
            printf("  %s再见！%s\n", C_GREEN, C_RESET);
            break;
        default:
            printf("  %s无效选择%s\n", C_RED, C_RESET);
        }
    } while (choice != 0);
}

/* ======================== 内置自测（≥15项） ======================== */

static int g_tests_passed = 0;
static int g_tests_failed = 0;

static void test_check(const char *name, int condition)
{
    if (condition) {
        printf("  %s[PASS]%s %s\n", C_GREEN, C_RESET, name);
        g_tests_passed++;
    } else {
        printf("  %s[FAIL]%s %s\n", C_RED, C_RESET, name);
        g_tests_failed++;
    }
}

static int run_self_tests(void)
{
    printf("\n%s%s═══ 运行内置自测 ═══%s\n\n", C_BOLD, C_MAGENTA, C_RESET);
    g_tests_passed = 0;
    g_tests_failed = 0;

    Database test_db;
    db_init(&test_db);

    /* T1: 初始状态 */
    test_check("T1  初始状态：账户数为0", test_db.accounts.count == 0);
    test_check("T2  初始状态：交易数为0", test_db.transactions.count == 0);
    test_check("T3  初始状态：下一个账户ID=1", test_db.next_acc_id == 1);

    /* T4-T7: 账户 CRUD */
    int id1 = 0, id2 = 0;
    FinStatus s1 = account_add(&test_db, "现金钱包", 1000.0, &id1);
    FinStatus s2 = account_add(&test_db, "工商银行", 5000.0, &id2);
    test_check("T4  添加账户1成功", s1 == FIN_OK && id1 == 1);
    test_check("T5  添加账户2成功", s2 == FIN_OK && id2 == 2);
    test_check("T6  账户列表count=2", test_db.accounts.count == 2);
    Account *a1 = account_get(&test_db, id1);
    test_check("T7  查询账户1余额正确", a1 != NULL && (a1->balance - 1000.0) < EPSILON && (a1->balance - 1000.0) > -EPSILON);

    /* T8-T11: 交易 CRUD */
    FinStatus s3 = tx_add(&test_db, id1, TX_INCOME, 500.0, "工资", "2026-06-01", "六月工资");
    test_check("T8  添加收入交易成功", s3 == FIN_OK && test_db.transactions.count == 1);
    test_check("T9  收入后账户余额=1500", (account_get(&test_db, id1)->balance - 1500.0) < EPSILON);
    FinStatus s4 = tx_add(&test_db, id1, TX_EXPENSE, 200.0, "餐饮", "2026-06-02", "午餐");
    test_check("T10 添加支出交易成功", s4 == FIN_OK && test_db.transactions.count == 2);
    test_check("T11 支出后账户余额=1300", (account_get(&test_db, id1)->balance - 1300.0) < EPSILON);

    /* T12: 收支统计 */
    double inc = calc_total_income(&test_db);
    double exp = calc_total_expense(&test_db);
    double net = calc_net_balance(&test_db);
    test_check("T12 总收入统计正确", (inc - 500.0) < EPSILON);
    test_check("T13 总支出统计正确", (exp - 200.0) < EPSILON);
    test_check("T14 净结余=300", (net - 300.0) < EPSILON);

    /* T15: 删除交易（余额回滚） */
    int tx_id = test_db.transactions.data[1].id;
    FinStatus s5 = tx_delete(&test_db, tx_id);
    test_check("T15 删除交易成功并回滚余额", s5 == FIN_OK &&
               (account_get(&test_db, id1)->balance - 1500.0) < EPSILON);

    /* T16: 交易计数 */
    int ic = count_tx_by_type(&test_db, TX_INCOME);
    int ec = count_tx_by_type(&test_db, TX_EXPENSE);
    test_check("T16 收入笔数=1,支出笔数=0", ic == 1 && ec == 0);

    /* T17-T18: 更新账户名 */
    FinStatus s6 = account_update(&test_db, id2, "建设银行");
    test_check("T17 更新账户名成功", s6 == FIN_OK);
    test_check("T18 账户名已更新", strcmp(account_get(&test_db, id2)->name, "建设银行") == 0);

    /* T19-T21: 文件I/O */
    const char *test_file = "_test_finance.dat";
    FinStatus s7 = db_save(&test_db, test_file);
    test_check("T19 保存到文件成功", s7 == FIN_OK);
    Database loaded;
    db_init(&loaded);
    FinStatus s8 = db_load(&loaded, test_file);
    test_check("T20 从文件加载成功", s8 == FIN_OK);
    test_check("T21 加载后账户数一致", loaded.accounts.count == test_db.accounts.count);
    test_check("T22 加载后交易数一致", loaded.transactions.count == test_db.transactions.count);
    test_check("T23 Magic header验证", s8 == FIN_OK);
    Account *la1 = account_get(&loaded, id1);
    test_check("T24 加载后余额一致", la1 != NULL && (la1->balance - account_get(&test_db, id1)->balance) < EPSILON);

    /* T25: 错误码验证 - 不存在账户 */
    test_check("T25 查询不存在账户返回NULL", account_get(&test_db, 9999) == NULL);

    /* T26: 错误码验证 - 删除有交易的账户失败 */
    FinStatus s9 = account_delete(&test_db, id1);
    test_check("T26 有交易的账户不可删除", s9 == FIN_ERR_INVALID_PARAM);

    /* 清理测试文件 */
    remove(test_file);
    db_free(&test_db);
    db_free(&loaded);

    printf("\n");
    print_sep();
    printf("  %s测试结果：%s%d 通过, %s%d 失败%s\n",
           C_BOLD, C_GREEN, g_tests_passed, C_RED, g_tests_failed, C_RESET);
    print_sep();
    return g_tests_failed == 0 ? 0 : 1;
}

/* ======================== CLI 帮助 ======================== */

static void print_help(const char *prog)
{
    printf("%s%s家庭理财管理系统 V2.0%s\n\n", C_BOLD, C_CYAN, C_RESET);
    printf("  用法: %s [选项]\n\n", prog);
    printf("  %s选项:%s\n", C_BOLD, C_RESET);
    printf("    -h, --help     显示帮助信息\n");
    printf("    -t, --test     运行内置自测套件（≥15项测试）\n");
    printf("    (无参数)       启动交互式菜单\n\n");
    printf("  %s数据文件:%s %s\n", C_BOLD, C_RESET, DATA_FILE);
    printf("  %sMagic Header:%s %s\n", C_BOLD, C_RESET, MAGIC_HEADER);
}

/* ======================== 主函数 ======================== */

int main(int argc, char *argv[])
{
    console_init();

    /* 解析 CLI 参数 */
    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        }
        if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0) {
            return run_self_tests();
        }
        printf("%s未知选项: %s%s\n", C_RED, argv[1], C_RESET);
        print_help(argv[0]);
        return 1;
    }

    /* 正常启动 */
    db_init(&g_db);
    FinStatus st = db_load(&g_db, DATA_FILE);
    if (st != FIN_OK) {
        if (st == FIN_ERR_FILE_OPEN) {
            printf("  %sℹ 未找到数据文件，创建新数据库%s\n", C_YELLOW, C_RESET);
        } else if (st == FIN_ERR_MAGIC) {
            printf("  %s⚠ 数据文件格式不匹配，创建新数据库%s\n", C_YELLOW, C_RESET);
            db_init(&g_db);
        }
        /* 预置示例数据 */
        int id_cash = 0, id_bank = 0;
        account_add(&g_db, "现金钱包", 2000.0, &id_cash);
        account_add(&g_db, "工商银行", 10000.0, &id_bank);
        tx_add(&g_db, id_bank, TX_INCOME, 15000.0, "工资", "2026-06-01", "六月工资");
        tx_add(&g_db, id_cash, TX_EXPENSE, 50.0,   "餐饮", "2026-06-02", "午餐");
        tx_add(&g_db, id_bank, TX_EXPENSE, 3000.0, "房租", "2026-06-03", "六月房租");
        db_save(&g_db, DATA_FILE);
    }

    main_menu();
    db_free(&g_db);
    return 0;
}
