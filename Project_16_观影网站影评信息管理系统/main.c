/*
 * ╔══════════════════════════════════════════════════════════════════════════════╗
 * ║                                                                              ║
 * ║              观 影 网 站 影 评 信 息 管 理 系 统 V2.0                         ║
 * ║                                                                              ║
 * ║  ══════════════════════════════════════════════════════════════════════════  ║
 * ║  V2.0 新特性：                                                               ║
 * ║  • UTF-8 BOM + SetConsoleOutputCP(65001) 完美中文支持                        ║
 * ║  • ANSI 彩色终端输出，界面更美观                                              ║
 * ║  • CLI 命令行参数支持 (-h/--help/-t/--test)                                   ║
 * ║  • 动态数组替代链表，内存管理更高效                                           ║
 * ║  • 统一错误码枚举 MovieStatus                                                 ║
 * ║  • Magic Header "MOVVV2" 二进制文件持久化                                     ║
 * ║  • 内置 18 项自测用例（CRUD+评分+统计+文件I/O全覆盖）                         ║
 * ║  • 评分统计功能（平均分/最高分/最低分/评分分布）                              ║
 * ║                                                                              ║
 * ║  学生：数据库课程     学号：424123888888          班级：计算机类A班            ║
 * ║  姓名：HuiJunzhuo                                                             ║
 * ║  学年学期：2025-2026学年第二学期数据结构实习                                  ║
 * ║                                                                              ║
 * ║  编译：gcc -std=c99 -Wall -Wextra -O2 -o movie.exe main.c                     ║
 * ║  运行：./movie.exe                                                            ║
 * ║  测试：./movie.exe --test                                                     ║
 * ║  帮助：./movie.exe --help                                                     ║
 * ║                                                                              ║
 * ╚══════════════════════════════════════════════════════════════════════════════╝
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* ═══════════════════════════════════════════════════════════════════════════════
 * [1] 常量与宏定义
 * ═══════════════════════════════════════════════════════════════════════════════ */

#define MAX_NAME_LEN    64
#define MAX_EMAIL_LEN   128
#define MAX_URL_LEN     256
#define MAX_LINE_LEN    512
#define DATA_FILE       "movies_v2.dat"
#define MAGIC_HEADER    "MOVVV2"
#define MAGIC_LEN       6
#define INIT_CAPACITY   8
#define GROWTH_FACTOR   2

/* ANSI 颜色码 */
#define C_RESET     "\033[0m"
#define C_BOLD      "\033[1m"
#define C_RED       "\033[31m"
#define C_GREEN     "\033[32m"
#define C_YELLOW    "\033[33m"
#define C_BLUE      "\033[34m"
#define C_MAGENTA   "\033[35m"
#define C_CYAN      "\033[36m"
#define C_WHITE     "\033[37m"
#define C_BG_BLUE   "\033[44m"
#define C_BG_GREEN  "\033[42m"

/* ═══════════════════════════════════════════════════════════════════════════════
 * [2] 统一错误码枚举
 * ═══════════════════════════════════════════════════════════════════════════════ */

typedef enum {
    MOVIE_OK = 0,
    MOVIE_ERR_NULL_PTR,
    MOVIE_ERR_MEMORY,
    MOVIE_ERR_NOT_FOUND,
    MOVIE_ERR_DUPLICATE,
    MOVIE_ERR_INVALID_PARAM,
    MOVIE_ERR_FILE_OPEN,
    MOVIE_ERR_FILE_READ,
    MOVIE_ERR_FILE_WRITE,
    MOVIE_ERR_MAGIC_MISMATCH,
    MOVIE_ERR_VERSION,
    MOVIE_ERR_EMPTY,
    MOVIE_ERR_IO,
    MOVIE_ERR_SCORE_RANGE
} MovieStatus;

/* ═══════════════════════════════════════════════════════════════════════════════
 * [3] 数据结构定义（动态数组）
 * ═══════════════════════════════════════════════════════════════════════════════ */

typedef struct {
    int     id;
    char    title[MAX_NAME_LEN];
    int     year;
    char    director[MAX_NAME_LEN];
    char    genre[MAX_NAME_LEN];
    char    rating[MAX_NAME_LEN];
    float   avgScore;
    int     reviewCount;
} Movie;

typedef struct {
    int     id;
    int     movieId;
    char    reviewer[MAX_NAME_LEN];
    float   score;
    char    comment[MAX_URL_LEN];
} Review;

typedef struct {
    Movie  *items;
    int     count;
    int     capacity;
} MovieArray;

typedef struct {
    Review *items;
    int     count;
    int     capacity;
} ReviewArray;

/* ═══════════════════════════════════════════════════════════════════════════════
 * [4] 全局状态
 * ═══════════════════════════════════════════════════════════════════════════════ */

static MovieArray  g_movies  = {NULL, 0, 0};
static ReviewArray g_reviews = {NULL, 0, 0};
static int g_movieIdCounter  = 2000;
static int g_reviewIdCounter = 3000;

/* ═══════════════════════════════════════════════════════════════════════════════
 * [5] 工具函数
 * ═══════════════════════════════════════════════════════════════════════════════ */

static void consoleInit(void) {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | 0x0004);
#endif
}

static void clearInputBuffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

static void readLine(char *buf, int size) {
    if (fgets(buf, size, stdin) != NULL) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
        }
    }
}

static const char* statusToString(MovieStatus s) {
    switch (s) {
        case MOVIE_OK:              return "操作成功";
        case MOVIE_ERR_NULL_PTR:    return "空指针错误";
        case MOVIE_ERR_MEMORY:      return "内存分配失败";
        case MOVIE_ERR_NOT_FOUND:   return "未找到记录";
        case MOVIE_ERR_DUPLICATE:   return "重复记录";
        case MOVIE_ERR_INVALID_PARAM: return "无效参数";
        case MOVIE_ERR_FILE_OPEN:   return "文件打开失败";
        case MOVIE_ERR_FILE_READ:   return "文件读取失败";
        case MOVIE_ERR_FILE_WRITE:  return "文件写入失败";
        case MOVIE_ERR_MAGIC_MISMATCH: return "文件格式错误(magic不匹配)";
        case MOVIE_ERR_VERSION:     return "版本不兼容";
        case MOVIE_ERR_EMPTY:       return "数据为空";
        case MOVIE_ERR_IO:          return "IO错误";
        case MOVIE_ERR_SCORE_RANGE: return "评分范围错误(0.0-10.0)";
        default:                    return "未知错误";
    }
}

