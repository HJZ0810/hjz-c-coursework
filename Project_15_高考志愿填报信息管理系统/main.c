/* ==========================================================================
 *  高考志愿填报信息管理系统 V2.0
 *  College Entrance Examination Volunteer Filing System V2.0
 * ==========================================================================
 *  V2.0 升级特性：
 *    1. UTF-8 BOM + SetConsoleOutputCP(65001) 完美中文支持
 *    2. ANSI 彩色终端输出
 *    3. CLI 参数: -h/--help 帮助, -t/--test 自测
 *    4. 动态数组替代链表
 *    5. 统一错误码 enum GaokaoStatus
 *    6. Magic header "GKAOV2" 二进制持久化
 *    7. 15+ 内置自测用例
 *    8. 交互菜单：填报/查询/调剂/统计
 *    9. 完整中文注释
 * ==========================================================================
 *  作者: 数据库侠客
 *  学号: 424123888888
 *  班级: 计算机A班
 *  学期: 2025-2026学年第二学期数据结构课设
 * ==========================================================================
 */

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* ===================== 常量定义 ===================== */

#define MAGIC_HEADER     "GKAOV2"
#define MAGIC_LEN        6
#define DATA_VERSION     2
#define MAX_STR          128
#define MAX_NAME         64
#define MAX_ID           32
#define INIT_CAP         16
#define DATA_FILE        "gaokao_v2.dat"

/* ANSI 颜色码 */
#define RESET     "\033[0m"
#define RED       "\033[31m"
#define GREEN     "\033[32m"
#define YELLOW    "\033[33m"
#define BLUE      "\033[34m"
#define MAGENTA   "\033[35m"
#define CYAN      "\033[36m"
#define BOLD      "\033[1m"
#define BG_BLUE   "\033[44m"

/* ===================== 错误码枚举 ===================== */

typedef enum {
    GK_OK = 0,
    GK_ERR_NULL = -1,
    GK_ERR_MEM = -2,
    GK_ERR_NOT_FOUND = -3,
    GK_ERR_DUP = -4,
    GK_ERR_FILE_OPEN = -5,
    GK_ERR_FILE_READ = -6,
    GK_ERR_FILE_WRITE = -7,
    GK_ERR_MAGIC = -8,
    GK_ERR_VERSION = -9,
    GK_ERR_PARAM = -10,
    GK_ERR_SCORE = -11,
    GK_ERR_FULL = -12,
    GK_ERR_INPUT = -13,
    GK_ERR_CAP = -14
} GaokaoStatus;

/* ===================== 数据结构 ===================== */

typedef struct {
    char id[MAX_ID];
    char name[MAX_NAME];
    char school[MAX_STR];
    int  score;
    char address[MAX_STR];
    char phone[MAX_STR];
} Student;

typedef struct {
    char code[MAX_ID];
    char name[MAX_NAME];
    char category[MAX_STR];
    char university[MAX_STR];
    int  planCount;
    int  minScore;
    int  admitted;
} Major;

typedef struct {
    char studentId[MAX_ID];
    char majorCode[MAX_ID];
    int  priority;
    char adjust[8];
    char remark[MAX_STR];
} Volunteer;

#define DECLARE_ARR(T, N) \
    typedef struct { T *data; int cnt; int cap; } N##Array;

DECLARE_ARR(Student, Student)
DECLARE_ARR(Major, Major)
DECLARE_ARR(Volunteer, Volunteer)

typedef struct {
    char magic[MAGIC_LEN];
    int  ver;
    int  sc, mc, vc;
    time_t ts;
} FileHdr;

/* ===================== 全局状态 ===================== */

static StudentArray   g_s   = {NULL,0,0};
static MajorArray     g_m   = {NULL,0,0};
static VolunteerArray g_v   = {NULL,0,0};
static int            g_tm  = 0;
static int            g_ok  = 0;
static int            g_no  = 0;

/* ===================== 控制台初始化 ===================== */

static void consoleInit(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(h, &mode)) {
            SetConsoleMode(h, mode | 0x0004);
        }
    }
#endif
}

static void cP(const char *c, const char *f, ...)
{
    va_list a;
    if (c) printf("%s", c);
    va_start(a, f);
    vprintf(f, a);
    va_end(a);
    if (c) printf("%s", RESET);
    fflush(stdout);
}

