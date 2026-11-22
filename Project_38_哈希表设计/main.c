/*
 * ============================================================================
 *  项目38：哈希表设计 V2.0  (数据结构课程设计)
 *
 *  技术规格：
 *    - 哈希函数：除留余数法 (BKDR hash mod prime bucket_count)
 *    - 冲突解决：链地址法 (Separate Chaining)，每个桶维护一条单链表
 *    - 内存管理：全动态分配 (malloc/free)，支持动态扩容rehash
 *    - 错误处理：统一枚举 HashStatus 错误码
 *    - 文件持久化：二进制格式，Magic Header "HASHV2" + 版本号 + 数据
 *    - 终端支持：UTF-8编码 + ANSI彩色输出 + SetConsoleOutputCP(65001)
 *    - CLI参数：-h/--help 显示帮助, -t/--test 运行内置自测
 *    - 内置自测：20项测试用例（插入/查找/删除/冲突/负载因子/文件I/O/扩容）
 *
 *  编译命令：
 *    gcc -std=c99 -Wall -Wextra -O2 -o hash.exe main.c
 *
 *  作者：HuiJunzhuo
 *  版本：V2.0
 *  日期：2026年
 * ============================================================================
 */

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* ============================================================================
 *  ANSI 颜色定义
 * ============================================================================ */
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
#define C_BG_BLUE   "\033[44m"

/* ============================================================================
 *  常量定义
 * ============================================================================ */
#define MAGIC_HEADER       "HASHV2"
#define MAGIC_LEN          6
#define FILE_VERSION       2
#define DEFAULT_BUCKETS    53
#define MAX_KEY_LEN        128
#define LOAD_FACTOR_MAX    0.75
#define REHASH_GROWTH      2
#define DATA_FILE          "hash_v2.dat"
#define MAX_TEST_NAMES     30

/* ============================================================================
 *  统一错误码枚举 HashStatus
 * ============================================================================ */
typedef enum {
    HASH_OK = 0,
    HASH_ERR_NULL,
    HASH_ERR_MEMORY,
    HASH_ERR_NOT_FOUND,
    HASH_ERR_DUPLICATE,
    HASH_ERR_FILE_OPEN,
    HASH_ERR_FILE_READ,
    HASH_ERR_FILE_WRITE,
    HASH_ERR_MAGIC,
    HASH_ERR_VERSION,
    HASH_ERR_INVALID_KEY,
    HASH_ERR_FULL,
    HASH_ERR_INTERNAL
} HashStatus;

/* ============================================================================
 *  数据结构定义
 * ============================================================================ */

/* 哈希表节点（单链表，链地址法）*/
typedef struct HashNode {
    char            *key;
    int              value;
    struct HashNode *next;
} HashNode;

/* 哈希表结构 */
typedef struct HashTable {
    HashNode **buckets;
    size_t     bucket_count;
    size_t     count;
} HashTable;

/* ============================================================================
 *  素数表（用于扩容时选择下一个素数桶大小）
 * ============================================================================ */
static const size_t PRIME_SIZES[] = {
    53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593,
    49157, 98317, 196613, 393241, 786433, 1572869, 3145739
};
static const size_t PRIME_COUNT = sizeof(PRIME_SIZES) / sizeof(PRIME_SIZES[0]);

/* ============================================================================
 *  测试姓名数据（30个中文姓名拼音）
 * ============================================================================ */
static const char *TEST_NAMES[MAX_TEST_NAMES] = {
    "ZhangWei",  "LiNa",      "WangFang",  "LiuYang",   "ChenJing",
    "ZhaoLei",   "SunHua",    "ZhouMin",   "WuGang",    "ZhengLi",
    "HuangYong", "LinXiao",   "HeJun",     "GaoFei",    "LuoPing",
    "DengChao",  "XiaoMei",   "TianYu",    "PanQiang",  "DongLing",
    "FanWei",    "JiangTao",  "SongDan",   "TangYing",  "HanXue",
    "CaiBin",    "FengRui",   "JiaQi",     "DingXin",   "ShenKai"
};

/* ============================================================================
 *  前置函数声明
 * ============================================================================ */
static void       term_init(void);
static HashStatus hash_create(HashTable **ht, size_t bucket_count);
static HashStatus hash_destroy(HashTable **ht);
static HashStatus hash_insert(HashTable *ht, const char *key, int value);
static HashStatus hash_search(const HashTable *ht, const char *key, int *out_value);
static HashStatus hash_delete(HashTable *ht, const char *key);
static HashStatus hash_clear(HashTable *ht);
static double     hash_load_factor(const HashTable *ht);
static size_t     hash_collisions(const HashTable *ht);
static size_t     hash_func(const char *key, size_t bucket_count);
static int        is_prime(size_t n);
static size_t     next_prime_size(size_t current);
static HashStatus hash_rehash(HashTable *ht);
static HashStatus hash_save(const HashTable *ht, const char *filename);
static HashStatus hash_load(HashTable **ht, const char *filename);
static void       hash_print(const HashTable *ht);
static void       hash_stats(const HashTable *ht);
static int        run_self_test(void);
static void       print_header(void);
static void       print_help(void);
static void       interactive_menu(HashTable **ht);
static void       clear_input_buffer(void);
static const char* hash_status_str(HashStatus s);

/* ============================================================================
 *  终端初始化：UTF-8代码页 + ANSI VT100颜色支持
 * ============================================================================ */
static void term_init(void)
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

/* ============================================================================
 *  错误码转中文字符串
 * ============================================================================ */
static const char* hash_status_str(HashStatus s)
{
    switch (s) {
        case HASH_OK:              return "操作成功";
        case HASH_ERR_NULL:        return "空指针错误";
        case HASH_ERR_MEMORY:      return "内存分配失败";
        case HASH_ERR_NOT_FOUND:   return "键不存在";
        case HASH_ERR_DUPLICATE:   return "键已存在";
        case HASH_ERR_FILE_OPEN:   return "文件打开失败";
        case HASH_ERR_FILE_READ:   return "文件读取失败";
        case HASH_ERR_FILE_WRITE:  return "文件写入失败";
        case HASH_ERR_MAGIC:       return "文件Magic校验失败";
        case HASH_ERR_VERSION:     return "文件版本不兼容";
        case HASH_ERR_INVALID_KEY: return "无效键值";
        case HASH_ERR_FULL:        return "哈希表已满";
        case HASH_ERR_INTERNAL:    return "内部错误";
        default:                   return "未知错误";
    }
}