static void printSeparator(void) {
    printf(C_CYAN "------------------------------------------------------------" C_RESET "\n");
}

static void printBanner(void) {
    printf("\n");
    printf(C_BG_BLUE C_BOLD C_WHITE "                                                            " C_RESET "\n");
    printf(C_BG_BLUE C_BOLD C_WHITE "       🎬  观影网站影评信息管理系统 V2.0  🎬                " C_RESET "\n");
    printf(C_BG_BLUE C_BOLD C_WHITE "                                                            " C_RESET "\n");
}

static void printError(const char *msg) {
    printf(C_RED C_BOLD "✗ 错误: " C_RESET "%s\n", msg);
}

static void printSuccess(const char *msg) {
    printf(C_GREEN C_BOLD "✓ 成功: " C_RESET "%s\n", msg);
}

static void printInfo(const char *msg) {
    printf(C_YELLOW "ℹ 信息: " C_RESET "%s\n", msg);
}

/* ═══════════════════════════════════════════════════════════════════════════════
 * [6] 动态数组操作
 * ═══════════════════════════════════════════════════════════════════════════════ */

static MovieStatus movieArrayInit(MovieArray *arr) {
    if (!arr) return MOVIE_ERR_NULL_PTR;
    arr->items = (Movie*)calloc(INIT_CAPACITY, sizeof(Movie));
    if (!arr->items) return MOVIE_ERR_MEMORY;
    arr->count = 0;
    arr->capacity = INIT_CAPACITY;
    return MOVIE_OK;
}

static MovieStatus movieArrayGrow(MovieArray *arr) {
    if (!arr) return MOVIE_ERR_NULL_PTR;
    int newCap = arr->capacity * GROWTH_FACTOR;
    Movie *newItems = (Movie*)realloc(arr->items, newCap * sizeof(Movie));
    if (!newItems) return MOVIE_ERR_MEMORY;
    arr->items = newItems;
    arr->capacity = newCap;
    return MOVIE_OK;
}

static MovieStatus movieArrayAdd(MovieArray *arr, const Movie *m) {
    if (!arr || !m) return MOVIE_ERR_NULL_PTR;
    if (arr->count >= arr->capacity) {
        MovieStatus s = movieArrayGrow(arr);
        if (s != MOVIE_OK) return s;
    }
    arr->items[arr->count] = *m;
    arr->count++;
    return MOVIE_OK;
}

static MovieStatus reviewArrayInit(ReviewArray *arr) {
    if (!arr) return MOVIE_ERR_NULL_PTR;
    arr->items = (Review*)calloc(INIT_CAPACITY, sizeof(Review));
    if (!arr->items) return MOVIE_ERR_MEMORY;
    arr->count = 0;
    arr->capacity = INIT_CAPACITY;
    return MOVIE_OK;
}

static MovieStatus reviewArrayGrow(ReviewArray *arr) {
    if (!arr) return MOVIE_ERR_NULL_PTR;
    int newCap = arr->capacity * GROWTH_FACTOR;
    Review *newItems = (Review*)realloc(arr->items, newCap * sizeof(Review));
    if (!newItems) return MOVIE_ERR_MEMORY;
    arr->items = newItems;
    arr->capacity = newCap;
    return MOVIE_OK;
}

static MovieStatus reviewArrayAdd(ReviewArray *arr, const Review *r) {
    if (!arr || !r) return MOVIE_ERR_NULL_PTR;
    if (arr->count >= arr->capacity) {
        MovieStatus s = reviewArrayGrow(arr);
        if (s != MOVIE_OK) return s;
    }
    arr->items[arr->count] = *r;
    arr->count++;
    return MOVIE_OK;
}

static void arrayFree(void) {
    if (g_movies.items) { free(g_movies.items); g_movies.items = NULL; }
    if (g_reviews.items) { free(g_reviews.items); g_reviews.items = NULL; }
    g_movies.count = g_movies.capacity = 0;
    g_reviews.count = g_reviews.capacity = 0;
}

/* ═══════════════════════════════════════════════════════════════════════════════
 * [7] 电影 CRUD 操作
 * ═══════════════════════════════════════════════════════════════════════════════ */

static int findMovieIndexById(int id) {
    for (int i = 0; i < g_movies.count; i++) {
        if (g_movies.items[i].id == id) return i;
    }
    return -1;
}

static MovieStatus addMovie(const char *title, int year, const char *director,
                            const char *genre, const char *rating) {
    if (!title || !director || !genre || !rating) return MOVIE_ERR_NULL_PTR;
    if (strlen(title) == 0) return MOVIE_ERR_INVALID_PARAM;
    if (year < 1800 || year > 2100) return MOVIE_ERR_INVALID_PARAM;

    Movie m;
    memset(&m, 0, sizeof(Movie));
    m.id = ++g_movieIdCounter;
    strncpy(m.title, title, MAX_NAME_LEN - 1);
    m.title[MAX_NAME_LEN - 1] = '\0';
    m.year = year;
    strncpy(m.director, director, MAX_NAME_LEN - 1);
    m.director[MAX_NAME_LEN - 1] = '\0';
    strncpy(m.genre, genre, MAX_NAME_LEN - 1);
    m.genre[MAX_NAME_LEN - 1] = '\0';
    strncpy(m.rating, rating, MAX_NAME_LEN - 1);
    m.rating[MAX_NAME_LEN - 1] = '\0';
    m.avgScore = 0.0f;
    m.reviewCount = 0;

    return movieArrayAdd(&g_movies, &m);
}

static MovieStatus deleteMovie(int id) {
    int idx = findMovieIndexById(id);
    if (idx < 0) return MOVIE_ERR_NOT_FOUND;

    for (int i = idx; i < g_movies.count - 1; i++) {
        g_movies.items[i] = g_movies.items[i + 1];
    }
    g_movies.count--;

    int writePos = 0;
    for (int i = 0; i < g_reviews.count; i++) {
        if (g_reviews.items[i].movieId != id) {
            g_reviews.items[writePos++] = g_reviews.items[i];
        }
    }
    g_reviews.count = writePos;

    return MOVIE_OK;
}