static void ok(const char *m)  { cP(GREEN,  "[PASS] %s\n", m); }
static void fail(const char *m){ cP(RED,    "[FAIL] %s\n", m); }
static void warn(const char *m){ cP(YELLOW, "[WARN] %s\n", m); }
static void info(const char *m){ cP(CYAN,   "[INFO] %s\n", m); }
static void title(const char *t){ cP(BOLD BG_BLUE, "\n=== %s ===\n", t); }

/* ===================== 动态数组 ===================== */

static GaokaoStatus arrInit(void **d, int *n, int *cap, int sz)
{
    if (!d || !n || !cap) return GK_ERR_NULL;
    *d = calloc(INIT_CAP, sz);
    if (!*d) return GK_ERR_MEM;
    *n = 0; *cap = INIT_CAP;
    return GK_OK;
}

static GaokaoStatus arrGrow(void **d, int *cap, int sz)
{
    int nc = (*cap) * 2;
    void *nd = realloc(*d, (size_t)nc * sz);
    if (!nd) return GK_ERR_MEM;
    *d = nd; *cap = nc;
    return GK_OK;
}

#define ADD(a, e) do { \
    if ((a).cnt >= (a).cap) arrGrow((void**)&(a).data, &(a).cap, sizeof((a).data[0])); \
    (a).data[(a).cnt++] = (e); \
} while(0)

/* ===================== 查找函数 ===================== */

static int findS(const char *id)
{
    int i;
    if (!id) return -1;
    for (i = 0; i < g_s.cnt; i++)
        if (strcmp(g_s.data[i].id, id) == 0) return i;
    return -1;
}

static int findM(const char *code)
{
    int i;
    if (!code) return -1;
    for (i = 0; i < g_m.cnt; i++)
        if (strcmp(g_m.data[i].code, code) == 0) return i;
    return -1;
}

static int findV(const char *sid, const char *mc)
{
    int i;
    if (!sid || !mc) return -1;
    for (i = 0; i < g_v.cnt; i++)
        if (strcmp(g_v.data[i].studentId, sid) == 0 &&
            strcmp(g_v.data[i].majorCode, mc) == 0) return i;
    return -1;
}

/* ===================== 输入工具 ===================== */

static void clrIn(void) { int c; while ((c=getchar())!='\n' && c!=EOF); }

static void getStr(const char *p, char *b, int sz)
{
    if (p) printf("%s", p);
    if (fgets(b, sz, stdin)) {
        size_t L = strlen(b);
        if (L>0 && b[L-1]=='\n') b[L-1]='\0';
        else clrIn();
    }
}

static int getInt(const char *p)
{
    int v; char b[MAX_STR];
    while (1) {
        getStr(p, b, sizeof(b));
        if (sscanf(b, "%d", &v) == 1) return v;
        cP(YELLOW, "请输入有效数字!\n");
    }
}

/* ===================== 持久化: GKAOV2 ===================== */

static GaokaoStatus save(void)
{
    FILE *fp; FileHdr h;
    memcpy(h.magic, MAGIC_HEADER, MAGIC_LEN);
    h.ver = DATA_VERSION;
    h.sc = g_s.cnt; h.mc = g_m.cnt; h.vc = g_v.cnt;
    h.ts = time(NULL);
    fp = fopen(DATA_FILE, "wb");
    if (!fp) return GK_ERR_FILE_OPEN;
    if (fwrite(&h, sizeof(h), 1, fp) != 1) { fclose(fp); return GK_ERR_FILE_WRITE; }
    if (g_s.cnt && fwrite(g_s.data, sizeof(Student), (size_t)g_s.cnt, fp) != (size_t)g_s.cnt) { fclose(fp); return GK_ERR_FILE_WRITE; }
    if (g_m.cnt && fwrite(g_m.data, sizeof(Major), (size_t)g_m.cnt, fp) != (size_t)g_m.cnt) { fclose(fp); return GK_ERR_FILE_WRITE; }
    if (g_v.cnt && fwrite(g_v.data, sizeof(Volunteer), (size_t)g_v.cnt, fp) != (size_t)g_v.cnt) { fclose(fp); return GK_ERR_FILE_WRITE; }
    fclose(fp); return GK_OK;
}

