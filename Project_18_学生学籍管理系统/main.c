#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define MAX_NAME        50
#define MAX_ID          20
#define MAX_CLASS       30
#define MAX_PHONE       20
#define MAX_ADDRESS     200
#define MAX_FILENAME    256
#define INIT_CAPACITY   8
#define MAGIC_HEADER    "STUSV2"
#define MAGIC_LEN       6
#define FILE_VERSION    2

#define COLOR_RESET     "\033[0m"
#define COLOR_RED       "\033[31m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_BLUE      "\033[34m"
#define COLOR_MAGENTA   "\033[35m"
#define COLOR_CYAN      "\033[36m"
#define COLOR_WHITE     "\033[37m"
#define COLOR_BOLD      "\033[1m"
#define COLOR_BG_BLUE   "\033[44m"

typedef enum {
    STU_OK = 0,
    STU_ERR_NULL_PTR = -1,
    STU_ERR_MEMORY = -2,
    STU_ERR_NOT_FOUND = -3,
    STU_ERR_DUPLICATE = -4,
    STU_ERR_INVALID_PARAM = -5,
    STU_ERR_FILE_OPEN = -6,
    STU_ERR_FILE_READ = -7,
    STU_ERR_FILE_WRITE = -8,
    STU_ERR_FILE_MAGIC = -9,
    STU_ERR_FILE_VERSION = -10,
    STU_ERR_EMPTY = -11,
    STU_ERR_INDEX_OUT = -12
} StuStatus;

typedef struct {
    char id[MAX_ID];
    char name[MAX_NAME];
    char gender[8];
    int  age;
    char className[MAX_CLASS];
    char phone[MAX_PHONE];
    char address[MAX_ADDRESS];
    float gpa;
} Student;

typedef struct {
    Student *data;
    int count;
    int capacity;
    char filename[MAX_FILENAME];
} StudentDB;

static int g_test_passed = 0;
static int g_test_failed = 0;

static void console_init(void)
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

static void console_cleanup(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(936);
    SetConsoleCP(936);
#endif
}

static void c_print(const char *color, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    printf("%s", color);
    vprintf(fmt, args);
    printf("%s", COLOR_RESET);
    va_end(args);
}

static void print_error(const char *msg) { c_print(COLOR_RED, "  [错误] %s\n", msg); }
static void print_success(const char *msg) { c_print(COLOR_GREEN, "  [成功] %s\n", msg); }
static void print_info(const char *msg) { c_print(COLOR_CYAN, "  [信息] %s\n", msg); }
static void print_warn(const char *msg) { c_print(COLOR_YELLOW, "  [警告] %s\n", msg); }

static StuStatus db_init(StudentDB *db, const char *filename)
{
    if (db == NULL) return STU_ERR_NULL_PTR;
    db->data = (Student*)malloc(sizeof(Student) * INIT_CAPACITY);
    if (db->data == NULL) return STU_ERR_MEMORY;
    db->count = 0;
    db->capacity = INIT_CAPACITY;
    if (filename != NULL) {
        strncpy(db->filename, filename, MAX_FILENAME - 1);
        db->filename[MAX_FILENAME - 1] = '\0';
    } else {
        strcpy(db->filename, "students.dat");
    }
    memset(db->data, 0, sizeof(Student) * INIT_CAPACITY);
    return STU_OK;
}

static StuStatus db_free(StudentDB *db)
{
    if (db == NULL) return STU_ERR_NULL_PTR;
    if (db->data != NULL) { free(db->data); db->data = NULL; }
    db->count = 0;
    db->capacity = 0;
    return STU_OK;
}

static StuStatus db_resize(StudentDB *db)
{
    if (db == NULL || db->data == NULL) return STU_ERR_NULL_PTR;
    int new_capacity = db->capacity * 2;
    Student *new_data = (Student*)realloc(db->data, sizeof(Student) * new_capacity);
    if (new_data == NULL) return STU_ERR_MEMORY;
    db->data = new_data;
    db->capacity = new_capacity;
    return STU_OK;
}

static int db_find_by_id(StudentDB *db, const char *id)
{
    if (db == NULL || id == NULL) return -1;
    for (int i = 0; i < db->count; i++) {
        if (strcmp(db->data[i].id, id) == 0) return i;
    }
    return -1;
}

static StuStatus student_add(StudentDB *db, const Student *stu)
{
    if (db == NULL || stu == NULL) return STU_ERR_NULL_PTR;
    if (strlen(stu->id) == 0 || strlen(stu->name) == 0) return STU_ERR_INVALID_PARAM;
    if (db_find_by_id(db, stu->id) >= 0) return STU_ERR_DUPLICATE;
    if (db->count >= db->capacity) {
        StuStatus s = db_resize(db);
        if (s != STU_OK) return s;
    }
    memcpy(&db->data[db->count], stu, sizeof(Student));
    db->count++;
    return STU_OK;
}

