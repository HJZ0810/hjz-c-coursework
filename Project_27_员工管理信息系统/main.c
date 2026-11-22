#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define MAGIC_HEADER "EMPV2"
#define MAGIC_LEN 5
#define FILE_VERSION 2
#define MAX_STR 64
#define INIT_CAP 8
#define DATA_FILE "empv2.dat"

#define C_RESET   "\033[0m"
#define C_RED     "\033[31m"
#define C_GREEN   "\033[32m"
#define C_YELLOW  "\033[33m"
#define C_CYAN    "\033[36m"

typedef enum {
    EMP_OK = 0,
    EMP_ERR_MEMORY,
    EMP_ERR_NOT_FOUND,
    EMP_ERR_DUPLICATE,
    EMP_ERR_INVALID_PARAM,
    EMP_ERR_FILE_OPEN,
    EMP_ERR_FILE_READ,
    EMP_ERR_FILE_WRITE,
    EMP_ERR_FILE_MAGIC,
    EMP_ERR_FILE_VERSION,
    EMP_ERR_FILE_CORRUPT,
    EMP_ERR_LOGIN_FAILED
} EmpStatus;

typedef struct {
    int id;
    char name[MAX_STR];
    double base_salary;
} Job;

typedef struct {
    int id;
    char name[MAX_STR];
    char manager[MAX_STR];
    char phone[MAX_STR];
} Dept;

typedef struct {
    int id;
    char name[MAX_STR];
    char gender[8];
    int age;
    int job_id;
    int dept_id;
    char phone[MAX_STR];
} Emp;

typedef struct {
    int id;
    int emp_id;
    int year;
    int month;
    double base_salary;
    double bonus;
    double deduction;
    double net_income;
} Income;

#define DEF_DYN_ARR(TypeName, type) \
typedef struct { type *data; int count; int cap; } TypeName##Array; \
static void TypeName##_init(TypeName##Array *a) { a->data=NULL; a->count=0; a->cap=0; } \
static EmpStatus TypeName##_push(TypeName##Array *a, const type *item) { \
    if (a->count >= a->cap) { \
        int nc = a->cap ? a->cap*2 : INIT_CAP; \
        type *nd = (type*)realloc(a->data, (size_t)nc*sizeof(type)); \
        if (!nd) { return EMP_ERR_MEMORY; } \
        a->data = nd; \
        a->cap = nc; \
    } \
    a->data[a->count++] = *item; \
    return EMP_OK; \
} \
static void TypeName##_free(TypeName##Array *a) { free(a->data); a->data=NULL; a->count=0; a->cap=0; }

DEF_DYN_ARR(Job, Job)
DEF_DYN_ARR(Dept, Dept)
DEF_DYN_ARR(Emp, Emp)
DEF_DYN_ARR(Income, Income)

static JobArray g_jobs;
static DeptArray g_depts;
static EmpArray g_emps;
static IncomeArray g_incomes;
static int nxt_job = 1, nxt_dept = 1, nxt_emp = 1, nxt_inc = 1;

static void console_init(void) {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD m = 0;
    GetConsoleMode(h, &m);
    SetConsoleMode(h, m | 0x0004);
#endif
}

static void line(char c, int n) { for (int i=0;i<n;i++) putchar(c); putchar('\n'); }

static void title(const char *s) {
    printf(C_CYAN); line('=',60); printf("  %s\n", s); line('=',60); printf(C_RESET);
}

static const char *status_str(EmpStatus s) {
    switch(s) {
        case EMP_OK: return "操作成功";
        case EMP_ERR_MEMORY: return "内存分配失败";
        case EMP_ERR_NOT_FOUND: return "未找到记录";
        case EMP_ERR_DUPLICATE: return "存在关联记录无法删除";
        case EMP_ERR_INVALID_PARAM: return "参数无效";
        case EMP_ERR_FILE_OPEN: return "无法打开文件";
        case EMP_ERR_FILE_READ: return "文件读取失败";
        case EMP_ERR_FILE_WRITE: return "文件写入失败";
        case EMP_ERR_FILE_MAGIC: return "文件格式错误";
        case EMP_ERR_FILE_VERSION: return "文件版本不兼容";
        case EMP_ERR_FILE_CORRUPT: return "文件损坏";
        case EMP_ERR_LOGIN_FAILED: return "登录失败";
        default: return "未知错误";
    }
}

static void err(EmpStatus s) { printf(C_RED "  [错误] %s\n" C_RESET, status_str(s)); }
static void ok(const char *m) { printf(C_GREEN "  [成功] %s\n" C_RESET, m); }