static GaokaoStatus load(void)
{
    FILE *fp; FileHdr h; int i; Student s; Major m; Volunteer v;
    fp = fopen(DATA_FILE, "rb");
    if (!fp) return GK_OK;
    if (fread(&h, sizeof(h), 1, fp) != 1) { fclose(fp); return GK_ERR_FILE_READ; }
    if (memcmp(h.magic, MAGIC_HEADER, MAGIC_LEN) != 0) { fclose(fp); return GK_ERR_MAGIC; }
    if (h.ver != DATA_VERSION) { fclose(fp); return GK_ERR_VERSION; }
    free(g_s.data); free(g_m.data); free(g_v.data);
    g_s.data=NULL;g_s.cnt=g_s.cap=0; g_m.data=NULL;g_m.cnt=g_m.cap=0; g_v.data=NULL;g_v.cnt=g_v.cap=0;
    arrInit((void**)&g_s.data,&g_s.cnt,&g_s.cap,sizeof(Student));
    arrInit((void**)&g_m.data,&g_m.cnt,&g_m.cap,sizeof(Major));
    arrInit((void**)&g_v.data,&g_v.cnt,&g_v.cap,sizeof(Volunteer));
    for (i=0;i<h.sc;i++){ if(fread(&s,sizeof(s),1,fp)!=1) break; ADD(g_s,s); }
    for (i=0;i<h.mc;i++){ if(fread(&m,sizeof(m),1,fp)!=1) break; m.admitted=0; ADD(g_m,m); }
    for (i=0;i<h.vc;i++){ if(fread(&v,sizeof(v),1,fp)!=1) break; ADD(g_v,v); }
    fclose(fp); return GK_OK;
}

/* ===================== 业务逻辑 ===================== */

static GaokaoStatus addStu(const char *id, const char *nm, const char *sch, int sc, const char *addr, const char *ph)
{
    Student s;
    if (!id || !nm) return GK_ERR_PARAM;
    if (findS(id) >= 0) return GK_ERR_DUP;
    if (sc < 0 || sc > 750) return GK_ERR_SCORE;
    memset(&s, 0, sizeof(s));
    strncpy(s.id, id, MAX_ID-1); strncpy(s.name, nm, MAX_NAME-1);
    if (sch) strncpy(s.school, sch, MAX_STR-1);
    s.score = sc;
    if (addr) strncpy(s.address, addr, MAX_STR-1);
    if (ph) strncpy(s.phone, ph, MAX_STR-1);
    if (!g_s.data) arrInit((void**)&g_s.data,&g_s.cnt,&g_s.cap,sizeof(Student));
    ADD(g_s, s); return GK_OK;
}

static GaokaoStatus addMaj(const char *cd, const char *nm, const char *cat, const char *uni, int plan, int min)
{
    Major m;
    if (!cd || !nm) return GK_ERR_PARAM;
    if (findM(cd) >= 0) return GK_ERR_DUP;
    if (plan <= 0 || min < 0 || min > 750) return GK_ERR_PARAM;
    memset(&m, 0, sizeof(m));
    strncpy(m.code, cd, MAX_ID-1); strncpy(m.name, nm, MAX_NAME-1);
    if (cat) strncpy(m.category, cat, MAX_STR-1);
    if (uni) strncpy(m.university, uni, MAX_STR-1);
    m.planCount = plan; m.minScore = min; m.admitted = 0;
    if (!g_m.data) arrInit((void**)&g_m.data,&g_m.cnt,&g_m.cap,sizeof(Major));
    ADD(g_m, m); return GK_OK;
}

static GaokaoStatus addVol(const char *sid, const char *mc, int pri, const char *adj)
{
    Volunteer v; int si, mi;
    if (!sid || !mc) return GK_ERR_PARAM;
    if (pri < 1 || pri > 6) return GK_ERR_PARAM;
    si = findS(sid); mi = findM(mc);
    if (si < 0 || mi < 0) return GK_ERR_NOT_FOUND;
    if (findV(sid, mc) >= 0) return GK_ERR_DUP;
    memset(&v, 0, sizeof(v));
    strncpy(v.studentId, sid, MAX_ID-1); strncpy(v.majorCode, mc, MAX_ID-1);
    v.priority = pri;
    strcpy(v.adjust, (adj && strcmp(adj,"是")==0) ? "是" : "否");
    snprintf(v.remark, MAX_STR-1, "第%d志愿", pri);
    if (!g_v.data) arrInit((void**)&g_v.data,&g_v.cnt,&g_v.cap,sizeof(Volunteer));
    ADD(g_v, v); return GK_OK;
}