static StuStatus student_delete(StudentDB *db, const char *id)
{
    if (db == NULL || id == NULL) return STU_ERR_NULL_PTR;
    int idx = db_find_by_id(db, id);
    if (idx < 0) return STU_ERR_NOT_FOUND;
    for (int i = idx; i < db->count - 1; i++) {
        memcpy(&db->data[i], &db->data[i + 1], sizeof(Student));
    }
    db->count--;
    memset(&db->data[db->count], 0, sizeof(Student));
    return STU_OK;
}

static StuStatus student_update(StudentDB *db, int index, const Student *stu)
{
    if (db == NULL || stu == NULL) return STU_ERR_NULL_PTR;
    if (index < 0 || index >= db->count) return STU_ERR_INDEX_OUT;
    if (strcmp(db->data[index].id, stu->id) != 0) {
        if (db_find_by_id(db, stu->id) >= 0) return STU_ERR_DUPLICATE;
    }
    memcpy(&db->data[index], stu, sizeof(Student));
    return STU_OK;
}

static Student* student_get(StudentDB *db, int index)
{
    if (db == NULL || index < 0 || index >= db->count) return NULL;
    return &db->data[index];
}

static int student_count(StudentDB *db)
{
    if (db == NULL) return 0;
    return db->count;
}

static int student_search_by_name(StudentDB *db, const char *keyword, int *results, int max_results)
{
    if (db == NULL || keyword == NULL || results == NULL) return 0;
    int found = 0;
    for (int i = 0; i < db->count && found < max_results; i++) {
        if (strstr(db->data[i].name, keyword) != NULL) results[found++] = i;
    }
    return found;
}

static float stats_avg_age(StudentDB *db)
{
    if (db == NULL || db->count == 0) return 0.0f;
    int sum = 0;
    for (int i = 0; i < db->count; i++) sum += db->data[i].age;
    return (float)sum / db->count;
}

static float stats_avg_gpa(StudentDB *db)
{
    if (db == NULL || db->count == 0) return 0.0f;
    float sum = 0.0f;
    for (int i = 0; i < db->count; i++) sum += db->data[i].gpa;
    return sum / db->count;
}

static int stats_count_by_class(StudentDB *db, const char *className)
{
    if (db == NULL || className == NULL) return 0;
    int count = 0;
    for (int i = 0; i < db->count; i++) {
        if (strcmp(db->data[i].className, className) == 0) count++;
    }
    return count;
}

static int stats_count_by_gender(StudentDB *db, const char *gender)
{
    if (db == NULL || gender == NULL) return 0;
    int count = 0;
    for (int i = 0; i < db->count; i++) {
        if (strcmp(db->data[i].gender, gender) == 0) count++;
    }
    return count;
}

static StuStatus db_save(StudentDB *db)
{
    if (db == NULL) return STU_ERR_NULL_PTR;
    FILE *fp = fopen(db->filename, "wb");
    if (fp == NULL) return STU_ERR_FILE_OPEN;
    int ver = FILE_VERSION;
    if (fwrite(MAGIC_HEADER, 1, MAGIC_LEN, fp) != MAGIC_LEN) { fclose(fp); return STU_ERR_FILE_WRITE; }
    if (fwrite(&ver, sizeof(int), 1, fp) != 1) { fclose(fp); return STU_ERR_FILE_WRITE; }
    if (fwrite(&db->count, sizeof(int), 1, fp) != 1) { fclose(fp); return STU_ERR_FILE_WRITE; }
    if (db->count > 0) {
        if (fwrite(db->data, sizeof(Student), db->count, fp) != (size_t)db->count) {
            fclose(fp); return STU_ERR_FILE_WRITE;
        }
    }
    fclose(fp);
    return STU_OK;
}

static StuStatus db_load(StudentDB *db)
{
    if (db == NULL) return STU_ERR_NULL_PTR;
    FILE *fp = fopen(db->filename, "rb");
    if (fp == NULL) return STU_ERR_FILE_OPEN;
    char magic[MAGIC_LEN + 1] = {0};
    if (fread(magic, 1, MAGIC_LEN, fp) != MAGIC_LEN) { fclose(fp); return STU_ERR_FILE_READ; }
    if (strcmp(magic, MAGIC_HEADER) != 0) { fclose(fp); return STU_ERR_FILE_MAGIC; }
    int version = 0;
    if (fread(&version, sizeof(int), 1, fp) != 1) { fclose(fp); return STU_ERR_FILE_READ; }
    if (version != FILE_VERSION) { fclose(fp); return STU_ERR_FILE_VERSION; }
    int count = 0;
    if (fread(&count, sizeof(int), 1, fp) != 1) { fclose(fp); return STU_ERR_FILE_READ; }
    if (count < 0 || count > 100000) { fclose(fp); return STU_ERR_FILE_READ; }
    while (db->capacity < count) {
        StuStatus s = db_resize(db);
        if (s != STU_OK) { fclose(fp); return s; }
    }
    if (count > 0) {
        if (fread(db->data, sizeof(Student), count, fp) != (size_t)count) {
            fclose(fp); return STU_ERR_FILE_READ;
        }
    }
    db->count = count;
    fclose(fp);
    return STU_OK;
}