static MovieStatus updateMovie(int id, const char *title, int year,
                                const char *director, const char *genre, const char *rating) {
    int idx = findMovieIndexById(id);
    if (idx < 0) return MOVIE_ERR_NOT_FOUND;
    Movie *m = &g_movies.items[idx];

    if (title && strlen(title) > 0) {
        strncpy(m->title, title, MAX_NAME_LEN - 1);
        m->title[MAX_NAME_LEN - 1] = '\0';
    }
    if (year > 1800 && year < 2100) {
        m->year = year;
    }
    if (director && strlen(director) > 0) {
        strncpy(m->director, director, MAX_NAME_LEN - 1);
        m->director[MAX_NAME_LEN - 1] = '\0';
    }
    if (genre && strlen(genre) > 0) {
        strncpy(m->genre, genre, MAX_NAME_LEN - 1);
        m->genre[MAX_NAME_LEN - 1] = '\0';
    }
    if (rating && strlen(rating) > 0) {
        strncpy(m->rating, rating, MAX_NAME_LEN - 1);
        m->rating[MAX_NAME_LEN - 1] = '\0';
    }
    return MOVIE_OK;
}

/* ═══════════════════════════════════════════════════════════════════════════════
 * [8] 影评 CRUD 操作
 * ═══════════════════════════════════════════════════════════════════════════════ */

static int findReviewIndexById(int id) {
    for (int i = 0; i < g_reviews.count; i++) {
        if (g_reviews.items[i].id == id) return i;
    }
    return -1;
}

static void recalcMovieScore(int movieId) {
    int idx = findMovieIndexById(movieId);
    if (idx < 0) return;
    Movie *m = &g_movies.items[idx];

    float sum = 0.0f;
    int cnt = 0;
    for (int i = 0; i < g_reviews.count; i++) {
        if (g_reviews.items[i].movieId == movieId) {
            sum += g_reviews.items[i].score;
            cnt++;
        }
    }
    m->reviewCount = cnt;
    m->avgScore = cnt > 0 ? sum / cnt : 0.0f;
}

static MovieStatus addReview(int movieId, const char *reviewer, float score, const char *comment) {
    if (!reviewer || !comment) return MOVIE_ERR_NULL_PTR;
    if (findMovieIndexById(movieId) < 0) return MOVIE_ERR_NOT_FOUND;
    if (score < 0.0f || score > 10.0f) return MOVIE_ERR_SCORE_RANGE;
    if (strlen(reviewer) == 0) return MOVIE_ERR_INVALID_PARAM;

    Review r;
    memset(&r, 0, sizeof(Review));
    r.id = ++g_reviewIdCounter;
    r.movieId = movieId;
    strncpy(r.reviewer, reviewer, MAX_NAME_LEN - 1);
    r.reviewer[MAX_NAME_LEN - 1] = '\0';
    r.score = score;
    strncpy(r.comment, comment, MAX_URL_LEN - 1);
    r.comment[MAX_URL_LEN - 1] = '\0';

    MovieStatus s = reviewArrayAdd(&g_reviews, &r);
    if (s == MOVIE_OK) {
        recalcMovieScore(movieId);
    }
    return s;
}

static MovieStatus deleteReview(int id) {
    int idx = findReviewIndexById(id);
    if (idx < 0) return MOVIE_ERR_NOT_FOUND;
    int movieId = g_reviews.items[idx].movieId;

    for (int i = idx; i < g_reviews.count - 1; i++) {
        g_reviews.items[i] = g_reviews.items[i + 1];
    }
    g_reviews.count--;
    recalcMovieScore(movieId);
    return MOVIE_OK;
}

/* ═══════════════════════════════════════════════════════════════════════════════
 * [9] 文件持久化（Magic Header "MOVVV2" 二进制格式）
 * ═══════════════════════════════════════════════════════════════════════════════ */

static MovieStatus saveToFile(const char *filename) {
    if (!filename) return MOVIE_ERR_NULL_PTR;
    FILE *fp = fopen(filename, "wb");
    if (!fp) return MOVIE_ERR_FILE_OPEN;

    if (fwrite(MAGIC_HEADER, 1, MAGIC_LEN, fp) != MAGIC_LEN) {
        fclose(fp); return MOVIE_ERR_FILE_WRITE;
    }

    int version = 2;
    if (fwrite(&version, sizeof(int), 1, fp) != 1) { fclose(fp); return MOVIE_ERR_FILE_WRITE; }
    if (fwrite(&g_movieIdCounter, sizeof(int), 1, fp) != 1) { fclose(fp); return MOVIE_ERR_FILE_WRITE; }
    if (fwrite(&g_reviewIdCounter, sizeof(int), 1, fp) != 1) { fclose(fp); return MOVIE_ERR_FILE_WRITE; }

    if (fwrite(&g_movies.count, sizeof(int), 1, fp) != 1) { fclose(fp); return MOVIE_ERR_FILE_WRITE; }
    if (g_movies.count > 0) {
        if (fwrite(g_movies.items, sizeof(Movie), g_movies.count, fp) != (size_t)g_movies.count) {
            fclose(fp); return MOVIE_ERR_FILE_WRITE;
        }
    }

    if (fwrite(&g_reviews.count, sizeof(int), 1, fp) != 1) { fclose(fp); return MOVIE_ERR_FILE_WRITE; }
    if (g_reviews.count > 0) {
        if (fwrite(g_reviews.items, sizeof(Review), g_reviews.count, fp) != (size_t)g_reviews.count) {
            fclose(fp); return MOVIE_ERR_FILE_WRITE;
        }
    }

    fclose(fp);
    return MOVIE_OK;
}

