/*
 * ╔══════════════════════════════════════════════════════════════╗
 * ║                                                              ║
 * ║              看 病 预 约 信 息 管 理 系 统  V2.0             ║
 * ║         Medical Appointment Information Management System    ║
 * ║                                                              ║
 * ╠══════════════════════════════════════════════════════════════╣
 * ║                                                              ║
 * ║  课程名称：数据结构                                          ║
 * ║  开课学院：计算机学院                                        ║
 * ║  学生学号：424123888888                                      ║
 * ║  班级：计算机科学与技术A班                                   ║
 * ║  作者：HuiJunzhuo                                            ║
 * ║  学年学期：2025-2026学年第二学期数据结构课程实习             ║
 * ║                                                              ║
 * ╠══════════════════════════════════════════════════════════════╣
 * ║  V2.0 新特性：                                               ║
 * ║  1. UTF-8 BOM 编码 + SetConsoleOutputCP(65001)               ║
 * ║  2. ANSI 彩色终端输出                                        ║
 * ║  3. CLI 参数支持 (-h/--help/-t/--test)                       ║
 * ║  4. 动态数组存储（医生+患者+预约记录）                       ║
 * ║  5. 统一错误码 enum(HospStatus)                              ║
 * ║  6. Magic Header "HOSPV2" 二进制文件持久化                   ║
 * ║  7. 内置自测框架（15+测试用例）                              ║
 * ║  8. 交互菜单（预约/取消/查询/统计）                          ║
 * ║                                                              ║
 * ╚══════════════════════════════════════════════════════════════╝
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* ======================== 常量定义 ======================== */

#define MAX_NAME        50
#define MAX_ID          20
#define MAX_GENDER      10
#define MAX_JOB         30
#define MAX_ADDR        100
#define MAX_SPECIALTY   50
#define MAX_DEPT        30
#define MAX_DATE        20
#define MAX_DESC        200
#define MAX_NOTE        200
#define INIT_CAPACITY   8
#define DATA_FILE       "hosp_v2.dat"
#define MAGIC_HEADER    "HOSPV2"
#define MAGIC_LEN       6

/* ======================== 错误码枚举 ======================== */

typedef enum {
    HOSP_OK                 = 0,
    HOSP_ERR_MEMORY         = -1,
    HOSP_ERR_NOT_FOUND      = -2,
    HOSP_ERR_DUPLICATE      = -3,
    HOSP_ERR_INVALID_PARAM  = -4,
    HOSP_ERR_FILE_OPEN      = -5,
    HOSP_ERR_FILE_READ      = -6,
    HOSP_ERR_FILE_WRITE     = -7,
    HOSP_ERR_FILE_MAGIC     = -8,
    HOSP_ERR_FILE_VERSION   = -9,
    HOSP_ERR_EMPTY          = -10,
    HOSP_ERR_PATIENT_EXISTS = -11,
    HOSP_ERR_DOCTOR_EXISTS  = -12,
    HOSP_ERR_INVALID_ID     = -13
} HospStatus;

/* ======================== ANSI 颜色宏 ======================== */

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
#define C_BG_RED    "\033[41m"
#define C_BG_GREEN  "\033[42m"
#define C_BG_BLUE   "\033[44m"

#define COLOR_OK     C_GREEN
#define COLOR_ERR    C_RED
#define COLOR_WARN   C_YELLOW
#define COLOR_INFO   C_CYAN
#define COLOR_TITLE  C_BOLD C_CYAN
#define COLOR_HEADER C_BOLD C_WHITE
#define MENU_COLOR   C_BOLD C_MAGENTA

/* ======================== 数据结构定义 ======================== */

typedef struct {
    char id[MAX_ID];
    char name[MAX_NAME];
    int  age;
    char gender[MAX_GENDER];
    char job[MAX_JOB];
    char addr[MAX_ADDR];
} Patient;

typedef struct {
    char id[MAX_ID];
    char name[MAX_NAME];
    char specialty[MAX_SPECIALTY];
    char dept[MAX_DEPT];
    int  years;
} Doctor;

typedef struct {
    int  id;
    char patientId[MAX_ID];
    char doctorId[MAX_ID];
    char datetime[MAX_DATE];
    char desc[MAX_DESC];
    char note[MAX_NOTE];
    int  status;
} Appointment;

#define APPT_STATUS_ACTIVE   1
#define APPT_STATUS_CANCELED 0

/* ======================== 动态数组结构 ======================== */

typedef struct {
    void   **data;
    size_t   size;
    size_t   capacity;
    size_t   elemSize;
} DynArray;

/* ======================== 全局系统状态 ======================== */

typedef struct {
    DynArray patients;
    DynArray doctors;
    DynArray appointments;
    int      nextApptId;
    int      dataModified;
} HospSystem;

static HospSystem g_sys;

/* ======================== 工具函数声明 ======================== */

static void  clearInput(void);
static void  readLine(char *buf, int size);
static void  printSeparator(void);
static void  printTitle(void);
static void  cPrint(const char *color, const char *fmt, ...);
static void  initConsole(void);
static const char* hospStatusStr(HospStatus s);

/* ======================== 动态数组函数声明 ======================== */

static HospStatus arrInit(DynArray *arr, size_t elemSize, size_t initCap);
static HospStatus arrPush(DynArray *arr, const void *elem);
static void*      arrGet(const DynArray *arr, size_t index);
static HospStatus arrRemove(DynArray *arr, size_t index);
static int        arrFind(const DynArray *arr, const void *key,
                          int (*cmp)(const void*, const void*));
static void       arrFree(DynArray *arr);
static size_t     arrSize(const DynArray *arr);

/* ======================== 业务函数声明 ======================== */

static HospStatus sysInit(void);
static void       sysShutdown(void);
static HospStatus sysLoad(void);
static HospStatus sysSave(void);

static HospStatus patientAdd(const char *id, const char *name, int age,
                             const char *gender, const char *job, const char *addr);
static Patient*   patientFind(const char *id);
static int        patientFindIndex(const char *id);
static HospStatus patientDelete(const char *id);
static void       patientList(void);

static HospStatus doctorAdd(const char *id, const char *name,
                            const char *specialty, const char *dept, int years);
static Doctor*    doctorFind(const char *id);
static int        doctorFindIndex(const char *id);
static HospStatus doctorDelete(const char *id);
static void       doctorList(void);

static HospStatus apptAdd(const char *patientId, const char *doctorId,
                          const char *datetime, const char *desc, const char *note, int *outId);
static HospStatus apptCancel(int apptId);
static HospStatus apptFind(int apptId, int *outIdx);
static void       apptList(void);
static void       apptListByPatient(const char *patientId);
static void       apptListByDoctor(const char *doctorId);

static void       menuMain(void);
static void       menuPatient(void);
static void       menuDoctor(void);
static void       menuAppointment(void);
static void       menuStatistics(void);

/* ======================== 自测相关声明 ======================== */

typedef struct {
    const char *name;
    int (*func)(void);
} TestCase;