static void clear_input_buffer(void) { int c; while ((c = getchar()) != '\n' && c != EOF); }

static void safe_input(const char *prompt, char *buf, int size)
{
    printf("%s", prompt); fflush(stdout);
    if (fgets(buf, size, stdin) != NULL) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
        else clear_input_buffer();
    }
}

static int input_int(const char *prompt)
{
    char buf[64]; int val;
    while (1) {
        printf("%s", prompt); fflush(stdout);
        if (fgets(buf, sizeof(buf), stdin) != NULL) {
            if (sscanf(buf, "%d", &val) == 1) return val;
        }
        print_error("请输入有效整数！");
    }
}

static float input_float(const char *prompt)
{
    char buf[64]; float val;
    while (1) {
        printf("%s", prompt); fflush(stdout);
        if (fgets(buf, sizeof(buf), stdin) != NULL) {
            if (sscanf(buf, "%f", &val) == 1) return val;
        }
        print_error("请输入有效数字！");
    }
}

static void print_line(void) { c_print(COLOR_BLUE, "------------------------------------------------------------\n"); }

static void print_title(const char *title)
{
    printf("\n"); print_line();
    c_print(COLOR_BOLD COLOR_CYAN, "  %s\n", title);
    print_line();
}

static void print_student(const Student *s, int index)
{
    if (s == NULL) return;
    c_print(COLOR_YELLOW, "  [%d] ", index + 1);
    printf("学号: %-15s 姓名: %-10s 性别: %-4s\n", s->id, s->name, s->gender);
    printf("       年龄: %-5d 班级: %-15s GPA: %.2f\n", s->age, s->className, s->gpa);
    printf("       电话: %-15s 地址: %s\n", s->phone, s->address);
    print_line();
}

static void print_student_table(StudentDB *db)
{
    if (db == NULL || db->count == 0) { print_warn("暂无学生记录！"); return; }
    print_title("学生信息列表");
    c_print(COLOR_BOLD, "  %-4s %-15s %-10s %-4s %-5s %-15s %-6s\n",
            "序号", "学号", "姓名", "性别", "年龄", "班级", "GPA");
    print_line();
    for (int i = 0; i < db->count; i++) {
        Student *s = &db->data[i];
        printf("  %-4d %-15s %-10s %-4s %-5d %-15s %-6.2f\n",
               i + 1, s->id, s->name, s->gender, s->age, s->className, s->gpa);
    }
    print_line();
    c_print(COLOR_GREEN, "  共 %d 条记录\n\n", db->count);
}

static void menu_add(StudentDB *db)
{
    Student s; memset(&s, 0, sizeof(s));
    print_title("添加学生信息");
    safe_input("  请输入学号: ", s.id, MAX_ID);
    safe_input("  请输入姓名: ", s.name, MAX_NAME);
    safe_input("  请输入性别(男/女): ", s.gender, 8);
    s.age = input_int("  请输入年龄: ");
    safe_input("  请输入班级: ", s.className, MAX_CLASS);
    s.gpa = input_float("  请输入GPA(0-4.0): ");
    safe_input("  请输入联系电话: ", s.phone, MAX_PHONE);
    safe_input("  请输入家庭住址: ", s.address, MAX_ADDRESS);
    StuStatus status = student_add(db, &s);
    switch (status) {
    case STU_OK: print_success("学生信息添加成功！"); db_save(db); break;
    case STU_ERR_DUPLICATE: print_error("该学号已存在！"); break;
    case STU_ERR_INVALID_PARAM: print_error("学号和姓名不能为空！"); break;
    case STU_ERR_MEMORY: print_error("内存分配失败！"); break;
    default: print_error("添加失败！");
    }
}

static void menu_delete(StudentDB *db)
{
    char id[MAX_ID];
    print_title("删除学生信息");
    safe_input("  请输入要删除的学号: ", id, MAX_ID);
    int idx = db_find_by_id(db, id);
    if (idx < 0) { print_error("未找到该学生！"); return; }
    Student *s = student_get(db, idx);
    printf("\n  即将删除以下学生信息:\n");
    print_student(s, idx);
    char confirm[8];
    safe_input("  确认删除？(y/n): ", confirm, 8);
    if (tolower(confirm[0]) == 'y') {
        StuStatus status = student_delete(db, id);
        if (status == STU_OK) { print_success("删除成功！"); db_save(db); }
        else print_error("删除失败！");
    } else print_info("已取消删除操作。");
}

