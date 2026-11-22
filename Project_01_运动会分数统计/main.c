/*
* ============================================================================
*
*    运动会分数统计系统 V2.0 (Sports Meet Scoring System V2.0)
*    Enhanced Edition with Dynamic Memory & Colored UI
*
* ============================================================================
*
*  功能概述:
*    运动会各参赛学校得分统计系统，支持：
*      [1] 学校管理（动态添加，无数量上限）
*      [2] 比赛项目管理（男子/女子，TOP3/TOP5计分规则）
*      [3] 成绩录入与自动计分（增量更新+全量重算双保险）
*      [4] 多维度排序（总分/男团/女团/编号）
*      [5] 按学校/项目双维度查询
*      [6] 统计面板（学校/项目/成绩总数，TOP学校排行）
*      [7] 文件持久化（magic header二进制格式，版本兼容）
*      [8] 命令行模式（--help/-f/-t/--stats等参数）
*      [9] 彩色终端界面（ANSI颜色，Windows 10+/Linux）
*     [10] 进度反馈条
*
*  数据结构:
*    School       — 学校信息（动态数组存储，按需realloc扩容）
*    Project      — 比赛项目（动态数组存储）
*    Result       — 成绩记录（动态数组存储）
*    DataFileHeader — .dat文件头（magic+version+计数）
*
*  计分规则:
*    TOP5 (取前五名): 第1名7分, 第2名5分, 第3名3分, 第4名2分, 第5名1分
*    TOP3 (取前三名): 第1名5分, 第2名3分, 第3名2分
*
*  文件格式 (.dat):
*    偏移  长度         字段              说明
*    0     4 bytes     magic "SPRT"      文件标识
*    4     2 bytes     version           格式版本(当前=2)
*    6     4 bytes     school_count      学校数量
*    10    4 bytes     project_count     项目数量
*    14    4 bytes     result_count      成绩记录数
*    18    ...         School[]          学校数据块
*    ...   ...         Project[]         项目数据块
*    ...   ...         Result[]          成绩数据块
*
*  编译命令:
*    gcc -std=c99 -Wall -O2 -o sports.exe main.c
*
*  命令行用法:
*    sports.exe                     交互模式（菜单）
*    sports.exe --help / -h         显示帮助
*    sports.exe -f <file.dat>       指定数据文件启动
*    sports.exe -t                  运行内置测试
*    sports.exe --stats <file.dat>  显示数据文件统计信息
*
*  交互模式:
*    直接运行 sports.exe 进入菜单交互模式
*
*  编码: UTF-8
*  标准: C99
* ============================================================================
*/

/* ======================== 标准头文件 ======================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>

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
#define MAX_NAME        64
#define INIT_CAPACITY   8
#define GROWTH_FACTOR   2
#define DATA_MAGIC      "SPRT"
#define DATA_VERSION    2
#define DEFAULT_FILE    "data.dat"
#define MAX_PROGRESS    40

/* ======================== ANSI颜色宏 ======================== */
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
#define C_ORANGE    "\033[38;5;208m"
#define BG_RED      "\033[41m"
#define BG_GREEN    "\033[42m"
#define BG_YELLOW   "\033[43m"
#define BG_BLUE     "\033[44m"
#define BG_ORANGE   "\033[48;5;208m"

/* ======================== 枚举类型 ======================== */

typedef enum {
    MALE = 0,
    FEMALE = 1
} ProjectCategory;

typedef enum {
    TOP3 = 3,
    TOP5 = 5
} ScoringRule;

typedef enum {
    SPT_OK = 0,
    SPT_ERR_OPEN_FILE,
    SPT_ERR_READ_FILE,
    SPT_ERR_WRITE_FILE,
    SPT_ERR_MEMORY,
    SPT_ERR_CORRUPT,
    SPT_ERR_MAGIC,
    SPT_ERR_VERSION,
    SPT_ERR_EMPTY,
    SPT_ERR_DUPLICATE,
    SPT_ERR_NOT_FOUND,
    SPT_ERR_INVALID_RANK,
    SPT_ERR_CAPACITY,
    SPT_ERR_CANCEL,
    SPT_ERR_INVALID_INPUT,
} SptStatus;

/* ======================== 数据结构 ======================== */

typedef struct {
    int id;
    char name[MAX_NAME];
    int total_score;
    int male_score;
    int female_score;
} School;

typedef struct {
    int id;
    char name[MAX_NAME];
    ProjectCategory category;
    ScoringRule rule;
} Project;

typedef struct {
    int project_id;
    int school_id;
    int rank;
    int score;
} Result;

typedef struct {
    char magic[4];
    unsigned short version;
    int school_count;
    int project_count;
    int result_count;
} DataFileHeader;

/* ======================== 动态数组全局状态 ======================== */
static School   *g_schools   = NULL;
static Project  *g_projects  = NULL;
static Result   *g_results   = NULL;
static int       g_school_cap  = 0;
static int       g_project_cap = 0;
static int       g_result_cap  = 0;
static int       g_school_cnt  = 0;
static int       g_project_cnt = 0;
static int       g_result_cnt  = 0;
static int       g_color_enabled = 1;
static char      g_data_file[MAX_NAME] = DEFAULT_FILE;

/* ======================== 前向声明 ======================== */
static void free_all_data(void);
static SptStatus load_from_file(const char *filename);
static SptStatus save_to_file(const char *filename);

/* ======================== 工具函数：颜色与终端 ======================== */

static void enable_vt_mode(void) {
#ifdef _WIN32
    /* Source strings are UTF-8, so make the Windows console interpret them
       as UTF-8 instead of the machine's legacy OEM/ANSI code page. */
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) { g_color_enabled = 0; return; }
    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) { g_color_enabled = 0; return; }
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
    g_color_enabled = 1;
#endif
}

static void c_print(const char *color, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (g_color_enabled && color) fputs(color, stdout);
    vprintf(fmt, args);
    if (g_color_enabled && color) fputs(C_RESET, stdout);
    va_end(args);
}

static void print_ok(const char *msg)    { c_print(C_GREEN, "  [OK] "); printf("%s\n", msg); }
static void print_err(const char *msg)   { c_print(C_RED,   "  [ERROR] "); printf("%s\n", msg); }
static void print_warn(const char *msg)  { c_print(C_YELLOW,"  [WARN] "); printf("%s\n", msg); }

/* ======================== 工具函数：UI ======================== */

static void print_banner(void) {
    printf("\n");
    c_print(C_BOLD C_ORANGE,
        "  ============================================================================\n");
    c_print(C_BOLD C_WHITE,
        "  |          Sports Meet Scoring System V2.0 (Enhanced Edition)              |\n");
    c_print(C_BOLD C_CYAN,
        "  |                     运 动 会 分 数 统 计 系 统  增强版                    |\n");
    c_print(C_BOLD C_ORANGE,
        "  ============================================================================\n");
    printf("\n");
    c_print(C_GRAY, "  作者：惠俊卓  |  学号：424123888888  |  班级：华信竞赛队A部\n");
    c_print(C_GRAY, "  学年：2025-2026学年第二学期  |  编码：UTF-8  |  标准：C99\n\n");
}

