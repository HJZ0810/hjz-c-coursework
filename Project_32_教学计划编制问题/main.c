#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* ========================================================================
 *  教学计划编制系统 V2.0
 *  Teaching Schedule Planning System
 *
 *  功能:
 *    - 基于AOV网拓扑排序算法自动排课
 *    - 支持两种排课策略:均匀分布/集中前置
 *    - 动态内存管理:课程和先修关系均使用动态数组
 *    - SCHDV2魔数文件持久化,二进制格式存储
 *    - 内置18项自测:拓扑排序/环检测/文件I/O
 *    - ANSI彩色终端输出,Windows UTF-8支持
 *    - CLI参数:-h帮助/-t测试/--help/--test
 *
 *  编译: gcc -std=c99 -Wall -Wextra -O2 -o sched.exe main.c
 * ======================================================================== */

#define MAGIC_HEADER        "SCHDV2"
#define MAGIC_LEN           6
#define MAX_ID_LEN          16
#define MAX_NAME_LEN        64
#define MAX_LINE_LEN        256
#define DATA_FILE_V2        "courses.v2.dat"

/* ANSI颜色宏定义 */
#define ANSI_RESET          "\033[0m"
#define ANSI_BOLD           "\033[1m"
#define ANSI_RED            "\033[31m"
#define ANSI_GREEN          "\033[32m"
#define ANSI_YELLOW         "\033[33m"
#define ANSI_BLUE           "\033[34m"
#define ANSI_MAGENTA        "\033[35m"
#define ANSI_CYAN           "\033[36m"
#define ANSI_WHITE          "\033[37m"
#define ANSI_BG_BLUE        "\033[44m"
#define ANSI_BG_GREEN       "\033[42m"
#define ANSI_BG_RED         "\033[41m"

/* ========================================================================
 *  统一错误码枚举
 * ======================================================================== */
typedef enum {
    SCHED_OK                    = 0,
    SCHED_ERR_MEMORY            = -1,
    SCHED_ERR_FILE_OPEN         = -2,
    SCHED_ERR_FILE_READ         = -3,
    SCHED_ERR_FILE_WRITE        = -4,
    SCHED_ERR_FILE_MAGIC        = -5,
    SCHED_ERR_FILE_VERSION      = -6,
    SCHED_ERR_COURSE_NOT_FOUND  = -7,
    SCHED_ERR_COURSE_EXISTS     = -8,
    SCHED_ERR_CREDIT_INVALID    = -9,
    SCHED_ERR_CYCLE_DETECTED    = -10,
    SCHED_ERR_CAPACITY_FULL     = -11,
    SCHED_ERR_SELF_PREREQ       = -12,
    SCHED_ERR_PARAM_INVALID     = -13,
    SCHED_ERR_NO_COURSES        = -14,
    SCHED_ERR_CREDIT_EXCEED     = -15
} SchedStatus;

/* ========================================================================
 *  数据结构定义 - 动态内存版本
 * ======================================================================== */

typedef struct Prereq {
    int course_idx;
} Prereq;

typedef struct {
    char    id[MAX_ID_LEN];
    char    name[MAX_NAME_LEN];
    int     credits;
    Prereq *prereqs;
    int     prereq_count;
    int     prereq_cap;
} Course;

typedef struct {
    Course *courses;
    int     course_count;
    int     course_cap;
    int    *adj;
    int     semester_total;
    int     credit_limit;
} Schedule;

typedef struct {
    int    *sem_courses;
    int     sem_cap;
    int     sem_count;
} Semester;

/* ========================================================================
 *  终端颜色初始化
 * ======================================================================== */
static void terminal_init(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | 0x0004);
#endif
}

static void color_line(void)
{
    printf(ANSI_CYAN "═══════════════════════════════════════════════════════════════" ANSI_RESET "\n");
}

static void color_print_banner(void)
{
    color_line();
    printf(ANSI_BOLD ANSI_CYAN "  教学计划编制系统 V2.0" ANSI_RESET "\n");
    printf(ANSI_CYAN "  Teaching Schedule Planning System" ANSI_RESET "\n");
    color_line();
}

static void color_error(const char *msg)
{
    printf(ANSI_RED "  [错误] %s" ANSI_RESET "\n", msg);
}

static void color_success(const char *msg)
{
    printf(ANSI_GREEN "  [成功] %s" ANSI_RESET "\n", msg);
}

static void color_info(const char *msg)
{
    printf(ANSI_YELLOW "  [信息] %s" ANSI_RESET "\n", msg);
}

/* ========================================================================
 *  动态数组初始化与扩容
 * ======================================================================== */
static SchedStatus sched_init(Schedule *s, int init_cap)
{
    s->course_count = 0;
    s->course_cap = init_cap > 0 ? init_cap : 16;
    s->semester_total = 4;
    s->credit_limit = 20;

    s->courses = (Course *)calloc((size_t)s->course_cap, sizeof(Course));
    if (!s->courses) return SCHED_ERR_MEMORY;

    s->adj = (int *)calloc((size_t)s->course_cap * (size_t)s->course_cap, sizeof(int));
    if (!s->adj) {
        free(s->courses);
        return SCHED_ERR_MEMORY;
    }
    return SCHED_OK;
}