static void menu_update(StudentDB *db)
{
    char id[MAX_ID];
    print_title("修改学生信息");
    safe_input("  请输入要修改的学号: ", id, MAX_ID);
    int idx = db_find_by_id(db, id);
    if (idx < 0) { print_error("未找到该学生！"); return; }
    Student *old = student_get(db, idx);
    printf("\n  当前信息:\n");
    print_student(old, idx);
    printf("  请输入新信息（直接回车保留原值）:\n\n");
    Student new_s; memcpy(&new_s, old, sizeof(Student));
    char buf[MAX_ADDRESS];
    size_t len;
    printf("  学号 [%s]: ", old->id);
    fgets(buf, sizeof(buf), stdin);
    len = strlen(buf); if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    if (strlen(buf) > 0) { snprintf(new_s.id, MAX_ID, "%s", buf); }
    printf("  姓名 [%s]: ", old->name);
    fgets(buf, sizeof(buf), stdin);
    len = strlen(buf); if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    if (strlen(buf) > 0) { snprintf(new_s.name, MAX_NAME, "%s", buf); }
    printf("  性别 [%s]: ", old->gender);
    fgets(buf, sizeof(buf), stdin);
    len = strlen(buf); if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    if (strlen(buf) > 0) { snprintf(new_s.gender, 8, "%s", buf); }
    printf("  年龄 [%d]: ", old->age);
    fgets(buf, sizeof(buf), stdin);
    len = strlen(buf); if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    if (strlen(buf) > 0) { int tmp; if (sscanf(buf, "%d", &tmp) == 1) new_s.age = tmp; }
    printf("  班级 [%s]: ", old->className);
    fgets(buf, sizeof(buf), stdin);
    len = strlen(buf); if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    if (strlen(buf) > 0) { snprintf(new_s.className, MAX_CLASS, "%s", buf); }
    printf("  GPA [%.2f]: ", old->gpa);
    fgets(buf, sizeof(buf), stdin);
    len = strlen(buf); if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    if (strlen(buf) > 0) { float tmp; if (sscanf(buf, "%f", &tmp) == 1) new_s.gpa = tmp; }
    printf("  电话 [%s]: ", old->phone);
    fgets(buf, sizeof(buf), stdin);
    len = strlen(buf); if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    if (strlen(buf) > 0) { snprintf(new_s.phone, MAX_PHONE, "%s", buf); }
    printf("  地址 [%s]: ", old->address);
    fgets(buf, sizeof(buf), stdin);
    len = strlen(buf); if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    if (strlen(buf) > 0) { snprintf(new_s.address, MAX_ADDRESS, "%s", buf); }
    StuStatus status = student_update(db, idx, &new_s);
    if (status == STU_OK) { print_success("修改成功！"); db_save(db); }
    else if (status == STU_ERR_DUPLICATE) print_error("新学号已存在！");
    else print_error("修改失败！");
}

static void menu_query(StudentDB *db)
{
    char id[MAX_ID];
    print_title("查询学生信息");
    safe_input("  请输入要查询的学号: ", id, MAX_ID);
    int idx = db_find_by_id(db, id);
    if (idx < 0) { print_error("未找到该学生！"); return; }
    print_student(student_get(db, idx), idx);
}

static void menu_search(StudentDB *db)
{
    char keyword[MAX_NAME];
    int results[256];
    print_title("按姓名模糊搜索");
    safe_input("  请输入姓名关键词: ", keyword, MAX_NAME);
    int found = student_search_by_name(db, keyword, results, 256);
    if (found == 0) { print_warn("未找到匹配的学生！"); return; }
    c_print(COLOR_GREEN, "\n  找到 %d 条匹配记录:\n\n", found);
    for (int i = 0; i < found; i++) print_student(student_get(db, results[i]), results[i]);
}

static void menu_stats(StudentDB *db)
{
    print_title("统计信息");
    if (db->count == 0) { print_warn("暂无数据，无法统计！"); return; }
    c_print(COLOR_CYAN, "  学生总数:     %d\n", student_count(db));
    c_print(COLOR_CYAN, "  平均年龄:     %.2f 岁\n", stats_avg_age(db));
    c_print(COLOR_CYAN, "  平均GPA:      %.2f\n", stats_avg_gpa(db));
    c_print(COLOR_CYAN, "  男生人数:     %d\n", stats_count_by_gender(db, "男"));
    c_print(COLOR_CYAN, "  女生人数:     %d\n", stats_count_by_gender(db, "女"));
    char cls[MAX_CLASS];
    safe_input("\n  输入班级名称统计人数(直接回车跳过): ", cls, MAX_CLASS);
    if (strlen(cls) > 0) c_print(COLOR_MAGENTA, "  班级 [%s] 人数: %d\n", cls, stats_count_by_class(db, cls));
    printf("\n");
}