static void print_divider(void) {
    c_print(C_GRAY, "  ----------------------------------------------------------------------------\n");
}

static void print_thick_divider(void) {
    c_print(C_ORANGE, "  ============================================================================\n");
}

static int read_int(const char *prompt, int min, int max) {
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
        if (min == -2147483647 && sscanf(buf, "%d", &val) == 1)
            return val;
        c_print(C_YELLOW, "  [!] 请输入 [%d, %d] 范围内的整数\n", min, max);
    }
}

static void read_string(const char *prompt, char *buf, int size) {
    while (1) {
        printf("  %s", prompt);
        fflush(stdout);
        if (fgets(buf, size, stdin) == NULL) { clearerr(stdin); continue; }
        char *nl = strchr(buf, '\n');
        if (nl) *nl = '\0'; else { int c; while ((c=getchar())!='\n' && c!=EOF); }
        return;
    }
}

static int file_exists(const char *path) {
    return access(path, 0) == 0;
}

static void print_progress(long current, long total) {
    if (total <= 0) return;
    int barW = MAX_PROGRESS;
    double ratio = (double)current / total;
    if (ratio > 1.0) ratio = 1.0;
    int filled = (int)(ratio * barW);
    printf("\r  [");
    for (int i = 0; i < barW; i++) {
        if (i < filled) c_print(C_GREEN, "█");
        else            c_print(C_GRAY,  "░");
    }
    printf("] %5.1f%%", ratio * 100.0);
    fflush(stdout);
    if (current >= total) printf("\n");
}

/* ======================== 动态数组管理 ======================== */

static SptStatus ensure_school_cap(int need) {
    if (need <= g_school_cap) return SPT_OK;
    int new_cap = g_school_cap == 0 ? INIT_CAPACITY : g_school_cap;
    while (new_cap < need) new_cap *= GROWTH_FACTOR;
    School *tmp = (School *)realloc(g_schools, (size_t)new_cap * sizeof(School));
    if (!tmp) return SPT_ERR_MEMORY;
    g_schools = tmp;
    g_school_cap = new_cap;
    return SPT_OK;
}

static SptStatus ensure_project_cap(int need) {
    if (need <= g_project_cap) return SPT_OK;
    int new_cap = g_project_cap == 0 ? INIT_CAPACITY : g_project_cap;
    while (new_cap < need) new_cap *= GROWTH_FACTOR;
    Project *tmp = (Project *)realloc(g_projects, (size_t)new_cap * sizeof(Project));
    if (!tmp) return SPT_ERR_MEMORY;
    g_projects = tmp;
    g_project_cap = new_cap;
    return SPT_OK;
}

static SptStatus ensure_result_cap(int need) {
    if (need <= g_result_cap) return SPT_OK;
    int new_cap = g_result_cap == 0 ? INIT_CAPACITY : g_result_cap;
    while (new_cap < need) new_cap *= GROWTH_FACTOR;
    Result *tmp = (Result *)realloc(g_results, (size_t)new_cap * sizeof(Result));
    if (!tmp) return SPT_ERR_MEMORY;
    g_results = tmp;
    g_result_cap = new_cap;
    return SPT_OK;
}

static void free_all_data(void) {
    if (g_schools)  { free(g_schools);  g_schools  = NULL; }
    if (g_projects) { free(g_projects); g_projects = NULL; }
    if (g_results)  { free(g_results);  g_results  = NULL; }
    g_school_cap = g_project_cap = g_result_cap = 0;
    g_school_cnt = g_project_cnt = g_result_cnt = 0;
}

/* ======================== 查找辅助 ======================== */

static School *find_school_by_id(int id) {
    for (int i = 0; i < g_school_cnt; i++) {
        if (g_schools[i].id == id) return &g_schools[i];
    }
    return NULL;
}

static Project *find_project_by_id(int id) {
    for (int i = 0; i < g_project_cnt; i++) {
        if (g_projects[i].id == id) return &g_projects[i];
    }
    return NULL;
}

static int school_id_exists(int id) {
    return find_school_by_id(id) != NULL;
}

static int project_id_exists(int id) {
    return find_project_by_id(id) != NULL;
}

/* ======================== 计分计算 ======================== */

static int calc_score(ScoringRule rule, int rank) {
    static const int top5_scores[] = {0, 7, 5, 3, 2, 1};
    static const int top3_scores[] = {0, 5, 3, 2};

    if (rule == TOP5) {
        if (rank >= 1 && rank <= 5) return top5_scores[rank];
    } else if (rule == TOP3) {
        if (rank >= 1 && rank <= 3) return top3_scores[rank];
    }
    return 0;
}

/* ======================== 数据管理函数 ======================== */

static SptStatus add_school(int id, const char *name) {
    if (school_id_exists(id)) return SPT_ERR_DUPLICATE;
    SptStatus s = ensure_school_cap(g_school_cnt + 1);
    if (s != SPT_OK) return s;

    School *sch = &g_schools[g_school_cnt];
    sch->id = id;
    strncpy(sch->name, name, MAX_NAME - 1);
    sch->name[MAX_NAME - 1] = '\0';
    sch->total_score = 0;
    sch->male_score = 0;
    sch->female_score = 0;
    g_school_cnt++;
    return SPT_OK;
}

static SptStatus add_project(int id, const char *name, ProjectCategory cat, ScoringRule rule) {
    if (project_id_exists(id)) return SPT_ERR_DUPLICATE;
    SptStatus s = ensure_project_cap(g_project_cnt + 1);
    if (s != SPT_OK) return s;

    Project *proj = &g_projects[g_project_cnt];
    proj->id = id;
    strncpy(proj->name, name, MAX_NAME - 1);
    proj->name[MAX_NAME - 1] = '\0';
    proj->category = cat;
    proj->rule = rule;
    g_project_cnt++;
    return SPT_OK;
}

static int result_exists(int project_id, int school_id) {
    for (int i = 0; i < g_result_cnt; i++) {
        if (g_results[i].project_id == project_id &&
            g_results[i].school_id == school_id)
            return 1;
    }
    return 0;
}

static SptStatus add_result(int project_id, int school_id, int rank) {
    Project *proj = find_project_by_id(project_id);
    School  *sch  = find_school_by_id(school_id);
    if (!proj || !sch) return SPT_ERR_NOT_FOUND;

    int score = calc_score(proj->rule, rank);
    if (score == 0) return SPT_ERR_INVALID_RANK;
    if (result_exists(project_id, school_id)) return SPT_ERR_DUPLICATE;

    SptStatus s = ensure_result_cap(g_result_cnt + 1);
    if (s != SPT_OK) return s;

    Result *r = &g_results[g_result_cnt];
    r->project_id = project_id;
    r->school_id = school_id;
    r->rank = rank;
    r->score = score;
    g_result_cnt++;

    sch->total_score += score;
    if (proj->category == MALE)
        sch->male_score += score;
    else
        sch->female_score += score;

    return SPT_OK;
}

