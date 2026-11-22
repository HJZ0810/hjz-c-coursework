/*
* ============================================================================
*
*    哈夫曼编码/译码系统  (完全体·增强版)
*    Huffman Encoding / Decoding System  (Full Enhanced Version)
*
* ============================================================================
*
*  功能概述:
*    基于哈夫曼算法实现真正的文件压缩/解压系统，支持：
*      [1] 用户登录认证（3次密码重试，密码不回显）
*      [2] 初始化哈夫曼树（默认英文频率 / 自定义权重 / 从文件自动统计频率）
*      [3] 哈夫曼编码 —— 真正的bit-level比特压缩，输出独立.huf压缩包
*      [4] 哈夫曼译码 —— 从.huf文件解压，自动重建哈夫曼树
*      [5] 编码文件查看 —— 格式化打印0/1编码（带对齐边框）
*      [6] 哈夫曼树可视化 —— 彩色缩进式树形结构打印
*      [7] 压缩包信息查看 —— 不解压直接查看压缩率等信息
*      [8] 操作日志记录 —— 追加写入 huffman.log
*      [9] 命令行模式 —— 支持 -c/-d/-t 非交互调用及拖放文件
*     [10] 彩色终端界面 —— ANSI颜色支持（Windows 10+ / Linux）
*
*  数据结构:
*    HTNode    —— 哈夫曼树节点（动态数组存储，下标1起始）
*                叶子[1..n]:   存储字符和权重
*                内部[n+1..2n-1]: 权重=两子树权重之和
*                parent/lchild/rchild=0表示"无/未使用"
*    HCItem    —— 编码表项（ch -> 编码串）
*    BitStream —— 比特流读写器（bit-level I/O）
*    HufHeader —— .huf压缩文件头
*
*  编译命令:
*    gcc -std=c99 -Wall -Wextra -O2 -o huffman.exe main.c
*
*  命令行用法:
*    huffman.exe -c <输入文件> <输出.huf>    压缩
*    huffman.exe -d <输入.huf> <输出文件>    解压
*    huffman.exe -t <文件.huf>               查看压缩包信息
*    （Windows下直接拖放文件到huffman.exe即可自动压缩/解压）
*
*  交互模式:
*    直接运行 huffman.exe 进入菜单交互模式
*
*  编码: UTF-8
* ============================================================================
*/

/* ======================== 标准头文件 ======================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
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
#include <termios.h>
#include <fcntl.h>
#endif

/* ======================== 常量与宏 ======================== */
#define MAX_CODELEN     256
#define HUFF_MAGIC      "HUFF"
#define HUFF_VERSION    2
#define LOG_FILE        "huffman.log"
#define TREE_FILE       "hfmTree"
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

#define LOGIN_USER  "admin"
#define LOGIN_PASS  "123456"

/* ======================== 错误码 ======================== */
typedef enum {
    HUF_OK = 0,
    HUF_ERR_OPEN_FILE,
    HUF_ERR_READ_FILE,
    HUF_ERR_WRITE_FILE,
    HUF_ERR_MEMORY,
    HUF_ERR_CORRUPT,
    HUF_ERR_MAGIC,
    HUF_ERR_VERSION,
    HUF_ERR_EMPTY,
    HUF_ERR_LOGIN,
    HUF_ERR_NO_TREE,
    HUF_ERR_VERIFY,
    HUF_ERR_CANCEL,
} HufStatus;

/* ======================== 数据结构 ======================== */

typedef struct {
    int  weight;
    int  parent;
    int  lchild;
    int  rchild;
    unsigned char ch;
} HTNode;

typedef struct {
    unsigned char ch;
    char code[MAX_CODELEN];
} HCItem;

typedef struct {
    FILE *fp;
    unsigned char buf;
    int bitPos;
    long totalBits;
} BitStream;

typedef struct {
    char magic[4];
    unsigned short version;
    unsigned short n;
    unsigned int origLen;
    unsigned int totalBits;
} HufHeader;

/* ======================== 全局状态（动态分配） ======================== */
static HTNode *huffmanTree = NULL;
static HCItem *codeTable  = NULL;
static const char **codeMap = NULL;
static int    charCount = 0;
static int    treeSize  = 0;
static int    treeBuilt = 0;
static int    colorEnabled = 1;

/* ======================== 前向声明 ======================== */
static void saveTreeToFile(void);
static HufStatus verifyCompression(const char *origFile, const char *hufFile);
static void printSubTree(FILE *fp, int idx, int depth, char *prefix, int isLeft);

/* ======================== 工具函数：颜色与终端 ======================== */

static void enableVTMode(void) {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) { colorEnabled = 0; return; }
    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) { colorEnabled = 0; return; }
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
    colorEnabled = 1;
#endif
}

static void cPrint(const char *color, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (colorEnabled && color) fputs(color, stdout);
    vprintf(fmt, args);
    if (colorEnabled && color) fputs(C_RESET, stdout);
    va_end(args);
}

static void printOK(const char *msg)    { cPrint(C_GREEN, "  [OK] "); printf("%s\n", msg); }
static void printErr(const char *msg)   { cPrint(C_RED,   "  [ERROR] "); printf("%s\n", msg); }
static void printWarn(const char *msg)  { cPrint(C_YELLOW,"  [WARN] "); printf("%s\n", msg); }
static void printInfo(const char *msg)  { cPrint(C_CYAN,  "  [INFO] "); printf("%s\n", msg); }

static void logMessage(const char *fmt, ...) {
    FILE *fp = fopen(LOG_FILE, "a");
    if (!fp) return;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(fp, "[%04d-%02d-%02d %02d:%02d:%02d] ",
            t->tm_year+1900, t->tm_mon+1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);
    va_list args;
    va_start(args, fmt);
    vfprintf(fp, fmt, args);
    va_end(args);
    fputc('\n', fp);
    fclose(fp);
}

/* ======================== 工具函数：UI ======================== */