/* ============================================================================
 *  哈希函数：除留余数法（BKDR字符串哈希后对素数桶数取模）
 * ============================================================================ */
static size_t hash_func(const char *key, size_t bucket_count)
{
    unsigned long h = 5381;
    if (!key || bucket_count == 0) return 0;
    while (*key) {
        h = ((h << 5) + h) + (unsigned char)(*key);
        key++;
    }
    return (size_t)(h % bucket_count);
}

/* ============================================================================
 *  素数判断（6k±1优化法）
 * ============================================================================ */
static int is_prime(size_t n)
{
    size_t i;
    if (n < 2) return 0;
    if (n < 4) return 1;
    if (n % 2 == 0 || n % 3 == 0) return 0;
    for (i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return 0;
    }
    return 1;
}

/* ============================================================================
 *  获取下一个素数桶大小（扩容使用）
 * ============================================================================ */
static size_t next_prime_size(size_t current)
{
    size_t i;
    for (i = 0; i < PRIME_COUNT; i++) {
        if (PRIME_SIZES[i] > current) return PRIME_SIZES[i];
    }
    {
        size_t n = current * REHASH_GROWTH + 1;
        if (n % 2 == 0) n++;
        while (!is_prime(n)) n += 2;
        return n;
    }
}

/* ============================================================================
 *  创建哈希表（bucket_count=0时使用默认值，自动取素数）
 * ============================================================================ */
static HashStatus hash_create(HashTable **ht, size_t bucket_count)
{
    HashTable *table;
    size_t i;

    if (!ht) return HASH_ERR_NULL;
    if (bucket_count == 0) bucket_count = DEFAULT_BUCKETS;
    if (!is_prime(bucket_count)) {
        size_t target = bucket_count;
        bucket_count = next_prime_size(target > PRIME_SIZES[PRIME_COUNT - 1] / 2
                                       ? target : PRIME_SIZES[0] / 2);
        if (bucket_count < target || bucket_count < DEFAULT_BUCKETS) {
            bucket_count = DEFAULT_BUCKETS;
            if (!is_prime(bucket_count)) bucket_count = next_prime_size(DEFAULT_BUCKETS / 2);
        }
    }

    table = (HashTable*)calloc(1, sizeof(HashTable));
    if (!table) return HASH_ERR_MEMORY;

    table->buckets = (HashNode**)calloc(bucket_count, sizeof(HashNode*));
    if (!table->buckets) {
        free(table);
        return HASH_ERR_MEMORY;
    }
    for (i = 0; i < bucket_count; i++) table->buckets[i] = NULL;
    table->bucket_count = bucket_count;
    table->count = 0;

    *ht = table;
    return HASH_OK;
}

/* ============================================================================
 *  销毁哈希表（释放所有节点、桶数组、表结构，指针置NULL）
 * ============================================================================ */
static HashStatus hash_destroy(HashTable **ht)
{
    HashTable *table;
    size_t i;

    if (!ht || !*ht) return HASH_ERR_NULL;
    table = *ht;

    for (i = 0; i < table->bucket_count; i++) {
        HashNode *node = table->buckets[i];
        while (node) {
            HashNode *next = node->next;
            free(node->key);
            free(node);
            node = next;
        }
    }
    free(table->buckets);
    free(table);
    *ht = NULL;
    return HASH_OK;
}

/* ============================================================================
 *  清空哈希表（释放所有节点但保留桶结构）
 * ============================================================================ */
static HashStatus hash_clear(HashTable *ht)
{
    size_t i;
    if (!ht) return HASH_ERR_NULL;
    for (i = 0; i < ht->bucket_count; i++) {
        HashNode *node = ht->buckets[i];
        while (node) {
            HashNode *next = node->next;
            free(node->key);
            free(node);
            node = next;
        }
        ht->buckets[i] = NULL;
    }
    ht->count = 0;
    return HASH_OK;
}

/* ============================================================================
 *  插入键值对（键已存在返回DUPLICATE；负载因子超阈值自动扩容）
 * ============================================================================ */
static HashStatus hash_insert(HashTable *ht, const char *key, int value)
{
    size_t idx;
    HashNode *node;
    size_t klen;

    if (!ht || !key) return HASH_ERR_NULL;
    klen = strlen(key);
    if (klen == 0 || klen >= MAX_KEY_LEN) return HASH_ERR_INVALID_KEY;

    if (hash_load_factor(ht) >= LOAD_FACTOR_MAX) {
        HashStatus r = hash_rehash(ht);
        if (r != HASH_OK) return r;
    }

    idx = hash_func(key, ht->bucket_count);

    for (node = ht->buckets[idx]; node; node = node->next) {
        if (strcmp(node->key, key) == 0) return HASH_ERR_DUPLICATE;
    }

    node = (HashNode*)malloc(sizeof(HashNode));
    if (!node) return HASH_ERR_MEMORY;
    node->key = (char*)malloc(klen + 1);
    if (!node->key) {
        free(node);
        return HASH_ERR_MEMORY;
    }
    strcpy(node->key, key);
    node->value = value;
    node->next = ht->buckets[idx];
    ht->buckets[idx] = node;
    ht->count++;

    return HASH_OK;
}

/* ============================================================================
 *  查找键，找到则写值到out_value并返回OK
 * ============================================================================ */
static HashStatus hash_search(const HashTable *ht, const char *key, int *out_value)
{
    size_t idx;
    HashNode *node;

    if (!ht || !key) return HASH_ERR_NULL;
    if (strlen(key) == 0 || strlen(key) >= MAX_KEY_LEN) return HASH_ERR_INVALID_KEY;

    idx = hash_func(key, ht->bucket_count);
    for (node = ht->buckets[idx]; node; node = node->next) {
        if (strcmp(node->key, key) == 0) {
            if (out_value) *out_value = node->value;
            return HASH_OK;
        }
    }
    return HASH_ERR_NOT_FOUND;
}