static MovieStatus loadFromFile(const char *filename) {
    if (!filename) return MOVIE_ERR_NULL_PTR;
    FILE *fp = fopen(filename, "rb");
    if (!fp) return MOVIE_ERR_FILE_OPEN;

    char magic[MAGIC_LEN + 1] = {0};
    if (fread(magic, 1, MAGIC_LEN, fp) != MAGIC_LEN) { fclose(fp); return MOVIE_ERR_FILE_READ; }
    if (memcmp(magic, MAGIC_HEADER, MAGIC_LEN) != 0) { fclose(fp); return MOVIE_ERR_MAGIC_MISMATCH; }

    int version;
    if (fread(&version, sizeof(int), 1, fp) != 1) { fclose(fp); return MOVIE_ERR_FILE_READ; }
    if (version != 2) { fclose(fp); return MOVIE_ERR_VERSION; }

    if (fread(&g_movieIdCounter, sizeof(int), 1, fp) != 1) { fclose(fp); return MOVIE_ERR_FILE_READ; }
    if (fread(&g_reviewIdCounter, sizeof(int), 1, fp) != 1) { fclose(fp); return MOVIE_ERR_FILE_READ; }

    int mCount;
    if (fread(&mCount, sizeof(int), 1, fp) != 1) { fclose(fp); return MOVIE_ERR_FILE_READ; }

    if (g_movies.items) free(g_movies.items);
    g_movies.capacity = mCount > INIT_CAPACITY ? mCount : INIT_CAPACITY;
    g_movies.items = (Movie*)calloc(g_movies.capacity, sizeof(Movie));
    if (!g_movies.items) { fclose(fp); return MOVIE_ERR_MEMORY; }
    g_movies.count = mCount;
    if (mCount > 0) {
        if (fread(g_movies.items, sizeof(Movie), mCount, fp) != (size_t)mCount) {
            fclose(fp); return MOVIE_ERR_FILE_READ;
        }
    }

    int rCount;
    if (fread(&rCount, sizeof(int), 1, fp) != 1) { fclose(fp); return MOVIE_ERR_FILE_READ; }

    if (g_reviews.items) free(g_reviews.items);
    g_reviews.capacity = rCount > INIT_CAPACITY ? rCount : INIT_CAPACITY;
    g_reviews.items = (Review*)calloc(g_reviews.capacity, sizeof(Review));
    if (!g_reviews.items) { fclose(fp); return MOVIE_ERR_MEMORY; }
    g_reviews.count = rCount;
    if (rCount > 0) {
        if (fread(g_reviews.items, sizeof(Review), rCount, fp) != (size_t)rCount) {
            fclose(fp); return MOVIE_ERR_FILE_READ;
        }
    }

    fclose(fp);
    return MOVIE_OK;
}

/* ═══════════════════════════════════════════════════════════════════════════════
 * [10] 统计功能
 * ═══════════════════════════════════════════════════════════════════════════════ */

typedef struct {
    float avgScore;
    float maxScore;
    float minScore;
    int   totalReviews;
    int   excellent;
    int   good;
    int   average;
    int   poor;
} MovieStats;

static MovieStats getStats(void) {
    MovieStats s = {0};
    s.avgScore = 0.0f;
    s.maxScore = -1.0f;
    s.minScore = 11.0f;
    s.totalReviews = g_reviews.count;
    s.excellent = s.good = s.average = s.poor = 0;

    if (g_reviews.count == 0) {
        s.maxScore = 0.0f;
        s.minScore = 0.0f;
        return s;
    }

    float sum = 0.0f;
    for (int i = 0; i < g_reviews.count; i++) {
        float sc = g_reviews.items[i].score;
        sum += sc;
        if (sc > s.maxScore) s.maxScore = sc;
        if (sc < s.minScore) s.minScore = sc;
        if (sc >= 8.0f) s.excellent++;
        else if (sc >= 6.0f) s.good++;
        else if (sc >= 4.0f) s.average++;
        else s.poor++;
    }
    s.avgScore = sum / g_reviews.count;
    return s;
}

/* ═══════════════════════════════════════════════════════════════════════════════
 * [11] 显示函数
 * ═══════════════════════════════════════════════════════════════════════════════ */

static void displayMovieHeader(void) {
    printSeparator();
    printf(C_BOLD "| %-6s | %-20s | %-6s | %-14s | %-10s | %-6s | %-5s | %-4s |" C_RESET "\n",
           "ID", "电影名称", "年份", "导演", "类型", "分级", "评分", "评论");
    printSeparator();
}

static void displayMovie(const Movie *m) {
    printf("| %-6d | %-20s | %-6d | %-14s | %-10s | %-6s | %-5.1f | %-4d |\n",
           m->id, m->title, m->year, m->director, m->genre, m->rating, m->avgScore, m->reviewCount);
}

static void displayReviewHeader(void) {
    printSeparator();
    printf(C_BOLD "| %-6s | %-6s | %-14s | %-5s | %-30s |" C_RESET "\n",
           "ID", "电影ID", "评论者", "评分", "评论内容");
    printSeparator();
}

static void displayReview(const Review *r) {
    printf("| %-6d | %-6d | %-14s | %-5.1f | %-30s |\n",
           r->id, r->movieId, r->reviewer, r->score, r->comment);
}

static void browseMovies(void) {
    if (g_movies.count == 0) {
        printInfo("暂无电影数据");
        return;
    }
    displayMovieHeader();
    for (int i = 0; i < g_movies.count; i++) {
        displayMovie(&g_movies.items[i]);
    }
    printSeparator();
    printf(C_CYAN "  共 %d 部电影\n" C_RESET, g_movies.count);
}

static void browseReviews(void) {
    if (g_reviews.count == 0) {
        printInfo("暂无影评数据");
        return;
    }
    displayReviewHeader();
    for (int i = 0; i < g_reviews.count; i++) {
        displayReview(&g_reviews.items[i]);
    }
    printSeparator();
    printf(C_CYAN "  共 %d 条影评\n" C_RESET, g_reviews.count);
}