static void menu_main(StudentDB *db)
{
    int choice;
    while (1) {
        printf("\n");
        c_print(COLOR_BOLD COLOR_BG_BLUE, "============================================================");
        printf("\n");
        c_print(COLOR_BOLD COLOR_WHITE, "           学生学籍管理系统 V2.0 - 主菜单");
        printf("\n");
        c_print(COLOR_BOLD COLOR_BG_BLUE, "============================================================");
        printf("\n\n");
        c_print(COLOR_GREEN, "  1. 添加学生信息\n");
        c_print(COLOR_GREEN, "  2. 删除学生信息\n");
        c_print(COLOR_GREEN, "  3. 修改学生信息\n");
        c_print(COLOR_GREEN, "  4. 按学号查询\n");
        c_print(COLOR_GREEN, "  5. 按姓名搜索\n");
        c_print(COLOR_GREEN, "  6. 显示所有学生\n");
        c_print(COLOR_GREEN, "  7. 统计信息\n");
        c_print(COLOR_GREEN, "  8. 保存到文件\n");
        c_print(COLOR_RED, "  0. 退出系统\n");
        print_line();
        choice = input_int("  请选择功能: ");
        switch (choice) {
        case 1: menu_add(db); break;
        case 2: menu_delete(db); break;
        case 3: menu_update(db); break;
        case 4: menu_query(db); break;
        case 5: menu_search(db); break;
        case 6: print_student_table(db); break;
        case 7: menu_stats(db); break;
        case 8:
            if (db_save(db) == STU_OK) print_success("数据保存成功！");
            else print_error("保存失败！");
            break;
        case 0:
            print_info("正在保存数据...");
            db_save(db);
            c_print(COLOR_BOLD COLOR_MAGENTA, "\n  感谢使用，再见！\n\n");
            return;
        default: print_error("无效选择，请重新输入！");
        }
    }
}

static void test_begin(const char *name) { c_print(COLOR_BOLD COLOR_CYAN, "\n=== 测试: %s ===\n", name); }

static void assert_true(int cond, const char *desc)
{
    if (cond) { c_print(COLOR_GREEN, "  [PASS] %s\n", desc); g_test_passed++; }
    else { c_print(COLOR_RED, "  [FAIL] %s\n", desc); g_test_failed++; }
}

static void assert_status(StuStatus actual, StuStatus expected, const char *desc)
{
    if (actual == expected) { c_print(COLOR_GREEN, "  [PASS] %s (返回值=%d)\n", desc, actual); g_test_passed++; }
    else { c_print(COLOR_RED, "  [FAIL] %s (期望=%d, 实际=%d)\n", desc, expected, actual); g_test_failed++; }
}

static void assert_int_eq(int actual, int expected, const char *desc)
{
    if (actual == expected) { c_print(COLOR_GREEN, "  [PASS] %s (%d)\n", desc, actual); g_test_passed++; }
    else { c_print(COLOR_RED, "  [FAIL] %s (期望=%d, 实际=%d)\n", desc, expected, actual); g_test_failed++; }
}

static void assert_float_eq(float actual, float expected, float eps, const char *desc)
{
    float diff = actual - expected;
    if (diff < 0) diff = -diff;
    if (diff <= eps) { c_print(COLOR_GREEN, "  [PASS] %s (%.2f)\n", desc, actual); g_test_passed++; }
    else { c_print(COLOR_RED, "  [FAIL] %s (期望=%.2f, 实际=%.2f)\n", desc, expected, actual); g_test_failed++; }
}

static void assert_str_eq(const char *actual, const char *expected, const char *desc)
{
    if (strcmp(actual, expected) == 0) { c_print(COLOR_GREEN, "  [PASS] %s (\"%s\")\n", desc, actual); g_test_passed++; }
    else { c_print(COLOR_RED, "  [FAIL] %s (期望=\"%s\", 实际=\"%s\")\n", desc, expected, actual); g_test_failed++; }
}

static void assert_not_null(const void *ptr, const char *desc)
{
    if (ptr != NULL) { c_print(COLOR_GREEN, "  [PASS] %s\n", desc); g_test_passed++; }
    else { c_print(COLOR_RED, "  [FAIL] %s (指针为NULL)\n", desc); g_test_failed++; }
}

static void assert_null(const void *ptr, const char *desc)
{
    if (ptr == NULL) { c_print(COLOR_GREEN, "  [PASS] %s\n", desc); g_test_passed++; }
    else { c_print(COLOR_RED, "  [FAIL] %s (指针非NULL)\n", desc); g_test_failed++; }
}