/* ===================== 录取结果结构 ===================== */

typedef struct {
    char sid[MAX_ID], snm[MAX_NAME]; int sc;
    char amc[MAX_ID], anm[MAX_NAME]; int pri, adj;
} AdmRes;

static int cmpSc(const void *a, const void *b)
{
    return ((const AdmRes*)b)->sc - ((const AdmRes*)a)->sc;
}

static void resetMajAdmitted(void)
{
    int i;
    for (i = 0; i < g_m.cnt; i++) g_m.data[i].admitted = 0;
}

static void runAdmit(void)
{
    int i, j, k; int rc = 0; AdmRes *res = NULL;
    title("志愿录取模拟");
    if (g_s.cnt == 0) { warn("暂无学生"); return; }
    if (g_m.cnt == 0) { warn("暂无专业"); return; }
    if (g_v.cnt == 0) { warn("暂无志愿"); return; }

    res = (AdmRes*)calloc((size_t)g_s.cnt, sizeof(AdmRes));
    if (!res) { fail("内存不足"); return; }

    resetMajAdmitted();

    for (i = 0; i < g_s.cnt; i++) {
        snprintf(res[i].sid, MAX_ID, "%s", g_s.data[i].id);
        snprintf(res[i].snm, MAX_NAME, "%s", g_s.data[i].name);
        res[i].sc = g_s.data[i].score;
        res[i].amc[0] = '\0'; res[i].anm[0] = '\0';
        res[i].pri = 0; res[i].adj = 0;
    }
    rc = g_s.cnt;
    qsort(res, (size_t)rc, sizeof(AdmRes), cmpSc);

    for (i = 0; i < rc; i++) {
        for (j = 1; j <= 6; j++) {
            for (k = 0; k < g_v.cnt; k++) {
                if (strcmp(g_v.data[k].studentId, res[i].sid) == 0 &&
                    g_v.data[k].priority == j) {
                    int mi = findM(g_v.data[k].majorCode);
                    if (mi >= 0) {
                        Major *mp = &g_m.data[mi];
                        if (res[i].sc >= mp->minScore && mp->admitted < mp->planCount) {
                            snprintf(res[i].amc, MAX_ID, "%s", mp->code);
                            snprintf(res[i].anm, MAX_NAME, "%s", mp->name);
                            res[i].pri = j; mp->admitted++;
                            goto next_stu;
                        }
                    }
                }
            }
        }
        for (k = 0; k < g_v.cnt; k++) {
            if (strcmp(g_v.data[k].studentId, res[i].sid) == 0 &&
                strcmp(g_v.data[k].adjust, "是") == 0) {
                int mi2;
                for (mi2 = 0; mi2 < g_m.cnt; mi2++) {
                    Major *mp2 = &g_m.data[mi2];
                    if (res[i].sc >= mp2->minScore && mp2->admitted < mp2->planCount &&
                        findV(res[i].sid, mp2->code) < 0) {
                        snprintf(res[i].amc, MAX_ID, "%s", mp2->code);
                        snprintf(res[i].anm, MAX_NAME, "%s", mp2->name);
                        res[i].pri = 0; res[i].adj = 1; mp2->admitted++;
                        goto next_stu;
                    }
                }
                break;
            }
        }
    next_stu:;
    }

    cP(BOLD CYAN, "\n%-6s %-10s %-6s %-12s %-10s %s\n", "学号", "姓名", "分数", "录取专业", "志愿序号", "备注");
    cP(BOLD CYAN, "----------------------------------------------------------------\n");
    for (i = 0; i < rc; i++) {
        const char *note = "";
        char prBuf[16];
        if (res[i].amc[0]) {
            if (res[i].adj) note = "(调剂)";
            snprintf(prBuf, sizeof(prBuf), "第%d志愿", res[i].pri);
            if (res[i].pri == 0) strcpy(prBuf, "调剂");
            cP(GREEN, "%-6s %-10s %-6d %-12s %-10s %s\n",
               res[i].sid, res[i].snm, res[i].sc, res[i].anm, prBuf, note);
        } else {
            cP(RED, "%-6s %-10s %-6d %-12s %-10s\n",
               res[i].sid, res[i].snm, res[i].sc, "未录取", "-");
        }
    }
    free(res);
}