static int  testRunAll(void);
static int  test_arr_basic(void);
static int  test_arr_grow(void);
static int  test_sys_init_shutdown(void);
static int  test_patient_add(void);
static int  test_patient_duplicate(void);
static int  test_patient_find(void);
static int  test_patient_delete(void);
static int  test_doctor_add(void);
static int  test_doctor_duplicate(void);
static int  test_doctor_find(void);
static int  test_doctor_delete(void);
static int  test_appt_add_valid(void);
static int  test_appt_add_invalid_patient(void);
static int  test_appt_add_invalid_doctor(void);
static int  test_appt_cancel(void);
static int  test_appt_find(void);
static int  test_file_persistence(void);
static int  test_error_codes(void);

static int   g_testsPassed = 0;
static int   g_testsFailed = 0;
static void  testAssert(int condition, const char *msg, const char *file, int line);
#define TEST_ASSERT(cond) testAssert((cond) != 0, #cond, __FILE__, __LINE__)
#define TEST_ASSERT_EQ(a, b) testAssert((a) == (b), #a " == " #b, __FILE__, __LINE__)
#define TEST_ASSERT_STR(a, b) testAssert(strcmp((a), (b)) == 0, #a " == " #b, __FILE__, __LINE__)
#define TEST_ASSERT_STATUS_OK(s) testAssert((s) == HOSP_OK, hospStatusStr(s), __FILE__, __LINE__)

/* ======================== CLI 处理 ======================== */

static void printHelp(const char *progName)
{
    printf("用法: %s [选项]\n\n", progName);
    printf("选项:\n");
    printf("  -h, --help      显示帮助信息\n");
    printf("  -t, --test      运行内置自测套件\n");
    printf("  (无参数)        启动交互菜单模式\n");
    printf("\n");
    printf("看病预约信息管理系统 V2.0\n");
}

/* ======================== 工具函数实现 ======================== */

static void initConsole(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | 0x0004);
#endif
}

static void clearInput(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

static void readLine(char *buf, int size)
{
    if (fgets(buf, size, stdin) != NULL) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
        } else {
            clearInput();
        }
    }
}

static void printSeparator(void)
{
    printf("%s", C_DIM);
    printf("────────────────────────────────────────────────────────────\n");
    printf("%s", C_RESET);
}