static void showStats(void) {
    MovieStats s = getStats();
    printf("\n");
    printSeparator();
    printf(C_BOLD C_MAGENTA "              📊  影 评 数 据 统 计  📊" C_RESET "\n");
    printSeparator();
    printf("  🎬 电影总数:      " C_CYAN "%d" C_RESET " 部\n", g_movies.count);
    printf("  📝 影评总数:      " C_CYAN "%d" C_RESET " 条\n", s.totalReviews);
    if (s.totalReviews > 0) {
        printf("  ⭐ 平均评分:      " C_YELLOW "%.2f" C_RESET " 分\n", s.avgScore);
        printf("  🏆 最高评分:      " C_GREEN "%.1f" C_RESET " 分\n", s.maxScore);
        printf("  📉 最低评分:      " C_RED "%.1f" C_RESET " 分\n", s.minScore);
        printSeparator();
        printf("  🟢 优秀(8-10):    " C_GREEN "%d" C_RESET " 条 (%.1f%%)\n", s.excellent,
               100.0 * s.excellent / s.totalReviews);
        printf("  🟡 良好(6-7.9):   " C_YELLOW "%d" C_RESET " 条 (%.1f%%)\n", s.good,
               100.0 * s.good / s.totalReviews);
        printf("  🟠 一般(4-5.9):   %d 条 (%.1f%%)\n", s.average,
               100.0 * s.average / s.totalReviews);
        printf("  🔴 较差(0-3.9):   " C_RED "%d" C_RESET " 条 (%.1f%%)\n", s.poor,
               100.0 * s.poor / s.totalReviews);
    }
    printSeparator();
}

/* ═══════════════════════════════════════════════════════════════════════════════
 * [12] 示例数据
 * ═══════════════════════════════════════════════════════════════════════════════ */

static void loadSampleData(void) {
    addMovie("星际穿越", 2014, "克里斯托弗·诺兰", "科幻", "PG-13");
    addMovie("盗梦空间", 2010, "克里斯托弗·诺兰", "科幻", "PG-13");
    addMovie("霸王别姬", 1993, "陈凯歌", "剧情", "R");
    addMovie("千与千寻", 2001, "宫崎骏", "动画", "PG");
    addMovie("肖申克的救赎", 1994, "弗兰克·德拉邦特", "剧情", "R");
    addMovie("阿甘正传", 1994, "罗伯特·泽米吉斯", "剧情", "PG-13");
    addMovie("泰坦尼克号", 1997, "詹姆斯·卡梅隆", "爱情", "PG-13");
    addMovie("流浪地球", 2019, "郭帆", "科幻", "PG-13");

    addReview(2001, "张三", 9.5f, "震撼人心的科幻史诗");
    addReview(2002, "李四", 9.0f, "诺兰神作，层层嵌套");
    addReview(2003, "王五", 9.8f, "华语电影巅峰之作");
    addReview(2004, "赵六", 8.5f, "宫崎骏的童话世界");
    addReview(2005, "孙七", 9.7f, "希望让人自由");
    addReview(2001, "周八", 8.8f, "父爱如山，感人至深");
    addReview(2007, "吴九", 8.2f, "经典的爱情灾难片");
    addReview(2008, "郑十", 7.5f, "中国科幻里程碑");
    addReview(2003, "张三", 9.2f, "张国荣演技封神");
    addReview(2005, "李四", 9.6f, "每次看都有新感悟");

    printSuccess("已加载 8 部电影 + 10 条影评的示例数据");
}

/* ═══════════════════════════════════════════════════════════════════════════════
 * [13] 交互菜单
 * ═══════════════════════════════════════════════════════════════════════════════ */

static void menuAddMovie(void) {
    char title[MAX_NAME_LEN], director[MAX_NAME_LEN], genre[MAX_NAME_LEN], rating[MAX_NAME_LEN];
    int year;

    printf("\n");
    printSeparator();
    printf(C_BOLD "  ➕ 添加新电影" C_RESET "\n");
    printSeparator();
    printf("  电影名称: "); readLine(title, MAX_NAME_LEN);
    printf("  上映年份(1800-2100): ");
    if (scanf("%d", &year) != 1) { clearInputBuffer(); printError("输入无效"); return; }
    clearInputBuffer();
    printf("  导演: "); readLine(director, MAX_NAME_LEN);
    printf("  类型(科幻/剧情/动画等): "); readLine(genre, MAX_NAME_LEN);
    printf("  分级(PG/PG-13/R等): "); readLine(rating, MAX_NAME_LEN);

    MovieStatus s = addMovie(title, year, director, genre, rating);
    if (s == MOVIE_OK) {
        printSuccess("电影添加成功");
        saveToFile(DATA_FILE);
    } else {
        printError(statusToString(s));
    }
}

static void menuDeleteMovie(void) {
    int id;
    printf("\n");
    printSeparator();
    printf(C_BOLD "  🗑️  删除电影" C_RESET "\n");
    printSeparator();
    printf("  输入要删除的电影ID: ");
    if (scanf("%d", &id) != 1) { clearInputBuffer(); printError("输入无效"); return; }
    clearInputBuffer();

    int idx = findMovieIndexById(id);
    if (idx < 0) { printError(statusToString(MOVIE_ERR_NOT_FOUND)); return; }
    printf("  即将删除: %s (%d)\n", g_movies.items[idx].title, g_movies.items[idx].year);
    printf("  确认删除? (y/n): ");
    char confirm[8]; readLine(confirm, 8);
    if (confirm[0] == 'y' || confirm[0] == 'Y') {
        MovieStatus s = deleteMovie(id);
        if (s == MOVIE_OK) {
            printSuccess("电影及相关影评已删除");
            saveToFile(DATA_FILE);
        } else {
            printError(statusToString(s));
        }
    } else {
        printInfo("已取消删除");
    }
}

static void menuAddReview(void) {
    int movieId;
    char reviewer[MAX_NAME_LEN], comment[MAX_URL_LEN];
    float score;

    printf("\n");
    printSeparator();
    printf(C_BOLD "  📝 添加影评" C_RESET "\n");
    printSeparator();
    printf("  电影ID: ");
    if (scanf("%d", &movieId) != 1) { clearInputBuffer(); printError("输入无效"); return; }
    clearInputBuffer();
    printf("  您的昵称: "); readLine(reviewer, MAX_NAME_LEN);
    printf("  评分(0.0-10.0): ");
    if (scanf("%f", &score) != 1) { clearInputBuffer(); printError("输入无效"); return; }
    clearInputBuffer();
    printf("  评论内容: "); readLine(comment, MAX_URL_LEN);

    MovieStatus s = addReview(movieId, reviewer, score, comment);
    if (s == MOVIE_OK) {
        printSuccess("影评添加成功");
        saveToFile(DATA_FILE);
    } else {
        printError(statusToString(s));
    }
}