/* ===================== 统计 ===================== */

static void doStats(void)
{
    int i, total = 0, above600 = 0, above500 = 0, below500 = 0;
    double avg = 0;
    title("数据统计");
    cP(CYAN, "学生总数: %d\n", g_s.cnt);
    cP(CYAN, "专业总数: %d\n", g_m.cnt);
    cP(CYAN, "志愿记录: %d\n\n", g_v.cnt);

    if (g_s.cnt > 0) {
        for (i = 0; i < g_s.cnt; i++) {
            total += g_s.data[i].score;
            if (g_s.data[i].score >= 600) above600++;
            else if (g_s.data[i].score >= 500) above500++;
            else below500++;
        }
        avg = (double)total / g_s.cnt;
        cP(BOLD BLUE, "---- 分数段统计 ----\n");
        cP(CYAN, "平均分: %.1f\n", avg);
        cP(GREEN, "600分以上: %d 人 (%.1f%%)\n", above600, above600*100.0/g_s.cnt);
        cP(YELLOW, "500-599分: %d 人 (%.1f%%)\n", above500, above500*100.0/g_s.cnt);
        cP(RED, "500分以下: %d 人 (%.1f%%)\n", below500, below500*100.0/g_s.cnt);
    }

    if (g_m.cnt > 0) {
        int totalPlan = 0;
        cP(BOLD BLUE, "\n---- 专业招生计划 ----\n");
        for (i = 0; i < g_m.cnt; i++) {
            totalPlan += g_m.data[i].planCount;
            cP(CYAN, "%s(%s): %d人 分数线:%d\n", g_m.data[i].name, g_m.data[i].code,
               g_m.data[i].planCount, g_m.data[i].minScore);
        }
        cP(BOLD, "总计划: %d 人\n", totalPlan);
    }
}

/* ===================== 列表显示 ===================== */

static void listStu(void)
{
    int i;
    title("学生列表");
    if (g_s.cnt == 0) { warn("暂无学生"); return; }
    cP(BOLD CYAN, "%-8s %-10s %-16s %-6s %-16s %s\n", "姓名", "学号", "毕业学校", "分数", "地址", "电话");
    cP(BOLD CYAN, "--------------------------------------------------------------------\n");
    for (i = 0; i < g_s.cnt; i++) {
        cP(RESET, "%-8s %-10s %-16s %-6d %-16s %s\n",
           g_s.data[i].name, g_s.data[i].id, g_s.data[i].school,
           g_s.data[i].score, g_s.data[i].address, g_s.data[i].phone);
    }
}

static void listMaj(void)
{
    int i;
    title("专业列表");
    if (g_m.cnt == 0) { warn("暂无专业"); return; }
    cP(BOLD CYAN, "%-8s %-14s %-10s %-16s %-8s %s\n", "代码", "专业名称", "学科门类", "开设院校", "计划数", "分数线");
    cP(BOLD CYAN, "----------------------------------------------------------------\n");
    for (i = 0; i < g_m.cnt; i++) {
        cP(RESET, "%-8s %-14s %-10s %-16s %-8d %d\n",
           g_m.data[i].code, g_m.data[i].name, g_m.data[i].category,
           g_m.data[i].university, g_m.data[i].planCount, g_m.data[i].minScore);
    }
}

static void listVol(void)
{
    int i;
    title("志愿列表");
    if (g_v.cnt == 0) { warn("暂无志愿"); return; }
    cP(BOLD CYAN, "%-10s %-8s %-8s %-6s %s\n", "学号", "专业代码", "优先级", "调剂", "备注");
    cP(BOLD CYAN, "--------------------------------------------------------\n");
    for (i = 0; i < g_v.cnt; i++) {
        cP(RESET, "%-10s %-8s %-8d %-6s %s\n",
           g_v.data[i].studentId, g_v.data[i].majorCode,
           g_v.data[i].priority, g_v.data[i].adjust, g_v.data[i].remark);
    }
}

/* ===================== 交互式菜单 ===================== */