static int read_int(const char *p) {
    int v; char b[64];
    while(1) {
        printf("%s", p);
        if (fgets(b,sizeof(b),stdin)) { char *e; long x=strtol(b,&e,10);
          if(e!=b&&(*e=='\n'||*e=='\0'||*e==' ')){v=(int)x;break;} }
        printf(C_YELLOW "  输入无效，请输入整数: " C_RESET);
        while(fgets(b,sizeof(b),stdin)&&!strchr(b,'\n'));
    }
    return v;
}

static double read_dbl(const char *p) {
    double v; char b[64];
    while(1) {
        printf("%s", p);
        if (fgets(b,sizeof(b),stdin)) { char *e; double x=strtod(b,&e);
          if(e!=b&&(*e=='\n'||*e=='\0'||*e==' ')){v=x;break;} }
        printf(C_YELLOW "  输入无效，请输入数字: " C_RESET);
        while(fgets(b,sizeof(b),stdin)&&!strchr(b,'\n'));
    }
    return v;
}

static void read_str(const char *p, char *b, int sz) {
    printf("%s", p);
    if (fgets(b,sz,stdin)) { int l=(int)strlen(b); if(l>0&&b[l-1]=='\n')b[l-1]='\0'; }
}

static void pause_(void) { printf(C_YELLOW "\n  按回车继续..." C_RESET); getchar(); }

static Job* job_find(int id) { int i; for(i=0;i<g_jobs.count;i++) if(g_jobs.data[i].id==id) return &g_jobs.data[i]; return NULL; }
static Dept* dept_find(int id) { int i; for(i=0;i<g_depts.count;i++) if(g_depts.data[i].id==id) return &g_depts.data[i]; return NULL; }
static Emp* emp_find(int id) { int i; for(i=0;i<g_emps.count;i++) if(g_emps.data[i].id==id) return &g_emps.data[i]; return NULL; }
static Income* inc_find(int id) { int i; for(i=0;i<g_incomes.count;i++) if(g_incomes.data[i].id==id) return &g_incomes.data[i]; return NULL; }

#define REMOVE_AT(a, idx) do { int _k; for(_k=(idx);_k<(a)->count-1;_k++) (a)->data[_k]=(a)->data[_k+1]; (a)->count--; } while(0)

static EmpStatus job_add(const char *name, double sal) {
    Job j; if(!name||sal<0) return EMP_ERR_INVALID_PARAM;
    j.id=nxt_job++; strncpy(j.name,name,MAX_STR-1); j.name[MAX_STR-1]='\0'; j.base_salary=sal;
    return Job_push(&g_jobs,&j);
}

static EmpStatus dept_add(const char *name, const char *mgr, const char *ph) {
    Dept d; if(!name||!mgr||!ph) return EMP_ERR_INVALID_PARAM;
    d.id=nxt_dept++; strncpy(d.name,name,MAX_STR-1); d.name[MAX_STR-1]='\0';
    strncpy(d.manager,mgr,MAX_STR-1); d.manager[MAX_STR-1]='\0';
    strncpy(d.phone,ph,MAX_STR-1); d.phone[MAX_STR-1]='\0';
    return Dept_push(&g_depts,&d);
}

static EmpStatus emp_add(const char *name, const char *gen, int age, int jid, int did, const char *ph) {
    Emp e; if(!name||!gen||!ph) return EMP_ERR_INVALID_PARAM;
    if(age<16||age>100) return EMP_ERR_INVALID_PARAM;
    if(!job_find(jid)||!dept_find(did)) return EMP_ERR_INVALID_PARAM;
    e.id=nxt_emp++; strncpy(e.name,name,MAX_STR-1); e.name[MAX_STR-1]='\0';
    strncpy(e.gender,gen,7); e.gender[7]='\0';
    e.age=age; e.job_id=jid; e.dept_id=did;
    strncpy(e.phone,ph,MAX_STR-1); e.phone[MAX_STR-1]='\0';
    return Emp_push(&g_emps,&e);
}

static EmpStatus inc_add(int eid, int yr, int mo, double base, double bonus, double ded) {
    Income ic; if(!emp_find(eid)) return EMP_ERR_NOT_FOUND;
    if(mo<1||mo>12||yr<2000||yr>2100) return EMP_ERR_INVALID_PARAM;
    if(base<0||bonus<0||ded<0) return EMP_ERR_INVALID_PARAM;
    ic.id=nxt_inc++; ic.emp_id=eid; ic.year=yr; ic.month=mo;
    ic.base_salary=base; ic.bonus=bonus; ic.deduction=ded;
    ic.net_income=base+bonus-ded;
    return Income_push(&g_incomes,&ic);
}