static void run_all_tests(void)
{
    StudentDB db;
    const char *test_file = "test_students_v2.dat";

    c_print(COLOR_BOLD COLOR_MAGENTA, "\n");
    c_print(COLOR_BOLD COLOR_MAGENTA, "============================================================\n");
    c_print(COLOR_BOLD COLOR_MAGENTA, "           学生学籍管理系统 V2.0 - 内置自测程序\n");
    c_print(COLOR_BOLD COLOR_MAGENTA, "============================================================\n");

    test_begin("1. 数据库初始化");
    StuStatus s = db_init(&db, test_file);
    assert_status(s, STU_OK, "数据库初始化成功");
    assert_not_null(db.data, "数据指针非空");
    assert_int_eq(db.count, 0, "初始学生数为0");
    assert_int_eq(db.capacity, INIT_CAPACITY, "初始容量正确");

    test_begin("2. 添加学生");
    Student s1 = {"2024001", "张三", "男", 20, "计算机A班", "138******01", "北京市海淀区", 3.5f};
    s = student_add(&db, &s1);
    assert_status(s, STU_OK, "添加第一个学生成功");
    assert_int_eq(student_count(&db), 1, "学生数为1");
    Student s2 = {"2024002", "李四", "女", 19, "计算机A班", "138******02", "上海市浦东新区", 3.8f};
    s = student_add(&db, &s2);
    assert_status(s, STU_OK, "添加第二个学生成功");
    assert_int_eq(student_count(&db), 2, "学生数为2");

    test_begin("3. 重复学号检测");
    Student s1_dup = {"2024001", "张三重复", "男", 20, "计算机A班", "", "", 0};
    s = student_add(&db, &s1_dup);
    assert_status(s, STU_ERR_DUPLICATE, "重复学号被拒绝");
    assert_int_eq(student_count(&db), 2, "学生数保持2");

    test_begin("4. 按ID查找学生");
    int idx = db_find_by_id(&db, "2024001");
    assert_true(idx >= 0, "找到学号2024001");
    Student *found = student_get(&db, idx);
    assert_not_null(found, "查到的学生指针非空");
    assert_str_eq(found->name, "张三", "姓名为张三");
    idx = db_find_by_id(&db, "9999999");
    assert_true(idx < 0, "不存在的学号查找返回-1");

    test_begin("5. 获取学生信息");
    found = student_get(&db, 0);
    assert_not_null(found, "索引0学生存在");
    assert_str_eq(found->id, "2024001", "第一个学生学号正确");
    assert_int_eq(found->age, 20, "第一个学生年龄正确");
    assert_float_eq(found->gpa, 3.5f, 0.01f, "第一个学生GPA正确");
    found = student_get(&db, -1);
    assert_null(found, "负索引返回NULL");
    found = student_get(&db, 100);
    assert_null(found, "越界索引返回NULL");

    test_begin("6. 修改学生信息");
    Student updated = {"2024001", "张三丰", "男", 21, "计算机B班", "139******01", "武汉市武昌区", 3.9f};
    s = student_update(&db, 0, &updated);
    assert_status(s, STU_OK, "修改学生成功");
    found = student_get(&db, 0);
    assert_str_eq(found->name, "张三丰", "姓名已更新");
    assert_str_eq(found->className, "计算机B班", "班级已更新");
    assert_float_eq(found->gpa, 3.9f, 0.01f, "GPA已更新");

    test_begin("7. 修改时重复学号检测");
    Student bad_update = {"2024002", "重复", "男", 20, "", "", "", 0};
    s = student_update(&db, 0, &bad_update);
    assert_status(s, STU_ERR_DUPLICATE, "修改为重复学号被拒绝");

    test_begin("8. 动态数组扩容");
    int old_cap = db.capacity;
    Student tmp; memset(&tmp, 0, sizeof(tmp));
    for (int i = 3; i <= INIT_CAPACITY + 5; i++) {
        sprintf(tmp.id, "2024%03d", i);
        sprintf(tmp.name, "学生%d", i);
        strcpy(tmp.gender, (i % 2 == 0) ? "女" : "男");
        tmp.age = 18 + (i % 5);
        strcpy(tmp.className, "测试班级");
        tmp.gpa = 2.5f + (i % 15) * 0.1f;
        student_add(&db, &tmp);
    }
    assert_true(db.capacity > old_cap, "数组容量已自动扩容");
    assert_int_eq(student_count(&db), INIT_CAPACITY + 5, "添加扩容后学生数正确");

    test_begin("9. 姓名模糊搜索");
    int results[100];
    int found_count = student_search_by_name(&db, "学生", results, 100);
    assert_true(found_count >= INIT_CAPACITY + 3, "搜索'学生'找到多条记录");
    found_count = student_search_by_name(&db, "李四", results, 100);
    assert_int_eq(found_count, 1, "搜索'李四'找到1条");
    assert_int_eq(results[0], 1, "李四在正确位置");
    found_count = student_search_by_name(&db, "不存在的人", results, 100);
    assert_int_eq(found_count, 0, "搜索不存在姓名返回0");

    test_begin("10. 统计功能 - 平均年龄/GPA");
    StudentDB db_stat;
    db_init(&db_stat, "test_stat.dat");
    Student sa = {"S001", "A", "男", 20, "C1", "", "", 3.0f};
    Student sb = {"S002", "B", "女", 22, "C1", "", "", 4.0f};
    Student sc = {"S003", "C", "男", 24, "C2", "", "", 2.0f};
    student_add(&db_stat, &sa);
    student_add(&db_stat, &sb);
    student_add(&db_stat, &sc);
    assert_float_eq(stats_avg_age(&db_stat), 22.0f, 0.01f, "平均年龄22.0岁");
    assert_float_eq(stats_avg_gpa(&db_stat), 3.0f, 0.01f, "平均GPA3.0");
    db_free(&db_stat);

    test_begin("11. 统计功能 - 班级/性别统计");
    db_init(&db_stat, "test_stat2.dat");
    student_add(&db_stat, &sa);
    student_add(&db_stat, &sb);
    student_add(&db_stat, &sc);
    assert_int_eq(stats_count_by_class(&db_stat, "C1"), 2, "C1班2人");
    assert_int_eq(stats_count_by_class(&db_stat, "C2"), 1, "C2班1人");
    assert_int_eq(stats_count_by_gender(&db_stat, "男"), 2, "男生2人");
    assert_int_eq(stats_count_by_gender(&db_stat, "女"), 1, "女生1人");
    assert_int_eq(stats_count_by_class(&db_stat, "不存在"), 0, "不存在班级0人");
    db_free(&db_stat);

    test_begin("12. 删除学生");
    int cnt_before = student_count(&db);
    s = student_delete(&db, "2024002");
    assert_status(s, STU_OK, "删除李四成功");
    assert_int_eq(student_count(&db), cnt_before - 1, "学生数减1");
    s = student_delete(&db, "9999999");
    assert_status(s, STU_ERR_NOT_FOUND, "删除不存在学生失败");
    assert_int_eq(student_count(&db), cnt_before - 1, "学生数不变");

    test_begin("13. 文件保存 - STUSV2 Magic Header");
    remove(test_file);
    s = db_save(&db);
    assert_status(s, STU_OK, "保存文件成功");
    FILE *fp = fopen(test_file, "rb");
    assert_not_null(fp, "数据文件存在且可打开");
    if (fp) {
        char magic[MAGIC_LEN + 1] = {0};
        fread(magic, 1, MAGIC_LEN, fp);
        assert_str_eq(magic, MAGIC_HEADER, "Magic Header为STUSV2");
        int ver = 0;
        fread(&ver, sizeof(int), 1, fp);
        assert_int_eq(ver, FILE_VERSION, "文件版本正确");
        int saved_count = 0;
        fread(&saved_count, sizeof(int), 1, fp);
        assert_int_eq(saved_count, db.count, "保存的记录数正确");
        fclose(fp);
    }

    test_begin("14. 文件加载");
    StudentDB db2;
    db_init(&db2, test_file);
    s = db_load(&db2);
    assert_status(s, STU_OK, "加载文件成功");
    assert_int_eq(student_count(&db2), db.count, "加载后记录数一致");
    Student *first = student_get(&db2, 0);
    assert_not_null(first, "第一条记录非空");
    assert_str_eq(first->name, "张三丰", "加载后第一条姓名正确");
    db_free(&db2);

    test_begin("15. 参数校验 - 空指针/非法参数");
    assert_status(db_init(NULL, NULL), STU_ERR_NULL_PTR, "db_init空指针检测");
    assert_status(student_add(NULL, NULL), STU_ERR_NULL_PTR, "student_add空指针检测");
    assert_status(student_add(&db, NULL), STU_ERR_NULL_PTR, "student_add学生空指针");
    Student bad_stu = {"", "", "男", 20, "", "", "", 0};
    assert_status(student_add(&db, &bad_stu), STU_ERR_INVALID_PARAM, "空学号姓名被拒绝");

    test_begin("16. 非法Magic Header检测");
    const char *bad_file = "test_bad.dat";
    fp = fopen(bad_file, "wb");
    if (fp) {
        fprintf(fp, "BADHDR");
        fclose(fp);
        StudentDB db_bad;
        db_init(&db_bad, bad_file);
        s = db_load(&db_bad);
        assert_status(s, STU_ERR_FILE_MAGIC, "错误Magic Header被检测");
        db_free(&db_bad);
        remove(bad_file);
    } else {
        c_print(COLOR_YELLOW, "  [SKIP] 无法创建测试文件\n");
    }

    test_begin("17. 空数据库操作");
    StudentDB db_empty;
    db_init(&db_empty, "test_empty.dat");
    assert_int_eq(student_count(&db_empty), 0, "空库学生数0");
    assert_float_eq(stats_avg_age(&db_empty), 0.0f, 0.01f, "空库平均年龄0");
    assert_float_eq(stats_avg_gpa(&db_empty), 0.0f, 0.01f, "空库平均GPA0");
    assert_int_eq(stats_count_by_gender(&db_empty, "男"), 0, "空库男生0");
    assert_null(student_get(&db_empty, 0), "空库获取学生返回NULL");
    assert_status(student_delete(&db_empty, "xxx"), STU_ERR_NOT_FOUND, "空库删除失败");
    db_free(&db_empty);

    db_free(&db);
    remove(test_file);

    printf("\n");
    c_print(COLOR_BOLD COLOR_BG_BLUE, "============================================================");
    printf("\n");
    c_print(COLOR_BOLD, "                       测试总结\n");
    c_print(COLOR_BOLD COLOR_BG_BLUE, "============================================================");
    printf("\n\n");
    c_print(COLOR_GREEN, "  通过: %d\n", g_test_passed);
    c_print(COLOR_RED, "  失败: %d\n", g_test_failed);
    c_print(COLOR_BOLD, "  总计: %d\n", g_test_passed + g_test_failed);
    printf("\n");
    if (g_test_failed == 0) c_print(COLOR_BOLD COLOR_GREEN, "  所有测试全部通过！\n\n");
    else c_print(COLOR_BOLD COLOR_RED, "  有 %d 个测试失败！\n\n", g_test_failed);
}