static void printTitle(void)
{
    printf("\n");
    cPrint(COLOR_TITLE, "╔══════════════════════════════════════════════════════════╗\n");
    cPrint(COLOR_TITLE, "║          看 病 预 约 信 息 管 理 系 统  V2.0             ║\n");
    cPrint(COLOR_TITLE, "║     Medical Appointment Management System V2.0           ║\n");
    cPrint(COLOR_TITLE, "╚══════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

static void cPrint(const char *color, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    printf("%s", color);
    vprintf(fmt, args);
    printf("%s", C_RESET);
    va_end(args);
}

static const char* hospStatusStr(HospStatus s)
{
    switch (s) {
    case HOSP_OK:                 return "操作成功";
    case HOSP_ERR_MEMORY:         return "内存分配失败";
    case HOSP_ERR_NOT_FOUND:      return "记录未找到";
    case HOSP_ERR_DUPLICATE:      return "编号重复";
    case HOSP_ERR_INVALID_PARAM:  return "无效参数";
    case HOSP_ERR_FILE_OPEN:      return "无法打开文件";
    case HOSP_ERR_FILE_READ:      return "文件读取错误";
    case HOSP_ERR_FILE_WRITE:     return "文件写入错误";
    case HOSP_ERR_FILE_MAGIC:     return "文件魔数不匹配";
    case HOSP_ERR_FILE_VERSION:   return "文件版本不兼容";
    case HOSP_ERR_EMPTY:          return "数据为空";
    case HOSP_ERR_PATIENT_EXISTS: return "患者已存在";
    case HOSP_ERR_DOCTOR_EXISTS:  return "医生已存在";
    case HOSP_ERR_INVALID_ID:     return "无效编号";
    default:                      return "未知错误";
    }
}

/* ======================== 动态数组实现 ======================== */

static HospStatus arrInit(DynArray *arr, size_t elemSize, size_t initCap)
{
    if (!arr || elemSize == 0) return HOSP_ERR_INVALID_PARAM;
    memset(arr, 0, sizeof(DynArray));
    arr->elemSize = elemSize;
    if (initCap == 0) initCap = INIT_CAPACITY;

    arr->data = (void**)malloc(initCap * sizeof(void*));
    if (!arr->data) return HOSP_ERR_MEMORY;
    arr->capacity = initCap;
    memset(arr->data, 0, initCap * sizeof(void*));
    return HOSP_OK;
}

static HospStatus arrPush(DynArray *arr, const void *elem)
{
    if (!arr || !elem) return HOSP_ERR_INVALID_PARAM;

    if (arr->size >= arr->capacity) {
        size_t newCap = arr->capacity * 2;
        void **newData = (void**)realloc(arr->data, newCap * sizeof(void*));
        if (!newData) return HOSP_ERR_MEMORY;
        arr->data = newData;
        arr->capacity = newCap;
    }

    void *copy = malloc(arr->elemSize);
    if (!copy) return HOSP_ERR_MEMORY;
    memcpy(copy, elem, arr->elemSize);
    arr->data[arr->size++] = copy;
    return HOSP_OK;
}

static void* arrGet(const DynArray *arr, size_t index)
{
    if (!arr || index >= arr->size) return NULL;
    return arr->data[index];
}

static HospStatus arrRemove(DynArray *arr, size_t index)
{
    if (!arr || index >= arr->size) return HOSP_ERR_INVALID_PARAM;
    free(arr->data[index]);
    for (size_t i = index; i < arr->size - 1; i++) {
        arr->data[i] = arr->data[i + 1];
    }
    arr->size--;
    arr->data[arr->size] = NULL;
    return HOSP_OK;
}

static int arrFind(const DynArray *arr, const void *key,
                   int (*cmp)(const void*, const void*))
{
    if (!arr || !key || !cmp) return -1;
    for (size_t i = 0; i < arr->size; i++) {
        if (cmp(arr->data[i], key) == 0) return (int)i;
    }
    return -1;
}

static void arrFree(DynArray *arr)
{
    if (!arr || !arr->data) return;
    for (size_t i = 0; i < arr->size; i++) {
        free(arr->data[i]);
        arr->data[i] = NULL;
    }
    free(arr->data);
    arr->data = NULL;
    arr->size = 0;
    arr->capacity = 0;
}

static size_t arrSize(const DynArray *arr)
{
    return arr ? arr->size : 0;
}

/* ======================== 系统初始化/持久化 ======================== */

static HospStatus sysInit(void)
{
    HospStatus st;
    st = arrInit(&g_sys.patients, sizeof(Patient), INIT_CAPACITY);
    if (st != HOSP_OK) return st;
    st = arrInit(&g_sys.doctors, sizeof(Doctor), INIT_CAPACITY);
    if (st != HOSP_OK) { arrFree(&g_sys.patients); return st; }
    st = arrInit(&g_sys.appointments, sizeof(Appointment), INIT_CAPACITY);
    if (st != HOSP_OK) {
        arrFree(&g_sys.patients);
        arrFree(&g_sys.doctors);
        return st;
    }
    g_sys.nextApptId = 1;
    g_sys.dataModified = 0;
    return HOSP_OK;
}

static void sysShutdown(void)
{
    if (g_sys.dataModified) {
        sysSave();
    }
    arrFree(&g_sys.patients);
    arrFree(&g_sys.doctors);
    arrFree(&g_sys.appointments);
}

static HospStatus sysSave(void)
{
    FILE *fp = fopen(DATA_FILE, "wb");
    if (!fp) return HOSP_ERR_FILE_OPEN;

    fwrite(MAGIC_HEADER, 1, MAGIC_LEN, fp);
    int version = 2;
    fwrite(&version, sizeof(int), 1, fp);

    size_t pCount = arrSize(&g_sys.patients);
    size_t dCount = arrSize(&g_sys.doctors);
    size_t aCount = arrSize(&g_sys.appointments);
    fwrite(&pCount, sizeof(size_t), 1, fp);
    fwrite(&dCount, sizeof(size_t), 1, fp);
    fwrite(&aCount, sizeof(size_t), 1, fp);
    fwrite(&g_sys.nextApptId, sizeof(int), 1, fp);

    for (size_t i = 0; i < pCount; i++) {
        Patient *p = (Patient*)arrGet(&g_sys.patients, i);
        if (p) fwrite(p, sizeof(Patient), 1, fp);
    }
    for (size_t i = 0; i < dCount; i++) {
        Doctor *d = (Doctor*)arrGet(&g_sys.doctors, i);
        if (d) fwrite(d, sizeof(Doctor), 1, fp);
    }
    for (size_t i = 0; i < aCount; i++) {
        Appointment *a = (Appointment*)arrGet(&g_sys.appointments, i);
        if (a) fwrite(a, sizeof(Appointment), 1, fp);
    }

    fclose(fp);
    g_sys.dataModified = 0;
    return HOSP_OK;
}

static HospStatus sysLoad(void)
{
    FILE *fp = fopen(DATA_FILE, "rb");
    if (!fp) return HOSP_OK;

    char magic[MAGIC_LEN + 1] = {0};
    if (fread(magic, 1, MAGIC_LEN, fp) != MAGIC_LEN) {
        fclose(fp);
        return HOSP_ERR_FILE_READ;
    }
    if (strcmp(magic, MAGIC_HEADER) != 0) {
        fclose(fp);
        return HOSP_ERR_FILE_MAGIC;
    }

    int version = 0;
    if (fread(&version, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return HOSP_ERR_FILE_READ;
    }
    if (version != 2) {
        fclose(fp);
        return HOSP_ERR_FILE_VERSION;
    }

    size_t pCount = 0, dCount = 0, aCount = 0;
    fread(&pCount, sizeof(size_t), 1, fp);
    fread(&dCount, sizeof(size_t), 1, fp);
    fread(&aCount, sizeof(size_t), 1, fp);
    fread(&g_sys.nextApptId, sizeof(int), 1, fp);

    for (size_t i = 0; i < pCount; i++) {
        Patient p;
        if (fread(&p, sizeof(Patient), 1, fp) == 1) {
            arrPush(&g_sys.patients, &p);
        }
    }
    for (size_t i = 0; i < dCount; i++) {
        Doctor d;
        if (fread(&d, sizeof(Doctor), 1, fp) == 1) {
            arrPush(&g_sys.doctors, &d);
        }
    }
    for (size_t i = 0; i < aCount; i++) {
        Appointment a;
        if (fread(&a, sizeof(Appointment), 1, fp) == 1) {
            arrPush(&g_sys.appointments, &a);
        }
    }

    fclose(fp);
    g_sys.dataModified = 0;
    return HOSP_OK;
}

/* ======================== 患者业务实现 ======================== */

static int cmpPatientId(const void *a, const void *b)
{
    const Patient *p = (const Patient*)a;
    const char *id = (const char*)b;
    return strcmp(p->id, id);
}

static int patientFindIndex(const char *id)
{
    return arrFind(&g_sys.patients, id, cmpPatientId);
}

static Patient* patientFind(const char *id)
{
    int idx = patientFindIndex(id);
    if (idx < 0) return NULL;
    return (Patient*)arrGet(&g_sys.patients, (size_t)idx);
}

static HospStatus patientAdd(const char *id, const char *name, int age,
                             const char *gender, const char *job, const char *addr)
{
    if (!id || !name || !gender || !job || !addr) return HOSP_ERR_INVALID_PARAM;
    if (strlen(id) == 0 || strlen(name) == 0) return HOSP_ERR_INVALID_PARAM;
    if (age < 0 || age > 150) return HOSP_ERR_INVALID_PARAM;

    if (patientFind(id) != NULL) return HOSP_ERR_DUPLICATE;

    Patient p;
    memset(&p, 0, sizeof(p));
    strncpy(p.id, id, MAX_ID - 1);
    strncpy(p.name, name, MAX_NAME - 1);
    p.age = age;
    strncpy(p.gender, gender, MAX_GENDER - 1);
    strncpy(p.job, job, MAX_JOB - 1);
    strncpy(p.addr, addr, MAX_ADDR - 1);

    HospStatus st = arrPush(&g_sys.patients, &p);
    if (st == HOSP_OK) g_sys.dataModified = 1;
    return st;
}

static HospStatus patientDelete(const char *id)
{
    int idx = patientFindIndex(id);
    if (idx < 0) return HOSP_ERR_NOT_FOUND;
    HospStatus st = arrRemove(&g_sys.patients, (size_t)idx);
    if (st == HOSP_OK) g_sys.dataModified = 1;
    return st;
}

static void patientList(void)
{
    size_t count = arrSize(&g_sys.patients);
    if (count == 0) {
        cPrint(COLOR_WARN, "  暂无患者记录\n");
        return;
    }
    cPrint(COLOR_HEADER, "  %-12s %-10s %-6s %-6s %-12s %-20s\n",
           "患者编号", "姓名", "年龄", "性别", "职业", "地址");
    printSeparator();
    for (size_t i = 0; i < count; i++) {
        Patient *p = (Patient*)arrGet(&g_sys.patients, i);
        if (p) {
            printf("  %-12s %-10s %-6d %-6s %-12s %-20s\n",
                   p->id, p->name, p->age, p->gender, p->job, p->addr);
        }
    }
    printSeparator();
    cPrint(COLOR_INFO, "  共 %zu 条患者记录\n", count);
}

/* ======================== 医生业务实现 ======================== */

static int cmpDoctorId(const void *a, const void *b)
{
    const Doctor *d = (const Doctor*)a;
    const char *id = (const char*)b;
    return strcmp(d->id, id);
}

static int doctorFindIndex(const char *id)
{
    return arrFind(&g_sys.doctors, id, cmpDoctorId);
}

static Doctor* doctorFind(const char *id)
{
    int idx = doctorFindIndex(id);
    if (idx < 0) return NULL;
    return (Doctor*)arrGet(&g_sys.doctors, (size_t)idx);
}

static HospStatus doctorAdd(const char *id, const char *name,
                            const char *specialty, const char *dept, int years)
{
    if (!id || !name || !specialty || !dept) return HOSP_ERR_INVALID_PARAM;
    if (strlen(id) == 0 || strlen(name) == 0) return HOSP_ERR_INVALID_PARAM;
    if (years < 0) return HOSP_ERR_INVALID_PARAM;

    if (doctorFind(id) != NULL) return HOSP_ERR_DUPLICATE;

    Doctor d;
    memset(&d, 0, sizeof(d));
    strncpy(d.id, id, MAX_ID - 1);
    strncpy(d.name, name, MAX_NAME - 1);
    strncpy(d.specialty, specialty, MAX_SPECIALTY - 1);
    strncpy(d.dept, dept, MAX_DEPT - 1);
    d.years = years;

    HospStatus st = arrPush(&g_sys.doctors, &d);
    if (st == HOSP_OK) g_sys.dataModified = 1;
    return st;
}

static HospStatus doctorDelete(const char *id)
{
    int idx = doctorFindIndex(id);
    if (idx < 0) return HOSP_ERR_NOT_FOUND;
    HospStatus st = arrRemove(&g_sys.doctors, (size_t)idx);
    if (st == HOSP_OK) g_sys.dataModified = 1;
    return st;
}

static void doctorList(void)
{
    size_t count = arrSize(&g_sys.doctors);
    if (count == 0) {
        cPrint(COLOR_WARN, "  暂无医生记录\n");
        return;
    }
    cPrint(COLOR_HEADER, "  %-12s %-10s %-16s %-12s %-10s\n",
           "医生编号", "姓名", "专长", "科室", "从业年限");
    printSeparator();
    for (size_t i = 0; i < count; i++) {
        Doctor *d = (Doctor*)arrGet(&g_sys.doctors, i);
        if (d) {
            printf("  %-12s %-10s %-16s %-12s %-10d\n",
                   d->id, d->name, d->specialty, d->dept, d->years);
        }
    }
    printSeparator();
    cPrint(COLOR_INFO, "  共 %zu 条医生记录\n", count);
}

/* ======================== 预约业务实现 ======================== */

static HospStatus apptFind(int apptId, int *outIdx)
{
    for (size_t i = 0; i < arrSize(&g_sys.appointments); i++) {
        Appointment *a = (Appointment*)arrGet(&g_sys.appointments, i);
        if (a && a->id == apptId) {
            if (outIdx) *outIdx = (int)i;
            return HOSP_OK;
        }
    }
    return HOSP_ERR_NOT_FOUND;
}

static HospStatus apptAdd(const char *patientId, const char *doctorId,
                          const char *datetime, const char *desc, const char *note, int *outId)
{
    if (!patientId || !doctorId || !datetime) return HOSP_ERR_INVALID_PARAM;
    if (!patientFind(patientId)) return HOSP_ERR_NOT_FOUND;
    if (!doctorFind(doctorId)) return HOSP_ERR_NOT_FOUND;

    Appointment a;
    memset(&a, 0, sizeof(a));
    a.id = g_sys.nextApptId++;
    strncpy(a.patientId, patientId, MAX_ID - 1);
    strncpy(a.doctorId, doctorId, MAX_ID - 1);
    strncpy(a.datetime, datetime, MAX_DATE - 1);
    if (desc) strncpy(a.desc, desc, MAX_DESC - 1);
    if (note) strncpy(a.note, note, MAX_NOTE - 1);
    a.status = APPT_STATUS_ACTIVE;

    HospStatus st = arrPush(&g_sys.appointments, &a);
    if (st == HOSP_OK) {
        g_sys.dataModified = 1;
        if (outId) *outId = a.id;
    }
    return st;
}

static HospStatus apptCancel(int apptId)
{
    int idx = -1;
    if (apptFind(apptId, &idx) != HOSP_OK) return HOSP_ERR_NOT_FOUND;
    Appointment *a = (Appointment*)arrGet(&g_sys.appointments, (size_t)idx);
    if (!a) return HOSP_ERR_NOT_FOUND;
    if (a->status == APPT_STATUS_CANCELED) return HOSP_ERR_INVALID_PARAM;
    a->status = APPT_STATUS_CANCELED;
    g_sys.dataModified = 1;
    return HOSP_OK;
}

static void apptList(void)
{
    size_t count = arrSize(&g_sys.appointments);
    if (count == 0) {
        cPrint(COLOR_WARN, "  暂无预约记录\n");
        return;
    }
    cPrint(COLOR_HEADER, "  %-8s %-12s %-12s %-18s %-8s %-20s\n",
           "预约号", "患者编号", "医生编号", "预约时间", "状态", "病情描述");
    printSeparator();
    for (size_t i = 0; i < count; i++) {
        Appointment *a = (Appointment*)arrGet(&g_sys.appointments, i);
        if (a) {
            const char *statusStr = (a->status == APPT_STATUS_ACTIVE) ? "有效" : "已取消";
            const char *statusColor = (a->status == APPT_STATUS_ACTIVE) ? C_GREEN : C_RED;
            printf("  %-8d %-12s %-12s %-18s ", a->id, a->patientId, a->doctorId, a->datetime);
            printf("%s%-8s%s %-20s\n", statusColor, statusStr, C_RESET, a->desc);
        }
    }
    printSeparator();
    cPrint(COLOR_INFO, "  共 %zu 条预约记录\n", count);
}

static void apptListByPatient(const char *patientId)
{
    int found = 0;
    for (size_t i = 0; i < arrSize(&g_sys.appointments); i++) {
        Appointment *a = (Appointment*)arrGet(&g_sys.appointments, i);
        if (a && strcmp(a->patientId, patientId) == 0) {
            if (!found) {
                cPrint(COLOR_HEADER, "  患者 %s 的预约记录:\n", patientId);
                found = 1;
            }
            Doctor *d = doctorFind(a->doctorId);
            const char *dname = d ? d->name : "未知";
            const char *statusStr = (a->status == APPT_STATUS_ACTIVE) ? "有效" : "已取消";
            printf("    [%d] 医生:%s(%s) 时间:%s 状态:%s 描述:%s\n",
                   a->id, dname, a->doctorId, a->datetime, statusStr, a->desc);
        }
    }
    if (!found) cPrint(COLOR_WARN, "  该患者无预约记录\n");
}

static void apptListByDoctor(const char *doctorId)
{
    int found = 0;
    for (size_t i = 0; i < arrSize(&g_sys.appointments); i++) {
        Appointment *a = (Appointment*)arrGet(&g_sys.appointments, i);
        if (a && strcmp(a->doctorId, doctorId) == 0) {
            if (!found) {
                cPrint(COLOR_HEADER, "  医生 %s 的预约记录:\n", doctorId);
                found = 1;
            }
            Patient *p = patientFind(a->patientId);
            const char *pname = p ? p->name : "未知";
            const char *statusStr = (a->status == APPT_STATUS_ACTIVE) ? "有效" : "已取消";
            printf("    [%d] 患者:%s(%s) 时间:%s 状态:%s 描述:%s\n",
                   a->id, pname, a->patientId, a->datetime, statusStr, a->desc);
        }
    }
    if (!found) cPrint(COLOR_WARN, "  该医生无预约记录\n");
}

/* ======================== 菜单交互实现 ======================== */

static void menuStatistics(void)
{
    printf("\n");
    cPrint(COLOR_TITLE, "┌─────────── 数据统计 ───────────┐\n");
    printf("│                                │\n");
    size_t pCount = arrSize(&g_sys.patients);
    size_t dCount = arrSize(&g_sys.doctors);
    size_t aCount = arrSize(&g_sys.appointments);
    size_t activeAppts = 0;
    for (size_t i = 0; i < aCount; i++) {
        Appointment *a = (Appointment*)arrGet(&g_sys.appointments, i);
        if (a && a->status == APPT_STATUS_ACTIVE) activeAppts++;
    }
    printf("│  %s患者总数:%s %-18zu │\n", C_CYAN, C_RESET, pCount);
    printf("│  %s医生总数:%s %-18zu │\n", C_CYAN, C_RESET, dCount);
    printf("│  %s预约总数:%s %-18zu │\n", C_CYAN, C_RESET, aCount);
    printf("│  %s有效预约:%s %-18zu │\n", C_GREEN, C_RESET, activeAppts);
    printf("│  %s已取消预约:%s %-16zu │\n", C_RED, C_RESET, aCount - activeAppts);
    printf("│                                │\n");
    cPrint(COLOR_TITLE, "└────────────────────────────────┘\n");
    printf("\n");
    printf("按回车键继续...");
    clearInput();
    getchar();
}

static void menuPatient(void)
{
    while (1) {
        printf("\n");
        cPrint(MENU_COLOR, "  ╔═══════ 患者管理 ═══════╗\n");
        printf("  ║  1. 添加患者            ║\n");
        printf("  ║  2. 查看所有患者        ║\n");
        printf("  ║  3. 查询患者            ║\n");
        printf("  ║  4. 删除患者            ║\n");
        printf("  ║  0. 返回主菜单          ║\n");
        cPrint(MENU_COLOR, "  ╚═════════════════════════╝\n");
        printf("  请选择: ");

        int choice;
        if (scanf("%d", &choice) != 1) {
            clearInput();
            cPrint(COLOR_ERR, "  输入无效，请重新输入\n");
            continue;
        }
        clearInput();

        if (choice == 0) break;

        switch (choice) {
        case 1: {
            char id[MAX_ID], name[MAX_NAME], gender[MAX_GENDER];
            char job[MAX_JOB], addr[MAX_ADDR];
            int age;
            printf("\n  --- 添加患者 ---\n");
            printf("  患者编号(如P2025001): "); readLine(id, MAX_ID);
            printf("  姓名: "); readLine(name, MAX_NAME);
            printf("  年龄: ");
            if (scanf("%d", &age) != 1) { clearInput(); cPrint(COLOR_ERR, "  年龄无效\n"); break; }
            clearInput();
            printf("  性别(男/女): "); readLine(gender, MAX_GENDER);
            printf("  职业: "); readLine(job, MAX_JOB);
            printf("  地址: "); readLine(addr, MAX_ADDR);
            HospStatus st = patientAdd(id, name, age, gender, job, addr);
            if (st == HOSP_OK) cPrint(COLOR_OK, "  ✓ 患者添加成功\n");
            else cPrint(COLOR_ERR, "  ✗ 添加失败: %s\n", hospStatusStr(st));
            break;
        }
        case 2:
            printf("\n");
            patientList();
            break;
        case 3: {
            char id[MAX_ID];
            printf("\n  --- 查询患者 ---\n");
            printf("  请输入患者编号: "); readLine(id, MAX_ID);
            Patient *p = patientFind(id);
            if (p) {
                printf("\n");
                printSeparator();
                printf("  编号: %s\n", p->id);
                printf("  姓名: %s\n", p->name);
                printf("  年龄: %d\n", p->age);
                printf("  性别: %s\n", p->gender);
                printf("  职业: %s\n", p->job);
                printf("  地址: %s\n", p->addr);
                printSeparator();
                apptListByPatient(id);
            } else {
                cPrint(COLOR_ERR, "  ✗ 未找到患者: %s\n", id);
            }
            break;
        }
        case 4: {
            char id[MAX_ID], confirm[10];
            printf("\n  --- 删除患者 ---\n");
            printf("  请输入要删除的患者编号: "); readLine(id, MAX_ID);
            Patient *p = patientFind(id);
            if (!p) { cPrint(COLOR_ERR, "  ✗ 未找到患者\n"); break; }
            printf("  确认删除患者 '%s'(%s)? (y/n): ", p->name, id);
            readLine(confirm, 10);
            if (confirm[0] == 'y' || confirm[0] == 'Y') {
                HospStatus st = patientDelete(id);
                if (st == HOSP_OK) cPrint(COLOR_OK, "  ✓ 删除成功\n");
                else cPrint(COLOR_ERR, "  ✗ 删除失败: %s\n", hospStatusStr(st));
            } else {
                cPrint(COLOR_WARN, "  已取消删除\n");
            }
            break;
        }
        default:
            cPrint(COLOR_ERR, "  无效选项\n");
        }
        printf("\n按回车键继续...");
        getchar();
    }
}

static void menuDoctor(void)
{
    while (1) {
        printf("\n");
        cPrint(MENU_COLOR, "  ╔═══════ 医生管理 ═══════╗\n");
        printf("  ║  1. 添加医生            ║\n");
        printf("  ║  2. 查看所有医生        ║\n");
        printf("  ║  3. 查询医生            ║\n");
        printf("  ║  4. 删除医生            ║\n");
        printf("  ║  0. 返回主菜单          ║\n");
        cPrint(MENU_COLOR, "  ╚═════════════════════════╝\n");
        printf("  请选择: ");

        int choice;
        if (scanf("%d", &choice) != 1) {
            clearInput();
            cPrint(COLOR_ERR, "  输入无效，请重新输入\n");
            continue;
        }
        clearInput();

        if (choice == 0) break;

        switch (choice) {
        case 1: {
            char id[MAX_ID], name[MAX_NAME], specialty[MAX_SPECIALTY], dept[MAX_DEPT];
            int years;
            printf("\n  --- 添加医生 ---\n");
            printf("  医生编号(如D001): "); readLine(id, MAX_ID);
            printf("  姓名: "); readLine(name, MAX_NAME);
            printf("  专长(如心血管内科): "); readLine(specialty, MAX_SPECIALTY);
            printf("  科室(如内科): "); readLine(dept, MAX_DEPT);
            printf("  从业年限: ");
            if (scanf("%d", &years) != 1) { clearInput(); cPrint(COLOR_ERR, "  年限无效\n"); break; }
            clearInput();
            HospStatus st = doctorAdd(id, name, specialty, dept, years);
            if (st == HOSP_OK) cPrint(COLOR_OK, "  ✓ 医生添加成功\n");
            else cPrint(COLOR_ERR, "  ✗ 添加失败: %s\n", hospStatusStr(st));
            break;
        }
        case 2:
            printf("\n");
            doctorList();
            break;
        case 3: {
            char id[MAX_ID];
            printf("\n  --- 查询医生 ---\n");
            printf("  请输入医生编号: "); readLine(id, MAX_ID);
            Doctor *d = doctorFind(id);
            if (d) {
                printf("\n");
                printSeparator();
                printf("  编号: %s\n", d->id);
                printf("  姓名: %s\n", d->name);
                printf("  专长: %s\n", d->specialty);
                printf("  科室: %s\n", d->dept);
                printf("  从业年限: %d\n", d->years);
                printSeparator();
                apptListByDoctor(id);
            } else {
                cPrint(COLOR_ERR, "  ✗ 未找到医生: %s\n", id);
            }
            break;
        }
        case 4: {
            char id[MAX_ID], confirm[10];
            printf("\n  --- 删除医生 ---\n");
            printf("  请输入要删除的医生编号: "); readLine(id, MAX_ID);
            Doctor *d = doctorFind(id);
            if (!d) { cPrint(COLOR_ERR, "  ✗ 未找到医生\n"); break; }
            printf("  确认删除医生 '%s'(%s)? (y/n): ", d->name, id);
            readLine(confirm, 10);
            if (confirm[0] == 'y' || confirm[0] == 'Y') {
                HospStatus st = doctorDelete(id);
                if (st == HOSP_OK) cPrint(COLOR_OK, "  ✓ 删除成功\n");
                else cPrint(COLOR_ERR, "  ✗ 删除失败: %s\n", hospStatusStr(st));
            } else {
                cPrint(COLOR_WARN, "  已取消删除\n");
            }
            break;
        }
        default:
            cPrint(COLOR_ERR, "  无效选项\n");
        }
        printf("\n按回车键继续...");
        getchar();
    }
}

static void menuAppointment(void)
{
    while (1) {
        printf("\n");
        cPrint(MENU_COLOR, "  ╔═══════ 预约管理 ═══════╗\n");
        printf("  ║  1. 新建预约            ║\n");
        printf("  ║  2. 查看所有预约        ║\n");
        printf("  ║  3. 按患者查询预约      ║\n");
        printf("  ║  4. 按医生查询预约      ║\n");
        printf("  ║  5. 取消预约            ║\n");
        printf("  ║  0. 返回主菜单          ║\n");
        cPrint(MENU_COLOR, "  ╚═════════════════════════╝\n");
        printf("  请选择: ");

        int choice;
        if (scanf("%d", &choice) != 1) {
            clearInput();
            cPrint(COLOR_ERR, "  输入无效，请重新输入\n");
            continue;
        }
        clearInput();

        if (choice == 0) break;

        switch (choice) {
        case 1: {
            char pid[MAX_ID], did[MAX_ID], dt[MAX_DATE], desc[MAX_DESC], note[MAX_NOTE];
            int newId = 0;
            printf("\n  --- 新建预约 ---\n");
            printf("  患者编号: "); readLine(pid, MAX_ID);
            printf("  医生编号: "); readLine(did, MAX_ID);
            printf("  预约时间(如2025-06-15 09:00): "); readLine(dt, MAX_DATE);
            printf("  病情描述: "); readLine(desc, MAX_DESC);
            printf("  备注(可留空): "); readLine(note, MAX_NOTE);
            HospStatus st = apptAdd(pid, did, dt, desc, note, &newId);
            if (st == HOSP_OK) cPrint(COLOR_OK, "  ✓ 预约成功！预约号: %d\n", newId);
            else cPrint(COLOR_ERR, "  ✗ 预约失败: %s\n", hospStatusStr(st));
            break;
        }
        case 2:
            printf("\n");
            apptList();
            break;
        case 3: {
            char pid[MAX_ID];
            printf("\n  --- 按患者查询 ---\n");
            printf("  患者编号: "); readLine(pid, MAX_ID);
            printf("\n");
            apptListByPatient(pid);
            break;
        }
        case 4: {
            char did[MAX_ID];
            printf("\n  --- 按医生查询 ---\n");
            printf("  医生编号: "); readLine(did, MAX_ID);
            printf("\n");
            apptListByDoctor(did);
            break;
        }
        case 5: {
            int apptId;
            char confirm[10];
            printf("\n  --- 取消预约 ---\n");
            printf("  预约号: ");
            if (scanf("%d", &apptId) != 1) { clearInput(); cPrint(COLOR_ERR, "  预约号无效\n"); break; }
            clearInput();
            printf("  确认取消预约号 %d? (y/n): ", apptId);
            readLine(confirm, 10);
            if (confirm[0] == 'y' || confirm[0] == 'Y') {
                HospStatus st = apptCancel(apptId);
                if (st == HOSP_OK) cPrint(COLOR_OK, "  ✓ 预约已取消\n");
                else cPrint(COLOR_ERR, "  ✗ 取消失败: %s\n", hospStatusStr(st));
            } else {
                cPrint(COLOR_WARN, "  已取消操作\n");
            }
            break;
        }
        default:
            cPrint(COLOR_ERR, "  无效选项\n");
        }
        printf("\n按回车键继续...");
        getchar();
    }
}

static void menuMain(void)
{
    while (1) {
        system("cls");
        printTitle();
        printf("  %s[1]%s 患者管理    %s[2]%s 医生管理    %s[3]%s 预约管理    %s[4]%s 数据统计\n",
               C_BOLD C_GREEN, C_RESET, C_BOLD C_GREEN, C_RESET,
               C_BOLD C_GREEN, C_RESET, C_BOLD C_GREEN, C_RESET);
        printf("  %s[5]%s 保存数据    %s[6]%s 查看患者列表  %s[7]%s 查看医生列表  %s[8]%s 查看预约列表\n",
               C_BOLD C_BLUE, C_RESET, C_BOLD C_BLUE, C_RESET,
               C_BOLD C_BLUE, C_RESET, C_BOLD C_BLUE, C_RESET);
        printf("  %s[0]%s 退出系统\n", C_BOLD C_RED, C_RESET);
        printSeparator();
        cPrint(COLOR_INFO, "  提示: 数据修改后退出时自动保存，也可手动选择[5]保存\n");
        printf("\n  请选择操作: ");

        int choice;
        if (scanf("%d", &choice) != 1) {
            clearInput();
            continue;
        }
        clearInput();

        switch (choice) {
        case 0:
            cPrint(COLOR_INFO, "\n  正在保存数据并退出...\n");
            return;
        case 1: menuPatient(); break;
        case 2: menuDoctor(); break;
        case 3: menuAppointment(); break;
        case 4: menuStatistics(); break;
        case 5: {
            HospStatus st = sysSave();
            if (st == HOSP_OK) cPrint(COLOR_OK, "  ✓ 数据保存成功\n");
            else cPrint(COLOR_ERR, "  ✗ 保存失败: %s\n", hospStatusStr(st));
            printf("按回车键继续..."); getchar();
            break;
        }
        case 6: printf("\n"); patientList(); printf("\n按回车键继续..."); getchar(); break;
        case 7: printf("\n"); doctorList(); printf("\n按回车键继续..."); getchar(); break;
        case 8: printf("\n"); apptList(); printf("\n按回车键继续..."); getchar(); break;
        default:
            cPrint(COLOR_ERR, "  无效选项，请重新选择\n");
            printf("按回车键继续..."); getchar();
        }
    }
}

/* ======================== 自测框架实现 ======================== */

static void testAssert(int condition, const char *msg, const char *file, int line)
{
    (void)file; (void)line;
    if (condition) {
        g_testsPassed++;
    } else {
        g_testsFailed++;
        cPrint(COLOR_ERR, "    ✗ 断言失败: %s (%s:%d)\n", msg, file, line);
    }
}

static int test_arr_basic(void)
{
    DynArray arr;
    HospStatus st;
    st = arrInit(&arr, sizeof(int), 4);
    TEST_ASSERT_STATUS_OK(st);
    TEST_ASSERT_EQ((int)arrSize(&arr), 0);
    int val = 42;
    st = arrPush(&arr, &val);
    TEST_ASSERT_STATUS_OK(st);
    TEST_ASSERT_EQ((int)arrSize(&arr), 1);
    int *got = (int*)arrGet(&arr, 0);
    TEST_ASSERT(got != NULL);
    TEST_ASSERT_EQ(*got, 42);
    arrFree(&arr);
    return g_testsFailed == 0;
}

static int test_arr_grow(void)
{
    DynArray arr;
    HospStatus st;
    st = arrInit(&arr, sizeof(int), 2);
    TEST_ASSERT_STATUS_OK(st);
    for (int i = 0; i < 20; i++) {
        st = arrPush(&arr, &i);
        TEST_ASSERT_STATUS_OK(st);
    }
    TEST_ASSERT_EQ((int)arrSize(&arr), 20);
    for (int i = 0; i < 20; i++) {
        int *v = (int*)arrGet(&arr, (size_t)i);
        TEST_ASSERT(v != NULL);
        if (v) TEST_ASSERT_EQ(*v, i);
    }
    arrFree(&arr);
    return g_testsFailed == 0;
}

static int test_sys_init_shutdown(void)
{
    DynArray testArr;
    HospStatus st;
    st = arrInit(&testArr, sizeof(int), 4);
    TEST_ASSERT_STATUS_OK(st);
    int val = 100;
    st = arrPush(&testArr, &val);
    TEST_ASSERT_STATUS_OK(st);
    TEST_ASSERT_EQ((int)arrSize(&testArr), 1);
    arrFree(&testArr);
    TEST_ASSERT_EQ((int)arrSize(&testArr), 0);
    TEST_ASSERT(testArr.data == NULL);
    return g_testsFailed == 0;
}

static int test_patient_add(void)
{
    size_t before = arrSize(&g_sys.patients);
    HospStatus st = patientAdd("P_TEST01", "测试患者", 30, "男", "工程师", "测试地址");
    TEST_ASSERT_STATUS_OK(st);
    TEST_ASSERT_EQ((int)arrSize(&g_sys.patients), (int)before + 1);
    Patient *p = patientFind("P_TEST01");
    TEST_ASSERT(p != NULL);
    TEST_ASSERT_STR(p->name, "测试患者");
    TEST_ASSERT_EQ(p->age, 30);
    return g_testsFailed == 0;
}

static int test_patient_duplicate(void)
{
    HospStatus st = patientAdd("P_TEST01", "重复患者", 25, "女", "教师", "地址2");
    TEST_ASSERT_EQ((int)st, (int)HOSP_ERR_DUPLICATE);
    return g_testsFailed == 0;
}

static int test_patient_find(void)
{
    Patient *p = patientFind("P_TEST01");
    TEST_ASSERT(p != NULL);
    if (p) {
        TEST_ASSERT_STR(p->id, "P_TEST01");
        TEST_ASSERT_STR(p->gender, "男");
    }
    Patient *np = patientFind("P_NOTEXIST");
    TEST_ASSERT(np == NULL);
    return g_testsFailed == 0;
}

static int test_patient_delete(void)
{
    size_t before = arrSize(&g_sys.patients);
    HospStatus st = patientDelete("P_TEST01");
    TEST_ASSERT_STATUS_OK(st);
    TEST_ASSERT_EQ((int)arrSize(&g_sys.patients), (int)before - 1);
    Patient *p = patientFind("P_TEST01");
    TEST_ASSERT(p == NULL);
    st = patientDelete("P_NOTEXIST");
    TEST_ASSERT_EQ((int)st, (int)HOSP_ERR_NOT_FOUND);
    return g_testsFailed == 0;
}

static int test_doctor_add(void)
{
    size_t before = arrSize(&g_sys.doctors);
    HospStatus st = doctorAdd("D_TEST01", "张医生", "心血管内科", "内科", 15);
    TEST_ASSERT_STATUS_OK(st);
    TEST_ASSERT_EQ((int)arrSize(&g_sys.doctors), (int)before + 1);
    Doctor *d = doctorFind("D_TEST01");
    TEST_ASSERT(d != NULL);
    TEST_ASSERT_STR(d->name, "张医生");
    TEST_ASSERT_EQ(d->years, 15);
    return g_testsFailed == 0;
}

static int test_doctor_duplicate(void)
{
    HospStatus st = doctorAdd("D_TEST01", "重复医生", "外科", "外科", 10);
    TEST_ASSERT_EQ((int)st, (int)HOSP_ERR_DUPLICATE);
    return g_testsFailed == 0;
}

static int test_doctor_find(void)
{
    Doctor *d = doctorFind("D_TEST01");
    TEST_ASSERT(d != NULL);
    if (d) {
        TEST_ASSERT_STR(d->specialty, "心血管内科");
        TEST_ASSERT_STR(d->dept, "内科");
    }
    Doctor *nd = doctorFind("D_NOTEXIST");
    TEST_ASSERT(nd == NULL);
    return g_testsFailed == 0;
}

static int test_doctor_delete(void)
{
    size_t before = arrSize(&g_sys.doctors);
    HospStatus st = doctorDelete("D_TEST01");
    TEST_ASSERT_STATUS_OK(st);
    TEST_ASSERT_EQ((int)arrSize(&g_sys.doctors), (int)before - 1);
    Doctor *d = doctorFind("D_TEST01");
    TEST_ASSERT(d == NULL);
    st = doctorDelete("D_NOTEXIST");
    TEST_ASSERT_EQ((int)st, (int)HOSP_ERR_NOT_FOUND);
    return g_testsFailed == 0;
}

static int test_appt_add_valid(void)
{
    patientAdd("P_APPT01", "预约患者A", 40, "男", "职员", "地址A");
    doctorAdd("D_APPT01", "李医生", "神经内科", "神经内科", 20);
    size_t before = arrSize(&g_sys.appointments);
    int newId = 0;
    HospStatus st = apptAdd("P_APPT01", "D_APPT01", "2025-07-01 10:00", "头痛", "无", &newId);
    TEST_ASSERT_STATUS_OK(st);
    TEST_ASSERT(newId > 0);
    TEST_ASSERT_EQ((int)arrSize(&g_sys.appointments), (int)before + 1);
    return g_testsFailed == 0;
}

static int test_appt_add_invalid_patient(void)
{
    doctorAdd("D_APPT02", "王医生", "骨科", "骨科", 12);
    int newId = 0;
    HospStatus st = apptAdd("P_NONEXIST", "D_APPT02", "2025-07-02 11:00", "骨折", "", &newId);
    TEST_ASSERT_EQ((int)st, (int)HOSP_ERR_NOT_FOUND);
    return g_testsFailed == 0;
}

static int test_appt_add_invalid_doctor(void)
{
    patientAdd("P_APPT02", "预约患者B", 25, "女", "学生", "地址B");
    int newId = 0;
    HospStatus st = apptAdd("P_APPT02", "D_NONEXIST", "2025-07-03 14:00", "感冒", "", &newId);
    TEST_ASSERT_EQ((int)st, (int)HOSP_ERR_NOT_FOUND);
    return g_testsFailed == 0;
}

static int test_appt_cancel(void)
{
    patientAdd("P_CANCEL", "取消测试患者", 50, "男", "退休", "地址C");
    doctorAdd("D_CANCEL", "取消测试医生", "全科", "全科", 30);
    int newId = 0;
    apptAdd("P_CANCEL", "D_CANCEL", "2025-07-04 09:00", "体检", "", &newId);
    TEST_ASSERT(newId > 0);
    HospStatus st = apptCancel(newId);
    TEST_ASSERT_STATUS_OK(st);
    int idx = -1;
    TEST_ASSERT_EQ((int)apptFind(newId, &idx), (int)HOSP_OK);
    Appointment *a = (Appointment*)arrGet(&g_sys.appointments, (size_t)idx);
    TEST_ASSERT(a != NULL);
    if (a) TEST_ASSERT_EQ(a->status, APPT_STATUS_CANCELED);
    st = apptCancel(99999);
    TEST_ASSERT_EQ((int)st, (int)HOSP_ERR_NOT_FOUND);
    return g_testsFailed == 0;
}

static int test_appt_find(void)
{
    patientAdd("P_FIND", "查找测试患者", 33, "女", "护士", "地址F");
    doctorAdd("D_FIND", "查找测试医生", "儿科", "儿科", 8);
    int newId = 0;
    apptAdd("P_FIND", "D_FIND", "2025-07-05 15:00", "儿童体检", "", &newId);
    int idx = -1;
    TEST_ASSERT_EQ((int)apptFind(newId, &idx), (int)HOSP_OK);
    TEST_ASSERT(idx >= 0);
    TEST_ASSERT_EQ((int)apptFind(88888, &idx), (int)HOSP_ERR_NOT_FOUND);
    return g_testsFailed == 0;
}

static int test_file_persistence(void)
{
    patientAdd("P_SAVE", "持久化患者", 45, "男", "经理", "地址S");
    doctorAdd("D_SAVE", "持久化医生", "皮肤科", "皮肤科", 18);
    int saveId = 0;
    apptAdd("P_SAVE", "D_SAVE", "2025-07-06 16:00", "皮疹", "", &saveId);
    size_t pCount = arrSize(&g_sys.patients);
    size_t dCount = arrSize(&g_sys.doctors);
    size_t aCount = arrSize(&g_sys.appointments);
    int nextId = g_sys.nextApptId;
    HospStatus st = sysSave();
    TEST_ASSERT_STATUS_OK(st);

    arrFree(&g_sys.patients);
    arrFree(&g_sys.doctors);
    arrFree(&g_sys.appointments);
    g_sys.patients.data = NULL;
    g_sys.doctors.data = NULL;
    g_sys.appointments.data = NULL;
    arrInit(&g_sys.patients, sizeof(Patient), INIT_CAPACITY);
    arrInit(&g_sys.doctors, sizeof(Doctor), INIT_CAPACITY);
    arrInit(&g_sys.appointments, sizeof(Appointment), INIT_CAPACITY);
    g_sys.nextApptId = 1;

    st = sysLoad();
    TEST_ASSERT_STATUS_OK(st);
    TEST_ASSERT_EQ((int)arrSize(&g_sys.patients), (int)pCount);
    TEST_ASSERT_EQ((int)arrSize(&g_sys.doctors), (int)dCount);
    TEST_ASSERT_EQ((int)arrSize(&g_sys.appointments), (int)aCount);
    TEST_ASSERT_EQ(g_sys.nextApptId, nextId);
    Patient *p = patientFind("P_SAVE");
    TEST_ASSERT(p != NULL);
    Doctor *d = doctorFind("D_SAVE");
    TEST_ASSERT(d != NULL);
    return g_testsFailed == 0;
}

static int test_error_codes(void)
{
    TEST_ASSERT_STR(hospStatusStr(HOSP_OK), "操作成功");
    TEST_ASSERT_STR(hospStatusStr(HOSP_ERR_MEMORY), "内存分配失败");
    TEST_ASSERT_STR(hospStatusStr(HOSP_ERR_NOT_FOUND), "记录未找到");
    TEST_ASSERT_STR(hospStatusStr(HOSP_ERR_DUPLICATE), "编号重复");
    TEST_ASSERT_STR(hospStatusStr(HOSP_ERR_INVALID_PARAM), "无效参数");
    TEST_ASSERT_STR(hospStatusStr(HOSP_ERR_FILE_OPEN), "无法打开文件");
    return g_testsFailed == 0;
}

static int testRunAll(void)
{
    g_testsPassed = 0;
    g_testsFailed = 0;

    cPrint(COLOR_TITLE, "\n╔══════════════════════════════════════════╗\n");
    cPrint(COLOR_TITLE, "║        看病预约系统 V2.0 自测套件        ║\n");
    cPrint(COLOR_TITLE, "╚══════════════════════════════════════════╝\n\n");

    sysInit();

    TestCase tests[] = {
        {"动态数组基础操作", test_arr_basic},
        {"动态数组自动扩容", test_arr_grow},
        {"系统初始化", test_sys_init_shutdown},
        {"患者添加", test_patient_add},
        {"患者编号重复检测", test_patient_duplicate},
        {"患者查找", test_patient_find},
        {"患者删除", test_patient_delete},
        {"医生添加", test_doctor_add},
        {"医生编号重复检测", test_doctor_duplicate},
        {"医生查找", test_doctor_find},
        {"医生删除", test_doctor_delete},
        {"有效预约创建", test_appt_add_valid},
        {"无效患者预约拒绝", test_appt_add_invalid_patient},
        {"无效医生预约拒绝", test_appt_add_invalid_doctor},
        {"预约取消", test_appt_cancel},
        {"预约查找", test_appt_find},
        {"文件持久化", test_file_persistence},
        {"错误码字符串", test_error_codes},
    };

    int total = (int)(sizeof(tests) / sizeof(tests[0]));

    for (int i = 0; i < total; i++) {
        int failedBefore = g_testsFailed;
        cPrint(COLOR_INFO, "  [%d/%d] %s... ", i + 1, total, tests[i].name);
        tests[i].func();
        if (g_testsFailed == failedBefore) {
            cPrint(COLOR_OK, "✓ 通过\n");
        } else {
            cPrint(COLOR_ERR, "✗ 失败\n");
        }
    }

    sysShutdown();
    remove(DATA_FILE);

    printf("\n");
    printSeparator();
    cPrint(COLOR_TITLE, "  测试结果: ");
    cPrint(COLOR_OK, "%d 通过", g_testsPassed);
    printf(", ");
    cPrint(COLOR_ERR, "%d 失败", g_testsFailed);
    printf(", 共 %d 项测试\n", total);
    printSeparator();

    return g_testsFailed == 0 ? 0 : 1;
}

/* ======================== 主函数 ======================== */

int main(int argc, char *argv[])
{
    initConsole();

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            printHelp(argv[0]);
            return 0;
        }
        if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0) {
            int ret = testRunAll();
            return ret;
        }
        cPrint(COLOR_ERR, "未知选项: %s\n\n", argv[1]);
        printHelp(argv[0]);
        return 1;
    }

    if (sysInit() != HOSP_OK) {
        cPrint(COLOR_ERR, "系统初始化失败！\n");
        return 1;
    }

    HospStatus loadSt = sysLoad();
    if (loadSt == HOSP_ERR_FILE_MAGIC) {
        cPrint(COLOR_WARN, "检测到V1旧数据文件或非本系统文件，将以空数据启动\n");
    } else if (loadSt != HOSP_OK && loadSt != HOSP_ERR_FILE_OPEN) {
        cPrint(COLOR_WARN, "数据加载警告: %s\n", hospStatusStr(loadSt));
    }

    if (arrSize(&g_sys.patients) == 0 && arrSize(&g_sys.doctors) == 0) {
        cPrint(COLOR_INFO, "首次运行，系统已准备就绪。\n");
    }

    menuMain();
    sysShutdown();
    cPrint(COLOR_OK, "\n感谢使用，再见！\n");
    return 0;
}