static void sched_free(Schedule *s)
{
    int i;
    if (!s) return;
    for (i = 0; i < s->course_count; i++) {
        free(s->courses[i].prereqs);
    }
    free(s->courses);
    free(s->adj);
    s->courses = NULL;
    s->adj = NULL;
    s->course_count = 0;
    s->course_cap = 0;
}

static SchedStatus sched_ensure_cap(Schedule *s, int new_cap)
{
    int *new_adj;
    Course *new_courses;
    if (new_cap <= s->course_cap) return SCHED_OK;

    new_courses = (Course *)realloc(s->courses, (size_t)new_cap * sizeof(Course));
    if (!new_courses) return SCHED_ERR_MEMORY;
    s->courses = new_courses;
    memset(&s->courses[s->course_cap], 0,
           (size_t)(new_cap - s->course_cap) * sizeof(Course));

    new_adj = (int *)realloc(s->adj, (size_t)new_cap * (size_t)new_cap * sizeof(int));
    if (!new_adj) return SCHED_ERR_MEMORY;

    memset(new_adj, 0, (size_t)new_cap * (size_t)new_cap * sizeof(int));
    {
        int i, j;
        for (i = 0; i < s->course_cap; i++)
            for (j = 0; j < s->course_cap; j++)
                new_adj[i * new_cap + j] = s->adj[i * s->course_cap + j];
    }
    s->adj = new_adj;
    s->course_cap = new_cap;
    return SCHED_OK;
}

static SchedStatus course_ensure_prereq_cap(Course *c, int new_cap)
{
    Prereq *p;
    if (new_cap <= c->prereq_cap) return SCHED_OK;
    p = (Prereq *)realloc(c->prereqs, (size_t)new_cap * sizeof(Prereq));
    if (!p) return SCHED_ERR_MEMORY;
    c->prereqs = p;
    c->prereq_cap = new_cap;
    return SCHED_OK;
}

/* ========================================================================
 *  课程查找与添加
 * ======================================================================== */
static int find_course_idx(const Schedule *s, const char *id)
{
    int i;
    for (i = 0; i < s->course_count; i++)
        if (strcmp(s->courses[i].id, id) == 0) return i;
    return -1;
}

static SchedStatus add_course(Schedule *s, const char *id, const char *name, int credits)
{
    int idx;
    Course *c;
    if (credits <= 0) return SCHED_ERR_CREDIT_INVALID;
    if (find_course_idx(s, id) >= 0) return SCHED_ERR_COURSE_EXISTS;
    if (s->course_count >= s->course_cap) {
        SchedStatus st = sched_ensure_cap(s, s->course_cap * 2);
        if (st != SCHED_OK) return st;
    }
    idx = s->course_count++;
    c = &s->courses[idx];
    memset(c, 0, sizeof(Course));
    strncpy(c->id, id, MAX_ID_LEN - 1);
    c->id[MAX_ID_LEN - 1] = '\0';
    strncpy(c->name, name, MAX_NAME_LEN - 1);
    c->name[MAX_NAME_LEN - 1] = '\0';
    c->credits = credits;
    c->prereqs = NULL;
    c->prereq_count = 0;
    c->prereq_cap = 0;
    return SCHED_OK;
}

static SchedStatus add_prereq(Schedule *s, int course_idx, int prereq_idx)
{
    int i;
    Course *c;
    SchedStatus st;
    if (course_idx < 0 || course_idx >= s->course_count) return SCHED_ERR_PARAM_INVALID;
    if (prereq_idx < 0 || prereq_idx >= s->course_count) return SCHED_ERR_COURSE_NOT_FOUND;
    if (course_idx == prereq_idx) return SCHED_ERR_SELF_PREREQ;

    c = &s->courses[course_idx];
    for (i = 0; i < c->prereq_count; i++)
        if (c->prereqs[i].course_idx == prereq_idx) return SCHED_OK;

    if (c->prereq_count >= c->prereq_cap) {
        int new_cap = c->prereq_cap == 0 ? 4 : c->prereq_cap * 2;
        st = course_ensure_prereq_cap(c, new_cap);
        if (st != SCHED_OK) return st;
    }
    c->prereqs[c->prereq_count].course_idx = prereq_idx;
    c->prereq_count++;
    return SCHED_OK;
}

static void rebuild_adj(Schedule *s)
{
    int i, j;
    memset(s->adj, 0, (size_t)s->course_cap * (size_t)s->course_cap * sizeof(int));
    for (i = 0; i < s->course_count; i++) {
        Course *c = &s->courses[i];
        for (j = 0; j < c->prereq_count; j++) {
            int pi = c->prereqs[j].course_idx;
            s->adj[pi * s->course_cap + i] = 1;
        }
    }
}

/* ========================================================================
 *  拓扑排序与环检测
 * ======================================================================== */