static void print_help(void)
{
    c_print(COLOR_BOLD COLOR_CYAN, "\n");
    c_print(COLOR_BOLD COLOR_CYAN, "============================================================\n");
    c_print(COLOR_BOLD COLOR_CYAN, "           学生学籍管理系统 V2.0 - 使用帮助\n");
    c_print(COLOR_BOLD COLOR_CYAN, "============================================================\n\n");
    c_print(COLOR_WHITE, "  用法: stu.exe [选项]\n\n");
    c_print(COLOR_YELLOW, "  选项:\n");
    c_print(COLOR_GREEN, "    -h, --help       显示此帮助信息\n");
    c_print(COLOR_GREEN, "    -t, --test       运行内置自测程序\n");
    c_print(COLOR_GREEN, "    (无参数)          启动交互模式\n\n");
    c_print(COLOR_YELLOW, "  交互模式功能:\n");
    c_print(COLOR_WHITE, "    1. 添加学生信息     6. 显示所有学生\n");
    c_print(COLOR_WHITE, "    2. 删除学生信息     7. 统计信息\n");
    c_print(COLOR_WHITE, "    3. 修改学生信息     8. 保存到文件\n");
    c_print(COLOR_WHITE, "    4. 按学号查询       0. 退出系统\n");
    c_print(COLOR_WHITE, "    5. 按姓名搜索\n\n");
    c_print(COLOR_YELLOW, "  文件格式:\n");
    c_print(COLOR_WHITE, "    - 二进制文件\n");
    c_print(COLOR_WHITE, "    - Magic Header: STUSV2\n");
    c_print(COLOR_WHITE, "    - 数据文件: students.dat\n\n");
    c_print(COLOR_BOLD COLOR_MAGENTA, "  作者: 数据库探索  |  学号: 424123888888  |  班级: 计算机科学A班\n\n");
}

