/*
 * ==================================================================================
 * 简单银行账户管理系统 V2.0 (Simple Bank Account System V2.0)
 * ==================================================================================
 *
 * @项目名称   简单银行账户系统 V2.0
 * @版本       2.0
 * @作者       课程项目
 * @日期       2026
 * @编译器     GCC (C99 标准)
 *
 * ----------------------------------------------------------------------------------
 * V2.0 升级特性
 * ----------------------------------------------------------------------------------
 * 1. UTF-8 BOM编码 + Windows控制台UTF-8输出支持
 * 2. ANSI彩色终端界面（绿色成功、红色错误、黄色提示、青色标题）
 * 3. 命令行参数支持：-h/--help帮助, -t/--test自测
 * 4. 动态内存管理：账户使用动态数组存储，支持自动扩容
 * 5. 统一错误码枚举：BankStatus定义所有操作返回状态
 * 6. 文件持久化：使用"BANKV2" magic header标识文件格式
 * 7. 内置自测系统：15+项测试用例覆盖全部核心功能
 * 8. 中文注释完整，代码结构清晰
 *
 * ----------------------------------------------------------------------------------
 * 功能模块
 * ----------------------------------------------------------------------------------
 * - 开户（借记卡/信用卡）
 * - 存款
 * - 取款/消费
 * - 转账
 * - 查询余额
 * - 销户
 * - 交易明细
 * - 文件持久化保存/加载
 * - 内置自测
 *
 * ==================================================================================
 */

#ifdef _WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

/* ======================== ANSI彩色终端定义 ======================== */

#define COLOR_RESET     "\033[0m"
#define COLOR_RED       "\033[31m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_BLUE      "\033[34m"
#define COLOR_MAGENTA   "\033[35m"
#define COLOR_CYAN      "\033[36m"
#define COLOR_WHITE     "\033[37m"
#define COLOR_BOLD      "\033[1m"

/* ======================== 系统常量定义 ======================== */

#define MAX_STR         64
#define MAX_RECORDS     200
#define DATA_FILE       "bankv2.dat"
#define MAGIC_HEADER    "BANKV2"
#define MAGIC_LEN       6
#define INIT_CAPACITY   4
#define CREDIT_FEE_RATE 0.001
#define CROSS_FEE_RATE  0.001

/* ======================== 统一错误码枚举 ======================== */

typedef enum {
    BANK_OK = 0,
    BANK_ERR_INVALID_PARAM,
    BANK_ERR_MEMORY,
    BANK_ERR_ACCOUNT_NOT_FOUND,
    BANK_ERR_ACCOUNT_EXISTS,
    BANK_ERR_INSUFFICIENT_BALANCE,
    BANK_ERR_NEGATIVE_AMOUNT,
    BANK_ERR_ZERO_AMOUNT,
    BANK_ERR_FILE_OPEN,
    BANK_ERR_FILE_READ,
    BANK_ERR_FILE_WRITE,
    BANK_ERR_FILE_MAGIC,
    BANK_ERR_FILE_VERSION,
    BANK_ERR_RECORDS_FULL,
    BANK_ERR_INVALID_TYPE,
    BANK_ERR_SAME_ACCOUNT,
    BANK_ERR_NOT_LOGGED_IN,
    BANK_ERR_MAX
} BankStatus;

/* ======================== 账户类型枚举 ======================== */

typedef enum {
    ACCOUNT_DEBIT = 1,
    ACCOUNT_CREDIT = 2
} AccountType;

/* ======================== 交易类型枚举 ======================== */

typedef enum {
    TRANS_DEPOSIT = 1,
    TRANS_WITHDRAW,
    TRANS_TRANSFER_OUT,
    TRANS_TRANSFER_IN,
    TRANS_FEE
} TransType;

/* ======================== 数据结构定义 ======================== */

typedef struct {
    int     type;
    double  amount;
    double  fee;
    char    region[MAX_STR];
    char    time_str[20];
    int     counterpart;
} Transaction;

typedef struct {
    int         account_no;
    int         type;
    double      balance;
    double      credit_limit;
    char        name[MAX_STR];
    char        region[MAX_STR];
    Transaction records[MAX_RECORDS];
    int         record_count;
} Account;

typedef struct {
    Account *items;
    int      count;
    int      capacity;
    int      next_account_no;
} Bank;

/* ======================== 全局状态 ======================== */

static Bank g_bank;
static int  g_test_mode = 0;
static int  g_tests_passed = 0;
static int  g_tests_failed = 0;

/* ======================== 工具函数 ======================== */