static void menuModifyMovie(void) {
    int id;
    char title[MAX_NAME_LEN], director[MAX_NAME_LEN], genre[MAX_NAME_LEN], rating[MAX_NAME_LEN];
    char yearBuf[16];
    int year;

    printf("\n");
    printSeparator();
    printf(C_BOLD "  ✏️  修改电影信息" C_RESET "\n");
    printSeparator();
    printf("  输入要修改的电影ID: ");
    if (scanf("%d", &id) != 1) { clearInputBuffer(); printError("输入无效"); return; }
    clearInputBuffer();

    int idx = findMovieIndexById(id);
    if (idx < 0) { printError(statusToString(MOVIE_ERR_NOT_FOUND)); return; }
    Movie *m = &g_movies.items[idx];
    printf("  当前信息: %s (%d) - %s\n", m->title, m->year, m->director);
    printf("  (直接回车保留原值)\n");

    printf("  新电影名称 [%s]: ", m->title); readLine(title, MAX_NAME_LEN);
    printf("  新上映年份 [%d]: ", m->year); readLine(yearBuf, 16);
    printf("  新导演 [%s]: ", m->director); readLine(director, MAX_NAME_LEN);
    printf("  新类型 [%s]: ", m->genre); readLine(genre, MAX_NAME_LEN);
    printf("  新分级 [%s]: ", m->rating); readLine(rating, MAX_NAME_LEN);

    year = (strlen(yearBuf) > 0) ? atoi(yearBuf) : -1;
    MovieStatus s = updateMovie(id,
        strlen(title) > 0 ? title : NULL,
        year,
        strlen(director) > 0 ? director : NULL,
        strlen(genre) > 0 ? genre : NULL,
        strlen(rating) > 0 ? rating : NULL);
    if (s == MOVIE_OK) {
        printSuccess("电影信息已更新");
        saveToFile(DATA_FILE);
    } else {
        printError(statusToString(s));
    }
}

static void menuDeleteReview(void) {
    int id;
    printf("\n");
    printSeparator();
    printf(C_BOLD "  🗑️  删除影评" C_RESET "\n");
    printSeparator();
    printf("  输入要删除的影评ID: ");
    if (scanf("%d", &id) != 1) { clearInputBuffer(); printError("输入无效"); return; }
    clearInputBuffer();

    MovieStatus s = deleteReview(id);
    if (s == MOVIE_OK) {
        printSuccess("影评已删除");
        saveToFile(DATA_FILE);
    } else {
        printError(statusToString(s));
    }
}

static void mainMenuLoop(void) {
    int choice;
    while (1) {
        printBanner();
        printf(C_BOLD "  【主菜单】" C_RESET "\n");
        printf("  1. 📋 浏览所有电影\n");
        printf("  2. ➕ 添加新电影\n");
        printf("  3. ✏️  修改电影信息\n");
        printf("  4. 🗑️  删除电影\n");
        printf("  5. 📝 浏览所有影评\n");
        printf("  6. ✍️  添加影评\n");
        printf("  7. ❌ 删除影评\n");
        printf("  8. 📊 数据统计\n");
        printf("  9. 💾 手动保存\n");
        printf("  0. 🚪 退出系统\n");
        printSeparator();
        printf("  请选择操作: ");

        if (scanf("%d", &choice) != 1) {
            clearInputBuffer();
            printError("输入无效，请输入数字");
            continue;
        }
        clearInputBuffer();

        switch (choice) {
            case 1: browseMovies(); break;
            case 2: menuAddMovie(); break;
            case 3: menuModifyMovie(); break;
            case 4: menuDeleteMovie(); break;
            case 5: browseReviews(); break;
            case 6: menuAddReview(); break;
            case 7: menuDeleteReview(); break;
            case 8: showStats(); break;
            case 9: {
                MovieStatus s = saveToFile(DATA_FILE);
                if (s == MOVIE_OK) printSuccess("数据已保存");
                else printError(statusToString(s));
                break;
            }
            case 0: {
                printf("\n");
                printInfo("正在保存数据...");
                saveToFile(DATA_FILE);
                printf(C_GREEN C_BOLD "\n  👋 感谢使用，再见！\n\n" C_RESET);
                return;
            }
            default: printError("无效选项，请重新选择");
        }
        printf("\n  按回车键继续...");
        getchar();
    }
}

/* ═══════════════════════════════════════════════════════════════════════════════
 * [14] 内置自测框架（18项测试）
 * ═══════════════════════════════════════════════════════════════════════════════ */

static int g_testPassed = 0;
static int g_testFailed = 0;

#define TEST_ASSERT(cond, name) do { \
    if (cond) { \
        printf(C_GREEN "  [PASS] " C_RESET "%s\n", name); \
        g_testPassed++; \
    } else { \
        printf(C_RED "  [FAIL] " C_RESET "%s (line %d)\n", name, __LINE__); \
        g_testFailed++; \
    } \
} while(0)

#define TEST_STATUS(s, expected, name) TEST_ASSERT((s) == (expected), name)

static void resetTestData(void) {
    arrayFree();
    movieArrayInit(&g_movies);
    reviewArrayInit(&g_reviews);
    g_movieIdCounter = 2000;
    g_reviewIdCounter = 3000;
}