static void calc_in_degree(const Schedule *s, int *in_deg)
{
    int i, j;
    for (i = 0; i < s->course_count; i++) in_deg[i] = 0;
    for (i = 0; i < s->course_count; i++)
        for (j = 0; j < s->course_count; j++)
            if (s->adj[i * s->course_cap + j]) in_deg[j]++;
}

static SchedStatus detect_cycle(const Schedule *s)
{
    int *in_deg, *temp, count = 0, changed, i;
    in_deg = (int *)malloc((size_t)s->course_count * sizeof(int));
    temp = (int *)malloc((size_t)s->course_count * sizeof(int));
    if (!in_deg || !temp) { free(in_deg); free(temp); return SCHED_ERR_MEMORY; }

    calc_in_degree(s, in_deg);
    memcpy(temp, in_deg, (size_t)s->course_count * sizeof(int));

    changed = 1;
    while (changed) {
        changed = 0;
        for (i = 0; i < s->course_count; i++) {
            if (temp[i] == 0) {
                int j;
                temp[i] = -1;
                count++;
                for (j = 0; j < s->course_count; j++)
                    if (s->adj[i * s->course_cap + j]) temp[j]--;
                changed = 1;
            }
        }
    }
    free(in_deg); free(temp);
    return count < s->course_count ? SCHED_ERR_CYCLE_DETECTED : SCHED_OK;
}

/* ========================================================================
 *  两种排课策略
 * ======================================================================== */
static SchedStatus topo_sort_uniform(const Schedule *s, Semester **out_sems, int *out_nsems)
{
    int *in_deg, *done, i, a, b, done_cnt = 0, nsems;
    Semester *sems;
    SchedStatus cyc;

    cyc = detect_cycle(s);
    if (cyc != SCHED_OK) return cyc;

    nsems = s->semester_total;
    sems = (Semester *)calloc((size_t)nsems, sizeof(Semester));
    if (!sems) return SCHED_ERR_MEMORY;
    for (i = 0; i < nsems; i++) {
        sems[i].sem_cap = 16;
        sems[i].sem_count = 0;
        sems[i].sem_courses = (int *)malloc((size_t)sems[i].sem_cap * sizeof(int));
        if (!sems[i].sem_courses) {
            int k;
            for (k = 0; k < i; k++) free(sems[k].sem_courses);
            free(sems);
            return SCHED_ERR_MEMORY;
        }
    }

    in_deg = (int *)malloc((size_t)s->course_count * sizeof(int));
    done = (int *)calloc((size_t)s->course_count, sizeof(int));
    if (!in_deg || !done) { free(in_deg); free(done); free(sems); return SCHED_ERR_MEMORY; }
    calc_in_degree(s, in_deg);

    while (done_cnt < s->course_count) {
        int ready[256], rcnt = 0, credits_ready[256];
        int progress = 0;

        for (i = 0; i < s->course_count; i++)
            if (in_deg[i] == 0 && !done[i]) {
                ready[rcnt] = i;
                credits_ready[rcnt] = s->courses[i].credits;
                rcnt++;
            }
        if (rcnt == 0) break;

        for (a = 0; a < rcnt - 1; a++)
            for (b = 0; b < rcnt - 1 - a; b++)
                if (credits_ready[b] < credits_ready[b + 1]) {
                    int t = ready[b]; ready[b] = ready[b + 1]; ready[b + 1] = t;
                    t = credits_ready[b]; credits_ready[b] = credits_ready[b + 1]; credits_ready[b + 1] = t;
                }

        for (a = 0; a < rcnt; a++) {
            int ci = ready[a], best = -1, min_c = s->credit_limit + 1;
            int sem_credits[256];
            for (i = 0; i < nsems; i++) {
                sem_credits[i] = 0;
                for (b = 0; b < sems[i].sem_count; b++)
                    sem_credits[i] += s->courses[sems[i].sem_courses[b]].credits;
            }
            for (b = 0; b < nsems; b++) {
                if (sem_credits[b] + s->courses[ci].credits <= s->credit_limit
                    && sem_credits[b] < min_c) {
                    min_c = sem_credits[b];
                    best = b;
                }
            }
            if (best >= 0) {
                if (sems[best].sem_count >= sems[best].sem_cap) {
                    int *tmp;
                    sems[best].sem_cap *= 2;
                    tmp = (int *)realloc(sems[best].sem_courses,
                                         (size_t)sems[best].sem_cap * sizeof(int));
                    if (!tmp) continue;
                    sems[best].sem_courses = tmp;
                }
                sems[best].sem_courses[sems[best].sem_count++] = ci;
                done[ci] = 1;
                done_cnt++;
                {
                    int j;
                    for (j = 0; j < s->course_count; j++)
                        if (s->adj[ci * s->course_cap + j]) in_deg[j]--;
                }
                progress = 1;
            }
        }
        if (!progress) break;
    }
    free(in_deg); free(done);
    if (done_cnt != s->course_count) {
        for (i = 0; i < nsems; i++) free(sems[i].sem_courses);
        free(sems);
        return SCHED_ERR_CREDIT_EXCEED;
    }
    *out_sems = sems;
    *out_nsems = nsems;
    return SCHED_OK;
}