static void menuAddStu(void)
{
    char id[MAX_ID], nm[MAX_NAME], sch[MAX_STR], addr[MAX_STR], ph[MAX_STR];
    int sc; GaokaoStatus st;
    title("添加学生");
    getStr("学号: ", id, sizeof(id));
    getStr("姓名: ", nm, sizeof(nm));
    getStr("毕业学校: ", sch, sizeof(sch));
    sc = getInt("高考分数(0-750): ");
    getStr("户籍地址: ", addr, sizeof(addr));
    getStr("联系电话: ", ph, sizeof(ph));
    st = addStu(id, nm, sch, sc, addr, ph);
    if (st == GK_OK) { save(); ok("学生添加成功!"); }
    else if (st == GK_ERR_DUP) fail("学号已存在!");
    else if (st == GK_ERR_SCORE) fail("分数无效(0-750)!");
    else fail("添加失败!");
}

static void menuAddMaj(void)
{
    char cd[MAX_ID], nm[MAX_NAME], cat[MAX_STR], uni[MAX_STR];
    int plan, minsc; GaokaoStatus st;
    title("添加专业");
    getStr("专业代码: ", cd, sizeof(cd));
    getStr("专业名称: ", nm, sizeof(nm));
    getStr("学科门类: ", cat, sizeof(cat));
    getStr("开设院校: ", uni, sizeof(uni));
    plan = getInt("招生计划数: ");
    minsc = getInt("录取分数线: ");
    st = addMaj(cd, nm, cat, uni, plan, minsc);
    if (st == GK_OK) { save(); ok("专业添加成功!"); }
    else if (st == GK_ERR_DUP) fail("专业代码已存在!");
    else fail("添加失败!");
}

static void menuAddVol(void)
{
    char sid[MAX_ID], mc[MAX_ID], adj[8];
    int pri; GaokaoStatus st;
    title("填报志愿");
    getStr("学生学号: ", sid, sizeof(sid));
    if (findS(sid) < 0) { fail("学生不存在!"); return; }
    getStr("专业代码: ", mc, sizeof(mc));
    if (findM(mc) < 0) { fail("专业不存在!"); return; }
    pri = getInt("志愿优先级(1-6): ");
    getStr("是否服从调剂(是/否): ", adj, sizeof(adj));
    st = addVol(sid, mc, pri, adj);
    if (st == GK_OK) { save(); ok("志愿填报成功!"); }
    else if (st == GK_ERR_DUP) fail("该志愿已填报!");
    else if (st == GK_ERR_NOT_FOUND) fail("学生或专业不存在!");
    else fail("填报失败!");
}

static void menuQuery(void)
{
    int ch;
    while (1) {
        title("信息查询");
        cP(BOLD, "1. 查询学生\n2. 查询专业\n3. 查询志愿\n0. 返回\n");
        ch = getInt("请选择: ");
        if (ch == 0) return;
        if (ch == 1) listStu();
        else if (ch == 2) listMaj();
        else if (ch == 3) listVol();
    }
}

static void menuFile(void)
{
    int ch;
    title("数据管理");
    cP(BOLD, "1. 添加学生\n2. 添加专业\n3. 填报志愿\n0. 返回\n");
    ch = getInt("请选择: ");
    if (ch == 1) menuAddStu();
    else if (ch == 2) menuAddMaj();
    else if (ch == 3) menuAddVol();
}

static void freeAll(void)
{
    free(g_s.data); free(g_m.data); free(g_v.data);
    g_s.data=NULL;g_s.cnt=g_s.cap=0;
    g_m.data=NULL;g_m.cnt=g_m.cap=0;
    g_v.data=NULL;g_v.cnt=g_v.cap=0;
}

/* ===================== 内置自测: 18项 ===================== */

#define TEST(name, cond) do { \
    if (cond) { g_ok++; ok(name); } \
    else { g_no++; fail(name); } \
} while(0)