static EmpStatus job_del(int id) {
    int i;
    for(i=0;i<g_emps.count;i++) if(g_emps.data[i].job_id==id) return EMP_ERR_DUPLICATE;
    for(i=0;i<g_jobs.count;i++) if(g_jobs.data[i].id==id){REMOVE_AT(&g_jobs,i);return EMP_OK;}
    return EMP_ERR_NOT_FOUND;
}

static EmpStatus dept_del(int id) {
    int i;
    for(i=0;i<g_emps.count;i++) if(g_emps.data[i].dept_id==id) return EMP_ERR_DUPLICATE;
    for(i=0;i<g_depts.count;i++) if(g_depts.data[i].id==id){REMOVE_AT(&g_depts,i);return EMP_OK;}
    return EMP_ERR_NOT_FOUND;
}

static EmpStatus emp_del(int id) {
    int i,found=0;
    for(i=g_incomes.count-1;i>=0;i--) if(g_incomes.data[i].emp_id==id) REMOVE_AT(&g_incomes,i);
    for(i=0;i<g_emps.count;i++) if(g_emps.data[i].id==id){REMOVE_AT(&g_emps,i);found=1;break;}
    return found?EMP_OK:EMP_ERR_NOT_FOUND;
}

static EmpStatus inc_del(int id) {
    int i;
    for(i=0;i<g_incomes.count;i++) if(g_incomes.data[i].id==id){REMOVE_AT(&g_incomes,i);return EMP_OK;}
    return EMP_ERR_NOT_FOUND;
}

typedef struct {
    int total_emps, total_depts, total_jobs, total_incomes;
    int male, female;
    double total_sal, avg_sal, max_sal, min_sal;
} Stats;

static Stats calc_stats(void) {
    Stats s; memset(&s,0,sizeof(s));
    s.total_emps=g_emps.count; s.total_depts=g_depts.count;
    s.total_jobs=g_jobs.count; s.total_incomes=g_incomes.count;
    s.max_sal=0; s.min_sal=1e18; s.male=0; s.female=0; s.total_sal=0;
    int i;
    for(i=0;i<g_emps.count;i++){
        if(strcmp(g_emps.data[i].gender,"男")==0) s.male++;
        else if(strcmp(g_emps.data[i].gender,"女")==0) s.female++;
    }
    for(i=0;i<g_incomes.count;i++){
        double n=g_incomes.data[i].net_income;
        s.total_sal+=n; if(n>s.max_sal)s.max_sal=n; if(n<s.min_sal)s.min_sal=n;
    }
    if(g_incomes.count>0) s.avg_sal=s.total_sal/g_incomes.count; else s.min_sal=0;
    return s;
}

static EmpStatus save_file(const char *path) {
    FILE *fp=fopen(path,"wb"); int ver;
    if(!fp)return EMP_ERR_FILE_OPEN;
    if(fwrite(MAGIC_HEADER,1,MAGIC_LEN,fp)!=MAGIC_LEN){fclose(fp);return EMP_ERR_FILE_WRITE;}
    ver=FILE_VERSION;
    if(fwrite(&ver,sizeof(int),1,fp)!=1){fclose(fp);return EMP_ERR_FILE_WRITE;}
    if(fwrite(&g_jobs.count,sizeof(int),1,fp)!=1){fclose(fp);return EMP_ERR_FILE_WRITE;}
    if(fwrite(&g_depts.count,sizeof(int),1,fp)!=1){fclose(fp);return EMP_ERR_FILE_WRITE;}
    if(fwrite(&g_emps.count,sizeof(int),1,fp)!=1){fclose(fp);return EMP_ERR_FILE_WRITE;}
    if(fwrite(&g_incomes.count,sizeof(int),1,fp)!=1){fclose(fp);return EMP_ERR_FILE_WRITE;}
    if(fwrite(&nxt_job,sizeof(int),1,fp)!=1){fclose(fp);return EMP_ERR_FILE_WRITE;}
    if(fwrite(&nxt_dept,sizeof(int),1,fp)!=1){fclose(fp);return EMP_ERR_FILE_WRITE;}
    if(fwrite(&nxt_emp,sizeof(int),1,fp)!=1){fclose(fp);return EMP_ERR_FILE_WRITE;}
    if(fwrite(&nxt_inc,sizeof(int),1,fp)!=1){fclose(fp);return EMP_ERR_FILE_WRITE;}
    if(g_jobs.count>0) if(fwrite(g_jobs.data,sizeof(Job),(size_t)g_jobs.count,fp)!=(size_t)g_jobs.count){fclose(fp);return EMP_ERR_FILE_WRITE;}
    if(g_depts.count>0) if(fwrite(g_depts.data,sizeof(Dept),(size_t)g_depts.count,fp)!=(size_t)g_depts.count){fclose(fp);return EMP_ERR_FILE_WRITE;}
    if(g_emps.count>0) if(fwrite(g_emps.data,sizeof(Emp),(size_t)g_emps.count,fp)!=(size_t)g_emps.count){fclose(fp);return EMP_ERR_FILE_WRITE;}
    if(g_incomes.count>0) if(fwrite(g_incomes.data,sizeof(Income),(size_t)g_incomes.count,fp)!=(size_t)g_incomes.count){fclose(fp);return EMP_ERR_FILE_WRITE;}
    fclose(fp); return EMP_OK;
}