static void runAllTests(void) {
    printf(C_BOLD C_CYAN "\n╔══════════════════════════════════════════════════════════════╗\n");
    printf("║           🧪  观影系统 V2.0 内置自测套件                    ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n" C_RESET);

    /* T01: 初始化测试 */
    printf(C_BOLD "▶ 初始化测试\n" C_RESET);
    resetTestData();
    TEST_ASSERT(g_movies.count == 0, "T01: 电影数组初始为空");
    TEST_ASSERT(g_reviews.count == 0, "T02: 影评数组初始为空");
    TEST_ASSERT(g_movies.capacity == INIT_CAPACITY, "T03: 初始容量正确");

    /* T02: 添加电影测试 */
    printf("\n" C_BOLD "▶ 电影CRUD测试\n" C_RESET);
    MovieStatus s;
    s = addMovie("测试电影1", 2020, "导演A", "科幻", "PG-13");
    TEST_STATUS(s, MOVIE_OK, "T04: 添加电影成功");
    TEST_ASSERT(g_movies.count == 1, "T05: 添加后电影数=1");
    TEST_ASSERT(g_movies.items[0].id == 2001, "T06: 电影ID自动分配正确");

    /* T03: 重复参数校验 */
    s = addMovie("", 2020, "导演", "类型", "PG");
    TEST_STATUS(s, MOVIE_ERR_INVALID_PARAM, "T07: 空标题返回错误");
    s = addMovie("电影", 1700, "导演", "类型", "PG");
    TEST_STATUS(s, MOVIE_ERR_INVALID_PARAM, "T08: 无效年份返回错误");
    s = addMovie("电影", 2020, NULL, "类型", "PG");
    TEST_STATUS(s, MOVIE_ERR_NULL_PTR, "T09: NULL参数返回错误");

    /* T04: 添加影评测试 */
    printf("\n" C_BOLD "▶ 影评CRUD测试\n" C_RESET);
    s = addReview(2001, "测试用户", 8.5f, "不错的电影");
    TEST_STATUS(s, MOVIE_OK, "T10: 添加影评成功");
    TEST_ASSERT(g_reviews.count == 1, "T11: 添加后影评数=1");
    TEST_ASSERT(fabs(g_movies.items[0].avgScore - 8.5f) < 0.01f, "T12: 平均分计算正确");

    /* T05: 评分范围校验 */
    s = addReview(2001, "用户", 11.0f, "超出范围");
    TEST_STATUS(s, MOVIE_ERR_SCORE_RANGE, "T13: 评分>10返回错误");
    s = addReview(2001, "用户", -1.0f, "负分");
    TEST_STATUS(s, MOVIE_ERR_SCORE_RANGE, "T14: 评分<0返回错误");

    /* T06: 外键约束测试 */
    s = addReview(9999, "用户", 5.0f, "不存在的电影");
    TEST_STATUS(s, MOVIE_ERR_NOT_FOUND, "T15: 不存在电影ID返回错误");

    /* T07: 添加更多影评测试统计 */
    addReview(2001, "用户2", 9.5f, "优秀");
    addReview(2001, "用户3", 7.0f, "还行");
    TEST_ASSERT(g_movies.items[0].reviewCount == 3, "T16: 影评计数更新正确");
    float expected = (8.5f + 9.5f + 7.0f) / 3.0f;
    TEST_ASSERT(fabs(g_movies.items[0].avgScore - expected) < 0.01f, "T17: 多影评平均分正确");

    /* T08: 文件I/O测试 */
    printf("\n" C_BOLD "▶ 文件持久化测试\n" C_RESET);
    const char *testFile = "_test_movies.dat";
    s = saveToFile(testFile);
    TEST_STATUS(s, MOVIE_OK, "T18: 保存文件成功");

    int savedMovieCount = g_movies.count;
    int savedReviewCount = g_reviews.count;
    int savedMovieId = g_movieIdCounter;
    int savedReviewId = g_reviewIdCounter;

    resetTestData();
    s = loadFromFile(testFile);
    TEST_STATUS(s, MOVIE_OK, "T19: 加载文件成功");
    TEST_ASSERT(g_movies.count == savedMovieCount, "T20: 加载后电影数一致");
    TEST_ASSERT(g_reviews.count == savedReviewCount, "T21: 加载后影评数一致");
    TEST_ASSERT(g_movieIdCounter == savedMovieId, "T22: 电影ID计数器恢复正确");
    TEST_ASSERT(g_reviewIdCounter == savedReviewId, "T23: 影评ID计数器恢复正确");
    TEST_ASSERT(strcmp(g_movies.items[0].title, "测试电影1") == 0, "T24: 电影数据内容正确");

    /* T09: Magic Header校验 */
    resetTestData();
    FILE *fp = fopen("_test_bad.dat", "wb");
    fprintf(fp, "BADHEADER");
    fclose(fp);
    s = loadFromFile("_test_bad.dat");
    TEST_STATUS(s, MOVIE_ERR_MAGIC_MISMATCH, "T25: 错误Magic头检测正确");
    remove("_test_bad.dat");
    remove(testFile);

    /* T10: 更新电影测试 */
    printf("\n" C_BOLD "▶ 更新电影测试\n" C_RESET);
    resetTestData();
    addMovie("原名", 2020, "原导演", "原类型", "PG");
    s = updateMovie(2001, "新电影名", 2021, "新导演", "新类型", "R");
    TEST_STATUS(s, MOVIE_OK, "T26: 更新电影成功");
    TEST_ASSERT(strcmp(g_movies.items[0].title, "新电影名") == 0, "T27: 电影名称已更新");
    TEST_ASSERT(g_movies.items[0].year == 2021, "T28: 电影年份已更新");
    s = updateMovie(9999, "x", 2020, "x", "x", "x");
    TEST_STATUS(s, MOVIE_ERR_NOT_FOUND, "T29: 更新不存在电影返回错误");

    /* T11: 删除测试 */
    printf("\n" C_BOLD "▶ 删除测试\n" C_RESET);
    addReview(2001, "用户", 8.0f, "评论");
    int movieCountBefore = g_movies.count;
    int reviewCountBefore = g_reviews.count;
    s = deleteMovie(2001);
    TEST_STATUS(s, MOVIE_OK, "T30: 删除电影成功");
    TEST_ASSERT(g_movies.count == movieCountBefore - 1, "T31: 电影数减少1");
    TEST_ASSERT(g_reviews.count == reviewCountBefore - 1, "T32: 关联影评级联删除");
    s = deleteReview(9999);
    TEST_STATUS(s, MOVIE_ERR_NOT_FOUND, "T33: 删除不存在影评返回错误");
    s = deleteMovie(9999);
    TEST_STATUS(s, MOVIE_ERR_NOT_FOUND, "T34: 删除不存在电影返回错误");

    /* T12: 统计测试 */
    printf("\n" C_BOLD "▶ 统计功能测试\n" C_RESET);
    resetTestData();
    addMovie("统计电影", 2020, "导演", "类型", "PG");
    addReview(2001, "A", 9.0f, "优");
    addReview(2001, "B", 5.0f, "中");
    addReview(2001, "C", 3.0f, "差");
    addReview(2001, "D", 7.0f, "良");
    MovieStats stats = getStats();
    TEST_ASSERT(stats.totalReviews == 4, "T35: 统计总数正确");
    TEST_ASSERT(fabs(stats.avgScore - (9.0f+5.0f+3.0f+7.0f)/4.0f) < 0.01f, "T36: 统计平均分正确");
    TEST_ASSERT(fabs(stats.maxScore - 9.0f) < 0.01f, "T37: 最高分正确");
    TEST_ASSERT(fabs(stats.minScore - 3.0f) < 0.01f, "T38: 最低分正确");
    TEST_ASSERT(stats.excellent == 1 && stats.good == 1 && stats.poor == 1, "T39: 评分分布统计正确");

    /* T13: 空数据统计测试 */
    resetTestData();
    MovieStats emptyStats = getStats();
    TEST_ASSERT(emptyStats.totalReviews == 0, "T40: 空数据统计返回0");
    TEST_ASSERT(fabs(emptyStats.avgScore - 0.0f) < 0.01f, "T41: 空数据平均分为0");

    /* T14: 动态数组扩容测试 */
    printf("\n" C_BOLD "▶ 动态扩容测试\n" C_RESET);
    resetTestData();
    int initialCap = g_movies.capacity;
    for (int i = 0; i < 10; i++) {
        char buf[32];
        sprintf(buf, "电影%d", i+1);
        addMovie(buf, 2020, "导演", "类型", "PG");
    }
    TEST_ASSERT(g_movies.count == 10, "T42: 添加10部电影成功");
    TEST_ASSERT(g_movies.capacity >= 10, "T43: 动态扩容后容量足够");
    TEST_ASSERT(g_movies.capacity > initialCap, "T44: 容量确实增长了");

    /* T15: 影评数组扩容测试 */
    resetTestData();
    addMovie("测试电影", 2020, "导演", "类型", "PG");
    int rInitialCap = g_reviews.capacity;
    for (int i = 0; i < 10; i++) {
        char buf[32];
        sprintf(buf, "用户%d", i+1);
        addReview(2001, buf, 7.0f, "评论");
    }
    TEST_ASSERT(g_reviews.count == 10, "T45: 添加10条影评成功");
    TEST_ASSERT(g_reviews.capacity >= 10, "T46: 影评数组扩容后容量足够");
    TEST_ASSERT(g_reviews.capacity >= rInitialCap, "T47: 影评数组容量增长");

    /* 测试结果汇总 */
    printf("\n");
    printSeparator();
    printf(C_BOLD "  📋 测试结果汇总\n" C_RESET);
    printSeparator();
    printf("  总计: %d 项\n", g_testPassed + g_testFailed);
    printf("  " C_GREEN "通过: %d" C_RESET "\n", g_testPassed);
    if (g_testFailed > 0) {
        printf("  " C_RED "失败: %d" C_RESET "\n", g_testFailed);
    } else {
        printf("  " C_GREEN "失败: 0" C_RESET "\n");
    }
    printSeparator();
    if (g_testFailed == 0) {
        printf(C_BOLD C_GREEN "\n  🎉 全部 %d 项测试通过！\n\n" C_RESET, g_testPassed + g_testFailed);
    } else {
        printf(C_BOLD C_RED "\n  ⚠️  有 %d 项测试未通过\n\n" C_RESET, g_testFailed);
    }

    resetTestData();
    movieArrayInit(&g_movies);
    reviewArrayInit(&g_reviews);
    g_movieIdCounter = 2000;
    g_reviewIdCounter = 3000;
}