static void printBanner(void) {
    printf("\n");
    cPrint(C_BOLD C_BLUE,
        "  ============================================================================\n");
    cPrint(C_BOLD C_CYAN,
        "  |          Huffman Encoding / Decoding System  (Enhanced Edition)          |\n");
    cPrint(C_BOLD C_CYAN,
        "  |                         哈夫曼编码 / 译码系统  完全体增强版               |\n");
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

static void readPassword(char *buf, int size) {
#ifdef _WIN32
    int i = 0; int ch;
    while (i < size - 1) {
        ch = _getch();
        if (ch == '\r' || ch == '\n') { break; }
        if (ch == '\b' || ch == 127) {
            if (i > 0) { i--; printf("\b \b"); fflush(stdout); }
        } else {
            buf[i++] = (char)ch;
            printf("*"); fflush(stdout);
        }
    }
    buf[i] = '\0';
    printf("\n");
#else
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    int i = 0; int ch;
    while (i < size - 1) {
        ch = getchar();
        if (ch == '\n' || ch == '\r') break;
        if (ch == 127 || ch == '\b') {
            if (i > 0) { i--; printf("\b \b"); fflush(stdout); }
        } else {
            buf[i++] = (char)ch;
            printf("*"); fflush(stdout);
        }
    }
    buf[i] = '\0';
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    printf("\n");
#endif
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

static void printProgress(long current, long total) {
    if (total <= 0) return;
    int barW = MAX_PROGRESS;
    double ratio = (double)current / total;
    if (ratio > 1.0) ratio = 1.0;
    int filled = (int)(ratio * barW);
    printf("\r  [");
    for (int i = 0; i < barW; i++) {
        if (i < filled) cPrint(C_GREEN, "█");
        else            cPrint(C_GRAY,  "░");
    }
    printf("] %5.1f%%", ratio * 100.0);
    fflush(stdout);
    if (current >= total) printf("\n");
}

/* ======================== 内存管理 ======================== */

static void freeHuffmanState(void) {
    if (huffmanTree) { free(huffmanTree); huffmanTree = NULL; }
    if (codeTable)  { free(codeTable);  codeTable  = NULL; }
    if (codeMap)    { free(codeMap);    codeMap    = NULL; }
    charCount = 0; treeSize = 0; treeBuilt = 0;
}

static int allocHuffmanState(int n) {
    freeHuffmanState();
    huffmanTree = (HTNode *)calloc((size_t)(2 * n + 1), sizeof(HTNode));
    codeTable  = (HCItem *)calloc((size_t)n, sizeof(HCItem));
    codeMap    = (const char **)calloc(256, sizeof(const char *));
    if (!huffmanTree || !codeTable || !codeMap) {
        freeHuffmanState();
        return 0;
    }
    charCount = n;
    treeSize  = 2 * n - 1;
    return 1;
}

/* ======================== BitStream 比特流读写 ======================== */

static void bs_initWrite(BitStream *bs, FILE *fp) {
    bs->fp = fp; bs->buf = 0; bs->bitPos = 0; bs->totalBits = 0;
}

static void bs_writeBit(BitStream *bs, int bit) {
    if (bit) bs->buf |= (unsigned char)(1 << (7 - bs->bitPos));
    bs->bitPos++;
    bs->totalBits++;
    if (bs->bitPos == 8) {
        fwrite(&bs->buf, 1, 1, bs->fp);
        bs->buf = 0;
        bs->bitPos = 0;
    }
}

static void bs_writeBits(BitStream *bs, const char *bits) {
    for (int i = 0; bits[i]; i++) {
        bs_writeBit(bs, bits[i] == '1' ? 1 : 0);
    }
}

static void bs_flush(BitStream *bs) {
    if (bs->bitPos > 0) {
        fwrite(&bs->buf, 1, 1, bs->fp);
        bs->buf = 0;
        bs->bitPos = 0;
    }
}

static void bs_initRead(BitStream *bs, FILE *fp) {
    bs->fp = fp; bs->buf = 0; bs->bitPos = 8; bs->totalBits = 0;
}

static int bs_readBit(BitStream *bs) {
    if (bs->bitPos == 8) {
        if (fread(&bs->buf, 1, 1, bs->fp) != 1) return -1;
        bs->bitPos = 0;
    }
    int bit = (bs->buf >> (7 - bs->bitPos)) & 1;
    bs->bitPos++;
    bs->totalBits++;
    return bit;
}

/* ======================== 哈夫曼树构建 ======================== */

static int findMinWeight(int limit, int exclude) {
    int minIdx = 0;
    for (int j = 1; j <= limit; j++) {
        if (huffmanTree[j].parent == 0 && j != exclude) {
            if (minIdx == 0 || huffmanTree[j].weight < huffmanTree[minIdx].weight)
                minIdx = j;
        }
    }
    return minIdx;
}

static void buildHuffmanTree(void) {
    int n = charCount;
    treeSize = 2 * n - 1;
    for (int i = 1; i <= treeSize; i++) {
        huffmanTree[i].parent = 0;
        huffmanTree[i].lchild = 0;
        huffmanTree[i].rchild = 0;
        if (i > n) {
            huffmanTree[i].weight = 0;
            huffmanTree[i].ch = 0;
        }
    }
    if (n == 1) {
        treeSize = 1;
        treeBuilt = 1;
        return;
    }
    for (int i = n + 1; i <= treeSize; i++) {
        int m1 = findMinWeight(i - 1, 0);
        int m2 = findMinWeight(i - 1, m1);
        huffmanTree[m1].parent = i;
        huffmanTree[m2].parent = i;
        huffmanTree[i].lchild  = m1;
        huffmanTree[i].rchild  = m2;
        huffmanTree[i].weight  = huffmanTree[m1].weight + huffmanTree[m2].weight;
    }
    treeBuilt = 1;
}

/* ======================== 编码表生成 ======================== */

static void generateCodes(void) {
    if (!treeBuilt) return;
    memset(codeMap, 0, 256 * sizeof(const char *));
    if (charCount == 1) {
        codeTable[0].ch = huffmanTree[1].ch;
        strcpy(codeTable[0].code, "0");
        codeMap[huffmanTree[1].ch] = "0";
        return;
    }
    char temp[MAX_CODELEN];
    for (int i = 1; i <= charCount; i++) {
        int pos = 0, c = i, f = huffmanTree[c].parent;
        while (f != 0 && pos < MAX_CODELEN - 1) {
            temp[pos++] = (huffmanTree[f].lchild == c) ? '0' : '1';
            c = f; f = huffmanTree[c].parent;
        }
        temp[pos] = '\0';
        codeTable[i - 1].ch = huffmanTree[i].ch;
        for (int j = 0; j < pos; j++)
            codeTable[i - 1].code[j] = temp[pos - 1 - j];
        codeTable[i - 1].code[pos] = '\0';
        codeMap[huffmanTree[i].ch] = codeTable[i - 1].code;
    }
}

/* ======================== 权重初始化 ======================== */

static void initDefaultTree(void) {
    static const unsigned char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static const int weights[] = {
        816,149,278,425,1270,223,201,609,697, 15,
         77,402,240,675,751,193, 9,599,633,906,
        276, 98,236, 15,197, 7
    };
    if (!allocHuffmanState(26)) { printErr("内存分配失败"); return; }
    for (int i = 0; i < 26; i++) {
        huffmanTree[i+1].ch = chars[i];
        huffmanTree[i+1].weight = weights[i];
    }
    buildHuffmanTree();
    saveTreeToFile();
    printOK("已加载默认英文频率表（26个大写字母）并构建哈夫曼树");
    logMessage("INIT: 默认英文频率表，%d叶子", charCount);
}

static void initCustomTree(void) {
    int n = readInt("请输入字符种类数 n (2~256): ", 2, 256);
    if (!allocHuffmanState(n)) { printErr("内存分配失败"); return; }
    char line[256];
    int used[256] = {0};
    for (int i = 0; i < n; i++) {
        while (1) {
            printf("  第 %d 个字符（可输入任意ASCII字符）: ", i+1);
            fflush(stdout);
            if (!fgets(line, sizeof(line), stdin)) continue;
            unsigned char c = (unsigned char)line[0];
            if (c == '\n' || c == '\r') { printf("  [!] 空输入，请重试\n"); continue; }
            if (used[c]) { printf("  [!] 字符 '%c' 已存在，请输入其他字符\n", c); continue; }
            if (line[1] != '\n' && line[1] != '\r') { int ch; while((ch=getchar())!='\n'&&ch!=EOF); }
            huffmanTree[i+1].ch = c;
            used[c] = 1;
            char prompt[128];
            snprintf(prompt, sizeof(prompt), "字符 '%c' 的权重(1~99999): ", c);
            huffmanTree[i+1].weight = readInt(prompt, 1, 99999);
            break;
        }
    }
    buildHuffmanTree();
    saveTreeToFile();
    printOK("已构建自定义哈夫曼树");
    logMessage("INIT: 自定义权重，%d叶子", charCount);
}

static HufStatus initFromFile(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) { printErr("无法打开文件"); logMessage("ERROR: 无法打开%s", filename); return HUF_ERR_OPEN_FILE; }
    int freq[256] = {0};
    int ch; long total = 0; int distinct = 0;
    long fsize = getFileSize(fp);
    long processed = 0;
    while ((ch = fgetc(fp)) != EOF) {
        if (freq[ch] == 0) distinct++;
        freq[ch]++;
        total++;
        processed++;
        if (fsize > 100000 && processed % 50000 == 0) printProgress(processed, fsize);
    }
    if (fsize > 100000) printProgress(fsize, fsize);
    fclose(fp);
    if (total == 0) { printErr("文件为空"); return HUF_ERR_EMPTY; }
    if (!allocHuffmanState(distinct)) { printErr("内存分配失败"); return HUF_ERR_MEMORY; }
    int idx = 0;
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            idx++;
            huffmanTree[idx].ch = (unsigned char)i;
            huffmanTree[idx].weight = freq[i];
        }
    }
    buildHuffmanTree();
    saveTreeToFile();
    cPrint(C_GREEN, "  [OK] 统计完成: %ld 字节, %d 种不同字节\n", total, distinct);
    logMessage("INIT: 从文件%s统计，%ld字节，%d种字节", filename, total, distinct);
    return HUF_OK;
}