static EmpStatus load_file(const char *path) {
    FILE *fp=fopen(path,"rb"); char mg[MAGIC_LEN+1]={0}; int ver,nj,nd,ne,ni;
    if(!fp)return EMP_ERR_FILE_OPEN;
    if(fread(mg,1,MAGIC_LEN,fp)!=MAGIC_LEN){fclose(fp);return EMP_ERR_FILE_READ;}
    if(memcmp(mg,MAGIC_HEADER,MAGIC_LEN)!=0){fclose(fp);return EMP_ERR_FILE_MAGIC;}
    if(fread(&ver,sizeof(int),1,fp)!=1){fclose(fp);return EMP_ERR_FILE_READ;}
    if(ver!=FILE_VERSION){fclose(fp);return EMP_ERR_FILE_VERSION;}
    if(fread(&nj,sizeof(int),1,fp)!=1){fclose(fp);return EMP_ERR_FILE_READ;}
    if(fread(&nd,sizeof(int),1,fp)!=1){fclose(fp);return EMP_ERR_FILE_READ;}
    if(fread(&ne,sizeof(int),1,fp)!=1){fclose(fp);return EMP_ERR_FILE_READ;}
    if(fread(&ni,sizeof(int),1,fp)!=1){fclose(fp);return EMP_ERR_FILE_READ;}
    if(fread(&nxt_job,sizeof(int),1,fp)!=1){fclose(fp);return EMP_ERR_FILE_READ;}
    if(fread(&nxt_dept,sizeof(int),1,fp)!=1){fclose(fp);return EMP_ERR_FILE_READ;}
    if(fread(&nxt_emp,sizeof(int),1,fp)!=1){fclose(fp);return EMP_ERR_FILE_READ;}
    if(fread(&nxt_inc,sizeof(int),1,fp)!=1){fclose(fp);return EMP_ERR_FILE_READ;}
    Job_free(&g_jobs); Dept_free(&g_depts); Emp_free(&g_emps); Income_free(&g_incomes);
    Job_init(&g_jobs); Dept_init(&g_depts); Emp_init(&g_emps); Income_init(&g_incomes);
    if(nj>0){g_jobs.data=(Job*)malloc((size_t)nj*sizeof(Job));g_jobs.cap=nj;
      if(fread(g_jobs.data,sizeof(Job),(size_t)nj,fp)!=(size_t)nj){fclose(fp);return EMP_ERR_FILE_CORRUPT;}g_jobs.count=nj;}
    if(nd>0){g_depts.data=(Dept*)malloc((size_t)nd*sizeof(Dept));g_depts.cap=nd;
      if(fread(g_depts.data,sizeof(Dept),(size_t)nd,fp)!=(size_t)nd){fclose(fp);return EMP_ERR_FILE_CORRUPT;}g_depts.count=nd;}
    if(ne>0){g_emps.data=(Emp*)malloc((size_t)ne*sizeof(Emp));g_emps.cap=ne;
      if(fread(g_emps.data,sizeof(Emp),(size_t)ne,fp)!=(size_t)ne){fclose(fp);return EMP_ERR_FILE_CORRUPT;}g_emps.count=ne;}
    if(ni>0){g_incomes.data=(Income*)malloc((size_t)ni*sizeof(Income));g_incomes.cap=ni;
      if(fread(g_incomes.data,sizeof(Income),(size_t)ni,fp)!=(size_t)ni){fclose(fp);return EMP_ERR_FILE_CORRUPT;}g_incomes.count=ni;}
    fclose(fp); return EMP_OK;
}

static void init_sample(void) {
    job_add("软件工程师",8000.0); job_add("产品经理",9000.0); job_add("测试工程师",7000.0);
    dept_add("研发部","张明","13800001111"); dept_add("市场部","李华","13800002222"); dept_add("人事部","王芳","13800003333");
    emp_add("张三","男",28,1,1,"13900001111"); emp_add("李四","女",25,2,2,"13900002222"); emp_add("王五","男",32,1,1,"13900003333");
    inc_add(1,2025,1,8000,2000,500); inc_add(2,2025,1,9000,1500,300); inc_add(3,2025,1,8000,1000,400);
}