int main(int argc, char *argv[])
{
    console_init();
    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_help(); console_cleanup(); return 0;
        }
        if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0) {
            run_all_tests(); console_cleanup(); return (g_test_failed == 0) ? 0 : 1;
        }
        c_print(COLOR_RED, "未知选项: %s\n", argv[1]);
        c_print(COLOR_YELLOW, "使用 -h 或 --help 查看帮助\n\n");
        console_cleanup(); return 1;
    }
    StudentDB db;
    StuStatus s = db_init(&db, "students.dat");
    if (s != STU_OK) { print_error("数据库初始化失败！"); console_cleanup(); return 1; }
    s = db_load(&db);
    if (s == STU_OK) {
        char msg[128];
        sprintf(msg, "已加载 %d 条学生记录", db.count);
        print_success(msg);
    } else if (s != STU_ERR_FILE_OPEN) {
        print_warn("数据文件加载失败，将使用空数据库");
    }
    c_print(COLOR_BOLD COLOR_MAGENTA, "\n");
    c_print(COLOR_BOLD COLOR_MAGENTA, "  +----------------------------------------------------------+\n");
    c_print(COLOR_BOLD COLOR_MAGENTA, "  |");
    c_print(COLOR_BOLD COLOR_WHITE, "           欢迎使用学生学籍管理系统 V2.0                 ");
    c_print(COLOR_BOLD COLOR_MAGENTA, "|\n");
    c_print(COLOR_BOLD COLOR_MAGENTA, "  +----------------------------------------------------------+\n");
    menu_main(&db);
    db_free(&db);
    console_cleanup();
    return 0;
}