/* ======================== 树的保存/加载（文本格式，用于持久化） ======================== */

void saveTreeToFile(void) {
    if (!treeBuilt) return;
    FILE *fp = fopen(TREE_FILE, "wb");
    if (!fp) { printWarn("无法写入树文件"); return; }
    fprintf(fp, "%d\n", charCount);
    for (int i = 1; i <= charCount; i++)
        fprintf(fp, "%d %d\n", huffmanTree[i].ch, huffmanTree[i].weight);
    fprintf(fp, "%d\n", treeSize);
    for (int i = 1; i <= treeSize; i++)
        fprintf(fp, "%d %d %d %d\n",
                huffmanTree[i].weight, huffmanTree[i].parent,
                huffmanTree[i].lchild, huffmanTree[i].rchild);
    fclose(fp);
}

static int loadTreeFromFile(void) {
    FILE *fp = fopen(TREE_FILE, "rb");
    if (!fp) return 0;
    int n;
    if (fscanf(fp, "%d", &n) != 1 || n < 1 || n > 256) { fclose(fp); return 0; }
    if (!allocHuffmanState(n)) { fclose(fp); return 0; }
    for (int i = 1; i <= charCount; i++) {
        int ch;
        if (fscanf(fp, "%d %d", &ch, &huffmanTree[i].weight) != 2)
            { fclose(fp); freeHuffmanState(); return 0; }
        huffmanTree[i].ch = (unsigned char)ch;
    }
    int ts;
    if (fscanf(fp, "%d", &ts) != 1) { fclose(fp); freeHuffmanState(); return 0; }
    treeSize = ts;
    for (int i = 1; i <= treeSize; i++) {
        if (fscanf(fp, "%d %d %d %d",
            &huffmanTree[i].weight, &huffmanTree[i].parent,
            &huffmanTree[i].lchild, &huffmanTree[i].rchild) != 4)
            { fclose(fp); freeHuffmanState(); return 0; }
    }
    fclose(fp);
    treeBuilt = 1;
    generateCodes();
    printInfo("已从 " C_BOLD TREE_FILE C_RESET C_CYAN " 加载上次构建的哈夫曼树");
    return 1;
}

/* ======================== 压缩率统计 ======================== */

static void printCompressionStats(long origBytes, long compBytes, long totalBits) {
    double ratio = (origBytes > 0) ? (double)compBytes / origBytes * 100.0 : 0;
    double saved = 100.0 - ratio;
    double avgLen = (origBytes > 0) ? (double)totalBits / origBytes : 0;
    double entropy = 0.0;
    long totalW = 0;
    for (int i = 1; i <= charCount; i++) totalW += huffmanTree[i].weight;
    if (charCount == 1) { entropy = 0.0; avgLen = 1.0; }
    else for (int i = 1; i <= charCount; i++) {
        double p = (double)huffmanTree[i].weight / totalW;
        if (p > 0) entropy -= p * (log(p) / log(2.0));
    }
    double eff = (avgLen > 0.001) ? entropy / avgLen * 100.0 : 0;
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  压缩统计报告\n");
    printDivider();
    printf("  原始大小:     "); cPrint(C_WHITE, "%ld 字节 (%.2f KB)\n", origBytes, origBytes/1024.0);
    printf("  压缩后大小:   "); cPrint(C_WHITE, "%ld 字节 (%.2f KB)\n", compBytes, compBytes/1024.0);
    printf("  压缩率:       "); cPrint(ratio<100?C_GREEN:C_RED, "%.2f%%\n", ratio);
    printf("  节省空间:     "); cPrint(saved>0?C_GREEN:C_RED, "%.2f%%\n", saved);
    printf("  总编码bit数:  "); cPrint(C_WHITE, "%ld\n", totalBits);
    printf("  平均码长:     "); cPrint(C_WHITE, "%.3f bit/字节\n", avgLen);
    printf("  信息熵:       "); cPrint(C_WHITE, "%.3f bit/字节\n", entropy);
    printf("  编码效率:     "); cPrint(eff>90?C_GREEN:C_YELLOW, "%.2f%%\n", eff);
    printf("  字符种类数:   "); cPrint(C_WHITE, "%d\n", charCount);
    printThickDivider();
}