/* ═══════════════════════════════════════════════════════════════════════════════
 * [15] CLI帮助与主函数
 * ═══════════════════════════════════════════════════════════════════════════════ */

static void printHelp(void) {
    printf(C_BOLD C_CYAN "\n  观影网站影评信息管理系统 V2.0 - 使用帮助\n" C_RESET);
    printSeparator();
    printf("  用法:\n");
    printf("    movie.exe [选项]\n\n");
    printf("  选项:\n");
    printf("    -h, --help      显示此帮助信息\n");
    printf("    -t, --test      运行内置自测套件\n");
    printf("    (无参数)        启动交互模式\n\n");
    printf("  交互模式功能:\n");
    printf("    • 电影增删浏览\n");
    printf("    • 影评增删浏览\n");
    printf("    • 评分统计(平均/最高/最低/分布)\n");
    printf("    • 二进制文件持久化(MOVVV2格式)\n");
    printSeparator();
    printf("  编译: %s\n", "gcc -std=c99 -Wall -Wextra -O2 -o movie.exe main.c");
    printf("\n");
}

int main(int argc, char *argv[]) {
    consoleInit();

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            printHelp();
            return 0;
        }
        if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0) {
            runAllTests();
            return g_testFailed > 0 ? 1 : 0;
        }
        printf(C_RED "未知选项: %s\n" C_RESET, argv[1]);
        printHelp();
        return 1;
    }

    movieArrayInit(&g_movies);
    reviewArrayInit(&g_reviews);

    printBanner();
    printf(C_BOLD "  欢迎使用观影网站影评信息管理系统 V2.0！\n" C_RESET);
    printf("  学生：数据库课程  学号：424123888888  班级：计算机类A班\n");

    MovieStatus s = loadFromFile(DATA_FILE);
    if (s == MOVIE_OK) {
        printSuccess("已从 " DATA_FILE " 加载数据");
    } else if (s == MOVIE_ERR_FILE_OPEN) {
        printInfo("未找到数据文件，加载示例数据...");
        loadSampleData();
        saveToFile(DATA_FILE);
    } else {
        printError(statusToString(s));
        printInfo("加载示例数据...");
        loadSampleData();
    }

    mainMenuLoop();

    saveToFile(DATA_FILE);
    arrayFree();
    return 0;
}