static int login(void) {
    char u[64],p[64]; int a=3;
    while(a>0){
        printf(C_CYAN "\n  ===== 员工管理系统 V2.0 登录 =====\n" C_RESET);
        printf("  剩余尝试: %d\n",a);
        read_str("  用户名: ",u,sizeof(u)); read_str("  密  码: ",p,sizeof(p));
        if(strcmp(u,"admin")==0&&strcmp(p,"123456")==0){ok("登录成功！");return 1;}
        printf(C_RED "  用户名或密码错误!\n" C_RESET); a--;
    }
    err(EMP_ERR_LOGIN_FAILED); return 0;
}

static void list_jobs(void) {
    int i;
    title("职位管理");
    printf("  %-4s %-20s %-12s\n","ID","职位名称","基本工资");
    line('-',50);
    for(i=0;i<g_jobs.count;i++) printf("  %-4d %-20s %-12.2f\n",g_jobs.data[i].id,g_jobs.data[i].name,g_jobs.data[i].base_salary);
    printf("\n  共 %d 个职位\n",g_jobs.count);
}

static void list_depts(void) {
    int i;
    title("部门管理");
    printf("  %-4s %-16s %-12s %-16s\n","ID","部门名称","负责人","电话");
    line('-',60);
    for(i=0;i<g_depts.count;i++) printf("  %-4d %-16s %-12s %-16s\n",g_depts.data[i].id,g_depts.data[i].name,g_depts.data[i].manager,g_depts.data[i].phone);
    printf("\n  共 %d 个部门\n",g_depts.count);
}

static void list_emps(void) {
    int i;
    title("员工管理");
    printf("  %-4s %-10s %-6s %-6s %-10s %-12s %-16s\n","ID","姓名","性别","年龄","职位","部门","电话");
    line('-',70);
    for(i=0;i<g_emps.count;i++){
        Job *j=job_find(g_emps.data[i].job_id); Dept *d=dept_find(g_emps.data[i].dept_id);
        printf("  %-4d %-10s %-6s %-6d %-10s %-12s %-16s\n",g_emps.data[i].id,g_emps.data[i].name,g_emps.data[i].gender,
          g_emps.data[i].age,j?j->name:"?",d?d->name:"?",g_emps.data[i].phone);
    }
    printf("\n  共 %d 名员工\n",g_emps.count);
}

static void list_incomes(void) {
    int i;
    title("薪资管理");
    printf("  %-4s %-10s %-8s %-8s %-10s %-10s %-10s %-10s\n","ID","员工","年","月","基本工资","奖金","扣除","实发");
    line('-',80);
    for(i=0;i<g_incomes.count;i++){
        Emp *e=emp_find(g_incomes.data[i].emp_id);
        printf("  %-4d %-10s %-8d %-8d %-10.2f %-10.2f %-10.2f %-10.2f\n",g_incomes.data[i].id,e?e->name:"?",
          g_incomes.data[i].year,g_incomes.data[i].month,g_incomes.data[i].base_salary,g_incomes.data[i].bonus,
          g_incomes.data[i].deduction,g_incomes.data[i].net_income);
    }
    printf("\n  共 %d 条薪资记录\n",g_incomes.count);
}

static void menu_job(void) {
    int c;
    while(1){
        title("职位管理菜单");
        printf("  1. 查看职位列表\n  2. 添加职位\n  3. 删除职位\n  0. 返回\n");
        line('-',40);
        c=read_int("  请选择: ");
        if(c==0) break;
        if(c==1){list_jobs(); pause_();}
        else if(c==2){char n[MAX_STR]; double s=read_dbl("  基本工资: ");
          read_str("  职位名称: ",n,sizeof(n)); EmpStatus r=job_add(n,s);
          if(r==EMP_OK) ok("职位添加成功"); else err(r);
          save_file(DATA_FILE); pause_();}
        else if(c==3){int id=read_int("  职位ID: "); EmpStatus r=job_del(id);
          if(r==EMP_OK) ok("职位删除成功"); else err(r);
          save_file(DATA_FILE); pause_();}
    }
}

static void menu_dept(void) {
    int c;
    while(1){
        title("部门管理菜单");
        printf("  1. 查看部门列表\n  2. 添加部门\n  3. 删除部门\n  0. 返回\n");
        line('-',40);
        c=read_int("  请选择: ");
        if(c==0) break;
        if(c==1){list_depts(); pause_();}
        else if(c==2){char n[MAX_STR],m[MAX_STR],p[MAX_STR];
          read_str("  部门名称: ",n,sizeof(n)); read_str("  负责人: ",m,sizeof(m)); read_str("  电话: ",p,sizeof(p));
          EmpStatus r=dept_add(n,m,p); if(r==EMP_OK) ok("部门添加成功"); else err(r);
          save_file(DATA_FILE); pause_();}
        else if(c==3){int id=read_int("  部门ID: "); EmpStatus r=dept_del(id);
          if(r==EMP_OK) ok("部门删除成功"); else err(r);
          save_file(DATA_FILE); pause_();}
    }
}