/* ======================== 编码表显示 ======================== */

static void displayCodeTable(int maxShow) {
    if (!treeBuilt) return;
    int cnt = charCount;
    if (maxShow > 0 && cnt > maxShow) cnt = maxShow;
    cPrint(C_BOLD C_CYAN, "\n  哈夫曼编码表%s:\n", (charCount>cnt)?"(前若干项)":"");
    printDivider();
    cPrint(C_BOLD, "  %-6s  %-12s  %-10s\n", "字符", "编码", "码长");
    printDivider();
    for (int i = 0; i < cnt; i++) {
        unsigned char c = codeTable[i].ch;
        int len = (int)strlen(codeTable[i].code);
        if (c >= 32 && c < 127)
            printf("  '%c'    %-12s  %d\n", c, codeTable[i].code, len);
        else if (c == '\n')
            cPrint(C_GRAY, "  '\\n'   %-12s  %d\n", codeTable[i].code, len);
        else if (c == '\r')
            cPrint(C_GRAY, "  '\\r'   %-12s  %d\n", codeTable[i].code, len);
        else if (c == '\t')
            cPrint(C_GRAY, "  '\\t'   %-12s  %d\n", codeTable[i].code, len);
        else if (c == ' ')
            cPrint(C_GRAY, "  ' '    %-12s  %d\n", codeTable[i].code, len);
        else
            cPrint(C_GRAY, "  0x%02X  %-12s  %d\n", c, codeTable[i].code, len);
    }
    if (charCount > cnt) cPrint(C_GRAY, "  ... 还有 %d 个字符未显示\n", charCount - cnt);
    printDivider();
}

/* ======================== 哈夫曼树可视化打印 ======================== */

static void printSubTree(FILE *fp, int idx, int depth, char *prefix, int isLeft) {
    char newPrefix[512];
    if (idx == 0) return;
    const char *rightBr = depth == 0 ? "" : (isLeft ? "└──" : "├──");
    snprintf(newPrefix, sizeof(newPrefix), "%s%s",
             prefix, depth==0 ? "" : (isLeft ? "    " : "│   "));
    if (huffmanTree[idx].rchild != 0)
        printSubTree(fp, huffmanTree[idx].rchild, depth+1, newPrefix, 0);
    char line[1024];
    if (huffmanTree[idx].lchild == 0 && huffmanTree[idx].rchild == 0) {
        unsigned char c = huffmanTree[idx].ch;
        if (c >= 32 && c < 127)
            snprintf(line, sizeof(line), "%s%s(%c) w=%d\n", prefix, rightBr, c, huffmanTree[idx].weight);
        else
            snprintf(line, sizeof(line), "%s%s(0x%02X) w=%d\n", prefix, rightBr, c, huffmanTree[idx].weight);
        cPrint(C_GREEN, "%s", line);
    } else {
        snprintf(line, sizeof(line), "%s%s(#) w=%d\n", prefix, rightBr, huffmanTree[idx].weight);
        cPrint(C_CYAN, "%s", line);
    }
    if (fp) fputs(line, fp);
    if (huffmanTree[idx].lchild != 0)
        printSubTree(fp, huffmanTree[idx].lchild, depth+1, newPrefix, 1);
}

static void printTreeVisual(void) {
    FILE *fp = NULL;
    if (!treeBuilt) { printErr("请先初始化哈夫曼树（菜单 I）"); return; }
    fp = fopen("TreePrint", "w");
    if (!fp) printWarn("无法创建TreePrint文件，将只在屏幕显示");
    cPrint(C_BOLD C_MAGENTA, "\n  哈夫曼树结构可视化（左0 右1）\n\n");
    generateCodes();
    displayCodeTable(30);
    printf("\n");
    char prefix[512] = "";
    printSubTree(fp, treeSize, 0, prefix, 1);
    printf("\n");
    if (fp) {
        fprintf(fp, "\n编码表:\n字符\t编码\t码长\n");
        for (int i = 0; i < charCount; i++)
            fprintf(fp, "%c\t%s\t%d\n", codeTable[i].ch, codeTable[i].code, (int)strlen(codeTable[i].code));
        fclose(fp);
        printOK("树形结构已写入 TreePrint");
    }
    logMessage("TREE: 显示树形，%d叶子", charCount);
}

/* ======================== 编码文件打印 ======================== */

static void printCodeFile(const char *filename) {
    FILE *fin = fopen(filename, "rb");
    if (!fin) { printErr("文件不存在"); return; }
    long fsize = getFileSize(fin);
    fseek(fin, 0, SEEK_SET);
    unsigned char *buf = (unsigned char *)malloc((size_t)fsize + 1);
    if (!buf) { fclose(fin); printErr("内存不足"); return; }
    size_t n = fread(buf, 1, (size_t)fsize, fin);
    buf[n] = 0;
    fclose(fin);

    cPrint(C_BOLD C_CYAN, "\n  文件内容（二进制十六进制） %s:\n", filename);
    printDivider();
    for (size_t i = 0; i < n; i++) {
        if (i % 16 == 0) { if (i > 0) printf("\n"); cPrint(C_GRAY, "  %04zx  ", i); }
        printf("%02X ", buf[i]);
    }
    printf("\n");
    printDivider();
    cPrint(C_GRAY, "  共 %zu 字节\n\n", n);

    cPrint(C_BOLD C_CYAN, "  编码字符串（将bit数据展开为0/1显示）:\n");
    printDivider();
    if (n >= (long)sizeof(HufHeader)) {
        HufHeader hdr;
        memcpy(&hdr, buf, sizeof(hdr));
        if (memcmp(hdr.magic, HUFF_MAGIC, 4) == 0) {
            long dataStart = (long)sizeof(HufHeader) + (long)hdr.n * 5L;
            int count = 0;
            cPrint(C_GRAY, "  ");
            unsigned char *dptr = buf + dataStart;
            long dataLen = n - dataStart;
            long bitsShown = 0;
            for (long i = 0; i < dataLen && bitsShown < (long)hdr.totalBits; i++) {
                for (int b = 7; b >= 0 && bitsShown < (long)hdr.totalBits; b--) {
                    int bit = (dptr[i] >> b) & 1;
                    printf("%d", bit);
                    count++; bitsShown++;
                    if (count == 70) { printf("\n  "); count = 0; }
                }
            }
            printf("\n");
            printDivider();
            cPrint(C_GRAY, "  有效编码长度: %u bits\n", hdr.totalBits);
        }
    }
    free(buf);
}

/* ======================== 核心：压缩（到.huf文件） ======================== */