static SchedStatus topo_sort_concentrated(const Schedule *s, Semester **out_sems, int *out_nsems)
{
    int *in_deg, *done, i, a, b, done_cnt = 0, nsems;
    Semester *sems;
    SchedStatus cyc;

    cyc = detect_cycle(s);
    if (cyc != SCHED_OK) return cyc;

    nsems = s->semester_total;
    sems = (Semester *)calloc((size_t)nsems, sizeof(Semester));
    if (!sems) return SCHED_ERR_MEMORY;
    for (i = 0; i < nsems; i++) {
        sems[i].sem_cap = 16;
        sems[i].sem_count = 0;
        sems[i].sem_courses = (int *)malloc((size_t)sems[i].sem_cap * sizeof(int));
        if (!sems[i].sem_courses) {
            int k;
            for (k = 0; k < i; k++) free(sems[k].sem_courses);
            free(sems);
            return SCHED_ERR_MEMORY;
        }
    }

    in_deg = (int *)malloc((size_t)s->course_count * sizeof(int));
    done = (int *)calloc((size_t)s->course_count, sizeof(int));
    if (!in_deg || !done) { free(in_deg); free(done); free(sems); return SCHED_ERR_MEMORY; }
    calc_in_degree(s, in_deg);

    while (done_cnt < s->course_count) {
        int ready[256], rcnt = 0, credits_ready[256];
        int progress = 0;

        for (i = 0; i < s->course_count; i++)
            if (in_deg[i] == 0 && !done[i]) {
                ready[rcnt] = i;
                credits_ready[rcnt] = s->courses[i].credits;
                rcnt++;
            }
        if (rcnt == 0) break;

        for (a = 0; a < rcnt - 1; a++)
            for (b = 0; b < rcnt - 1 - a; b++)
                if (credits_ready[b] < credits_ready[b + 1]) {
                    int t = ready[b]; ready[b] = ready[b + 1]; ready[b + 1] = t;
                    t = credits_ready[b]; credits_ready[b] = credits_ready[b + 1]; credits_ready[b + 1] = t;
                }

        for (a = 0; a < rcnt; a++) {
            int ci = ready[a];
            for (b = 0; b < nsems; b++) {
                int sem_cred = 0, j;
                for (j = 0; j < sems[b].sem_count; j++)
                    sem_cred += s->courses[sems[b].sem_courses[j]].credits;
                if (sem_cred + s->courses[ci].credits <= s->credit_limit) {
                    if (sems[b].sem_count >= sems[b].sem_cap) {
                        int *tmp;
                        sems[b].sem_cap *= 2;
                        tmp = (int *)realloc(sems[b].sem_courses,
                                             (size_t)sems[b].sem_cap * sizeof(int));
                        if (!tmp) continue;
                        sems[b].sem_courses = tmp;
                    }
                    sems[b].sem_courses[sems[b].sem_count++] = ci;
                    done[ci] = 1;
                    done_cnt++;
                    {
                        int j;
                        for (j = 0; j < s->course_count; j++)
                            if (s->adj[ci * s->course_cap + j]) in_deg[j]--;
                    }
                    progress = 1;
                    break;
                }
            }
        }
        if (!progress) break;
    }
    free(in_deg); free(done);
    if (done_cnt != s->course_count) {
        for (i = 0; i < nsems; i++) free(sems[i].sem_courses);
        free(sems);
        return SCHED_ERR_CREDIT_EXCEED;
    }
    *out_sems = sems;
    *out_nsems = nsems;
    return SCHED_OK;
}

static void free_semesters(Semester *sems, int n)
{
    int i;
    if (!sems) return;
    for (i = 0; i < n; i++) free(sems[i].sem_courses);
    free(sems);
}

/* ========================================================================
 *  SCHDV2魔数文件持久化
 * ======================================================================== */
static SchedStatus save_to_file(const Schedule *s, const char *fname)
{
    FILE *fp;
    int i, j;
    fp = fopen(fname, "wb");
    if (!fp) return SCHED_ERR_FILE_OPEN;

    fwrite(MAGIC_HEADER, 1, MAGIC_LEN, fp);
    fwrite(&s->course_count, sizeof(int), 1, fp);
    fwrite(&s->semester_total, sizeof(int), 1, fp);
    fwrite(&s->credit_limit, sizeof(int), 1, fp);

    for (i = 0; i < s->course_count; i++) {
        Course *c = &s->courses[i];
        fwrite(c->id, 1, MAX_ID_LEN, fp);
        fwrite(c->name, 1, MAX_NAME_LEN, fp);
        fwrite(&c->credits, sizeof(int), 1, fp);
        fwrite(&c->prereq_count, sizeof(int), 1, fp);
        for (j = 0; j < c->prereq_count; j++) {
            fwrite(&c->prereqs[j].course_idx, sizeof(int), 1, fp);
        }
    }
    fclose(fp);
    return SCHED_OK;
}