static void recalc_all_scores(void) {
    for (int i = 0; i < g_school_cnt; i++) {
        g_schools[i].total_score = 0;
        g_schools[i].male_score = 0;
        g_schools[i].female_score = 0;
    }
    for (int i = 0; i < g_result_cnt; i++) {
        Project *proj = find_project_by_id(g_results[i].project_id);
        School  *sch  = find_school_by_id(g_results[i].school_id);
        if (!proj || !sch) continue;

        sch->total_score += g_results[i].score;
        if (proj->category == MALE)
            sch->male_score += g_results[i].score;
        else
            sch->female_score += g_results[i].score;
    }
}

/* ======================== 文件持久化（带magic header） ======================== */

static SptStatus save_to_file(const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) return SPT_ERR_OPEN_FILE;

    DataFileHeader hdr;
    memcpy(hdr.magic, DATA_MAGIC, 4);
    hdr.version = DATA_VERSION;
    hdr.school_count = g_school_cnt;
    hdr.project_count = g_project_cnt;
    hdr.result_count = g_result_cnt;

    if (fwrite(&hdr, sizeof(hdr), 1, fp) != 1) { fclose(fp); return SPT_ERR_WRITE_FILE; }

    if (g_school_cnt > 0) {
        for (int i = 0; i < g_school_cnt; i++) {
            if (fwrite(&g_schools[i], sizeof(School), 1, fp) != 1) {
                fclose(fp); return SPT_ERR_WRITE_FILE;
            }
            if (g_school_cnt > 50 && i % (g_school_cnt / 10 + 1) == 0)
                print_progress(i + 1, g_school_cnt + g_project_cnt + g_result_cnt);
        }
    }
    if (g_project_cnt > 0) {
        for (int i = 0; i < g_project_cnt; i++) {
            if (fwrite(&g_projects[i], sizeof(Project), 1, fp) != 1) {
                fclose(fp); return SPT_ERR_WRITE_FILE;
            }
        }
    }
    if (g_result_cnt > 0) {
        for (int i = 0; i < g_result_cnt; i++) {
            if (fwrite(&g_results[i], sizeof(Result), 1, fp) != 1) {
                fclose(fp); return SPT_ERR_WRITE_FILE;
            }
            if (g_result_cnt > 50 && i % (g_result_cnt / 10 + 1) == 0)
                print_progress(g_school_cnt + g_project_cnt + i + 1,
                               g_school_cnt + g_project_cnt + g_result_cnt);
        }
    }
    if (g_school_cnt + g_project_cnt + g_result_cnt > 50)
        print_progress(g_school_cnt + g_project_cnt + g_result_cnt,
                       g_school_cnt + g_project_cnt + g_result_cnt);

    fclose(fp);
    return SPT_OK;
}

static SptStatus load_from_file(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) return SPT_ERR_OPEN_FILE;

    DataFileHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) { fclose(fp); return SPT_ERR_READ_FILE; }

    if (memcmp(hdr.magic, DATA_MAGIC, 4) != 0) {
        fclose(fp);
        return SPT_ERR_MAGIC;
    }
    if (hdr.version > DATA_VERSION) { fclose(fp); return SPT_ERR_VERSION; }
    if (hdr.school_count < 0 || hdr.project_count < 0 || hdr.result_count < 0) {
        fclose(fp); return SPT_ERR_CORRUPT;
    }

    free_all_data();

    if (hdr.school_count > 0) {
        SptStatus s = ensure_school_cap(hdr.school_count);
        if (s != SPT_OK) { fclose(fp); return s; }
        for (int i = 0; i < hdr.school_count; i++) {
            if (fread(&g_schools[i], sizeof(School), 1, fp) != 1) {
                fclose(fp); free_all_data(); return SPT_ERR_READ_FILE;
            }
        }
        g_school_cnt = hdr.school_count;
    }
    if (hdr.project_count > 0) {
        SptStatus s = ensure_project_cap(hdr.project_count);
        if (s != SPT_OK) { fclose(fp); free_all_data(); return s; }
        for (int i = 0; i < hdr.project_count; i++) {
            if (fread(&g_projects[i], sizeof(Project), 1, fp) != 1) {
                fclose(fp); free_all_data(); return SPT_ERR_READ_FILE;
            }
        }
        g_project_cnt = hdr.project_count;
    }
    if (hdr.result_count > 0) {
        SptStatus s = ensure_result_cap(hdr.result_count);
        if (s != SPT_OK) { fclose(fp); free_all_data(); return s; }
        for (int i = 0; i < hdr.result_count; i++) {
            if (fread(&g_results[i], sizeof(Result), 1, fp) != 1) {
                fclose(fp); free_all_data(); return SPT_ERR_READ_FILE;
            }
            long total = (long)hdr.school_count + hdr.project_count + hdr.result_count;
            long cur   = (long)hdr.school_count + hdr.project_count + i + 1;
            if (total > 50 && i % (total / 10 + 1) == 0)
                print_progress(cur, total);
        }
        g_result_cnt = hdr.result_count;
    }
    long total = (long)hdr.school_count + hdr.project_count + hdr.result_count;
    if (total > 50) print_progress(total, total);

    fclose(fp);
    recalc_all_scores();
    return SPT_OK;
}

/* ======================== 统计面板 ======================== */

static void show_statistics(void) {
    printf("\n");
    print_thick_divider();
    c_print(C_BOLD C_CYAN, "  数据统计面板\n");
    print_divider();

    printf("  学校总数:     "); c_print(C_WHITE, "%d 所\n", g_school_cnt);
    printf("  项目总数:     "); c_print(C_WHITE, "%d 个\n", g_project_cnt);
    printf("  成绩记录数:   "); c_print(C_WHITE, "%d 条\n", g_result_cnt);

    int male_proj = 0, female_proj = 0;
    int top3_proj = 0, top5_proj = 0;
    for (int i = 0; i < g_project_cnt; i++) {
        if (g_projects[i].category == MALE) male_proj++; else female_proj++;
        if (g_projects[i].rule == TOP3) top3_proj++; else top5_proj++;
    }
    printf("  男子项目:     "); c_print(C_CYAN, "%d 个\n", male_proj);
    printf("  女子项目:     "); c_print(C_MAGENTA, "%d 个\n", female_proj);
    printf("  TOP3项目:     "); c_print(C_CYAN, "%d 个\n", top3_proj);
    printf("  TOP5项目:     "); c_print(C_CYAN, "%d 个\n", top5_proj);

    int total_scores = 0;
    for (int i = 0; i < g_school_cnt; i++) total_scores += g_schools[i].total_score;
    printf("  总得分:       "); c_print(C_GREEN, "%d 分\n", total_scores);

    print_divider();

    if (g_school_cnt > 0) {
        c_print(C_BOLD C_ORANGE, "  TOP 学校排行榜（按总分降序）\n");
        print_divider();

        School *sorted = (School *)malloc((size_t)g_school_cnt * sizeof(School));
        if (sorted) {
            memcpy(sorted, g_schools, (size_t)g_school_cnt * sizeof(School));
            for (int i = 0; i < g_school_cnt - 1; i++) {
                int best = i;
                for (int j = i + 1; j < g_school_cnt; j++) {
                    if (sorted[j].total_score > sorted[best].total_score)
                        best = j;
                }
                if (best != i) {
                    School tmp = sorted[i]; sorted[i] = sorted[best]; sorted[best] = tmp;
                }
            }
            int top_n = g_school_cnt < 5 ? g_school_cnt : 5;
            c_print(C_BOLD, "  %-4s %-6s %-22s %8s %8s %8s\n",
                   "排名", "编号", "学校名称", "总分", "男团", "女团");
            print_divider();
            for (int i = 0; i < top_n; i++) {
                const char *medal = "";
                if (i == 0) medal = C_ORANGE "🥇";
                else if (i == 1) medal = C_GRAY "🥈";
                else if (i == 2) medal = C_RED "🥉";
                else medal = C_WHITE "  ";
                c_print(medal, "  ");
                printf("%-2d", i + 1);
                c_print(C_WHITE, " %-6d %-22s %8d %8d %8d\n",
                       sorted[i].id, sorted[i].name,
                       sorted[i].total_score, sorted[i].male_score, sorted[i].female_score);
            }
            free(sorted);
        }
    } else {
        c_print(C_YELLOW, "  暂无学校数据。\n");
    }

    print_thick_divider();
    printf("\n");
}