static HufStatus compressFile(const char *inFile, const char *outFile) {
    if (fileExists(outFile)) {
        printf("\n  文件 "); cPrint(C_YELLOW, "%s", outFile); printf(" 已存在。\n");
        char buf[16];
        while (1) {
            printf("  是否覆盖？(Y/N): "); fflush(stdout);
            if (!fgets(buf, sizeof(buf), stdin)) return HUF_ERR_CANCEL;
            char c = (char)toupper((unsigned char)buf[0]);
            if (c == 'N') return HUF_ERR_CANCEL;
            if (c == 'Y') break;
            cPrint(C_YELLOW, "  [!] 请输入Y或N\n");
        }
    }
    FILE *fin = fopen(inFile, "rb");
    if (!fin) { printErr("无法打开输入文件"); logMessage("ERROR: 打开%s失败", inFile); return HUF_ERR_OPEN_FILE; }
    long fsize = getFileSize(fin);
    fseek(fin, 0, SEEK_SET);
    unsigned char *data = NULL;
    if (fsize > 0) {
        data = (unsigned char *)malloc((size_t)fsize + 1);
        if (!data) { fclose(fin); printErr("内存不足"); return HUF_ERR_MEMORY; }
        size_t rd = fread(data, 1, (size_t)fsize, fin);
        data[rd] = 0;
        fclose(fin);
        if ((long)rd != fsize) { printErr("读取文件不完整"); free(data); return HUF_ERR_READ_FILE; }
    } else {
        fclose(fin);
        data = (unsigned char *)malloc(1);
        if (!data) return HUF_ERR_MEMORY;
        data[0] = 0;
    }
    HufStatus s = initFromFile(inFile);
    if (s != HUF_OK) { free(data); return s; }
    generateCodes();
    FILE *fout = fopen(outFile, "wb");
    if (!fout) { free(data); printErr("无法创建输出文件"); return HUF_ERR_WRITE_FILE; }
    HufHeader hdr;
    memcpy(hdr.magic, HUFF_MAGIC, 4);
    hdr.version  = HUFF_VERSION;
    hdr.n        = (unsigned short)charCount;
    hdr.origLen  = (unsigned int)fsize;
    hdr.totalBits = 0;
    long headerPos = (long)ftell(fout);
    fwrite(&hdr, sizeof(hdr), 1, fout);
    for (int i = 1; i <= charCount; i++) {
        unsigned char c = huffmanTree[i].ch;
        fwrite(&c, 1, 1, fout);
        int w = huffmanTree[i].weight;
        fwrite(&w, 4, 1, fout);
    }
    BitStream bs;
    bs_initWrite(&bs, fout);
    if (charCount == 1) {
        for (long i = 0; i < fsize; i++) bs_writeBit(&bs, 0);
    } else {
        for (long i = 0; i < fsize; i++) {
            unsigned char c = data[i];
            const char *code = codeMap[c];
            if (code) bs_writeBits(&bs, code);
            else { bs_flush(&bs); fclose(fout); free(data); printErr("编码异常：字符无编码"); logMessage("ERROR: 编码异常"); return HUF_ERR_CORRUPT; }
            if (fsize > 100000 && i % 50000 == 0) printProgress(i, fsize);
        }
        if (fsize > 100000) printProgress(fsize, fsize);
    }
    bs_flush(&bs);
    long dataEnd = ftell(fout);
    hdr.totalBits = (unsigned int)bs.totalBits;
    fseek(fout, headerPos, SEEK_SET);
    fwrite(&hdr, sizeof(hdr), 1, fout);
    fseek(fout, dataEnd, SEEK_SET);
    fclose(fout);
    long compSize = dataEnd;
    free(data);
    cPrint(C_GREEN, "  [OK] 压缩完成: %s -> %s\n", inFile, outFile);
    printCompressionStats(fsize, compSize, bs.totalBits);
    HufStatus vs = verifyCompression(inFile, outFile);
    if (vs == HUF_OK) {
        cPrint(C_BOLD C_GREEN, "  [VERIFY] 校验通过，压缩无损！\n");
        logMessage("COMPRESS: %s->%s, %ld->%ld bytes, verify OK", inFile, outFile, fsize, compSize);
    } else {
        cPrint(C_BOLD C_RED, "  [VERIFY] 校验失败！\n");
        logMessage("ERROR: 校验失败%s", outFile);
    }
    printThickDivider();
    return HUF_OK;
}

/* ======================== 核心：解压（从.huf文件） ======================== */