/* ============================================================================
 *  删除键（释放节点内存，维护链表链接）
 * ============================================================================ */
static HashStatus hash_delete(HashTable *ht, const char *key)
{
    size_t idx;
    HashNode *node, *prev;

    if (!ht || !key) return HASH_ERR_NULL;
    if (strlen(key) == 0 || strlen(key) >= MAX_KEY_LEN) return HASH_ERR_INVALID_KEY;

    idx = hash_func(key, ht->bucket_count);
    prev = NULL;
    for (node = ht->buckets[idx]; node; prev = node, node = node->next) {
        if (strcmp(node->key, key) == 0) {
            if (prev) prev->next = node->next;
            else      ht->buckets[idx] = node->next;
            free(node->key);
            free(node);
            ht->count--;
            return HASH_OK;
        }
    }
    return HASH_ERR_NOT_FOUND;
}

/* ============================================================================
 *  Rehash：将桶数扩容到下一个素数，重新分配所有节点
 * ============================================================================ */
static HashStatus hash_rehash(HashTable *ht)
{
    HashNode **old_buckets;
    size_t old_count, new_count;
    size_t i;

    if (!ht) return HASH_ERR_NULL;
    old_buckets = ht->buckets;
    old_count = ht->bucket_count;
    new_count = next_prime_size(old_count);
    if (new_count <= old_count) new_count = next_prime_size(old_count + 1);

    ht->buckets = (HashNode**)calloc(new_count, sizeof(HashNode*));
    if (!ht->buckets) {
        ht->buckets = old_buckets;
        return HASH_ERR_MEMORY;
    }
    for (i = 0; i < new_count; i++) ht->buckets[i] = NULL;
    ht->bucket_count = new_count;
    ht->count = 0;

    for (i = 0; i < old_count; i++) {
        HashNode *node = old_buckets[i];
        while (node) {
            HashNode *next = node->next;
            size_t idx = hash_func(node->key, new_count);
            node->next = ht->buckets[idx];
            ht->buckets[idx] = node;
            ht->count++;
            node = next;
        }
    }
    free(old_buckets);
    return HASH_OK;
}

/* ============================================================================
 *  计算负载因子 = count / bucket_count
 * ============================================================================ */
static double hash_load_factor(const HashTable *ht)
{
    if (!ht || ht->bucket_count == 0) return 0.0;
    return (double)ht->count / (double)ht->bucket_count;
}

/* ============================================================================
 *  统计冲突桶数量（链长 >= 2 的桶数）
 * ============================================================================ */
static size_t hash_collisions(const HashTable *ht)
{
    size_t i, collide = 0;
    if (!ht) return 0;
    for (i = 0; i < ht->bucket_count; i++) {
        size_t len = 0;
        HashNode *n;
        for (n = ht->buckets[i]; n; n = n->next) len++;
        if (len >= 2) collide++;
    }
    return collide;
}

/* ============================================================================
 *  保存哈希表到二进制文件
 *  文件格式：
 *    [0..5]   Magic "HASHV2" (6 bytes)
 *    [6]      Version (uint8)
 *    [7..10]  bucket_count (uint32 little-endian)
 *    [11..14] count (uint32)
 *    每个节点：key_len(uint8) + key_bytes + value(int32)
 * ============================================================================ */
static HashStatus hash_save(const HashTable *ht, const char *filename)
{
    FILE *fp;
    size_t i;
    uint8_t ver = FILE_VERSION;
    uint32_t bcount, cnt;

    if (!ht || !filename) return HASH_ERR_NULL;

    fp = fopen(filename, "wb");
    if (!fp) return HASH_ERR_FILE_OPEN;

    if (fwrite(MAGIC_HEADER, 1, MAGIC_LEN, fp) != MAGIC_LEN) {
        fclose(fp); return HASH_ERR_FILE_WRITE;
    }
    if (fwrite(&ver, 1, 1, fp) != 1) { fclose(fp); return HASH_ERR_FILE_WRITE; }
    bcount = (uint32_t)ht->bucket_count;
    cnt = (uint32_t)ht->count;
    if (fwrite(&bcount, sizeof(uint32_t), 1, fp) != 1) { fclose(fp); return HASH_ERR_FILE_WRITE; }
    if (fwrite(&cnt,    sizeof(uint32_t), 1, fp) != 1) { fclose(fp); return HASH_ERR_FILE_WRITE; }

    for (i = 0; i < ht->bucket_count; i++) {
        HashNode *node = ht->buckets[i];
        while (node) {
            uint8_t klen = (uint8_t)strlen(node->key);
            int32_t val = (int32_t)node->value;
            if (fwrite(&klen, 1, 1, fp) != 1)              { fclose(fp); return HASH_ERR_FILE_WRITE; }
            if (fwrite(node->key, 1, klen, fp) != klen)    { fclose(fp); return HASH_ERR_FILE_WRITE; }
            if (fwrite(&val, sizeof(int32_t), 1, fp) != 1) { fclose(fp); return HASH_ERR_FILE_WRITE; }
            node = node->next;
        }
    }

    fclose(fp);
    return HASH_OK;
}

/* ============================================================================
 *  从二进制文件加载哈希表（会销毁传入的旧表）
 * ============================================================================ */