static void setup_console(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

static void get_time_str(char *buf, int size)
{
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

static const char *status_str(BankStatus s)
{
    switch (s) {
        case BANK_OK: return "操作成功";
        case BANK_ERR_INVALID_PARAM: return "参数无效";
        case BANK_ERR_MEMORY: return "内存分配失败";
        case BANK_ERR_ACCOUNT_NOT_FOUND: return "账户不存在";
        case BANK_ERR_ACCOUNT_EXISTS: return "账户已存在";
        case BANK_ERR_INSUFFICIENT_BALANCE: return "余额不足";
        case BANK_ERR_NEGATIVE_AMOUNT: return "金额不能为负";
        case BANK_ERR_ZERO_AMOUNT: return "金额不能为零";
        case BANK_ERR_FILE_OPEN: return "文件打开失败";
        case BANK_ERR_FILE_READ: return "文件读取失败";
        case BANK_ERR_FILE_WRITE: return "文件写入失败";
        case BANK_ERR_FILE_MAGIC: return "文件格式错误";
        case BANK_ERR_FILE_VERSION: return "文件版本不兼容";
        case BANK_ERR_RECORDS_FULL: return "交易记录已满";
        case BANK_ERR_INVALID_TYPE: return "账户类型无效";
        case BANK_ERR_SAME_ACCOUNT: return "不能向自己转账";
        case BANK_ERR_NOT_LOGGED_IN: return "未登录";
        default: return "未知错误";
    }
}

static void print_line(void)
{
    printf("%s", COLOR_CYAN);
    printf("══════════════════════════════════════════════════════════\n");
    printf("%s", COLOR_RESET);
}

static void print_title(const char *title)
{
    printf("\n");
    print_line();
    printf("%s%s  %s  %s\n", COLOR_BOLD, COLOR_CYAN, title, COLOR_RESET);
    print_line();
}

static void add_record(Account *acc, int type, double amount, double fee,
                       const char *region, int counterpart)
{
    if (acc->record_count >= MAX_RECORDS) return;
    Transaction *r = &acc->records[acc->record_count];
    r->type = type;
    r->amount = amount;
    r->fee = fee;
    r->counterpart = counterpart;
    strncpy(r->region, region, MAX_STR - 1);
    r->region[MAX_STR - 1] = '\0';
    get_time_str(r->time_str, sizeof(r->time_str));
    acc->record_count++;
}

/* ======================== 银行核心函数 ======================== */

static BankStatus bank_init(Bank *bank)
{
    bank->count = 0;
    bank->capacity = INIT_CAPACITY;
    bank->next_account_no = 1001;
    bank->items = (Account *)calloc(bank->capacity, sizeof(Account));
    if (!bank->items) return BANK_ERR_MEMORY;
    return BANK_OK;
}

static void bank_free(Bank *bank)
{
    if (bank->items) {
        free(bank->items);
        bank->items = NULL;
    }
    bank->count = 0;
    bank->capacity = 0;
}

static BankStatus bank_resize(Bank *bank)
{
    int new_cap = bank->capacity * 2;
    Account *new_items = (Account *)realloc(bank->items, new_cap * sizeof(Account));
    if (!new_items) return BANK_ERR_MEMORY;
    memset(new_items + bank->capacity, 0, (new_cap - bank->capacity) * sizeof(Account));
    bank->items = new_items;
    bank->capacity = new_cap;
    return BANK_OK;
}

static Account *bank_find(Bank *bank, int account_no)
{
    for (int i = 0; i < bank->count; i++) {
        if (bank->items[i].account_no == account_no) {
            return &bank->items[i];
        }
    }
    return NULL;
}

static double account_available(const Account *acc)
{
    if (acc->type == ACCOUNT_CREDIT) {
        return acc->balance + acc->credit_limit;
    }
    return acc->balance;
}

static BankStatus bank_open_account(Bank *bank, int type, const char *name,
                                    const char *region, double credit_limit,
                                    int *out_no)
{
    if (!name || !region) return BANK_ERR_INVALID_PARAM;
    if (type != ACCOUNT_DEBIT && type != ACCOUNT_CREDIT) return BANK_ERR_INVALID_TYPE;
    if (type == ACCOUNT_CREDIT && credit_limit <= 0) return BANK_ERR_INVALID_PARAM;

    if (bank->count >= bank->capacity) {
        BankStatus s = bank_resize(bank);
        if (s != BANK_OK) return s;
    }

    Account *acc = &bank->items[bank->count];
    memset(acc, 0, sizeof(Account));
    acc->account_no = bank->next_account_no++;
    acc->type = type;
    acc->balance = 0.0;
    acc->credit_limit = (type == ACCOUNT_CREDIT) ? credit_limit : 0.0;
    acc->record_count = 0;
    strncpy(acc->name, name, MAX_STR - 1);
    acc->name[MAX_STR - 1] = '\0';
    strncpy(acc->region, region, MAX_STR - 1);
    acc->region[MAX_STR - 1] = '\0';

    if (out_no) *out_no = acc->account_no;
    bank->count++;
    return BANK_OK;
}

static BankStatus bank_deposit(Bank *bank, int account_no, double amount, const char *region)
{
    if (amount <= 0) return BANK_ERR_ZERO_AMOUNT;
    Account *acc = bank_find(bank, account_no);
    if (!acc) return BANK_ERR_ACCOUNT_NOT_FOUND;
    if (!region) region = acc->region;

    acc->balance += amount;
    add_record(acc, TRANS_DEPOSIT, amount, 0.0, region, 0);
    return BANK_OK;
}

static BankStatus bank_withdraw(Bank *bank, int account_no, double amount,
                                const char *region, int is_atm)
{
    if (amount <= 0) return BANK_ERR_ZERO_AMOUNT;
    Account *acc = bank_find(bank, account_no);
    if (!acc) return BANK_ERR_ACCOUNT_NOT_FOUND;
    if (!region) region = acc->region;

    double fee = 0.0;
    if (acc->type == ACCOUNT_CREDIT && is_atm) {
        fee = amount * CREDIT_FEE_RATE;
    } else if (acc->type == ACCOUNT_DEBIT && strcmp(region, acc->region) != 0) {
        fee = amount * CROSS_FEE_RATE;
    }

    double total = amount + fee;
    if (total > account_available(acc)) {
        return BANK_ERR_INSUFFICIENT_BALANCE;
    }

    acc->balance -= total;
    add_record(acc, TRANS_WITHDRAW, amount, fee, region, 0);
    if (fee > 0) {
        add_record(acc, TRANS_FEE, 0.0, fee, region, 0);
    }
    return BANK_OK;
}

static BankStatus bank_transfer(Bank *bank, int from_no, int to_no, double amount)
{
    if (amount <= 0) return BANK_ERR_ZERO_AMOUNT;
    if (from_no == to_no) return BANK_ERR_SAME_ACCOUNT;

    Account *from = bank_find(bank, from_no);
    Account *to = bank_find(bank, to_no);
    if (!from) return BANK_ERR_ACCOUNT_NOT_FOUND;
    if (!to) return BANK_ERR_ACCOUNT_NOT_FOUND;

    if (amount > from->balance) {
        return BANK_ERR_INSUFFICIENT_BALANCE;
    }

    from->balance -= amount;
    to->balance += amount;
    add_record(from, TRANS_TRANSFER_OUT, amount, 0.0, to->region, to_no);
    add_record(to, TRANS_TRANSFER_IN, amount, 0.0, from->region, from_no);
    return BANK_OK;
}

static BankStatus bank_close_account(Bank *bank, int account_no)
{
    int idx = -1;
    for (int i = 0; i < bank->count; i++) {
        if (bank->items[i].account_no == account_no) {
            idx = i;
            break;
        }
    }
    if (idx == -1) return BANK_ERR_ACCOUNT_NOT_FOUND;

    for (int i = idx; i < bank->count - 1; i++) {
        bank->items[i] = bank->items[i + 1];
    }
    bank->count--;
    memset(&bank->items[bank->count], 0, sizeof(Account));
    return BANK_OK;
}

/* ======================== 文件持久化（BANKV2格式） ======================== */

static BankStatus bank_save(Bank *bank, const char *filename)
{
    FILE *fp = fopen(filename, "wb");
    if (!fp) return BANK_ERR_FILE_OPEN;

    fwrite(MAGIC_HEADER, 1, MAGIC_LEN, fp);
    int version = 2;
    fwrite(&version, sizeof(int), 1, fp);
    fwrite(&bank->count, sizeof(int), 1, fp);
    fwrite(&bank->next_account_no, sizeof(int), 1, fp);

    for (int i = 0; i < bank->count; i++) {
        Account *acc = &bank->items[i];
        fwrite(&acc->account_no, sizeof(int), 1, fp);
        fwrite(&acc->type, sizeof(int), 1, fp);
        fwrite(&acc->balance, sizeof(double), 1, fp);
        fwrite(&acc->credit_limit, sizeof(double), 1, fp);
        fwrite(acc->name, 1, MAX_STR, fp);
        fwrite(acc->region, 1, MAX_STR, fp);
        fwrite(&acc->record_count, sizeof(int), 1, fp);
        fwrite(acc->records, sizeof(Transaction), acc->record_count, fp);
    }

    fclose(fp);
    return BANK_OK;
}

static BankStatus bank_load(Bank *bank, const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp) return BANK_ERR_FILE_OPEN;

    char magic[MAGIC_LEN + 1] = {0};
    if (fread(magic, 1, MAGIC_LEN, fp) != MAGIC_LEN) {
        fclose(fp);
        return BANK_ERR_FILE_READ;
    }
    if (strcmp(magic, MAGIC_HEADER) != 0) {
        fclose(fp);
        return BANK_ERR_FILE_MAGIC;
    }

    int version;
    if (fread(&version, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return BANK_ERR_FILE_READ;
    }
    if (version != 2) {
        fclose(fp);
        return BANK_ERR_FILE_VERSION;
    }

    bank_free(bank);
    if (fread(&bank->count, sizeof(int), 1, fp) != 1) { fclose(fp); return BANK_ERR_FILE_READ; }
    if (fread(&bank->next_account_no, sizeof(int), 1, fp) != 1) { fclose(fp); return BANK_ERR_FILE_READ; }

    bank->capacity = bank->count > INIT_CAPACITY ? bank->count * 2 : INIT_CAPACITY;
    bank->items = (Account *)calloc(bank->capacity, sizeof(Account));
    if (!bank->items) { fclose(fp); return BANK_ERR_MEMORY; }

    for (int i = 0; i < bank->count; i++) {
        Account *acc = &bank->items[i];
        if (fread(&acc->account_no, sizeof(int), 1, fp) != 1) { fclose(fp); return BANK_ERR_FILE_READ; }
        if (fread(&acc->type, sizeof(int), 1, fp) != 1) { fclose(fp); return BANK_ERR_FILE_READ; }
        if (fread(&acc->balance, sizeof(double), 1, fp) != 1) { fclose(fp); return BANK_ERR_FILE_READ; }
        if (fread(&acc->credit_limit, sizeof(double), 1, fp) != 1) { fclose(fp); return BANK_ERR_FILE_READ; }
        if (fread(acc->name, 1, MAX_STR, fp) != MAX_STR) { fclose(fp); return BANK_ERR_FILE_READ; }
        if (fread(acc->region, 1, MAX_STR, fp) != MAX_STR) { fclose(fp); return BANK_ERR_FILE_READ; }
        if (fread(&acc->record_count, sizeof(int), 1, fp) != 1) { fclose(fp); return BANK_ERR_FILE_READ; }
        if (acc->record_count > 0) {
            if (fread(acc->records, sizeof(Transaction), acc->record_count, fp) != (size_t)acc->record_count) {
                fclose(fp);
                return BANK_ERR_FILE_READ;
            }
        }
    }

    fclose(fp);
    return BANK_OK;
}

/* ======================== 显示函数 ======================== */

static void account_display(const Account *acc)
{
    printf("  %s账户号码:%s %06d\n", COLOR_YELLOW, COLOR_RESET, acc->account_no);
    printf("  %s账户类型:%s %s\n", COLOR_YELLOW, COLOR_RESET,
           acc->type == ACCOUNT_DEBIT ? "借记卡" : "信用卡");
    printf("  %s开户姓名:%s %s\n", COLOR_YELLOW, COLOR_RESET, acc->name);
    printf("  %s开户地区:%s %s\n", COLOR_YELLOW, COLOR_RESET, acc->region);
    printf("  %s当前余额:%s %.2f 元\n", COLOR_YELLOW, COLOR_RESET, acc->balance);
    if (acc->type == ACCOUNT_CREDIT) {
        printf("  %s信用额度:%s %.2f 元\n", COLOR_YELLOW, COLOR_RESET, acc->credit_limit);
        printf("  %s可用额度:%s %.2f 元\n", COLOR_YELLOW, COLOR_RESET, account_available(acc));
    }
}

static void account_history(const Account *acc)
{
    print_title("交易明细");
    account_display(acc);
    print_line();
    for (int i = 0; i < acc->record_count; i++) {
        const Transaction *r = &acc->records[i];
        const char *type_str = "";
        const char *color = COLOR_WHITE;
        char extra[128] = "";

        switch (r->type) {
            case TRANS_DEPOSIT:
                type_str = "存款";
                color = COLOR_GREEN;
                sprintf(extra, "+%.2f", r->amount);
                break;
            case TRANS_WITHDRAW:
                type_str = "取款";
                color = COLOR_RED;
                sprintf(extra, "-%.2f", r->amount);
                break;
            case TRANS_TRANSFER_OUT:
                type_str = "转出";
                color = COLOR_RED;
                sprintf(extra, "-%.2f -> %06d", r->amount, r->counterpart);
                break;
            case TRANS_TRANSFER_IN:
                type_str = "转入";
                color = COLOR_GREEN;
                sprintf(extra, "+%.2f <- %06d", r->amount, r->counterpart);
                break;
            case TRANS_FEE:
                type_str = "手续费";
                color = COLOR_MAGENTA;
                sprintf(extra, "-%.2f", r->fee);
                break;
        }
        printf("  %s | %s%-6s%s | %s%s%s | %s\n",
               r->time_str, color, type_str, COLOR_RESET,
               color, extra, COLOR_RESET, r->region);
    }
    print_line();
}

static void bank_list(const Bank *bank)
{
    print_title("账户列表");
    if (bank->count == 0) {
        printf("  %s暂无账户%s\n", COLOR_YELLOW, COLOR_RESET);
        return;
    }
    for (int i = 0; i < bank->count; i++) {
        printf("  [%d] ", i + 1);
        account_display(&bank->items[i]);
        if (i < bank->count - 1) printf("\n");
    }
    print_line();
}

/* ======================== 交互菜单 ======================== */

static void menu_help(void)
{
    print_title("银行账户管理系统 V2.0 - 帮助");
    printf("  %s用法:%s bank.exe [选项]\n\n", COLOR_GREEN, COLOR_RESET);
    printf("  %s选项:%s\n", COLOR_CYAN, COLOR_RESET);
    printf("    -h, --help     显示帮助信息\n");
    printf("    -t, --test     运行内置自测\n\n");
    printf("  %s交互菜单命令:%s\n", COLOR_CYAN, COLOR_RESET);
    printf("    1 - 开借记卡账户\n");
    printf("    2 - 开信用卡账户\n");
    printf("    3 - 存款\n");
    printf("    4 - 取款\n");
    printf("    5 - 转账\n");
    printf("    6 - 查询余额\n");
    printf("    7 - 交易明细\n");
    printf("    8 - 销户\n");
    printf("    9 - 账户列表\n");
    printf("   10 - 保存数据\n");
    printf("   11 - 加载数据\n");
    printf("    0 - 退出\n");
    print_line();
}

static char *read_str(char *buf, int size, const char *prompt)
{
    printf("%s%s%s", COLOR_YELLOW, prompt, COLOR_RESET);
    if (fgets(buf, size, stdin) == NULL) return NULL;
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    return buf;
}

static double read_double(const char *prompt)
{
    char buf[64];
    read_str(buf, sizeof(buf), prompt);
    return atof(buf);
}

static int read_int(const char *prompt)
{
    char buf[64];
    read_str(buf, sizeof(buf), prompt);
    return atoi(buf);
}

static void interactive_loop(Bank *bank)
{
    char name[MAX_STR], region[MAX_STR], buf[64];
    int choice, no, no2;
    double amount, limit;
    BankStatus s;

    printf("\n%s%s", COLOR_BOLD, COLOR_CYAN);
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║       银行账户管理系统 V2.0                  ║\n");
    printf("║       Simple Bank System V2.0                ║\n");
    printf("╚══════════════════════════════════════════════╝\n");
    printf("%s\n", COLOR_RESET);

    bank_load(bank, DATA_FILE);

    while (1) {
        printf("\n%s%s 主菜单 %s\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
        printf("  1.开借记卡  2.开信用卡  3.存款      4.取款\n");
        printf("  5.转账      6.查余额    7.明细      8.销户\n");
        printf("  9.账户列表  10.保存     11.加载     0.退出\n");
        printf("%s请选择:%s ", COLOR_YELLOW, COLOR_RESET);

        if (!fgets(buf, sizeof(buf), stdin)) break;
        choice = atoi(buf);

        switch (choice) {
            case 0:
                bank_save(bank, DATA_FILE);
                printf("%s感谢使用，再见！%s\n", COLOR_GREEN, COLOR_RESET);
                return;
            case 1:
                read_str(name, MAX_STR, "请输入姓名: ");
                read_str(region, MAX_STR, "请输入地区(如北京/上海): ");
                s = bank_open_account(bank, ACCOUNT_DEBIT, name, region, 0, &no);
                if (s == BANK_OK)
                    printf("%s开户成功！账号: %06d%s\n", COLOR_GREEN, no, COLOR_RESET);
                else
                    printf("%s开户失败: %s%s\n", COLOR_RED, status_str(s), COLOR_RESET);
                break;
            case 2:
                read_str(name, MAX_STR, "请输入姓名: ");
                read_str(region, MAX_STR, "请输入地区: ");
                limit = read_double("请输入信用额度: ");
                s = bank_open_account(bank, ACCOUNT_CREDIT, name, region, limit, &no);
                if (s == BANK_OK)
                    printf("%s信用卡开户成功！账号: %06d，额度: %.2f%s\n", COLOR_GREEN, no, limit, COLOR_RESET);
                else
                    printf("%s开户失败: %s%s\n", COLOR_RED, status_str(s), COLOR_RESET);
                break;
            case 3:
                no = read_int("请输入账号: ");
                amount = read_double("请输入存款金额: ");
                s = bank_deposit(bank, no, amount, NULL);
                if (s == BANK_OK)
                    printf("%s存款成功！%.2f 元已存入%s\n", COLOR_GREEN, amount, COLOR_RESET);
                else
                    printf("%s存款失败: %s%s\n", COLOR_RED, status_str(s), COLOR_RESET);
                break;
            case 4:
                no = read_int("请输入账号: ");
                amount = read_double("请输入取款金额: ");
                s = bank_withdraw(bank, no, amount, NULL, 1);
                if (s == BANK_OK)
                    printf("%s取款成功！%.2f 元已取出%s\n", COLOR_GREEN, amount, COLOR_RESET);
                else
                    printf("%s取款失败: %s%s\n", COLOR_RED, status_str(s), COLOR_RESET);
                break;
            case 5:
                no = read_int("请输入转出账号: ");
                no2 = read_int("请输入转入账号: ");
                amount = read_double("请输入转账金额: ");
                s = bank_transfer(bank, no, no2, amount);
                if (s == BANK_OK)
                    printf("%s转账成功！%.2f 元已转账%s\n", COLOR_GREEN, amount, COLOR_RESET);
                else
                    printf("%s转账失败: %s%s\n", COLOR_RED, status_str(s), COLOR_RESET);
                break;
            case 6:
                no = read_int("请输入账号: ");
                {
                    Account *acc = bank_find(bank, no);
                    if (acc) {
                        print_title("账户查询");
                        account_display(acc);
                        print_line();
                    } else {
                        printf("%s账户不存在%s\n", COLOR_RED, COLOR_RESET);
                    }
                }
                break;
            case 7:
                no = read_int("请输入账号: ");
                {
                    Account *acc = bank_find(bank, no);
                    if (acc) account_history(acc);
                    else printf("%s账户不存在%s\n", COLOR_RED, COLOR_RESET);
                }
                break;
            case 8:
                no = read_int("请输入要销户的账号: ");
                s = bank_close_account(bank, no);
                if (s == BANK_OK)
                    printf("%s销户成功！%s\n", COLOR_GREEN, COLOR_RESET);
                else
                    printf("%s销户失败: %s%s\n", COLOR_RED, status_str(s), COLOR_RESET);
                break;
            case 9:
                bank_list(bank);
                break;
            case 10:
                s = bank_save(bank, DATA_FILE);
                if (s == BANK_OK)
                    printf("%s数据已保存到 %s%s\n", COLOR_GREEN, DATA_FILE, COLOR_RESET);
                else
                    printf("%s保存失败: %s%s\n", COLOR_RED, status_str(s), COLOR_RESET);
                break;
            case 11:
                s = bank_load(bank, DATA_FILE);
                if (s == BANK_OK)
                    printf("%s数据已从 %s 加载%s\n", COLOR_GREEN, DATA_FILE, COLOR_RESET);
                else
                    printf("%s加载失败: %s%s\n", COLOR_RED, status_str(s), COLOR_RESET);
                break;
            default:
                printf("%s无效选项，请重新输入%s\n", COLOR_RED, COLOR_RESET);
        }
    }
}

/* ======================== 内置自测系统 ======================== */

#define TEST_ASSERT(cond, msg) do { \
    if (cond) { \
        g_tests_passed++; \
        if (!g_test_mode) printf("%s[PASS]%s %s\n", COLOR_GREEN, COLOR_RESET, msg); \
    } else { \
        g_tests_failed++; \
        printf("%s[FAIL]%s %s (line %d)\n", COLOR_RED, COLOR_RESET, msg, __LINE__); \
    } \
} while(0)

#define TEST_ASSERT_STATUS(s, expected, msg) TEST_ASSERT((s) == (expected), msg)
#define TEST_ASSERT_DOUBLE(a, b, msg) TEST_ASSERT((a) > (b) - 0.001 && (a) < (b) + 0.001, msg)

static void run_self_test(void)
{
    Bank test_bank;
    BankStatus s;
    int no1, no2, no3;
    Account *acc;

    print_title("内置自测 - 开始");
    g_tests_passed = 0;
    g_tests_failed = 0;

    /* T01: 初始化银行 */
    s = bank_init(&test_bank);
    TEST_ASSERT_STATUS(s, BANK_OK, "T01: 银行初始化成功");

    /* T02: 开借记卡账户 */
    s = bank_open_account(&test_bank, ACCOUNT_DEBIT, "张三", "北京", 0, &no1);
    TEST_ASSERT_STATUS(s, BANK_OK, "T02: 借记卡开户成功");
    TEST_ASSERT(no1 == 1001, "T02b: 首个账号为1001");

    /* T03: 开信用卡账户 */
    s = bank_open_account(&test_bank, ACCOUNT_CREDIT, "李四", "上海", 20000.0, &no2);
    TEST_ASSERT_STATUS(s, BANK_OK, "T03: 信用卡开户成功");
    TEST_ASSERT(no2 == 1002, "T03b: 第二个账号为1002");

    /* T04: 无效账户类型 */
    s = bank_open_account(&test_bank, 999, "测试", "北京", 0, NULL);
    TEST_ASSERT_STATUS(s, BANK_ERR_INVALID_TYPE, "T04: 无效账户类型返回错误");

    /* T05: 存款测试 */
    s = bank_deposit(&test_bank, no1, 5000.0, "北京");
    TEST_ASSERT_STATUS(s, BANK_OK, "T05: 借记卡存款成功");
    acc = bank_find(&test_bank, no1);
    TEST_ASSERT(acc != NULL, "T05b: 存款后账户存在");
    TEST_ASSERT_DOUBLE(acc->balance, 5000.0, "T05c: 存款后余额正确");

    /* T06: 存款零金额 */
    s = bank_deposit(&test_bank, no1, 0, "北京");
    TEST_ASSERT_STATUS(s, BANK_ERR_ZERO_AMOUNT, "T06: 存款零金额返回错误");

    /* T07: 存款负金额 */
    s = bank_deposit(&test_bank, no1, -100, "北京");
    TEST_ASSERT_STATUS(s, BANK_ERR_ZERO_AMOUNT, "T07: 存款负金额返回错误");

    /* T08: 取款测试（余额充足） */
    s = bank_withdraw(&test_bank, no1, 1000.0, "北京", 0);
    TEST_ASSERT_STATUS(s, BANK_OK, "T08: 借记卡取款成功");
    acc = bank_find(&test_bank, no1);
    TEST_ASSERT_DOUBLE(acc->balance, 4000.0, "T08b: 取款后余额正确");

    /* T09: 取款测试（余额不足） */
    s = bank_withdraw(&test_bank, no1, 99999.0, "北京", 0);
    TEST_ASSERT_STATUS(s, BANK_ERR_INSUFFICIENT_BALANCE, "T09: 余额不足取款返回错误");

    /* T10: 向不存在账户存款 */
    s = bank_deposit(&test_bank, 99999, 100, "北京");
    TEST_ASSERT_STATUS(s, BANK_ERR_ACCOUNT_NOT_FOUND, "T10: 不存在账户存款返回错误");

    /* T11: 信用卡取款（使用额度） */
    s = bank_deposit(&test_bank, no2, 0, "上海");
    s = bank_withdraw(&test_bank, no2, 5000.0, "上海", 1);
    TEST_ASSERT_STATUS(s, BANK_OK, "T11: 信用卡ATM取款成功");
    acc = bank_find(&test_bank, no2);
    TEST_ASSERT(acc->balance < 0, "T11b: 信用卡取款后余额为负");

    /* T12: 转账测试 */
    s = bank_open_account(&test_bank, ACCOUNT_DEBIT, "王五", "广州", 0, &no3);
    s = bank_deposit(&test_bank, no3, 3000.0, "广州");
    s = bank_transfer(&test_bank, no3, no1, 1000.0);
    TEST_ASSERT_STATUS(s, BANK_OK, "T12: 转账成功");
    acc = bank_find(&test_bank, no3);
    TEST_ASSERT_DOUBLE(acc->balance, 2000.0, "T12b: 转出方余额正确");
    acc = bank_find(&test_bank, no1);
    TEST_ASSERT_DOUBLE(acc->balance, 5000.0, "T12c: 转入方余额正确(4000+1000)");

    /* T13: 转账给自己 */
    s = bank_transfer(&test_bank, no1, no1, 100);
    TEST_ASSERT_STATUS(s, BANK_ERR_SAME_ACCOUNT, "T13: 自己转账返回错误");

    /* T14: 转账余额不足 */
    s = bank_transfer(&test_bank, no1, no2, 99999.0);
    TEST_ASSERT_STATUS(s, BANK_ERR_INSUFFICIENT_BALANCE, "T14: 转账余额不足返回错误");

    /* T15: 账户查找 */
    acc = bank_find(&test_bank, no1);
    TEST_ASSERT(acc != NULL, "T15: 查找存在账户成功");
    acc = bank_find(&test_bank, 99999);
    TEST_ASSERT(acc == NULL, "T15b: 查找不存在账户返回NULL");

    /* T16: 销户测试 */
    int count_before = test_bank.count;
    s = bank_close_account(&test_bank, no3);
    TEST_ASSERT_STATUS(s, BANK_OK, "T16: 销户成功");
    TEST_ASSERT(test_bank.count == count_before - 1, "T16b: 销户后账户数减少");
    acc = bank_find(&test_bank, no3);
    TEST_ASSERT(acc == NULL, "T16c: 销户后账户不存在");

    /* T17: 销不存在的户 */
    s = bank_close_account(&test_bank, 99999);
    TEST_ASSERT_STATUS(s, BANK_ERR_ACCOUNT_NOT_FOUND, "T17: 销不存在的户返回错误");

    /* T18: 文件保存测试 */
    s = bank_save(&test_bank, "test_bankv2.dat");
    TEST_ASSERT_STATUS(s, BANK_OK, "T18: 文件保存成功");

    /* T19: 文件加载测试 */
    Bank loaded;
    bank_init(&loaded);
    s = bank_load(&loaded, "test_bankv2.dat");
    TEST_ASSERT_STATUS(s, BANK_OK, "T19: 文件加载成功");
    TEST_ASSERT(loaded.count == test_bank.count, "T19b: 加载后账户数一致");
    acc = bank_find(&loaded, no1);
    TEST_ASSERT(acc != NULL, "T19c: 加载后账户存在");
    bank_free(&loaded);
    remove("test_bankv2.dat");

    /* T20: 无效magic header文件 */
    {
        FILE *fp = fopen("test_bad.dat", "wb");
        fprintf(fp, "BADHDR");
        fclose(fp);
        Bank bad;
        bank_init(&bad);
        s = bank_load(&bad, "test_bad.dat");
        TEST_ASSERT_STATUS(s, BANK_ERR_FILE_MAGIC, "T20: 错误文件头检测成功");
        bank_free(&bad);
        remove("test_bad.dat");
    }

    /* T21: 动态扩容测试 */
    {
        Bank big;
        bank_init(&big);
        int all_ok = 1;
        for (int i = 0; i < 20; i++) {
            char n[32], r[32];
            sprintf(n, "用户%d", i);
            sprintf(r, "地区%d", i % 5);
            if (bank_open_account(&big, ACCOUNT_DEBIT, n, r, 0, NULL) != BANK_OK) {
                all_ok = 0;
                break;
            }
        }
        TEST_ASSERT(all_ok && big.count == 20, "T21: 动态扩容-20个账户创建成功");
        TEST_ASSERT(big.capacity >= 20, "T21b: 动态扩容后容量足够");
        bank_free(&big);
    }

    /* T22: 交易记录验证 */
    {
        s = bank_init(&test_bank);
        s = bank_open_account(&test_bank, ACCOUNT_DEBIT, "测试记录", "北京", 0, &no1);
        s = bank_deposit(&test_bank, no1, 1000.0, "北京");
        s = bank_withdraw(&test_bank, no1, 200.0, "北京", 0);
        acc = bank_find(&test_bank, no1);
        TEST_ASSERT(acc->record_count == 2, "T22: 交易记录数量正确");
        TEST_ASSERT(acc->records[0].type == TRANS_DEPOSIT, "T22b: 首条为存款记录");
        TEST_ASSERT(acc->records[1].type == TRANS_WITHDRAW, "T22c: 第二条为取款记录");
    }

    bank_free(&test_bank);

    print_line();
    printf("  %s自测结果:%s ", COLOR_BOLD, COLOR_RESET);
    printf("%s%d 通过%s, ", COLOR_GREEN, g_tests_passed, COLOR_RESET);
    printf("%s%d 失败%s\n", g_tests_failed > 0 ? COLOR_RED : COLOR_GREEN,
           g_tests_failed, COLOR_RESET);
    print_line();

    if (g_tests_failed == 0) {
        printf("%s  ✓ 所有测试通过！%s\n", COLOR_GREEN, COLOR_RESET);
    } else {
        printf("%s  ✗ 有测试失败！%s\n", COLOR_RED, COLOR_RESET);
    }
    print_line();
}

/* ======================== 主函数 ======================== */

int main(int argc, char *argv[])
{
    setup_console();

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            menu_help();
            return 0;
        }
        if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0) {
            g_test_mode = 0;
            run_self_test();
            return g_tests_failed > 0 ? 1 : 0;
        }
    }

    if (bank_init(&g_bank) != BANK_OK) {
        printf("%s初始化失败！%s\n", COLOR_RED, COLOR_RESET);
        return 1;
    }

    interactive_loop(&g_bank);
    bank_free(&g_bank);

    return 0;
}