static HufStatus decompressFile(const char *inFile, const char *outFile) {
    FILE *fin = fopen(inFile, "rb");
    if (!fin) { printErr("无法打开压缩文件"); return HUF_ERR_OPEN_FILE; }
    if (fileExists(outFile)) {
        fclose(fin);
        printf("  文件 "); cPrint(C_YELLOW, "%s", outFile); printf(" 已存在。\n");
        char buf[16];
        while (1) {
            printf("  是否覆盖？(Y/N): "); fflush(stdout);
            if (!fgets(buf, sizeof(buf), stdin)) return HUF_ERR_CANCEL;
            char c = (char)toupper((unsigned char)buf[0]);
            if (c == 'N') return HUF_ERR_CANCEL;
            if (c == 'Y') { fin = fopen(inFile, "rb"); break; }
            cPrint(C_YELLOW, "  [!] 请输入Y或N\n");
        }
        if (!fin) { printErr("无法打开压缩文件"); return HUF_ERR_OPEN_FILE; }
    }
    HufHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fin) != 1) { fclose(fin); printErr("读取文件头失败"); return HUF_ERR_CORRUPT; }
    if (memcmp(hdr.magic, HUFF_MAGIC, 4) != 0) { fclose(fin); printErr("不是有效的huf压缩文件（魔数错误）"); return HUF_ERR_MAGIC; }
    if (hdr.version > HUFF_VERSION) { fclose(fin); printErr("不支持的huf文件版本"); return HUF_ERR_VERSION; }
    int n = hdr.n;
    if (n < 1 || n > 256) { fclose(fin); printErr("文件头损坏：字符数异常"); return HUF_ERR_CORRUPT; }
    if (!allocHuffmanState(n)) { fclose(fin); printErr("内存不足"); return HUF_ERR_MEMORY; }
    for (int i = 1; i <= n; i++) {
        unsigned char c; int w;
        if (fread(&c, 1, 1, fin) != 1) { freeHuffmanState(); fclose(fin); printErr("读取权重表失败"); return HUF_ERR_CORRUPT; }
        if (fread(&w, 4, 1, fin) != 1) { freeHuffmanState(); fclose(fin); printErr("读取权重表失败"); return HUF_ERR_CORRUPT; }
        huffmanTree[i].ch = c;
        huffmanTree[i].weight = w;
    }
    buildHuffmanTree();
    generateCodes();
    FILE *fout = fopen(outFile, "wb");
    if (!fout) { freeHuffmanState(); fclose(fin); printErr("无法创建输出文件"); return HUF_ERR_WRITE_FILE; }
    BitStream bs;
    bs_initRead(&bs, fin);
    int root = treeSize;
    int cur = root;
    unsigned char *outBuf = NULL;
    if (hdr.origLen > 0) {
        outBuf = (unsigned char *)malloc((size_t)hdr.origLen + 1);
        if (!outBuf) { fclose(fin); fclose(fout); freeHuffmanState(); printErr("内存不足"); return HUF_ERR_MEMORY; }
    }
    long outLen = 0;
    long bitsToRead = (long)hdr.totalBits;
    if (n == 1) {
        for (unsigned int i = 0; i < hdr.origLen; i++) {
            int bit = bs_readBit(&bs); if (bit < 0) break;
            if (outBuf) outBuf[outLen] = huffmanTree[1].ch;
            outLen++;
            if (hdr.origLen > 100000 && i % 50000 == 0) printProgress((long)i, (long)hdr.origLen);
        }
    } else {
        for (long i = 0; i < bitsToRead; i++) {
            int bit = bs_readBit(&bs);
            if (bit < 0) break;
            cur = bit ? huffmanTree[cur].rchild : huffmanTree[cur].lchild;
            if (cur == 0) { cPrint(C_RED, "\n  [ERROR] 解码错误：无效编码路径\n"); break; }
            if (huffmanTree[cur].lchild == 0 && huffmanTree[cur].rchild == 0) {
                if (outBuf && outLen < (long)hdr.origLen) outBuf[outLen] = huffmanTree[cur].ch;
                outLen++;
                cur = root;
                if (hdr.origLen > 100000 && outLen % 50000 == 0) printProgress(outLen, (long)hdr.origLen);
            }
        }
    }
    if (hdr.origLen > 100000) printProgress((long)hdr.origLen, (long)hdr.origLen);
    if (outBuf && outLen > 0) fwrite(outBuf, 1, (size_t)outLen, fout);
    fclose(fout);
    fclose(fin);
    if (outBuf) free(outBuf);
    cPrint(C_GREEN, "  [OK] 解压完成: %s -> %s\n", inFile, outFile);
    cPrint(C_CYAN, "  解压得到 %ld 字节\n", outLen);
    logMessage("DECOMPRESS: %s->%s, %u->%ld bytes", inFile, outFile, hdr.origLen, outLen);
    freeHuffmanState();
    return HUF_OK;
}

/* ======================== 校验：压缩后立即解压对比 ======================== */

HufStatus verifyCompression(const char *origFile, const char *hufFile) {
    FILE *fo = fopen(origFile, "rb");
    if (!fo) return HUF_ERR_OPEN_FILE;
    long osize = getFileSize(fo);
    unsigned char *obuf = (unsigned char *)malloc((size_t)(osize + 1));
    if (!obuf) { fclose(fo); return HUF_ERR_MEMORY; }
    fseek(fo, 0, SEEK_SET);
    size_t on = fread(obuf, 1, (size_t)osize, fo);
    fclose(fo);
    FILE *fh = fopen(hufFile, "rb");
    if (!fh) { free(obuf); return HUF_ERR_OPEN_FILE; }
    HufHeader hdr;
    fread(&hdr, sizeof(hdr), 1, fh);
    int n = hdr.n;
    int needNodes = 2 * n + 2;
    HTNode *savedTree = (HTNode *)calloc((size_t)needNodes, sizeof(HTNode));
    HCItem *savedTab = (HCItem *)calloc(256, sizeof(HCItem));
    const char **savedMap = (const char **)calloc(256, sizeof(const char*));
    if (!savedTree || !savedTab || !savedMap) {
        free(savedTree); free(savedTab); free(savedMap); free(obuf); fclose(fh);
        return HUF_ERR_MEMORY;
    }
    for (int i = 1; i <= n; i++) {
        unsigned char c; int w;
        fread(&c,1,1,fh); fread(&w,4,1,fh);
        savedTree[i].ch = c; savedTree[i].weight = w;
    }
    HTNode *oldTree = huffmanTree; HCItem *oldTab = codeTable; const char **oldMap = codeMap;
    int oldN = charCount, oldTs = treeSize, oldBuilt = treeBuilt;
    huffmanTree = savedTree; codeTable = savedTab; codeMap = savedMap;
    charCount = n; treeSize = 2*n-1; treeBuilt = 1;
    for (int i = 1; i <= treeSize; i++) { savedTree[i].parent=savedTree[i].lchild=savedTree[i].rchild=0; }
    if (n > 1) {
        for (int i = n+1; i<=treeSize; i++) {
            int m1 = findMinWeight(i-1, 0);
            int m2 = findMinWeight(i-1, m1);
            savedTree[m1].parent = i; savedTree[m2].parent = i;
            savedTree[i].lchild = m1; savedTree[i].rchild = m2;
            savedTree[i].weight = savedTree[m1].weight + savedTree[m2].weight;
        }
    }
    generateCodes();
    BitStream bs;
    bs_initRead(&bs, fh);
    int root = treeSize, cur = root;
    HufStatus ret = HUF_OK;
    long oi = 0;
    if (n == 1) {
        for (unsigned int i = 0; i < hdr.origLen; i++) {
            bs_readBit(&bs);
            if (oi < (long)on && obuf[oi] != savedTree[1].ch) { ret = HUF_ERR_VERIFY; break; }
            oi++;
        }
    } else {
        for (long i = 0; i < (long)hdr.totalBits; i++) {
            int bit = bs_readBit(&bs); if (bit<0) break;
            cur = bit ? savedTree[cur].rchild : savedTree[cur].lchild;
            if (cur == 0) { ret = HUF_ERR_CORRUPT; break; }
            if (savedTree[cur].lchild == 0 && savedTree[cur].rchild == 0) {
                if (oi < (long)on && obuf[oi] != savedTree[cur].ch) {
                    cPrint(C_RED, "\n  [VERIFY] 第%ld字节不一致: 期望0x%02X，实际0x%02X\n", oi, obuf[oi], savedTree[cur].ch);
                    ret = HUF_ERR_VERIFY; break;
                }
                oi++; cur = root;
            }
        }
    }
    if (ret == HUF_OK && oi != (long)on) { cPrint(C_RED, "\n  [VERIFY] 长度不一致: 期望%ld，实际%ld\n", on, oi); ret = HUF_ERR_VERIFY; }
    fclose(fh);
    free(savedTree); free(savedTab); free(savedMap);
    huffmanTree = oldTree; codeTable = oldTab; codeMap = oldMap;
    charCount = oldN; treeSize = oldTs; treeBuilt = oldBuilt;
    free(obuf);
    return ret;
}