static SchedStatus load_from_file(Schedule *s, const char *fname)
{
    FILE *fp;
    char magic[MAGIC_LEN];
    int cc, st, cl, i, j;
    sched_free(s);
    st = sched_init(s, 16);
    if (st != SCHED_OK) return st;

    fp = fopen(fname, "rb");
    if (!fp) return SCHED_ERR_FILE_OPEN;

    if (fread(magic, 1, MAGIC_LEN, fp) != MAGIC_LEN ||
        memcmp(magic, MAGIC_HEADER, MAGIC_LEN) != 0) {
        fclose(fp);
        return SCHED_ERR_FILE_MAGIC;
    }
    if (fread(&cc, sizeof(int), 1, fp) != 1 ||
        fread(&st, sizeof(int), 1, fp) != 1 ||
        fread(&cl, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return SCHED_ERR_FILE_READ;
    }

    s->semester_total = st;
    s->credit_limit = cl;
    if (cc > s->course_cap) {
        SchedStatus es = sched_ensure_cap(s, cc + 8);
        if (es != SCHED_OK) { fclose(fp); return es; }
    }

    for (i = 0; i < cc; i++) {
        Course c;
        int pc;
        memset(&c, 0, sizeof(Course));
        if (fread(c.id, 1, MAX_ID_LEN, fp) != MAX_ID_LEN ||
            fread(c.name, 1, MAX_NAME_LEN, fp) != MAX_NAME_LEN ||
            fread(&c.credits, sizeof(int), 1, fp) != 1 ||
            fread(&pc, sizeof(int), 1, fp) != 1) {
            fclose(fp);
            return SCHED_ERR_FILE_READ;
        }
        c.prereq_cap = pc > 0 ? pc : 4;
        c.prereq_count = pc;
        c.prereqs = (Prereq *)calloc((size_t)c.prereq_cap, sizeof(Prereq));
        if (pc > 0 && c.prereqs) {
            for (j = 0; j < pc; j++) {
                if (fread(&c.prereqs[j].course_idx, sizeof(int), 1, fp) != 1) {
                    free(c.prereqs);
                    fclose(fp);
                    return SCHED_ERR_FILE_READ;
                }
            }
        }
        s->courses[s->course_count++] = c;
    }
    fclose(fp);
    rebuild_adj(s);
    return SCHED_OK;
}

/* ========================================================================
 *  内置12门课示例数据
 * ======================================================================== */
static void load_sample_data(Schedule *s)
{
    static const char *ids[] = {
        "C01","C02","C03","C04","C05","C06",
        "C07","C08","C09","C10","C11","C12"
    };
    static const char *names[] = {
        "程序设计基础","离散数学","数据结构","汇编语言",
        "高级语言","编译原理","操作系统","计算机系统",
        "高等数学","数值分析","算法设计","数值计算"
    };
    static const int credits[] = {2,3,4,3,2,3,4,4,7,5,2,3};
    static const int pt[][5] = {
        {-1,-1,-1,-1,-1},
        {-1,-1,-1,-1,-1},
        {0,1,-1,-1,-1},
        {-1,-1,-1,-1,-1},
        {0,3,-1,-1,-1},
        {2,3,4,-1,-1},
        {2,5,-1,-1,-1},
        {2,5,-1,-1,-1},
        {-1,-1,-1,-1,-1},
        {8,-1,-1,-1,-1},
        {8,9,-1,-1,-1},
        {8,9,-1,-1,-1}
    };
    int i, j;
    sched_free(s);
    sched_init(s, 16);
    for (i = 0; i < 12; i++)
        add_course(s, ids[i], names[i], credits[i]);
    for (i = 0; i < 12; i++) {
        for (j = 0; j < 5 && pt[i][j] >= 0; j++) {
            add_prereq(s, i, pt[i][j]);
        }
    }
    rebuild_adj(s);
}

/* ========================================================================
 *  显示功能
 * ======================================================================== */
static void show_courses(const Schedule *s)
{
    int i, j;
    color_line();
    printf(ANSI_BOLD "  课程列表 (共%d门课,学期数=%d,学分上限=%d)" ANSI_RESET "\n",
           s->course_count, s->semester_total, s->credit_limit);
    color_line();
    if (s->course_count == 0) {
        color_info("暂无课程");
        return;
    }
    printf(ANSI_CYAN "  %-8s %-20s %-8s 先修课程\n" ANSI_RESET,
           "编号","课程名","学分");
    color_line();
    for (i = 0; i < s->course_count; i++) {
        Course *c = &s->courses[i];
        printf("  %-8s %-20s %-8d ", c->id, c->name, c->credits);
        if (c->prereq_count == 0) {
            printf(ANSI_GREEN "无" ANSI_RESET);
        } else {
            for (j = 0; j < c->prereq_count; j++) {
                printf("%s", s->courses[c->prereqs[j].course_idx].id);
                if (j < c->prereq_count - 1) printf(",");
            }
        }
        printf("\n");
    }
}

static void show_schedule_result(const Schedule *s, const Semester *sems, int nsems, const char *name)
{
    int i, j, total = 0, max_c = 0, min_c = 999;
    color_line();
    printf(ANSI_BOLD ANSI_MAGENTA "  %s" ANSI_RESET "\n", name);
    color_line();
    for (i = 0; i < nsems; i++) {
        int sc = 0;
        for (j = 0; j < sems[i].sem_count; j++)
            sc += s->courses[sems[i].sem_courses[j]].credits;
        if (sems[i].sem_count > 0) {
            if (sc > max_c) max_c = sc;
            if (sc < min_c) min_c = sc;
        }
        total += sc;
        printf(ANSI_BOLD ANSI_BLUE "  第%d学期 (共%d门,%d学分):" ANSI_RESET "\n", i+1, sems[i].sem_count, sc);
        for (j = 0; j < sems[i].sem_count; j++) {
            int ci = sems[i].sem_courses[j];
            printf("    %-8s %-20s %d学分\n",
                   s->courses[ci].id, s->courses[ci].name, s->courses[ci].credits);
        }
    }
    color_line();
    printf(ANSI_GREEN "  总学分:%d | 最高学期:%d | 最低学期:%d | 学分差:%d" ANSI_RESET "\n",
           total, max_c, min_c == 999 ? 0 : min_c, max_c - (min_c == 999 ? 0 : min_c));
}

/* ========================================================================
 *  内置18项自测
 * ======================================================================== */
typedef struct {
    const char *name;
    int passed;
} TestCase;

static int run_tests(void)
{
    TestCase tests[18];
    int pass_count = 0, i;
    Schedule s;
    Semester *sems = NULL;
    int nsems = 0;

    printf(ANSI_BOLD ANSI_CYAN "\n  开始运行内置自测...\n" ANSI_RESET);
    color_line();

    /* T01: 初始化 */
    tests[0].name = "T01:系统初始化";
    tests[0].passed = (sched_init(&s, 8) == SCHED_OK);

    /* T02: 添加课程 */
    load_sample_data(&s);
    tests[1].name = "T02:加载12门示例课程";
    tests[1].passed = (s.course_count == 12);

    /* T03: 课程查找 */
    tests[2].name = "T03:课程查找C01";
    tests[2].passed = (find_course_idx(&s, "C01") == 0);

    /* T04: 重复课程检测 */
    tests[3].name = "T04:重复课程检测";
    tests[3].passed = (add_course(&s, "C01", "测试", 2) == SCHED_ERR_COURSE_EXISTS);

    /* T05: 学分无效 */
    tests[4].name = "T05:无效学分(0分)";
    tests[4].passed = (add_course(&s, "C99", "测试", 0) == SCHED_ERR_CREDIT_INVALID);

    /* T06: 无环检测 */
    tests[5].name = "T06:无环DAG检测";
    tests[5].passed = (detect_cycle(&s) == SCHED_OK);

    /* T07: 拓扑排序均匀策略 */
    tests[6].name = "T07:策略A-均匀分布排课";
    tests[6].passed = (topo_sort_uniform(&s, &sems, &nsems) == SCHED_OK);
    if (sems) { free_semesters(sems, nsems); sems = NULL; }

    /* T08: 拓扑排序集中策略 */
    tests[7].name = "T08:策略B-集中前置排课";
    tests[7].passed = (topo_sort_concentrated(&s, &sems, &nsems) == SCHED_OK);
    if (sems) { free_semesters(sems, nsems); sems = NULL; }

    /* T09: 文件保存 */
    tests[8].name = "T09:SCHDV2文件保存";
    tests[8].passed = (save_to_file(&s, "test_sched.dat") == SCHED_OK);

    /* T10: 文件加载 */
    {
        Schedule s2;
        tests[9].name = "T10:SCHDV2文件加载";
        tests[9].passed = (load_from_file(&s2, "test_sched.dat") == SCHED_OK &&
                          s2.course_count == 12);
        sched_free(&s2);
    }

    /* T11: 魔数校验 */
    {
        FILE *fp = fopen("test_bad.dat", "wb");
        const char bad[] = "BAD000";
        fwrite(bad, 1, 6, fp);
        fclose(fp);
        Schedule s2;
        sched_init(&s2, 4);
        tests[10].name = "T11:魔数校验错误";
        tests[10].passed = (load_from_file(&s2, "test_bad.dat") == SCHED_ERR_FILE_MAGIC);
        sched_free(&s2);
        remove("test_bad.dat");
    }

    /* T12: 自环检测 */
    tests[11].name = "T12:自环先修拒绝";
    tests[11].passed = (add_prereq(&s, 0, 0) == SCHED_ERR_SELF_PREREQ);

    /* T13: 构造环检测 */
    {
        Schedule sc;
        sched_init(&sc, 4);
        add_course(&sc, "A", "课A", 2);
        add_course(&sc, "B", "课B", 2);
        add_course(&sc, "C", "课C", 2);
        add_prereq(&sc, 0, 1);
        add_prereq(&sc, 1, 2);
        add_prereq(&sc, 2, 0);
        rebuild_adj(&sc);
        tests[12].name = "T13:环检测(A->B->C->A)";
        tests[12].passed = (detect_cycle(&sc) == SCHED_ERR_CYCLE_DETECTED);
        sched_free(&sc);
    }

    /* T14: 空课程排课错误 */
    {
        Schedule se;
        sched_init(&se, 4);
        tests[13].name = "T14:空系统排课";
        tests[13].passed = (topo_sort_uniform(&se, &sems, &nsems) == SCHED_OK);
        free_semesters(sems, nsems);
        sems = NULL;
        sched_free(&se);
    }

    /* T15: 动态扩容 */
    {
        Schedule sd;
        int ok = 1;
        sched_init(&sd, 2);
        for (i = 0; i < 20; i++) {
            char id[16], nm[32];
            sprintf(id, "T%02d", i);
            sprintf(nm, "测试课程%d", i);
            if (add_course(&sd, id, nm, 2) != SCHED_OK) { ok = 0; break; }
        }
        tests[14].name = "T15:动态数组扩容";
        tests[14].passed = (ok && sd.course_count == 20);
        sched_free(&sd);
    }

    /* T16: 文件不存在 */
    {
        Schedule sn;
        sched_init(&sn, 4);
        tests[15].name = "T16:文件不存在错误";
        tests[15].passed = (load_from_file(&sn, "nonexist_v2.dat") == SCHED_ERR_FILE_OPEN);
        sched_free(&sn);
    }

    /* T17: 持久化后拓扑排序一致性 */
    {
        Schedule s_orig, s_load;
        int r1, r2;
        load_sample_data(&s_orig);
        save_to_file(&s_orig, "test_roundtrip.dat");
        load_from_file(&s_load, "test_roundtrip.dat");
        r1 = detect_cycle(&s_orig);
        r2 = detect_cycle(&s_load);
        tests[16].name = "T17:持久化前后拓扑一致性";
        tests[16].passed = (r1 == SCHED_OK && r2 == SCHED_OK &&
                           s_orig.course_count == s_load.course_count);
        sched_free(&s_orig);
        sched_free(&s_load);
        remove("test_roundtrip.dat");
    }

    /* T18: 两种排课策略都完成所有课程 */
    {
        Semester *sa = NULL, *sb = NULL;
        int na = 0, nb = 0;
        int cnt_a = 0, cnt_b = 0, ra, rb, k, l;
        load_sample_data(&s);
        ra = topo_sort_uniform(&s, &sa, &na);
        rb = topo_sort_concentrated(&s, &sb, &nb);
        if (ra == SCHED_OK) {
            for (k = 0; k < na; k++) cnt_a += sa[k].sem_count;
        }
        if (rb == SCHED_OK) {
            for (l = 0; l < nb; l++) cnt_b += sb[l].sem_count;
        }
        tests[17].name = "T18:两种策略都排完所有12门课";
        tests[17].passed = (ra == SCHED_OK && rb == SCHED_OK &&
                           cnt_a == 12 && cnt_b == 12);
        free_semesters(sa, na);
        free_semesters(sb, nb);
    }

    remove("test_sched.dat");

    for (i = 0; i < 18; i++) {
        if (tests[i].passed) {
            printf(ANSI_GREEN "  [PASS] %s" ANSI_RESET "\n", tests[i].name);
            pass_count++;
        } else {
            printf(ANSI_RED "  [FAIL] %s" ANSI_RESET "\n", tests[i].name);
        }
    }
    color_line();
    printf(ANSI_BOLD "  测试结果: %d/%d 通过" ANSI_RESET "\n", pass_count, 18);
    sched_free(&s);
    return pass_count == 18 ? 0 : 1;
}

/* ========================================================================
 *  交互菜单
 * ======================================================================== */
static void read_line(char *buf, int size)
{
    if (fgets(buf, size, stdin)) {
        buf[strcspn(buf, "\r\n")] = '\0';
    }
}

static void print_help(void)
{
    color_print_banner();
    printf(ANSI_BOLD "  用法:" ANSI_RESET "\n");
    printf("    sched.exe           启动交互菜单\n");
    printf("    sched.exe -h        显示帮助信息\n");
    printf("    sched.exe --help    显示帮助信息\n");
    printf("    sched.exe -t        运行内置自测\n");
    printf("    sched.exe --test    运行内置自测\n");
    color_line();
}

static void interactive_menu(void)
{
    Schedule s;
    char buf[MAX_LINE_LEN];
    int logged_in = 0;
    int login_attempts = 3;

    terminal_init();
    color_print_banner();

    sched_init(&s, 16);
    load_from_file(&s, DATA_FILE_V2);
    if (s.course_count == 0) {
        load_sample_data(&s);
        color_info("已加载12门示例课程");
    } else {
        char msg[128];
        sprintf(msg, "从%s加载了%d门课程", DATA_FILE_V2, s.course_count);
        color_success(msg);
    }

    /* 登录 */
    while (login_attempts > 0 && !logged_in) {
        char user[64], pass[64];
        printf(ANSI_CYAN "  用户名: " ANSI_RESET);
        read_line(user, sizeof(user));
        printf(ANSI_CYAN "  密  码: " ANSI_RESET);
        read_line(pass, sizeof(pass));
        if (strcmp(user, "admin") == 0 && strcmp(pass, "123456") == 0) {
            color_success("登录成功!");
            logged_in = 1;
        } else {
            login_attempts--;
            sprintf(buf, "用户名或密码错误,剩余%d次机会", login_attempts);
            color_error(buf);
        }
    }
    if (!logged_in) {
        color_error("登录失败次数过多,退出");
        sched_free(&s);
        return;
    }

    while (1) {
        int choice;
        printf("\n");
        color_line();
        printf(ANSI_BOLD ANSI_BG_BLUE "  教学计划编制系统 V2.0 - 主菜单" ANSI_RESET "\n");
        color_line();
        printf(ANSI_CYAN "  1. 重置为示例数据\n");
        printf("  2. 查看所有课程\n");
        printf("  3. 添加课程\n");
        printf("  4. 设置学期/学分参数\n");
        printf("  5. 策略A-均匀分布排课\n");
        printf("  6. 策略B-集中前置排课\n");
        printf("  7. 检测先修关系环\n");
        printf("  8. 保存数据到文件\n");
        printf("  0. 退出系统\n" ANSI_RESET);
        color_line();
        printf(ANSI_YELLOW "  请选择: " ANSI_RESET);
        read_line(buf, sizeof(buf));
        choice = atoi(buf);

        switch (choice) {
        case 1:
            load_sample_data(&s);
            color_success("已重置为12门示例课程");
            break;
        case 2:
            show_courses(&s);
            break;
        case 3: {
            char id[MAX_ID_LEN], nm[MAX_NAME_LEN], crbuf[32];
            int cr;
            printf("  课程编号(如C13): "); read_line(id, sizeof(id));
            printf("  课程名称: "); read_line(nm, sizeof(nm));
            printf("  学分: "); read_line(crbuf, sizeof(crbuf));
            cr = atoi(crbuf);
            if (add_course(&s, id, nm, cr) == SCHED_OK) {
                rebuild_adj(&s);
                color_success("课程添加成功");
            } else {
                color_error("课程添加失败(编号可能已存在)");
            }
            break;
        }
        case 4: {
            char b1[32], b2[32];
            printf("  学期总数(当前%d): ", s.semester_total);
            read_line(b1, sizeof(b1));
            printf("  每学期学分上限(当前%d): ", s.credit_limit);
            read_line(b2, sizeof(b2));
            if (strlen(b1) > 0) {
                int v = atoi(b1);
                if (v > 0) s.semester_total = v;
            }
            if (strlen(b2) > 0) {
                int v = atoi(b2);
                if (v > 0) s.credit_limit = v;
            }
            sprintf(buf, "已更新:学期=%d,学分上限=%d", s.semester_total, s.credit_limit);
            color_success(buf);
            break;
        }
        case 5: {
            Semester *sr = NULL; int nr = 0;
            SchedStatus st = topo_sort_uniform(&s, &sr, &nr);
            if (st == SCHED_OK) {
                show_schedule_result(&s, sr, nr, "策略A:均匀分布排课");
                free_semesters(sr, nr);
            } else {
                color_error("排课失败(可能有环或学分上限不足)");
            }
            break;
        }
        case 6: {
            Semester *sr = NULL; int nr = 0;
            SchedStatus st = topo_sort_concentrated(&s, &sr, &nr);
            if (st == SCHED_OK) {
                show_schedule_result(&s, sr, nr, "策略B:集中前置排课");
                free_semesters(sr, nr);
            } else {
                color_error("排课失败(可能有环或学分上限不足)");
            }
            break;
        }
        case 7: {
            SchedStatus st = detect_cycle(&s);
            if (st == SCHED_OK) color_success("先修关系无环,可正常排课");
            else color_error("检测到先修关系存在环!");
            break;
        }
        case 8: {
            if (save_to_file(&s, DATA_FILE_V2) == SCHED_OK) {
                sprintf(buf, "数据已保存到 %s", DATA_FILE_V2);
                color_success(buf);
            } else {
                color_error("保存失败");
            }
            break;
        }
        case 0:
            save_to_file(&s, DATA_FILE_V2);
            color_success("数据已自动保存,感谢使用!");
            sched_free(&s);
            return;
        default:
            color_error("无效选择,请重新输入");
        }
    }
}

/* ========================================================================
 *  主函数: CLI参数解析
 * ======================================================================== */
int main(int argc, char *argv[])
{
    terminal_init();
    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_help();
            return 0;
        }
        if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0) {
            return run_tests();
        }
        print_help();
        return 1;
    }
    interactive_menu();
    return 0;
}