/* ======================== 功能模块 ======================== */

static void func_add_school(void) {
    printf("\n");
    print_thick_divider();
    c_print(C_BOLD C_CYAN, "  添加学校\n");
    print_divider();

    int n = read_int("请输入要添加的学校数量: ", 1, 1000);
    char name[MAX_NAME];
    for (int i = 0; i < n; i++) {
        int id = read_int("学校编号: ", 1, 99999);
        read_string("学校名称: ", name, MAX_NAME);
        SptStatus s = add_school(id, name);
        switch (s) {
            case SPT_OK:          print_ok("学校添加成功"); break;
            case SPT_ERR_DUPLICATE: print_err("编号重复，添加失败"); break;
            case SPT_ERR_MEMORY:    print_err("内存不足"); break;
            default:                print_err("添加失败"); break;
        }
    }
    printf("\n");
}

static void func_add_project(void) {
    printf("\n");
    print_thick_divider();
    c_print(C_BOLD C_CYAN, "  添加比赛项目\n");
    print_divider();

    int m = read_int("男子项目数量: ", 0, 1000);
    int w = read_int("女子项目数量: ", 0, 1000);
    int total = m + w;
    if (total == 0) { print_warn("未添加任何项目"); return; }

    char name[MAX_NAME];
    for (int i = 0; i < total; i++) {
        ProjectCategory pc = (i < m) ? MALE : FEMALE;
        const char *cat_str = (i < m) ? "男子" : "女子";
        c_print(C_CYAN, "\n  --- 第 %d 个项目（%s）---\n", i + 1, cat_str);
        int id = read_int("项目编号: ", 1, 99999);
        read_string("项目名称: ", name, MAX_NAME);
        int rule = read_int("计分规则 (3=TOP3/5=TOP5): ", 3, 5);
        ScoringRule sr = (rule == 3) ? TOP3 : TOP5;
        SptStatus s = add_project(id, name, pc, sr);
        switch (s) {
            case SPT_OK:          print_ok("项目添加成功"); break;
            case SPT_ERR_DUPLICATE: print_err("编号重复，添加失败"); break;
            case SPT_ERR_MEMORY:    print_err("内存不足"); break;
            default:                print_err("添加失败"); break;
        }
    }
    printf("\n");
}

static void func_input_result(void) {
    printf("\n");
    print_thick_divider();
    c_print(C_BOLD C_CYAN, "  录入比赛成绩\n");
    print_divider();

    if (g_school_cnt == 0) {
        c_print(C_YELLOW, "  [!] 系统中暂无学校，请先添加学校。\n\n");
        func_add_school();
    }
    if (g_project_cnt == 0) {
        c_print(C_YELLOW, "  [!] 系统中暂无项目，请先添加项目。\n\n");
        func_add_project();
    }
    if (g_school_cnt == 0 || g_project_cnt == 0) return;

    while (1) {
        int pid = read_int("请输入项目编号（输入0结束录入）: ", 0, 99999);
        if (pid == 0) break;

        Project *proj = find_project_by_id(pid);
        if (!proj) { print_err("项目不存在，请重新输入"); continue; }

        int max_rank = (proj->rule == TOP5) ? 5 : 3;
        c_print(C_CYAN, "  项目 [");
        c_print(C_BOLD C_WHITE, "%s", proj->name);
        c_print(C_CYAN, "] 取前%d名\n", max_rank);
        c_print(C_GRAY, "  格式: 学校编号 名次（输入 0 0 结束该项目）\n");

        while (1) {
            int sid, rk;
            char buf[128];
            printf("  > "); fflush(stdout);
            if (!fgets(buf, sizeof(buf), stdin)) continue;
            char *nl = strchr(buf, '\n');
            if (nl) *nl = '\0';
            if (sscanf(buf, "%d %d", &sid, &rk) != 2) {
                c_print(C_YELLOW, "  [!] 请输入两个整数（学校编号 名次）\n");
                continue;
            }
            if (sid == 0 && rk == 0) break;

            SptStatus s = add_result(pid, sid, rk);
            int sc = calc_score(proj->rule, rk);
            switch (s) {
                case SPT_OK:
                    c_print(C_GREEN, "    -> 学校 %d 在 [%s] 中获第%d名，得 %d 分\n",
                           sid, proj->name, rk, sc);
                    break;
                case SPT_ERR_NOT_FOUND:
                    print_err("学校或项目不存在"); break;
                case SPT_ERR_INVALID_RANK:
                    c_print(C_RED, "    -> [ERROR] 名次无效（应在1~%d之间）\n", max_rank); break;
                case SPT_ERR_DUPLICATE:
                    print_err("该学校在此项目中已有成绩记录"); break;
                case SPT_ERR_MEMORY:
                    print_err("内存不足"); break;
                default:
                    print_err("添加失败"); break;
            }
        }
    }
    c_print(C_GREEN, "  [OK] 成绩录入完毕\n\n");
}

static void func_show_all_scores(void) {
    printf("\n");
    print_thick_divider();
    c_print(C_BOLD C_CYAN, "  各学校得分统计\n");
    print_divider();

    if (g_school_cnt == 0) {
        c_print(C_YELLOW, "  暂无数据，请先录入成绩。\n\n");
        return;
    }

    c_print(C_BOLD, "  %-6s %-22s %8s %8s %8s\n",
           "编号", "学校名称", "总分", "男团", "女团");
    print_divider();

    for (int i = 0; i < g_school_cnt; i++) {
        c_print(C_CYAN, "  %-6d", g_schools[i].id);
        c_print(C_WHITE, "%-22s", g_schools[i].name);
        c_print(C_GREEN, " %8d", g_schools[i].total_score);
        c_print(C_CYAN,   " %8d", g_schools[i].male_score);
        c_print(C_MAGENTA, " %8d\n", g_schools[i].female_score);
    }
    printf("\n");
}