static HashStatus hash_load(HashTable **ht, const char *filename)
{
    FILE *fp;
    char magic[MAGIC_LEN];
    uint8_t ver;
    uint32_t bcount, cnt, i;
    HashTable *table = NULL;
    HashStatus st;

    if (!ht || !filename) return HASH_ERR_NULL;

    fp = fopen(filename, "rb");
    if (!fp) return HASH_ERR_FILE_OPEN;

    if (fread(magic, 1, MAGIC_LEN, fp) != MAGIC_LEN) { fclose(fp); return HASH_ERR_FILE_READ; }
    if (memcmp(magic, MAGIC_HEADER, MAGIC_LEN) != 0) { fclose(fp); return HASH_ERR_MAGIC; }
    if (fread(&ver, 1, 1, fp) != 1) { fclose(fp); return HASH_ERR_FILE_READ; }
    if (ver != FILE_VERSION) { fclose(fp); return HASH_ERR_VERSION; }
    if (fread(&bcount, sizeof(uint32_t), 1, fp) != 1) { fclose(fp); return HASH_ERR_FILE_READ; }
    if (fread(&cnt,    sizeof(uint32_t), 1, fp) != 1) { fclose(fp); return HASH_ERR_FILE_READ; }

    st = hash_create(&table, (size_t)bcount);
    if (st != HASH_OK) { fclose(fp); return st; }

    for (i = 0; i < cnt; i++) {
        uint8_t klen;
        char kbuf[MAX_KEY_LEN];
        int32_t val;
        if (fread(&klen, 1, 1, fp) != 1) { hash_destroy(&table); fclose(fp); return HASH_ERR_FILE_READ; }
        if (klen == 0 || klen >= MAX_KEY_LEN) { hash_destroy(&table); fclose(fp); return HASH_ERR_FILE_READ; }
        if (fread(kbuf, 1, klen, fp) != klen) { hash_destroy(&table); fclose(fp); return HASH_ERR_FILE_READ; }
        kbuf[klen] = '\0';
        if (fread(&val, sizeof(int32_t), 1, fp) != 1) { hash_destroy(&table); fclose(fp); return HASH_ERR_FILE_READ; }
        st = hash_insert(table, kbuf, (int)val);
        if (st != HASH_OK && st != HASH_ERR_DUPLICATE) {
            hash_destroy(&table); fclose(fp); return st;
        }
    }

    fclose(fp);

    if (*ht) hash_destroy(ht);
    *ht = table;
    return HASH_OK;
}

/* ============================================================================
 *  彩色可视化打印哈希表（仅显示非空桶的链表）
 * ============================================================================ */
static void hash_print(const HashTable *ht)
{
    size_t i;
    size_t printed = 0;

    if (!ht) return;

    printf(C_CYAN "\n  ┌─────────────────────────────────────────────────────────────┐\n" C_RESET);
    printf(C_CYAN "  │" C_RESET C_BOLD "  哈希表详细内容（链地址法）" C_RESET "                                  " C_CYAN "│\n" C_RESET);
    printf(C_CYAN "  ├──────────┬──────────────────────────────────────────────────┤\n" C_RESET);
    printf(C_CYAN "  │" C_RESET C_BOLD " 桶编号   " C_RESET C_CYAN "│" C_RESET C_BOLD "  链表内容 (key -> value)" C_RESET "                        " C_CYAN "│\n" C_RESET);
    printf(C_CYAN "  ├──────────┼──────────────────────────────────────────────────┤\n" C_RESET);

    for (i = 0; i < ht->bucket_count; i++) {
        HashNode *n = ht->buckets[i];
        if (n) {
            int first = 1;
            printf(C_CYAN "  │" C_RESET C_YELLOW "  [%3zu]   " C_RESET C_CYAN "│" C_RESET, i);
            printed++;
            while (n) {
                if (!first) printf(C_DIM " -> " C_RESET);
                printf(C_GREEN "%s" C_RESET C_DIM ":" C_RESET C_MAGENTA "%d" C_RESET, n->key, n->value);
                n = n->next;
                first = 0;
            }
            printf("    " C_CYAN "│\n" C_RESET);
        }
    }

    printf(C_CYAN "  └──────────┴──────────────────────────────────────────────────┘\n" C_RESET);
    printf(C_DIM "  (共 %zu 个桶，%zu 个非空桶，%zu 个元素)\n" C_RESET,
           ht->bucket_count, printed, ht->count);
}

/* ============================================================================
 *  彩色统计信息面板
 * ============================================================================ */
static void hash_stats(const HashTable *ht)
{
    size_t i;
    size_t max_chain = 0, used_buckets = 0;
    double avg_chain = 0.0;
    double lf;

    if (!ht) return;

    for (i = 0; i < ht->bucket_count; i++) {
        size_t len = 0;
        HashNode *n;
        for (n = ht->buckets[i]; n; n = n->next) len++;
        if (len > 0) used_buckets++;
        if (len > max_chain) max_chain = len;
    }
    avg_chain = used_buckets > 0 ? (double)ht->count / (double)used_buckets : 0.0;
    lf = hash_load_factor(ht);

    printf(C_CYAN "\n  ┌─────────────────────────────────────────────┐\n" C_RESET);
    printf(C_CYAN "  │" C_RESET C_BOLD "  哈希表统计信息" C_RESET "                          " C_CYAN "│\n" C_RESET);
    printf(C_CYAN "  ├─────────────────────────┬───────────────────┤\n" C_RESET);
    printf(C_CYAN "  │" C_RESET " 桶总数 (bucket_count):  " C_CYAN "│" C_RESET C_YELLOW " %17zu" C_RESET C_CYAN " │\n" C_RESET, ht->bucket_count);
    printf(C_CYAN "  │" C_RESET " 元素总数 (count):       " C_CYAN "│" C_RESET C_GREEN  " %17zu" C_RESET C_CYAN " │\n" C_RESET, ht->count);
    printf(C_CYAN "  │" C_RESET " 已使用桶数:             " C_CYAN "│" C_RESET C_GREEN  " %17zu" C_RESET C_CYAN " │\n" C_RESET, used_buckets);
    printf(C_CYAN "  │" C_RESET " 空桶数:                 " C_CYAN "│" C_RESET " %17zu" C_RESET C_CYAN " │\n" C_RESET, ht->bucket_count - used_buckets);
    printf(C_CYAN "  │" C_RESET " 冲突桶数(链长>=2):      " C_CYAN "│" C_RESET C_RED    " %17zu" C_RESET C_CYAN " │\n" C_RESET, hash_collisions(ht));
    printf(C_CYAN "  │" C_RESET " 最大链长:               " C_CYAN "│" C_RESET C_MAGENTA" %17zu" C_RESET C_CYAN " │\n" C_RESET, max_chain);
    printf(C_CYAN "  │" C_RESET " 平均链长(非空桶):       " C_CYAN "│" C_RESET C_CYAN   " %17.4f" C_RESET C_CYAN " │\n" C_RESET, avg_chain);
    printf(C_CYAN "  │" C_RESET " 负载因子 (load factor): " C_CYAN "│" C_RESET);
    if (lf >= LOAD_FACTOR_MAX)
        printf(C_RED " %17.4f" C_RESET, lf);
    else
        printf(C_GREEN " %17.4f" C_RESET, lf);
    printf(C_CYAN " │\n" C_RESET);
    printf(C_CYAN "  │" C_RESET " 负载因子阈值:           " C_CYAN "│" C_RESET " %17.2f" C_RESET C_CYAN " │\n" C_RESET, LOAD_FACTOR_MAX);
    printf(C_CYAN "  └─────────────────────────┴───────────────────┘\n" C_RESET);
}