static void menu_emp(void) {
    int c;
    while(1){
        title("员工管理菜单");
        printf("  1. 查看员工列表\n  2. 添加员工\n  3. 删除员工\n  0. 返回\n");
        line('-',40);
        c=read_int("  请选择: ");
        if(c==0) break;
        if(c==1){list_emps(); pause_();}
        else if(c==2){char n[MAX_STR],g[8],p[MAX_STR]; int age,jid,did;
          list_jobs(); list_depts();
          read_str("  姓名: ",n,sizeof(n)); read_str("  性别(男/女): ",g,sizeof(g));
          age=read_int("  年龄: "); jid=read_int("  职位ID: "); did=read_int("  部门ID: ");
          read_str("  电话: ",p,sizeof(p));
          EmpStatus r=emp_add(n,g,age,jid,did,p); if(r==EMP_OK) ok("员工添加成功"); else err(r);
          save_file(DATA_FILE); pause_();}
        else if(c==3){int id=read_int("  员工ID: "); EmpStatus r=emp_del(id);
          if(r==EMP_OK) ok("员工删除成功(相关薪资记录已级联删除)"); else err(r);
          save_file(DATA_FILE); pause_();}
    }
}

static void menu_inc(void) {
    int c;
    while(1){
        title("薪资管理菜单");
        printf("  1. 查看薪资记录\n  2. 添加薪资记录\n  3. 删除薪资记录\n  0. 返回\n");
        line('-',40);
        c=read_int("  请选择: ");
        if(c==0) break;
        if(c==1){list_incomes(); pause_();}
        else if(c==2){int eid,yr,mo; double b,bn,d;
          list_emps(); eid=read_int("  员工ID: "); yr=read_int("  年份: "); mo=read_int("  月份: ");
          b=read_dbl("  基本工资: "); bn=read_dbl("  奖金: "); d=read_dbl("  扣除: ");
          EmpStatus r=inc_add(eid,yr,mo,b,bn,d); if(r==EMP_OK) ok("薪资记录添加成功"); else err(r);
          save_file(DATA_FILE); pause_();}
        else if(c==3){int id=read_int("  记录ID: "); EmpStatus r=inc_del(id);
          if(r==EMP_OK) ok("薪资记录删除成功"); else err(r);
          save_file(DATA_FILE); pause_();}
    }
}

static void menu_stats(void) {
    Stats s=calc_stats(); int i;
    title("统计信息");
    printf("  职位总数: %d\n",s.total_jobs);
    printf("  部门总数: %d\n",s.total_depts);
    printf("  员工总数: %d (男: %d, 女: %d)\n",s.total_emps,s.male,s.female);
    printf("  薪资记录: %d\n",s.total_incomes);
    if(s.total_incomes>0){
        printf("  薪资总额: %.2f\n",s.total_sal);
        printf("  平均薪资: %.2f\n",s.avg_sal);
        printf("  最高薪资: %.2f\n",s.max_sal);
        printf("  最低薪资: %.2f\n",s.min_sal);
    }
    line('-',40);
    printf("  各部门人数:\n");
    for(i=0;i<g_depts.count;i++){
        int cnt=0,j;
        for(j=0;j<g_emps.count;j++) if(g_emps.data[j].dept_id==g_depts.data[i].id) cnt++;
        printf("    %s: %d人\n",g_depts.data[i].name,cnt);
    }
    pause_();
}

static void main_menu(void) {
    int c;
    while(1){
        title("员工管理信息系统 V2.0");
        printf("  1. 职位管理\n  2. 部门管理\n  3. 员工管理\n  4. 薪资管理\n  5. 统计信息\n  6. 保存数据\n  0. 退出系统\n");
        line('-',40);
        c=read_int("  请选择: ");
        if(c==0){ok("感谢使用，再见!"); save_file(DATA_FILE); break;}
        if(c==1) menu_job();
        else if(c==2) menu_dept();
        else if(c==3) menu_emp();
        else if(c==4) menu_inc();
        else if(c==5) menu_stats();
        else if(c==6){EmpStatus r=save_file(DATA_FILE); if(r==EMP_OK) ok("数据保存成功"); else err(r); pause_();}
    }
}