static void func_sort_schools(void) {
    printf("\n");
    print_divider();
    c_print(C_BOLD C_CYAN, "  排序输出\n");
    print_divider();
    printf("  1. 按学校编号升序\n");
    printf("  2. 按总分降序\n");
    printf("  3. 按男子团体总分降序\n");
    printf("  4. 按女子团体总分降序\n");
    int choice = read_int("请选择排序方式 (1-4): ", 1, 4);

    if (g_school_cnt == 0) { c_print(C_YELLOW, "  暂无数据。\n\n"); return; }

    School *sorted = (School *)malloc((size_t)g_school_cnt * sizeof(School));
    if (!sorted) { print_err("内存不足"); return; }
    memcpy(sorted, g_schools, (size_t)g_school_cnt * sizeof(School));

    for (int i = 0; i < g_school_cnt - 1; i++) {
        int best = i;
        for (int j = i + 1; j < g_school_cnt; j++) {
            int cmp = 0;
            switch (choice) {
                case 1: cmp = sorted[j].id < sorted[best].id; break;
                case 2: cmp = sorted[j].total_score > sorted[best].total_score; break;
                case 3: cmp = sorted[j].male_score > sorted[best].male_score; break;
                case 4: cmp = sorted[j].female_score > sorted[best].female_score; break;
            }
            if (cmp) best = j;
        }
        if (best != i) { School tmp = sorted[i]; sorted[i] = sorted[best]; sorted[best] = tmp; }
    }

    printf("\n");
    c_print(C_BOLD, "  %-4s %-6s %-22s %8s %8s %8s\n",
           "排名", "编号", "学校名称", "总分", "男团", "女团");
    print_divider();
    for (int i = 0; i < g_school_cnt; i++) {
        const char *rc = C_WHITE;
        if (choice == 2 && i < 3) rc = (i == 0) ? C_ORANGE : (i == 1) ? C_GRAY : C_RED;
        c_print(rc, "  %-4d", i + 1);
        c_print(C_CYAN, " %-6d", sorted[i].id);
        c_print(C_WHITE, "%-22s", sorted[i].name);
        c_print(C_GREEN, " %8d", sorted[i].total_score);
        c_print(C_CYAN,  " %8d", sorted[i].male_score);
        c_print(C_MAGENTA," %8d\n", sorted[i].female_score);
    }
    free(sorted);
    printf("\n");
}

static void func_query_by_school(void) {
    printf("\n");
    print_thick_divider();
    c_print(C_BOLD C_CYAN, "  按学校查询\n");
    print_divider();
    int sid = read_int("请输入学校编号: ", 1, 99999);
    School *sch = find_school_by_id(sid);
    if (!sch) { print_err("学校不存在\n"); return; }

    printf("\n");
    c_print(C_CYAN, "  学校: ");
    c_print(C_BOLD C_WHITE, "%s", sch->name);
    c_print(C_CYAN, "（编号 %d）\n", sch->id);
    c_print(C_GREEN, "  总分: %d", sch->total_score);
    c_print(C_CYAN, "  |  男子团体: %d", sch->male_score);
    c_print(C_MAGENTA, "  |  女子团体: %d\n\n", sch->female_score);

    c_print(C_BOLD, "  %-8s %-22s %-6s %-6s %-6s\n",
           "项目编号", "项目名称", "类别", "名次", "得分");
    print_divider();

    int found = 0;
    for (int i = 0; i < g_result_cnt; i++) {
        if (g_results[i].school_id == sid) {
            Project *proj = find_project_by_id(g_results[i].project_id);
            if (proj) {
                c_print(C_CYAN, "  %-8d", proj->id);
                c_print(C_WHITE, "%-22s", proj->name);
                c_print(proj->category == MALE ? C_CYAN : C_MAGENTA, "%-6s",
                       proj->category == MALE ? "男" : "女");
                c_print(C_YELLOW, " %-6d", g_results[i].rank);
                c_print(C_GREEN, " %-6d\n", g_results[i].score);
                found = 1;
            }
        }
    }
    if (!found) c_print(C_GRAY, "  该学校暂无成绩记录。\n");
    printf("\n");
}

static void func_query_by_project(void) {
    printf("\n");
    print_thick_divider();
    c_print(C_BOLD C_CYAN, "  按项目查询\n");
    print_divider();
    int pid = read_int("请输入项目编号: ", 1, 99999);
    Project *proj = find_project_by_id(pid);
    if (!proj) { print_err("项目不存在\n"); return; }

    printf("\n");
    c_print(C_CYAN, "  项目: ");
    c_print(C_BOLD C_WHITE, "%s", proj->name);
    c_print(C_CYAN, "（编号 %d）\n", proj->id);
    c_print(proj->category == MALE ? C_CYAN : C_MAGENTA, "  类别: %s",
           proj->category == MALE ? "男子" : "女子");
    c_print(C_CYAN, "  |  计分规则: 取前%d名\n\n", (int)proj->rule);

    c_print(C_BOLD, "  %-6s %-22s %-6s %-6s\n",
           "名次", "学校名称", "编号", "得分");
    print_divider();

    int found = 0;
    int max_rank = (proj->rule == TOP5) ? 5 : 3;
    for (int r = 1; r <= max_rank; r++) {
        for (int i = 0; i < g_result_cnt; i++) {
            if (g_results[i].project_id == pid && g_results[i].rank == r) {
                School *sch = find_school_by_id(g_results[i].school_id);
                if (sch) {
                    const char *rc = C_WHITE;
                    if (r == 1) rc = C_ORANGE;
                    else if (r == 2) rc = C_GRAY;
                    else if (r == 3) rc = C_RED;
                    c_print(rc, "  %-6d", r);
                    c_print(C_WHITE, "%-22s", sch->name);
                    c_print(C_CYAN, " %-6d", sch->id);
                    c_print(C_GREEN, " %-6d\n", g_results[i].score);
                    found = 1;
                }
            }
        }
    }
    if (!found) c_print(C_GRAY, "  该项目暂无成绩记录。\n");
    printf("\n");
}

static void func_manage_data(void) {
    while (1) {
        printf("\n");
        print_divider();
        c_print(C_BOLD C_BLUE, "  数据管理\n");
        print_divider();
        c_print(C_CYAN, "  1 "); printf("- 添加学校\n");
        c_print(C_CYAN, "  2 "); printf("- 添加比赛项目\n");
        c_print(C_CYAN, "  3 "); printf("- 查看学校列表\n");
        c_print(C_CYAN, "  4 "); printf("- 查看项目列表\n");
        c_print(C_CYAN, "  0 "); printf("- 返回主菜单\n");
        print_divider();
        int ch = read_int("请选择: ", 0, 4);
        if (ch == 0) break;
        switch (ch) {
            case 1: func_add_school(); break;
            case 2: func_add_project(); break;
            case 3:
                printf("\n");
                if (g_school_cnt == 0) { c_print(C_YELLOW, "  暂无学校\n\n"); break; }
                c_print(C_BOLD, "  %-6s %-22s %8s\n", "编号", "学校名称", "总分");
                print_divider();
                for (int i = 0; i < g_school_cnt; i++) {
                    c_print(C_CYAN, "  %-6d", g_schools[i].id);
                    c_print(C_WHITE, "%-22s", g_schools[i].name);
                    c_print(C_GREEN, " %8d\n", g_schools[i].total_score);
                }
                printf("\n");
                break;
            case 4:
                printf("\n");
                if (g_project_cnt == 0) { c_print(C_YELLOW, "  暂无项目\n\n"); break; }
                c_print(C_BOLD, "  %-6s %-22s %-6s %-6s\n", "编号", "项目名称", "类别", "规则");
                print_divider();
                for (int i = 0; i < g_project_cnt; i++) {
                    c_print(C_CYAN, "  %-6d", g_projects[i].id);
                    c_print(C_WHITE, "%-22s", g_projects[i].name);
                    c_print(g_projects[i].category==MALE?C_CYAN:C_MAGENTA,
                           "%-6s", g_projects[i].category==MALE?"男子":"女子");
                    c_print(C_YELLOW, " TOP%d\n", (int)g_projects[i].rule);
                }
                printf("\n");
                break;
        }
    }
}