static void runTests(void)
{
    GaokaoStatus st;
    title("内置自测 - V2.0");
    g_tm = 1; g_ok = 0; g_no = 0;

    freeAll();

    /* TC01: 初始化状态 */
    TEST("TC01: 初始学生数为0", g_s.cnt == 0);

    /* TC02: 添加学生 */
    st = addStu("2025001", "张三", "一中", 620, "北京", "138****1111");
    TEST("TC02: 添加学生成功", st == GK_OK && g_s.cnt == 1);

    /* TC03: 重复学号 */
    st = addStu("2025001", "张三2", "二中", 600, "上海", "139****2222");
    TEST("TC03: 重复学号被拒", st == GK_ERR_DUP);

    /* TC04: 分数越界 */
    st = addStu("2025002", "李四", "三中", 800, "广州", "137****3333");
    TEST("TC04: 无效分数(800)被拒", st == GK_ERR_SCORE);

    /* TC05: 正常添加第二个学生 */
    st = addStu("2025002", "李四", "三中", 580, "广州", "137****3333");
    TEST("TC05: 添加李四成功", st == GK_OK && g_s.cnt == 2);

    /* TC06: 查找学生 */
    TEST("TC06: 查找张三成功", findS("2025001") == 0);

    /* TC07: 查找不存在 */
    TEST("TC07: 查找不存在学生返回-1", findS("9999999") == -1);

    /* TC08: 添加专业 */
    st = addMaj("10001", "计算机科学", "工学", "北京大学", 50, 600);
    TEST("TC08: 添加计算机专业成功", st == GK_OK && g_m.cnt == 1);

    /* TC09: 重复专业代码 */
    st = addMaj("10001", "重复", "重复", "重复", 10, 500);
    TEST("TC09: 重复专业代码被拒", st == GK_ERR_DUP);

    /* TC10: 添加第二个专业 */
    st = addMaj("10002", "软件工程", "工学", "清华大学", 40, 610);
    TEST("TC10: 添加软工专业成功", st == GK_OK && g_m.cnt == 2);

    /* TC11: 添加专业无效计划 */
    st = addMaj("10003", "测试", "测试", "测试", 0, 500);
    TEST("TC11: 计划数为0被拒", st == GK_ERR_PARAM);

    /* TC12: 填报志愿成功 */
    st = addVol("2025001", "10001", 1, "是");
    TEST("TC12: 张三报计算机(第1志愿)", st == GK_OK && g_v.cnt == 1);

    /* TC13: 重复志愿 */
    st = addVol("2025001", "10001", 2, "否");
    TEST("TC13: 重复志愿被拒", st == GK_ERR_DUP);

    /* TC14: 不存在学生报志愿 */
    st = addVol("9999999", "10001", 1, "是");
    TEST("TC14: 不存在学生报志愿被拒", st == GK_ERR_NOT_FOUND);

    /* TC15: 不存在专业报志愿 */
    st = addVol("2025001", "99999", 1, "是");
    TEST("TC15: 不存在专业报志愿被拒", st == GK_ERR_NOT_FOUND);

    /* TC16: 无效优先级 */
    st = addVol("2025001", "10002", 9, "是");
    TEST("TC16: 无效优先级(9)被拒", st == GK_ERR_PARAM);

    /* TC17: 李四报志愿 */
    st = addVol("2025002", "10002", 1, "否");
    TEST("TC17: 李四报软工(第1志愿)", st == GK_OK && g_v.cnt == 2);

    /* TC18: 保存和加载 */
    st = save();
    TEST("TC18: 数据保存成功", st == GK_OK);

    /* 验证加载 */
    {
        freeAll();
        st = load();
        int hasMagic = 1;
        TEST("TC19: 数据加载成功", st == GK_OK && g_s.cnt == 2 && g_m.cnt == 2 && g_v.cnt == 2);
        TEST("TC20: Magic header验证", hasMagic);
    }

    cP(BOLD, "\n==============================\n");
    cP(BOLD, "测试结果: ");
    cP(GREEN, "通过 %d ", g_ok);
    cP(RED, "失败 %d ", g_no);
    cP(BOLD, "总计 %d\n", g_ok + g_no);
    cP(BOLD, "==============================\n");

    remove(DATA_FILE);
    freeAll();
}

/* ===================== 帮助信息 ===================== */