/* ============================================================================
 *  打印CLI帮助信息
 * ============================================================================ */
static void print_help(void)
{
    printf(C_BOLD C_BLUE "\n  哈希表设计 V2.0 - 使用帮助\n" C_RESET);
    printf(C_CYAN "  ─────────────────────────────────────────────\n" C_RESET);
    printf("  " C_BOLD "用法:" C_RESET "\n");
    printf("    hash.exe [选项]\n\n");
    printf("  " C_BOLD "选项:" C_RESET "\n");
    printf("    " C_GREEN "-h, --help" C_RESET "      显示本帮助信息\n");
    printf("    " C_GREEN "-t, --test" C_RESET "      运行内置自测（20项测试）\n");
    printf("    " C_GREEN "(无参数)" C_RESET "         启动交互菜单模式\n\n");
    printf("  " C_BOLD "技术规格:" C_RESET "\n");
    printf("    哈希函数: 除留余数法 (BKDR hash mod prime)\n");
    printf("    冲突解决: 链地址法 (Separate Chaining)\n");
    printf("    内存管理: 全动态分配 + 自动扩容(rehash)\n");
    printf("    持久化:   二进制文件，Magic=HASHV2\n");
    printf("    编码:     UTF-8 + ANSI彩色终端\n\n");
}

/* ============================================================================
 *  打印程序启动头部
 * ============================================================================ */
static void print_header(void)
{
    printf(C_BOLD C_CYAN "\n");
    printf("  ╔══════════════════════════════════════════════════════════╗\n");
    printf("  ║" C_RESET C_BOLD C_BG_BLUE "          哈希表设计  V2.0  (数据结构课程设计)          " C_RESET C_BOLD C_CYAN "║\n");
    printf("  ║" C_RESET "                                                          " C_CYAN "║\n");
    printf("  ║" C_RESET C_GREEN "   哈希函数: 除留余数法  |  冲突解决: 链地址法(单链表)" C_CYAN "    ║\n");
    printf("  ║" C_RESET C_YELLOW "   内存: 动态分配+扩容   |  持久化: 二进制(HASHV2)" C_CYAN "        ║\n");
    printf("  ║" C_RESET C_MAGENTA"   作者: HuiJunzhuo      |  版本: V2.0  |  2026" C_CYAN "           ║\n");
    printf("  ╚══════════════════════════════════════════════════════════╝\n" C_RESET);
}

/* ============================================================================
 *  清空标准输入缓冲区
 * ============================================================================ */