/* ======================== 内置测试模块 ======================== */

static int run_tests(void) {
    int pass = 0, fail = 0;
    printf("\n");
    print_thick_divider();
    c_print(C_BOLD C_CYAN, "  内 置 测 试 模 块 V2.0\n");
    print_thick_divider();
    printf("\n");

    free_all_data();

    /* 测试1：动态添加学校 */
    c_print(C_BOLD, "[测试1] 添加学校（动态内存）\n");
    {
        SptStatus r1 = add_school(1, "北京大学");
        SptStatus r2 = add_school(2, "清华大学");
        SptStatus r3 = add_school(3, "浙江大学");
        SptStatus r4 = add_school(4, "复旦大学");
        SptStatus r5 = add_school(5, "上海交大");
        if (r1==SPT_OK && r2==SPT_OK && r3==SPT_OK && r4==SPT_OK && r5==SPT_OK && g_school_cnt==5) {
            print_ok("添加5所学校成功"); pass++;
        } else { print_err("添加学校失败"); fail++; }

        SptStatus r_dup = add_school(1, "重复学校");
        if (r_dup == SPT_ERR_DUPLICATE) { print_ok("重复编号正确拒绝"); pass++; }
        else { print_err("重复编号未被拒绝"); fail++; }

        c_print(C_GRAY, "    容量: %d, 已用: %d\n", g_school_cap, g_school_cnt);
    }

    /* 测试2：添加项目 */
    c_print(C_BOLD, "\n[测试2] 添加项目\n");
    {
        SptStatus r1 = add_project(1, "男子100米",  MALE, TOP5);
        SptStatus r2 = add_project(2, "男子跳远",   MALE, TOP3);
        SptStatus r3 = add_project(3, "女子200米",  FEMALE, TOP5);
        SptStatus r4 = add_project(4, "女子铅球",   FEMALE, TOP3);
        if (r1==SPT_OK && r2==SPT_OK && r3==SPT_OK && r4==SPT_OK && g_project_cnt==4) {
            print_ok("添加4个项目成功"); pass++;
        } else { print_err("添加项目失败"); fail++; }
    }

    /* 测试3：计分函数 */
    c_print(C_BOLD, "\n[测试3] 计分函数\n");
    {
        int s1=calc_score(TOP5,1), s2=calc_score(TOP5,2), s3=calc_score(TOP5,3),
            s4=calc_score(TOP5,4), s5=calc_score(TOP5,5), s6=calc_score(TOP5,6);
        if (s1==7&&s2==5&&s3==3&&s4==2&&s5==1&&s6==0)
            { print_ok("TOP5计分正确 (7,5,3,2,1,0)"); pass++; }
        else { print_err("TOP5计分错误"); fail++; }

        int t1=calc_score(TOP3,1), t2=calc_score(TOP3,2), t3=calc_score(TOP3,3), t4=calc_score(TOP3,4);
        if (t1==5&&t2==3&&t3==2&&t4==0)
            { print_ok("TOP3计分正确 (5,3,2,0)"); pass++; }
        else { print_err("TOP3计分错误"); fail++; }
    }

    /* 测试4：添加成绩 */
    c_print(C_BOLD, "\n[测试4] 添加成绩\n");
    {
        SptStatus r1=add_result(1,1,1), r2=add_result(1,2,2), r3=add_result(1,3,3),
                  r4=add_result(1,4,4), r5=add_result(1,5,5);
        SptStatus r6=add_result(2,2,1), r7=add_result(2,3,2), r8=add_result(2,1,3);
        SptStatus r9=add_result(3,3,1), r10=add_result(3,1,2), r11=add_result(3,4,3);
        SptStatus r12=add_result(4,5,1), r13=add_result(4,4,2), r14=add_result(4,2,3);
        if (r1==SPT_OK&&r2==SPT_OK&&r3==SPT_OK&&r4==SPT_OK&&r5==SPT_OK&&
            r6==SPT_OK&&r7==SPT_OK&&r8==SPT_OK&&r9==SPT_OK&&r10==SPT_OK&&
            r11==SPT_OK&&r12==SPT_OK&&r13==SPT_OK&&r14==SPT_OK&&g_result_cnt==14) {
            print_ok("添加14条成绩记录成功"); pass++;
        } else { print_err("添加成绩失败"); fail++; }

        SptStatus r_dup = add_result(1,1,1);
        if (r_dup == SPT_ERR_DUPLICATE) { print_ok("重复记录正确拒绝"); pass++; }
        else { print_err("重复记录未被拒绝"); fail++; }

        SptStatus r_bad = add_result(1,5,6);
        if (r_bad == SPT_ERR_INVALID_RANK) { print_ok("名次越界正确拒绝"); pass++; }
        else { print_err("名次越界未被拒绝"); fail++; }
    }

    /* 测试5：分数统计 */
    c_print(C_BOLD, "\n[测试5] 分数统计验证\n");
    {
        School *s1=find_school_by_id(1), *s2=find_school_by_id(2),
               *s3=find_school_by_id(3), *s4=find_school_by_id(4),
               *s5=find_school_by_id(5);
        int ok = 1;
        if (!s1||s1->total_score!=14||s1->male_score!=9||s1->female_score!=5) ok=0;
        if (!s2||s2->total_score!=12||s2->male_score!=10||s2->female_score!=2) ok=0;
        if (!s3||s3->total_score!=13||s3->male_score!=6||s3->female_score!=7) ok=0;
        if (!s4||s4->total_score!=8||s4->male_score!=2||s4->female_score!=6) ok=0;
        if (!s5||s5->total_score!=6||s5->male_score!=1||s5->female_score!=5) ok=0;
        if (ok) { print_ok("所有学校分数统计正确"); pass++; }
        else { print_err("分数统计有误"); fail++; }
    }

    /* 测试6：动态扩容测试 */
    c_print(C_BOLD, "\n[测试6] 动态数组扩容\n");
    {
        int old_cap = g_school_cap;
        for (int i = 100; i < 100 + old_cap + 5; i++) {
            char nm[32]; snprintf(nm, sizeof(nm), "测试学校%d", i);
            add_school(i, nm);
        }
        if (g_school_cap > old_cap) {
            c_print(C_GREEN, "  [OK] 动态扩容成功（容量 %d -> %d，学校数 %d）\n",
                   old_cap, g_school_cap, g_school_cnt);
            pass++;
        } else { print_err("动态扩容失败"); fail++; }

        for (int i = 100; i < 100 + old_cap + 5; i++) {
            for (int j = 0; j < g_school_cnt; j++) {
                if (g_schools[j].id == i) {
                    for (int k = j; k < g_school_cnt - 1; k++)
                        g_schools[k] = g_schools[k+1];
                    g_school_cnt--;
                    j--;
                }
            }
        }
    }

    /* 测试7：文件读写 with magic header */
    c_print(C_BOLD, "\n[测试7] 文件读写（magic header）\n");
    {
        SptStatus s1 = save_to_file("test_data.dat");
        if (s1 == SPT_OK) { print_ok("文件保存成功"); pass++; }
        else { print_err("文件保存失败"); fail++; }

        int old_sc=g_school_cnt, old_pc=g_project_cnt, old_rc=g_result_cnt;
        free_all_data();

        SptStatus s2 = load_from_file("test_data.dat");
        if (s2 == SPT_OK) { print_ok("文件加载成功"); pass++; }
        else { print_err("文件加载失败"); fail++; }

        if (g_school_cnt==old_sc && g_project_cnt==old_pc && g_result_cnt==old_rc) {
            c_print(C_GREEN, "  [OK] 加载后数据计数一致（学校:%d 项目:%d 成绩:%d）\n",
                   g_school_cnt, g_project_cnt, g_result_cnt); pass++;
        } else { print_err("加载后数据计数不一致"); fail++; }

        School *s1chk = find_school_by_id(1);
        if (s1chk && s1chk->total_score == 14) { print_ok("加载后分数重算正确"); pass++; }
        else { print_err("加载后分数重算错误"); fail++; }

        remove("test_data.dat");
    }

    /* 测试8：magic校验 */
    c_print(C_BOLD, "\n[测试8] Magic/Version校验\n");
    {
        FILE *fp = fopen("test_bad.dat", "wb");
        if (fp) {
            DataFileHeader bhdr;
            memset(&bhdr, 0, sizeof(bhdr));
            memcpy(bhdr.magic, "XXXX", 4);
            bhdr.version = DATA_VERSION;
            fwrite(&bhdr, sizeof(bhdr), 1, fp);
            fclose(fp);
        }
        SptStatus s = load_from_file("test_bad.dat");
        if (s == SPT_ERR_MAGIC) { print_ok("Magic校验正确拒绝非SPRT文件"); pass++; }
        else { print_err("Magic校验失效"); fail++; }
        remove("test_bad.dat");
    }

    /* 测试9：边界值 */
    c_print(C_BOLD, "\n[测试9] 边界值测试\n");
    {
        SptStatus r1 = add_result(999, 1, 1);
        SptStatus r2 = add_result(1, 999, 1);
        if (r1==SPT_ERR_NOT_FOUND && r2==SPT_ERR_NOT_FOUND)
            { print_ok("不存在的学校/项目正确拒绝"); pass++; }
        else { print_err("不存在的学校/项目未被拒绝"); fail++; }

        SptStatus r3 = add_result(1, 1, 0);
        if (r3 == SPT_ERR_INVALID_RANK) { print_ok("名次0正确拒绝"); pass++; }
        else { print_err("名次0未被拒绝"); fail++; }
    }

    /* 结果汇总 */
    printf("\n");
    print_thick_divider();
    c_print(C_BOLD C_CYAN, "  测试结果: ");
    c_print(C_GREEN, "%d 通过", pass);
    printf(", ");
    c_print(fail==0?C_GREEN:C_RED, "%d 失败", fail);
    printf(", 共 %d 项\n", pass + fail);
    if (fail == 0) {
        c_print(C_BOLD C_GREEN, "  ★ 所有测试通过！★\n");
    } else {
        c_print(C_BOLD C_RED, "  ✗ 有 %d 项测试未通过\n", fail);
    }
    print_thick_divider();
    printf("\n");
    return fail == 0 ? 0 : 1;
}