/* ======================== 压缩包信息查看 ======================== */

static HufStatus showArchiveInfo(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) { printErr("无法打开文件"); return HUF_ERR_OPEN_FILE; }
    HufHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) { fclose(fp); printErr("读取文件头失败"); return HUF_ERR_CORRUPT; }
    if (memcmp(hdr.magic, HUFF_MAGIC, 4) != 0) { fclose(fp); printErr("不是有效的huf压缩文件"); return HUF_ERR_MAGIC; }
    long fsize = getFileSize(fp);
    fclose(fp);
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  压缩包信息: %s\n", filename);
    printDivider();
    printf("  魔数标识:     "); cPrint(C_GREEN, "%c%c%c%c\n", hdr.magic[0],hdr.magic[1],hdr.magic[2],hdr.magic[3]);
    printf("  格式版本:     %u\n", hdr.version);
    printf("  字符种类数:   %u\n", hdr.n);
    printf("  原始大小:     %u 字节 (%.2f KB)\n", hdr.origLen, hdr.origLen/1024.0);
    printf("  编码bit数:    %u\n", hdr.totalBits);
    printf("  压缩包大小:   %ld 字节 (%.2f KB)\n", fsize, fsize/1024.0);
    double ratio = hdr.origLen>0 ? (double)fsize/hdr.origLen*100.0 : 0;
    printf("  压缩率:       "); cPrint(ratio<100?C_GREEN:C_RED, "%.2f%%\n", ratio);
    printf("  节省空间:     "); cPrint(ratio<100?C_GREEN:C_RED, "%.2f%%\n", 100-ratio);
    double avgLen = hdr.origLen>0 ? (double)hdr.totalBits/hdr.origLen : 0;
    printf("  平均码长:     %.3f bit/字节\n", avgLen);
    printThickDivider();
    logMessage("INFO: 查看压缩包%s", filename);
    return HUF_OK;
}

/* ======================== 登录系统 ======================== */

static int doLogin(void) {
    char user[128], pass[128];
    int attempts = 3;
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  用户登录\n");
    printThickDivider();
    while (attempts-- > 0) {
        readString("用户名: ", user, sizeof(user));
        printf("  密  码: "); fflush(stdout);
        readPassword(pass, sizeof(pass));
        if (strcmp(user, LOGIN_USER) == 0 && strcmp(pass, LOGIN_PASS) == 0) {
            cPrint(C_GREEN, "\n  [OK] 登录成功，欢迎使用哈夫曼编码/译码系统！\n\n");
            logMessage("LOGIN: 用户%s登录成功", user);
            return 1;
        }
        cPrint(C_RED, "  [ERROR] 用户名或密码错误，剩余 %d 次机会\n\n", attempts);
    }
    cPrint(C_RED, "  [ERROR] 连续3次登录失败，程序退出\n");
    logMessage("LOGIN: 3次失败退出");
    return 0;
}

/* ======================== 编码功能（交互式，使用当前树） ======================== */

static void doEncodeInteractive(void) {
    if (!treeBuilt) { printErr("请先初始化哈夫曼树（菜单 I）"); return; }
    char inFile[256] = "ToBeTran", outFile[256] = "CodeFile.huf";
    printf("\n");
    readString("源文件路径（回车使用默认ToBeTran）: ", inFile, sizeof(inFile));
    if (inFile[0] == '\0') strcpy(inFile, "ToBeTran");
    readString("输出文件路径（回车使用CodeFile.huf）: ", outFile, sizeof(outFile));
    if (outFile[0] == '\0') strcpy(outFile, "CodeFile.huf");
    if (!fileExists(inFile)) {
        char create = readYesNo("源文件不存在，是否创建默认测试文本文件？");
        if (create) {
            FILE *f = fopen(inFile, "wb");
            if (f) {
                fprintf(f, "THIS PROGRAM IS MY FAVORITE\nHELLO WORLD\nHUFFMAN CODING IS FUN");
                fclose(f);
                printOK("已创建测试文件");
            } else { printErr("创建文件失败"); return; }
        } else return;
    }
    compressFile(inFile, outFile);
}

static void doDecodeInteractive(void) {
    char inFile[256] = "CodeFile.huf", outFile[256] = "TextFile";
    printf("\n");
    readString("压缩文件路径（回车使用CodeFile.huf）: ", inFile, sizeof(inFile));
    if (inFile[0] == '\0') strcpy(inFile, "CodeFile.huf");
    readString("输出文件路径（回车使用TextFile）: ", outFile, sizeof(outFile));
    if (outFile[0] == '\0') strcpy(outFile, "TextFile");
    decompressFile(inFile, outFile);
}

static void doPrintInteractive(void) {
    char fname[256] = "CodeFile.huf";
    printf("\n");
    readString("文件路径（回车使用CodeFile.huf）: ", fname, sizeof(fname));
    if (fname[0] == '\0') strcpy(fname, "CodeFile.huf");
    printCodeFile(fname);
}

static void doInfoInteractive(void) {
    char fname[256];
    printf("\n");
    readString("压缩文件路径: ", fname, sizeof(fname));
    if (fname[0] == '\0') return;
    showArchiveInfo(fname);
}

/* ======================== 帮助菜单 ======================== */

static void showHelp(void) {
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  使用帮助\n");
    printThickDivider();
    printf("\n");
    cPrint(C_BOLD, "  工作流程:\n");
    printf("    1. 选择 I 初始化哈夫曼树（推荐从文件自动统计频率）\n");
    printf("    2. 选择 E 编码（压缩）文件，生成 .huf 压缩包\n");
    printf("    3. 选择 D 译码（解压）.huf 文件，还原原始文件\n");
    printf("    4. 选择 T 可视化哈夫曼树结构\n");
    printf("    5. 选择 P 查看文件二进制/编码内容\n");
    printf("\n");
    cPrint(C_BOLD, "  命令行模式:\n");
    cPrint(C_CYAN, "    huffman.exe -c <输入> <输出.huf>   "); printf("压缩\n");
    cPrint(C_CYAN, "    huffman.exe -d <输入.huf> <输出>   "); printf("解压\n");
    cPrint(C_CYAN, "    huffman.exe -t <文件.huf>         "); printf("查看信息\n");
    cPrint(C_CYAN, "    （Windows下可直接拖放文件到huffman.exe）\n");
    printf("\n");
    cPrint(C_BOLD, "  默认账号: "); printf("admin / 123456\n");
    printf("\n");
    cPrint(C_BOLD, "  输出文件:\n");
    cPrint(C_CYAN, "    hfmTree       "); printf("哈夫曼树持久化文件\n");
    cPrint(C_CYAN, "    *.huf         "); printf("压缩包文件（含文件头+权重表+bit编码）\n");
    cPrint(C_CYAN, "    TreePrint     "); printf("树形结构文本\n");
    cPrint(C_CYAN, "    huffman.log   "); printf("操作日志\n");
    printf("\n");
    logMessage("HELP: 查看帮助");
}