static int run_tests(void) {
    int pass=0,fail=0; EmpStatus st; Stats s;
    Job *j; Dept *d; Emp *e; Income *ic;
    const char *TF=DATA_FILE;
    printf(C_CYAN "========== 员工管理系统 V2.0 自测 ==========\n" C_RESET);

    Job_init(&g_jobs); Dept_init(&g_depts); Emp_init(&g_emps); Income_init(&g_incomes);
    nxt_job=1; nxt_dept=1; nxt_emp=1; nxt_inc=1;

    /* T1: 添加职位 */
    st=job_add("工程师",8000); printf("T01 添加职位... ");
    if(st==EMP_OK&&g_jobs.count==1){printf(C_GREEN "PASS\n" C_RESET);pass++;}else{printf(C_RED "FAIL\n" C_RESET);fail++;}

    /* T2: 添加部门 */
    st=dept_add("技术部","张经理","13800000001"); printf("T02 添加部门... ");
    if(st==EMP_OK&&g_depts.count==1){printf(C_GREEN "PASS\n" C_RESET);pass++;}else{printf(C_RED "FAIL\n" C_RESET);fail++;}

    /* T3: 添加员工(正常) */
    st=emp_add("张三","男",28,1,1,"13900000001"); printf("T03 添加员工... ");
    if(st==EMP_OK&&g_emps.count==1){printf(C_GREEN "PASS\n" C_RESET);pass++;}else{printf(C_RED "FAIL\n" C_RESET);fail++;}

    /* T4: 添加员工(无效年龄) */
    st=emp_add("李四","男",10,1,1,"13900000002"); printf("T04 无效年龄校验... ");
    if(st==EMP_ERR_INVALID_PARAM){printf(C_GREEN "PASS\n" C_RESET);pass++;}else{printf(C_RED "FAIL\n" C_RESET);fail++;}

    /* T5: 添加员工(无效职位) */
    st=emp_add("王五","男",30,999,1,"13900000003"); printf("T05 无效职位校验... ");
    if(st==EMP_ERR_INVALID_PARAM){printf(C_GREEN "PASS\n" C_RESET);pass++;}else{printf(C_RED "FAIL\n" C_RESET);fail++;}

    /* T6: 添加薪资 */
    st=inc_add(1,2025,1,8000,2000,500); printf("T06 添加薪资... ");
    if(st==EMP_OK&&g_incomes.count==1){printf(C_GREEN "PASS\n" C_RESET);pass++;}else{printf(C_RED "FAIL\n" C_RESET);fail++;}

    /* T7: 薪资计算验证 */
    ic=inc_find(1); printf("T07 薪资实发计算... ");
    if(ic&&ic->net_income==9500.0){printf(C_GREEN "PASS\n" C_RESET);pass++;}else{printf(C_RED "FAIL\n" C_RESET);fail++;}

    /* T8: 查找职位 */
    j=job_find(1); printf("T08 查找职位... ");
    if(j&&strcmp(j->name,"工程师")==0){printf(C_GREEN "PASS\n" C_RESET);pass++;}else{printf(C_RED "FAIL\n" C_RESET);fail++;}

    /* T9: 查找部门 */
    d=dept_find(1); printf("T09 查找部门... ");
    if(d&&strcmp(d->name,"技术部")==0){printf(C_GREEN "PASS\n" C_RESET);pass++;}else{printf(C_RED "FAIL\n" C_RESET);fail++;}

    /* T10: 查找员工 */
    e=emp_find(1); printf("T10 查找员工... ");
    if(e&&strcmp(e->name,"张三")==0){printf(C_GREEN "PASS\n" C_RESET);pass++;}else{printf(C_RED "FAIL\n" C_RESET);fail++;}

    /* T11: 添加第二条薪资用于统计 */
    st=inc_add(1,2025,2,8000,3000,400); printf("T11 添加第二条薪资... ");
    if(st==EMP_OK){printf(C_GREEN "PASS\n" C_RESET);pass++;}else{printf(C_RED "FAIL\n" C_RESET);fail++;}

    /* T12: 统计功能 */
    s=calc_stats(); printf("T12 统计-员工数... ");
    if(s.total_emps==1&&s.total_incomes==2&&s.male==1){printf(C_GREEN "PASS\n" C_RESET);pass++;}else{printf(C_RED "FAIL\n" C_RESET);fail++;}

    /* T13: 统计-平均薪资 */
    printf("T13 统计-平均薪资... ");
    if(s.avg_sal>0){printf(C_GREEN "PASS\n" C_RESET);pass++;}else{printf(C_RED "FAIL\n" C_RESET);fail++;}

    /* T14: 保存文件 */
    st=save_file(TF); printf("T14 保存文件... ");
    if(st==EMP_OK){printf(C_GREEN "PASS\n" C_RESET);pass++;}else{printf(C_RED "FAIL (%s)\n" C_RESET,status_str(st));fail++;}

    /* 清空后加载 */
    Job_free(&g_jobs); Dept_free(&g_depts); Emp_free(&g_emps); Income_free(&g_incomes);
    Job_init(&g_jobs); Dept_init(&g_depts); Emp_init(&g_emps); Income_init(&g_incomes);
    nxt_job=1; nxt_dept=1; nxt_emp=1; nxt_inc=1;

    /* T15: 加载文件 */
    st=load_file(TF); printf("T15 加载文件... ");
    if(st==EMP_OK&&g_emps.count==1){printf(C_GREEN "PASS\n" C_RESET);pass++;}else{printf(C_RED "FAIL (%s)\n" C_RESET,status_str(st));fail++;}

    /* T16: 魔数验证 */
    {FILE*fp=fopen(TF,"rb");char mg[6]={0};int ok_=0;
      if(fp&&fread(mg,1,5,fp)==5){if(memcmp(mg,MAGIC_HEADER,5)==0)ok_=1;fclose(fp);}
      printf("T16 EMPV2魔数验证... ");
      if(ok_){printf(C_GREEN "PASS\n" C_RESET);pass++;}else{printf(C_RED "FAIL\n" C_RESET);fail++;}}

    /* T17: 删除职位(有员工关联应失败) */
    st=job_del(1); printf("T17 关联保护-删除职位... ");
    if(st==EMP_ERR_DUPLICATE){printf(C_GREEN "PASS\n" C_RESET);pass++;}else{printf(C_RED "FAIL\n" C_RESET);fail++;}

    /* T18: 删除员工 */
    st=emp_del(1); printf("T18 删除员工(级联薪资)... ");
    if(st==EMP_OK&&g_emps.count==0&&g_incomes.count==0){printf(C_GREEN "PASS\n" C_RESET);pass++;}else{printf(C_RED "FAIL\n" C_RESET);fail++;}

    /* T19: 删除不存在记录 */
    st=emp_del(999); printf("T19 删除不存在记录... ");
    if(st==EMP_ERR_NOT_FOUND){printf(C_GREEN "PASS\n" C_RESET);pass++;}else{printf(C_RED "FAIL\n" C_RESET);fail++;}

    /* T20: 动态扩容(批量添加) */
    printf("T20 动态数组扩容... ");
    {int ok20=1,i;
      for(i=0;i<20;i++){char nm[32];sprintf(nm,"员工%d",i);
        if(i<5){char jn[32];sprintf(jn,"职位%d",i);job_add(jn,5000+i*100);}
        if(i<3){char dn[32],mn[32];sprintf(dn,"部门%d",i);sprintf(mn,"经理%d",i);dept_add(dn,mn,"13800000000");}
        if(emp_add(nm,"男",25,(i%5)+1,(i%3)+1,"13900000000")!=EMP_OK){ok20=0;break;}
      }
      if(ok20&&g_emps.count==20){printf(C_GREEN "PASS\n" C_RESET);pass++;}else{printf(C_RED "FAIL\n" C_RESET);fail++;}
    }

    /* 清理 */
    Job_free(&g_jobs); Dept_free(&g_depts); Emp_free(&g_emps); Income_free(&g_incomes);
    remove(TF);

    line('=',60);
    printf("  测试结果: %d 通过, %d 失败, 共 %d 项\n",pass,fail,pass+fail);
    if(fail==0) printf(C_GREEN "  ✔ 全部测试通过!\n" C_RESET);
    else printf(C_RED "  ✘ 有测试失败!\n" C_RESET);
    line('=',60);
    return fail;
}