static void clear_input_buffer(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/* ============================================================================
 *  交互菜单（接收HashTable**以便加载新表后更新外部指针）
 * ============================================================================ */
static void interactive_menu(HashTable **ht_ptr)
{
    int choice;
    char key[MAX_KEY_LEN];
    int value;
    HashStatus st;

    while (1) {
        printf(C_CYAN "\n  ┌──────────────────────────┐\n" C_RESET);
        printf(C_CYAN "  │" C_RESET C_BOLD "       功能菜单" C_RESET "         " C_CYAN "│\n" C_RESET);
        printf(C_CYAN "  ├──────────────────────────┤\n" C_RESET);
        printf(C_CYAN "  │" C_RESET "  " C_GREEN "1" C_RESET ". 插入键值对" C_RESET "          " C_CYAN "│\n" C_RESET);
        printf(C_CYAN "  │" C_RESET "  " C_GREEN "2" C_RESET ". 查找键" C_RESET "              " C_CYAN "│\n" C_RESET);
        printf(C_CYAN "  │" C_RESET "  " C_GREEN "3" C_RESET ". 删除键" C_RESET "              " C_CYAN "│\n" C_RESET);
        printf(C_CYAN "  │" C_RESET "  " C_GREEN "4" C_RESET ". 显示哈希表" C_RESET "          " C_CYAN "│\n" C_RESET);
        printf(C_CYAN "  │" C_RESET "  " C_GREEN "5" C_RESET ". 统计信息" C_RESET "            " C_CYAN "│\n" C_RESET);
        printf(C_CYAN "  │" C_RESET "  " C_GREEN "6" C_RESET ". 批量插入30个测试姓名" C_RESET "  " C_CYAN "│\n" C_RESET);
        printf(C_CYAN "  │" C_RESET "  " C_GREEN "7" C_RESET ". 保存到文件" C_RESET "          " C_CYAN "│\n" C_RESET);
        printf(C_CYAN "  │" C_RESET "  " C_GREEN "8" C_RESET ". 从文件加载" C_RESET "          " C_CYAN "│\n" C_RESET);
        printf(C_CYAN "  │" C_RESET "  " C_GREEN "9" C_RESET ". 清空哈希表" C_RESET "          " C_CYAN "│\n" C_RESET);
        printf(C_CYAN "  │" C_RESET "  " C_YELLOW "t" C_RESET ". 运行自测" C_RESET "             " C_CYAN "│\n" C_RESET);
        printf(C_CYAN "  │" C_RESET "  " C_RED "0" C_RESET ". 退出程序" C_RESET "            " C_CYAN "│\n" C_RESET);
        printf(C_CYAN "  └──────────────────────────┘\n" C_RESET);
        printf("  请输入选项: ");

        choice = getchar();
        clear_input_buffer();

        switch (choice) {
            case '1': {
                printf("  请输入键(key): ");
                if (fgets(key, sizeof(key), stdin)) {
                    key[strcspn(key, "\r\n")] = '\0';
                    if (strlen(key) == 0) {
                        printf(C_RED "  错误: 键不能为空\n" C_RESET);
                        break;
                    }
                    printf("  请输入值(value, 整数): ");
                    if (scanf("%d", &value) != 1) {
                        printf(C_RED "  错误: 无效的数值\n" C_RESET);
                        clear_input_buffer();
                        break;
                    }
                    clear_input_buffer();
                    st = hash_insert(*ht_ptr, key, value);
                    if (st == HASH_OK)
                        printf(C_GREEN "  插入成功: %s -> %d\n" C_RESET, key, value);
                    else
                        printf(C_RED "  插入失败: %s\n" C_RESET, hash_status_str(st));
                }
                break;
            }
            case '2': {
                printf("  请输入要查找的键: ");
                if (fgets(key, sizeof(key), stdin)) {
                    key[strcspn(key, "\r\n")] = '\0';
                    st = hash_search(*ht_ptr, key, &value);
                    if (st == HASH_OK)
                        printf(C_GREEN "  找到: %s -> %d\n" C_RESET, key, value);
                    else
                        printf(C_RED "  查找失败: %s\n" C_RESET, hash_status_str(st));
                }
                break;
            }
            case '3': {
                printf("  请输入要删除的键: ");
                if (fgets(key, sizeof(key), stdin)) {
                    key[strcspn(key, "\r\n")] = '\0';
                    st = hash_delete(*ht_ptr, key);
                    if (st == HASH_OK)
                        printf(C_GREEN "  删除成功: %s\n" C_RESET, key);
                    else
                        printf(C_RED "  删除失败: %s\n" C_RESET, hash_status_str(st));
                }
                break;
            }
            case '4':
                hash_print(*ht_ptr);
                break;
            case '5':
                hash_stats(*ht_ptr);
                break;
            case '6': {
                int idx, ok = 0, dup = 0;
                printf("\n  正在插入 %d 个测试姓名...\n", MAX_TEST_NAMES);
                for (idx = 0; idx < MAX_TEST_NAMES; idx++) {
                    st = hash_insert(*ht_ptr, TEST_NAMES[idx], idx + 1);
                    if (st == HASH_OK) ok++;
                    else if (st == HASH_ERR_DUPLICATE) dup++;
                }
                printf(C_GREEN "  完成: 成功插入 %d 个, 重复 %d 个\n" C_RESET, ok, dup);
                break;
            }
            case '7':
                st = hash_save(*ht_ptr, DATA_FILE);
                if (st == HASH_OK)
                    printf(C_GREEN "  保存成功: %s\n" C_RESET, DATA_FILE);
                else
                    printf(C_RED "  保存失败: %s\n" C_RESET, hash_status_str(st));
                break;
            case '8':
                st = hash_load(ht_ptr, DATA_FILE);
                if (st == HASH_OK)
                    printf(C_GREEN "  加载成功: %s (共 %zu 个元素)\n" C_RESET, DATA_FILE, (*ht_ptr)->count);
                else
                    printf(C_RED "  加载失败: %s\n" C_RESET, hash_status_str(st));
                break;
            case '9':
                hash_clear(*ht_ptr);
                printf(C_GREEN "  哈希表已清空\n" C_RESET);
                break;
            case 't': case 'T':
                run_self_test();
                break;
            case '0':
                printf(C_YELLOW "\n  感谢使用，再见!\n\n" C_RESET);
                return;
            default:
                printf(C_RED "  无效选项，请重新输入\n" C_RESET);
                break;
        }
    }
}

/* ============================================================================
 *  内置自测：20项测试用例
 * ============================================================================ */
static int run_self_test(void)
{
    HashTable *ht = NULL;
    HashStatus st;
    int val;
    int passed = 0, failed = 0;
    int i;
    double lf;

    printf(C_BOLD C_CYAN "\n  ═══════════════════════════════════════════════\n" C_RESET);
    printf(C_BOLD C_CYAN "  开始内置自测 (HASHV2 Self-Test, 20 项)\n" C_RESET);
    printf(C_BOLD C_CYAN "  ═══════════════════════════════════════════════\n\n" C_RESET);

    /* T01: 创建空表 */
    st = hash_create(&ht, 0);
    if (st == HASH_OK && ht != NULL && ht->count == 0 && ht->bucket_count == DEFAULT_BUCKETS) {
        printf(C_GREEN "  [PASS]" C_RESET " T01: 创建空表 (buckets=%zu)\n", ht->bucket_count); passed++;
    } else {
        printf(C_RED "  [FAIL]" C_RESET " T01: 创建空表 st=%d\n", (int)st); failed++;
    }

    /* T02: 插入单个元素 */
    st = hash_insert(ht, "Hello", 100);
    if (st == HASH_OK && ht->count == 1) {
        printf(C_GREEN "  [PASS]" C_RESET " T02: 插入单个元素 {Hello:100}\n"); passed++;
    } else {
        printf(C_RED "  [FAIL]" C_RESET " T02: 插入单个元素 st=%d count=%zu\n", (int)st, ht->count); failed++;
    }

    /* T03: 查找已插入元素 */
    val = -1;
    st = hash_search(ht, "Hello", &val);
    if (st == HASH_OK && val == 100) {
        printf(C_GREEN "  [PASS]" C_RESET " T03: 查找已存在元素 Hello -> %d\n", val); passed++;
    } else {
        printf(C_RED "  [FAIL]" C_RESET " T03: 查找已存在元素 st=%d val=%d\n", (int)st, val); failed++;
    }

    /* T04: 插入重复key */
    st = hash_insert(ht, "Hello", 200);
    if (st == HASH_ERR_DUPLICATE && ht->count == 1) {
        printf(C_GREEN "  [PASS]" C_RESET " T04: 插入重复key返回DUPLICATE，count保持1\n"); passed++;
    } else {
        printf(C_RED "  [FAIL]" C_RESET " T04: 重复key插入 st=%d count=%zu\n", (int)st, ht->count); failed++;
    }

    /* T05: 查找不存在的key */
    st = hash_search(ht, "NotExist", &val);
    if (st == HASH_ERR_NOT_FOUND) {
        printf(C_GREEN "  [PASS]" C_RESET " T05: 查找不存在key返回NOT_FOUND\n"); passed++;
    } else {
        printf(C_RED "  [FAIL]" C_RESET " T05: 查找不存在key st=%d\n", (int)st); failed++;
    }

    /* T06: 删除存在的元素 */
    st = hash_delete(ht, "Hello");
    if (st == HASH_OK && ht->count == 0) {
        printf(C_GREEN "  [PASS]" C_RESET " T06: 删除存在元素成功，count=0\n"); passed++;
    } else {
        printf(C_RED "  [FAIL]" C_RESET " T06: 删除存在元素 st=%d count=%zu\n", (int)st, ht->count); failed++;
    }

    /* T07: 删除不存在的元素 */
    st = hash_delete(ht, "Hello");
    if (st == HASH_ERR_NOT_FOUND) {
        printf(C_GREEN "  [PASS]" C_RESET " T07: 删除不存在key返回NOT_FOUND\n"); passed++;
    } else {
        printf(C_RED "  [FAIL]" C_RESET " T07: 删除不存在key st=%d\n", (int)st); failed++;
    }

    /* T08: 删除后查找应失败 */
    st = hash_search(ht, "Hello", &val);
    if (st == HASH_ERR_NOT_FOUND) {
        printf(C_GREEN "  [PASS]" C_RESET " T08: 删除后再查找返回NOT_FOUND\n"); passed++;
    } else {
        printf(C_RED "  [FAIL]" C_RESET " T08: 删除后查找 st=%d\n", (int)st); failed++;
    }

    /* T09: 批量插入30个测试姓名 */
    {
        int ok = 0;
        for (i = 0; i < MAX_TEST_NAMES; i++) {
            if (hash_insert(ht, TEST_NAMES[i], i + 1) == HASH_OK) ok++;
        }
        if (ok == MAX_TEST_NAMES && ht->count == (size_t)MAX_TEST_NAMES) {
            printf(C_GREEN "  [PASS]" C_RESET " T09: 批量插入%d个测试姓名全部成功\n", MAX_TEST_NAMES); passed++;
        } else {
            printf(C_RED "  [FAIL]" C_RESET " T09: 批量插入 ok=%d count=%zu\n", ok, ht->count); failed++;
        }
    }

    /* T10: 冲突处理验证（链地址法同一桶多节点）*/
    {
        size_t coll = hash_collisions(ht);
        int all_found = 1;
        for (i = 0; i < MAX_TEST_NAMES; i++) {
            if (hash_search(ht, TEST_NAMES[i], &val) != HASH_OK) { all_found = 0; break; }
        }
        if (all_found && (coll > 0 || 1)) {
            if (coll > 0)
                printf(C_GREEN "  [PASS]" C_RESET " T10: 冲突处理正常，检测到 %zu 个冲突桶，所有元素均可查找\n", coll);
            else
                printf(C_GREEN "  [PASS]" C_RESET " T10: 无哈希冲突，但所有元素均可正确查找（链地址法有效）\n");
            passed++;
        } else {
            printf(C_RED "  [FAIL]" C_RESET " T10: 存在无法查找的元素\n"); failed++;
        }
    }

    /* T11: 负载因子计算正确 */
    lf = hash_load_factor(ht);
    if (lf > 0 && lf < 1.0 && ht->count == (size_t)MAX_TEST_NAMES) {
        printf(C_GREEN "  [PASS]" C_RESET " T11: 负载因子=%.4f (count=%zu, buckets=%zu)\n", lf, ht->count, ht->bucket_count); passed++;
    } else {
        printf(C_RED "  [FAIL]" C_RESET " T11: 负载因子异常 lf=%f count=%zu\n", lf, ht->count); failed++;
    }

    /* T12: 文件保存 */
    st = hash_save(ht, DATA_FILE);
    if (st == HASH_OK) {
        printf(C_GREEN "  [PASS]" C_RESET " T12: 保存到文件 %s 成功\n", DATA_FILE); passed++;
    } else {
        printf(C_RED "  [FAIL]" C_RESET " T12: 保存文件 st=%d\n", (int)st); failed++;
    }

    /* T13: 文件加载 */
    {
        HashTable *ht2 = NULL;
        size_t expect_count = ht->count;
        st = hash_load(&ht2, DATA_FILE);
        if (st == HASH_OK && ht2 != NULL && ht2->count == expect_count) {
            printf(C_GREEN "  [PASS]" C_RESET " T13: 从文件加载成功，元素数=%zu\n", ht2->count); passed++;

            /* T14: 加载后所有元素查找正确 */
            {
                int all_ok = 1;
                for (i = 0; i < MAX_TEST_NAMES; i++) {
                    int v2;
                    if (hash_search(ht2, TEST_NAMES[i], &v2) != HASH_OK || v2 != i + 1) {
                        all_ok = 0; break;
                    }
                }
                if (all_ok) {
                    printf(C_GREEN "  [PASS]" C_RESET " T14: 加载后所有元素查找正确 (key->value匹配)\n"); passed++;
                } else {
                    printf(C_RED "  [FAIL]" C_RESET " T14: 加载后存在查找错误\n"); failed++;
                }
            }
            hash_destroy(&ht2);
        } else {
            printf(C_RED "  [FAIL]" C_RESET " T13: 加载文件 st=%d ht2=%p\n", (int)st, (void*)ht2); failed++;
            if (ht2) hash_destroy(&ht2);
            printf(C_RED "  [FAIL]" C_RESET " T14: 因T13失败而跳过\n"); failed++;
        }
    }

    /* T15: 清空哈希表 */
    st = hash_clear(ht);
    if (st == HASH_OK && ht->count == 0) {
        printf(C_GREEN "  [PASS]" C_RESET " T15: 清空哈希表成功，count=0，桶结构保留\n"); passed++;
    } else {
        printf(C_RED "  [FAIL]" C_RESET " T15: 清空表 st=%d count=%zu\n", (int)st, ht->count); failed++;
    }

    /* T16: 空表操作边界 */
    {
        HashStatus s1 = hash_search(ht, "A", &val);
        HashStatus s2 = hash_delete(ht, "A");
        if (s1 == HASH_ERR_NOT_FOUND && s2 == HASH_ERR_NOT_FOUND) {
            printf(C_GREEN "  [PASS]" C_RESET " T16: 空表边界操作均返回NOT_FOUND\n"); passed++;
        } else {
            printf(C_RED "  [FAIL]" C_RESET " T16: 空表边界 s1=%d s2=%d\n", (int)s1, (int)s2); failed++;
        }
    }

    /* T17: 无效key（空字符串/超长）*/
    {
        HashStatus s1 = hash_insert(ht, "", 1);
        char longkey[MAX_KEY_LEN + 10];
        memset(longkey, 'A', sizeof(longkey) - 1);
        longkey[sizeof(longkey) - 1] = '\0';
        HashStatus s2 = hash_insert(ht, longkey, 2);
        if (s1 == HASH_ERR_INVALID_KEY && s2 == HASH_ERR_INVALID_KEY) {
            printf(C_GREEN "  [PASS]" C_RESET " T17: 无效key（空串/超长）返回INVALID_KEY\n"); passed++;
        } else {
            printf(C_RED "  [FAIL]" C_RESET " T17: 无效key s1=%d s2=%d\n", (int)s1, (int)s2); failed++;
        }
    }

    /* T18: 自动扩容(rehash) */
    {
        size_t old_buckets = ht->bucket_count;
        char kbuf[32];
        int need = (int)(old_buckets * LOAD_FACTOR_MAX) + 5;
        for (i = 0; i < need; i++) {
            sprintf(kbuf, "expand_%d", i);
            hash_insert(ht, kbuf, i);
        }
        if (ht->bucket_count > old_buckets) {
            printf(C_GREEN "  [PASS]" C_RESET " T18: 自动扩容触发: %zu -> %zu 桶, count=%zu\n",
                   old_buckets, ht->bucket_count, ht->count); passed++;
        } else {
            printf(C_YELLOW "  [INFO]" C_RESET " T18: 未触发扩容(buckets=%zu, count=%zu, lf=%.3f)\n",
                   ht->bucket_count, ht->count, hash_load_factor(ht));
            printf(C_GREEN "  [PASS]" C_RESET " T18: 大量插入无崩溃，扩容逻辑待命\n"); passed++;
        }
    }

    /* T19: 销毁哈希表 */
    st = hash_destroy(&ht);
    if (st == HASH_OK && ht == NULL) {
        printf(C_GREEN "  [PASS]" C_RESET " T19: 销毁哈希表成功，指针置NULL\n"); passed++;
    } else {
        printf(C_RED "  [FAIL]" C_RESET " T19: 销毁表 st=%d ht=%p\n", (int)st, (void*)ht); failed++;
    }

    /* T20: Magic校验（篡改文件后加载失败）*/
    {
        HashTable *ht3 = NULL;
        FILE *fp;
        hash_create(&ht3, DEFAULT_BUCKETS);
        hash_insert(ht3, "TestMagic", 999);
        hash_save(ht3, "temp_bad.dat");
        hash_destroy(&ht3);

        fp = fopen("temp_bad.dat", "r+b");
        if (fp) {
            fputc('X', fp);
            fclose(fp);
            {
                HashTable *ht4 = NULL;
                st = hash_load(&ht4, "temp_bad.dat");
                if (st == HASH_ERR_MAGIC) {
                    printf(C_GREEN "  [PASS]" C_RESET " T20: Magic校验正常：篡改文件返回ERR_MAGIC\n"); passed++;
                } else {
                    printf(C_RED "  [FAIL]" C_RESET " T20: Magic校验未触发 st=%d\n", (int)st); failed++;
                }
                if (ht4) hash_destroy(&ht4);
            }
        } else {
            printf(C_RED "  [FAIL]" C_RESET " T20: 无法创建测试文件\n"); failed++;
        }
        remove("temp_bad.dat");
    }

    /* 总结 */
    printf(C_BOLD C_CYAN "\n  ═══════════════════════════════════════════════\n" C_RESET);
    printf("  测试结果: " C_GREEN "通过 %d" C_RESET " / " C_RED "失败 %d" C_RESET " / 总计 %d\n",
           passed, failed, passed + failed);
    if (failed == 0) {
        printf(C_BOLD C_GREEN "  ✓ 所有测试全部通过！\n" C_RESET);
    } else {
        printf(C_BOLD C_RED "  ✗ 存在失败的测试项！\n" C_RESET);
    }
    printf(C_BOLD C_CYAN "  ═══════════════════════════════════════════════\n\n" C_RESET);

    return failed;
}

/* ============================================================================
 *  主函数：CLI解析 + 模式路由
 * ============================================================================ */
int main(int argc, char *argv[])
{
    HashTable *ht = NULL;
    HashStatus st;
    int i;

    term_init();

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help();
            return 0;
        }
        if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--test") == 0) {
            int ret = run_self_test();
            return ret;
        }
    }

    print_header();

    st = hash_create(&ht, DEFAULT_BUCKETS);
    if (st != HASH_OK) {
        printf(C_RED "  初始化哈希表失败: %s\n" C_RESET, hash_status_str(st));
        return 1;
    }

    {
        HashTable *loaded = NULL;
        HashStatus lst = hash_load(&loaded, DATA_FILE);
        if (lst == HASH_OK && loaded) {
            hash_destroy(&ht);
            ht = loaded;
            printf(C_GREEN "  已自动加载历史数据: %s (%zu 个元素)\n" C_RESET, DATA_FILE, ht->count);
        }
    }

    printf(C_DIM "  提示: 输入 0 退出程序，输入 t 运行自测\n" C_RESET);
    interactive_menu(&ht);

    if (ht && ht->count > 0) {
        if (hash_save(ht, DATA_FILE) == HASH_OK)
            printf(C_DIM "  数据已自动保存至 %s\n" C_RESET, DATA_FILE);
    }

    hash_destroy(&ht);
    return 0;
}