/* ======================== 菜单系统 ======================== */

static void showMenu(void) {
    printDivider();
    cPrint(C_BOLD C_BLUE, "  主菜单\n");
    printDivider();
    cPrint(C_CYAN, "  I "); printf("- 初始化哈夫曼树\n");
    cPrint(C_CYAN, "  E "); printf("- 编码（压缩）文件\n");
    cPrint(C_CYAN, "  D "); printf("- 译码（解压）文件\n");
    cPrint(C_CYAN, "  P "); printf("- 打印/查看文件内容\n");
    cPrint(C_CYAN, "  T "); printf("- 打印哈夫曼树结构\n");
    cPrint(C_CYAN, "  F "); printf("- 查看压缩包信息\n");
    cPrint(C_CYAN, "  H/?"); printf("- 帮助信息\n");
    cPrint(C_CYAN, "  Q "); printf("- 退出系统\n");
    printDivider();
    printf("  当前状态: 字符集 ");
    if (treeBuilt) { cPrint(C_GREEN, "%d 种字节", charCount); printf(" | 树已构建 "); cPrint(C_GREEN, "OK"); }
    else         { cPrint(C_YELLOW, "0"); printf(" | 树未构建 "); cPrint(C_YELLOW, "(!)"); }
    printf("\n");
    printDivider();
}

static void handleInit(void) {
    printf("\n");
    printThickDivider();
    cPrint(C_BOLD C_CYAN, "  初始化哈夫曼树\n");
    printDivider();
    printf("  1 - 使用默认英文频率表（26个大写字母，英文标准频率）\n");
    printf("  2 - 自定义字符和权重\n");
    printf("  3 - 从文件自动统计字符频率（推荐，压缩率最优）\n");
    printDivider();
    int ch = readInt("请选择 (1-3): ", 1, 3);
    if (ch == 1) initDefaultTree();
    else if (ch == 2) initCustomTree();
    else {
        char fname[256] = "ToBeTran";
        readString("文件路径（回车使用ToBeTran）: ", fname, sizeof(fname));
        if (fname[0] == '\0') strcpy(fname, "ToBeTran");
        if (!fileExists(fname)) { printErr("文件不存在"); return; }
        initFromFile(fname);
    }
    generateCodes();
}

static void runMenu(void) {
    char choice[64];
    while (1) {
        showMenu();
        printf("  请选择操作: "); fflush(stdout);
        if (fgets(choice, sizeof(choice), stdin) == NULL) break;
        char *nl = strchr(choice, '\n'); if (nl) *nl = '\0';
        if (choice[0] == '\0') continue;
        char c = (char)toupper((unsigned char)choice[0]);
        switch (c) {
        case 'I': handleInit();         break;
        case 'E': doEncodeInteractive(); break;
        case 'D': doDecodeInteractive(); break;
        case 'P': doPrintInteractive();  break;
        case 'T': printTreeVisual();    break;
        case 'F': doInfoInteractive();   break;
        case 'H':
        case '?': showHelp();           break;
        case 'Q':
            cPrint(C_GREEN, "\n  [OK] 感谢使用，再见！\n\n");
            logMessage("EXIT: 用户退出");
            return;
        default:
            cPrint(C_YELLOW, "  [!] 无效选项，请重新输入\n"); break;
        }
    }
}

/* ======================== 命令行处理 ======================== */

static void printUsage(const char *prog) {
    printf("Huffman Encoding/Decoding System - Enhanced Edition\n\n");
    printf("用法:\n");
    printf("  %s                     交互模式（菜单）\n", prog);
    printf("  %s -c <输入> <输出.huf>  压缩文件\n", prog);
    printf("  %s -d <输入.huf> <输出>  解压文件\n", prog);
    printf("  %s -t <文件.huf>        查看压缩包信息\n", prog);
    printf("  %s -h                  显示帮助\n", prog);
    printf("\n（Windows下直接拖放文件到%s可自动压缩/解压）\n", prog);
}

static int runCommandLine(int argc, char *argv[]) {
    if (argc < 2) return 0;
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        printUsage(argv[0]);
        return 1;
    }
    colorEnabled = 1;
    enableVTMode();
    if (strcmp(argv[1], "-c") == 0) {
        if (argc < 4) { printErr("用法: -c <输入文件> <输出.huf>"); return 1; }
        logMessage("CMD: 压缩 %s -> %s", argv[2], argv[3]);
        HufStatus s = compressFile(argv[2], argv[3]);
        return (s == HUF_OK) ? 0 : 1;
    }
    if (strcmp(argv[1], "-d") == 0) {
        if (argc < 4) { printErr("用法: -d <输入.huf> <输出文件>"); return 1; }
        logMessage("CMD: 解压 %s -> %s", argv[2], argv[3]);
        HufStatus s = decompressFile(argv[2], argv[3]);
        return (s == HUF_OK) ? 0 : 1;
    }
    if (strcmp(argv[1], "-t") == 0) {
        if (argc < 3) { printErr("用法: -t <文件.huf>"); return 1; }
        showArchiveInfo(argv[2]);
        return 0;
    }
    if (argc == 2) {
        const char *f = argv[1];
        size_t fl = strlen(f);
        char out[512];
        if (fl > 4 && strcmp(f+fl-4, ".huf") == 0) {
            strncpy(out, f, fl-4); out[fl-4] = '\0';
            strcat(out, ".out");
            cPrint(C_CYAN, "  [INFO] 拖放模式：解压 %s -> %s\n", f, out);
            logMessage("DRAG: 解压 %s -> %s", f, out);
            return decompressFile(f, out) == HUF_OK ? 0 : 1;
        } else {
            snprintf(out, sizeof(out), "%s.huf", f);
            cPrint(C_CYAN, "  [INFO] 拖放模式：压缩 %s -> %s\n", f, out);
            logMessage("DRAG: 压缩 %s -> %s", f, out);
            return compressFile(f, out) == HUF_OK ? 0 : 1;
        }
    }
    printUsage(argv[0]);
    return 1;
}

/* ======================== 入口 ======================== */

int main(int argc, char *argv[]) {
    enableVTMode();
    logMessage("PROGRAM START");
    if (argc > 1) {
        int ret = runCommandLine(argc, argv);
        if (huffmanTree) freeHuffmanState();
        return ret;
    }
    printBanner();
    if (!doLogin()) {
        if (huffmanTree) freeHuffmanState();
        return 1;
    }
    loadTreeFromFile();
    runMenu();
    if (huffmanTree) freeHuffmanState();
    return 0;
}