static void print_help(const char *prog) {
    printf("员工管理信息系统 V2.0\n\n");
    printf("用法: %s [选项]\n\n",prog);
    printf("选项:\n");
    printf("  -h, --help     显示帮助信息\n");
    printf("  -t, --test     运行内置自测\n");
    printf("  (无参数)        启动交互菜单\n\n");
    printf("默认账号: admin / 123456\n");
    printf("数据文件: %s (EMPV2二进制格式)\n",DATA_FILE);
}

int main(int argc, char *argv[]) {
    int i;
    console_init();
    for(i=1;i<argc;i++){
        if(strcmp(argv[i],"-h")==0||strcmp(argv[i],"--help")==0){print_help(argv[0]);return 0;}
        if(strcmp(argv[i],"-t")==0||strcmp(argv[i],"--test")==0){return run_tests();}
    }
    Job_init(&g_jobs); Dept_init(&g_depts); Emp_init(&g_emps); Income_init(&g_incomes);
    nxt_job=1; nxt_dept=1; nxt_emp=1; nxt_inc=1;
    if(load_file(DATA_FILE)!=EMP_OK){init_sample();save_file(DATA_FILE);}
    if(login()) main_menu();
    Job_free(&g_jobs); Dept_free(&g_depts); Emp_free(&g_emps); Income_free(&g_incomes);
    return 0;
}