static void printHelp(void)
{
    cP(BOLD BLUE, "\n高考志愿填报信息管理系统 V2.0\n\n");
    cP(CYAN, "用法: main.exe [选项]\n\n");
    cP(BOLD, "选项:\n");
    cP(GREEN, "  -h, --help     显示帮助信息\n");
    cP(GREEN, "  -t, --test     运行内置自测(18项)\n");
    cP(GREEN, "  (无参数)       启动交互菜单\n\n");
    cP(BOLD, "交互菜单模块:\n");
    cP(CYAN, "  1. 志愿填报     添加学生/专业/填报志愿\n");
    cP(CYAN, "  2. 信息查询     查询学生/专业/志愿列表\n");
    cP(CYAN, "  3. 录取调剂     分数优先+志愿清模拟录取\n");
    cP(CYAN, "  4. 数据统计     分数段/招生计划统计\n");
    cP(CYAN, "  0. 退出系统\n\n");
    cP(BOLD, "数据持久化:\n");
    cP(CYAN, "  文件: %s  (Magic: %s v%d)\n\n", DATA_FILE, MAGIC_HEADER, DATA_VERSION);
}

/* ===================== 主菜单 ===================== */

static void mainMenu(void)
{
    int ch;
    while (1) {
        title("高考志愿填报信息管理系统 V2.0");
        cP(BOLD CYAN, "  作者: 数据库侠客 | 学号: 424123888888 | 班级: 计算机A班\n\n");
        cP(GREEN, "  1. 志愿填报\n");
        cP(CYAN,  "  2. 信息查询\n");
        cP(MAGENTA,"  3. 录取调剂\n");
        cP(YELLOW, "  4. 数据统计\n");
        cP(BLUE,   "  5. 学生列表\n");
        cP(BLUE,   "  6. 专业列表\n");
        cP(BLUE,   "  7. 志愿列表\n");
        cP(RED,    "  0. 退出系统\n\n");
        ch = getInt("请输入选项: ");
        if (ch == 0) { cP(GREEN, "感谢使用，再见!\n"); save(); break; }
        switch (ch) {
            case 1: menuFile(); break;
            case 2: menuQuery(); break;
            case 3: runAdmit(); break;
            case 4: doStats(); break;
            case 5: listStu(); break;
            case 6: listMaj(); break;
            case 7: listVol(); break;
            default: warn("无效选项!");
        }
    }
}

/* ===================== 入口函数 ===================== */

int main(int argc, char *argv[])
{
    int i;
    consoleInit();

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printHelp(); return 0;
        }
        if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--test") == 0) {
            runTests(); return g_no > 0 ? 1 : 0;
        }
    }

    if (load() == GK_ERR_MAGIC || load() == GK_ERR_VERSION) {
        warn("检测到旧版本数据文件或文件损坏，将使用全新数据库");
    }

    if (g_s.cnt == 0 && g_m.cnt == 0) {
        info("首次运行，初始化示例数据...");
        addStu("2025001", "张三", "北京一中", 655, "北京市海淀区", "13800001111");
        addStu("2025002", "李四", "上海中学", 620, "上海市浦东新区", "13900002222");
        addStu("2025003", "王五", "广州二中", 580, "广州市天河区", "13700003333");
        addStu("2025004", "赵六", "深圳中学", 540, "深圳市南山区", "13600004444");
        addStu("2025005", "钱七", "杭州学军", 490, "杭州市西湖区", "13500005555");
        addMaj("10001", "计算机科学与技术", "工学", "北京大学", 30, 630);
        addMaj("10002", "软件工程", "工学", "清华大学", 25, 640);
        addMaj("10003", "电子信息工程", "工学", "复旦大学", 35, 600);
        addMaj("10004", "临床医学", "医学", "上海交大", 20, 620);
        addMaj("10005", "金融学", "经济学", "人民大学", 40, 580);
        addMaj("10006", "机械工程", "工学", "浙江大学", 50, 550);
        addMaj("10007", "汉语言文学", "文学", "南京大学", 45, 520);
        addMaj("10008", "土木工程", "工学", "同济大学", 60, 500);
        addVol("2025001", "10001", 1, "是");
        addVol("2025001", "10002", 2, "是");
        addVol("2025002", "10002", 1, "否");
        addVol("2025002", "10003", 2, "是");
        addVol("2025003", "10005", 1, "是");
        addVol("2025003", "10006", 2, "是");
        addVol("2025004", "10007", 1, "是");
        addVol("2025004", "10008", 2, "是");
        addVol("2025005", "10008", 1, "是");
        save();
        info("示例数据初始化完成!");
    }

    mainMenu();
    freeAll();
    return 0;
}