/* ======================== 菜单系统 ======================== */

static void show_menu(void) {
    print_divider();
    c_print(C_BOLD C_ORANGE, "  主菜单\n");
    print_divider();
    c_print(C_CYAN, "  1 "); printf("- 录入比赛成绩\n");
    c_print(C_CYAN, "  2 "); printf("- 查看所有学校得分\n");
    c_print(C_CYAN, "  3 "); printf("- 排序输出学校排名\n");
    c_print(C_CYAN, "  4 "); printf("- 按学校查询成绩\n");
    c_print(C_CYAN, "  5 "); printf("- 按项目查询名次\n");
    c_print(C_CYAN, "  6 "); printf("- 数据管理（添加学校/项目）\n");
    c_print(C_CYAN, "  7 "); printf("- 数据统计面板\n");
    c_print(C_CYAN, "  8 "); printf("- 保存数据到文件\n");
    c_print(C_CYAN, "  9 "); printf("- 从文件加载数据\n");
    c_print(C_CYAN, "  T "); printf("- 运行内置测试\n");
    c_print(C_CYAN, "  H/?");printf("- 帮助信息\n");
    c_print(C_CYAN, "  0 "); printf("- 退出系统\n");
    print_divider();
    printf("  当前状态: ");
    c_print(C_CYAN, "%d 所学校", g_school_cnt);
    printf(" | ");
    c_print(C_CYAN, "%d 个项目", g_project_cnt);
    printf(" | ");
    c_print(C_GREEN, "%d 条成绩", g_result_cnt);
    printf(" | 文件: ");
    c_print(C_YELLOW, "%s", g_data_file);
    printf("\n");
    print_divider();
}

static void show_help(void) {
    print_thick_divider();
    c_print(C_BOLD C_CYAN, "  使用帮助\n");
    print_thick_divider();
    printf("\n");
    c_print(C_BOLD, "  工作流程:\n");
    printf("    1. 首次使用: 选择6(数据管理)添加学校和比赛项目\n");
    printf("    2. 选择1录入各项目比赛成绩，自动计算得分\n");
    printf("    3. 选择2/3查看分数统计和排名\n");
    printf("    4. 选择4/5按学校或项目查询详细信息\n");
    printf("    5. 选择7查看统计面板（TOP学校排行）\n");
    printf("    6. 退出时自动保存数据到 %s\n", DEFAULT_FILE);
    printf("\n");
    c_print(C_BOLD, "  计分规则:\n");
    c_print(C_CYAN, "    TOP5（取前五名）: "); printf("第1名7分, 第2名5分, 第3名3分, 第4名2分, 第5名1分\n");
    c_print(C_CYAN, "    TOP3（取前三名）: "); printf("第1名5分, 第2名3分, 第3名2分\n");
    printf("\n");
    c_print(C_BOLD, "  命令行模式:\n");
    c_print(C_CYAN, "    sports.exe --help / -h      "); printf("显示帮助\n");
    c_print(C_CYAN, "    sports.exe -f <file.dat>    "); printf("指定数据文件启动\n");
    c_print(C_CYAN, "    sports.exe -t               "); printf("运行内置测试\n");
    c_print(C_CYAN, "    sports.exe --stats <file>   "); printf("显示数据文件统计信息\n");
    printf("\n");
    c_print(C_BOLD, "  V2.0 新特性:\n");
    c_print(C_ORANGE, "    • "); printf("UTF-8编码，彩色终端输出\n");
    c_print(C_ORANGE, "    • "); printf("动态内存(malloc/realloc)，不再有MAX_SCHOOLS=50限制\n");
    c_print(C_ORANGE, "    • "); printf("统一错误码枚举（SptStatus）\n");
    c_print(C_ORANGE, "    • "); printf("命令行参数支持（--help/-f/-t/--stats）\n");
    c_print(C_ORANGE, "    • "); printf("文件持久化使用magic header（SPRT + version）\n");
    c_print(C_ORANGE, "    • "); printf("统计面板、TOP学校排行、进度条反馈\n");
    printf("\n");
}

static void print_usage(const char *prog) {
    /* argv[0] may contain a Chinese path encoded in the legacy Windows code
       page.  Only display the ASCII executable name so it cannot be garbled. */
    const char *slash = strrchr(prog, '\\');
    const char *forward_slash = strrchr(prog, '/');
    if (slash || forward_slash) {
        const char *last = (!slash || (forward_slash && forward_slash > slash))
                         ? forward_slash : slash;
        prog = last + 1;
    }

    printf("Sports Meet Scoring System V2.0 - Enhanced Edition\n\n");
    printf("用法:\n");
    printf("  %s                      交互模式（菜单）\n", prog);
    printf("  %s --help / -h          显示帮助\n", prog);
    printf("  %s -f <file.dat>        指定数据文件启动\n", prog);
    printf("  %s -t                   运行内置测试\n", prog);
    printf("  %s --stats <file.dat>   显示数据文件统计信息\n", prog);
}

static int show_file_stats(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) { print_err("无法打开文件"); return 1; }
    DataFileHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) { fclose(fp); print_err("读取文件头失败"); return 1; }
    fclose(fp);

    print_thick_divider();
    c_print(C_BOLD C_CYAN, "  数据文件信息: %s\n", filename);
    print_divider();
    if (memcmp(hdr.magic, DATA_MAGIC, 4) != 0) {
        c_print(C_RED, "  [ERROR] 不是有效的数据文件（magic不匹配: %.4s）\n", hdr.magic);
        print_thick_divider();
        return 1;
    }
    printf("  魔数标识:     "); c_print(C_GREEN, "%c%c%c%c\n", hdr.magic[0],hdr.magic[1],hdr.magic[2],hdr.magic[3]);
    printf("  格式版本:     "); c_print(C_WHITE, "%u\n", hdr.version);
    printf("  学校数量:     "); c_print(C_WHITE, "%d\n", hdr.school_count);
    printf("  项目数量:     "); c_print(C_WHITE, "%d\n", hdr.project_count);
    printf("  成绩记录数:   "); c_print(C_WHITE, "%d\n", hdr.result_count);
    print_thick_divider();
    return 0;
}

static int run_command_line(int argc, char *argv[]) {
    if (argc < 2) return 0;
    if (strcmp(argv[1], "-h")==0 || strcmp(argv[1], "--help")==0) {
        print_usage(argv[0]);
        return 0;
    }
    if (strcmp(argv[1], "-t") == 0) {
        int t = run_tests();
        free_all_data();
        return t;
    }
    if (strcmp(argv[1], "-f") == 0) {
        if (argc < 3) { print_err("用法: -f <file.dat>"); return 1; }
        strncpy(g_data_file, argv[2], MAX_NAME - 1);
        g_data_file[MAX_NAME - 1] = '\0';
        return 0;
    }
    if (strcmp(argv[1], "--stats") == 0) {
        if (argc < 3) { print_err("用法: --stats <file.dat>"); return 1; }
        return show_file_stats(argv[2]);
    }
    print_usage(argv[0]);
    return 1;
}

static void run_menu(void) {
    char choice[64];
    while (1) {
        show_menu();
        printf("  请选择操作: "); fflush(stdout);
        if (fgets(choice, sizeof(choice), stdin) == NULL) break;
        char *nl = strchr(choice, '\n'); if (nl) *nl = '\0';
        if (choice[0] == '\0') continue;
        char c = (char)toupper((unsigned char)choice[0]);
        switch (c) {
        case '1': func_input_result();     break;
        case '2': func_show_all_scores();  break;
        case '3': func_sort_schools();     break;
        case '4': func_query_by_school();  break;
        case '5': func_query_by_project(); break;
        case '6': func_manage_data();      break;
        case '7': show_statistics();       break;
        case '8':
            if (save_to_file(g_data_file) == SPT_OK)
                c_print(C_GREEN, "\n  [OK] 数据已保存到 %s\n\n", g_data_file);
            else print_err("保存失败\n");
            break;
        case '9': {
            SptStatus s = load_from_file(g_data_file);
            if (s == SPT_OK) {
                c_print(C_GREEN, "\n  [OK] 已从 %s 加载数据\n", g_data_file);
                c_print(C_CYAN, "  加载了 %d 所学校、%d 个项目、%d 条成绩\n\n",
                       g_school_cnt, g_project_cnt, g_result_cnt);
            } else if (s == SPT_ERR_OPEN_FILE) {
                c_print(C_YELLOW, "\n  [!] 文件 %s 不存在\n\n", g_data_file);
            } else if (s == SPT_ERR_MAGIC) {
                print_err("文件格式错误（非SPRT格式）\n");
            } else print_err("加载失败\n");
            break;
        }
        case 'T': run_tests(); break;
        case 'H':
        case '?': show_help(); break;
        case '0':
            c_print(C_GREEN, "\n  [OK] 正在保存数据并退出...\n");
            save_to_file(g_data_file);
            c_print(C_GREEN, "  [OK] 数据已保存到 %s\n", g_data_file);
            c_print(C_BOLD C_ORANGE, "  感谢使用，再见！\n\n");
            return;
        default:
            c_print(C_YELLOW, "  [!] 无效选项，请重新输入\n");
            break;
        }
    }
}

/* ======================== 入口 ======================== */

int main(int argc, char *argv[]) {
    enable_vt_mode();

    int cl_ret = run_command_line(argc, argv);
    if (argc > 1) {
        if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--stats") == 0 ||
            strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            free_all_data();
            return cl_ret;
        }
    }

    print_banner();

    if (file_exists(g_data_file)) {
        SptStatus s = load_from_file(g_data_file);
        if (s == SPT_OK) {
            c_print(C_GREEN, "  [OK] 已从 %s 加载历史数据\n", g_data_file);
            c_print(C_CYAN, "  共加载 %d 所学校、%d 个项目、%d 条成绩记录\n\n",
                   g_school_cnt, g_project_cnt, g_result_cnt);
        } else {
            c_print(C_YELLOW, "  [WARN] 加载历史数据失败，将从空白数据开始\n\n");
        }
    } else {
        c_print(C_CYAN, "  [INFO] 未找到数据文件 %s，将从空白数据开始\n\n", g_data_file);
    }

    run_menu();
    free_all_data();
    return 0;
}
