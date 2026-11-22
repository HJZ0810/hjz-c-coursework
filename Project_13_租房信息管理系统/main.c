
/*
 * Rent Information Management System V2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

#define ANSI_RESET   "\x1b[0m"
#define ANSI_BOLD    "\x1b[1m"
#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN    "\x1b[36m"
#define ANSI_WHITE   "\x1b[37m"
#define C_HDR  ANSI_BOLD ANSI_CYAN
#define C_OK   ANSI_GREEN
#define C_ERR  ANSI_RED
#define C_WARN ANSI_YELLOW
#define C_INFO ANSI_BLUE
#define C_PMT  ANSI_MAGENTA

#define MAX_STR   128
#define INIT_CAP  8
#define MAGIC     "RENTV2"
#define MAGIC_LEN 6
#define DATAFILE  "rental_v2.dat"

typedef enum {
    RENT_OK = 0, RENT_ERR_MEMORY = -1, RENT_ERR_NOT_FOUND = -2,
    RENT_ERR_DUPLICATE = -3, RENT_ERR_PARAM = -4, RENT_ERR_IO = -5,
    RENT_ERR_MAGIC = -6, RENT_ERR_RENTED = -7, RENT_ERR_ACTIVE = -8,
    RENT_ERR_NOT_RENTED = -9, RENT_ERR_CONTRACT = -11
} RentStatus;

typedef struct { char addr[MAX_STR],cert[MAX_STR],layout[MAX_STR],deco[MAX_STR]; double area,rent; int isRented; } House;
typedef struct { char id[MAX_STR],name[MAX_STR],work[MAX_STR],phone[MAX_STR]; int years; double subsidy; } Tenant;
typedef struct { char tid[MAX_STR],hcert[MAX_STR],cin[MAX_STR],cno[MAX_STR],rmk[MAX_STR]; int active; } Rental;
typedef struct { House* d; int n,cap; } HA;
typedef struct { Tenant* d; int n,cap; } TenantArr;
typedef struct { Rental* d; int n,cap; } RA;
typedef struct { HA h; TenantArr t; RA r; } DB;

static DB g_db; static int gpass=0,gfail=0;

static void consoleInit(void) {
#ifdef _WIN32
  SetConsoleOutputCP(65001); SetConsoleCP(65001);
  HANDLE h=GetStdHandle(STD_OUTPUT_HANDLE); DWORD m=0;
  if(GetConsoleMode(h,&m)){m|=0x0004;SetConsoleMode(h,m);}
#endif
}

static void clrIn(void){int c;while((c=getchar())!='\n'&&c!=EOF);}
static void rStr(const char*p,char*b,int s){printf("%s%s"ANSI_RESET,C_PMT,p);if(fgets(b,s,stdin)){int l=strlen(b);if(l>0&&b[l-1]=='\n')b[l-1]=0;}}
static int rInt(const char*p){int v;printf("%s%s"ANSI_RESET,C_PMT,p);while(scanf("%d",&v)!=1){clrIn();}clrIn();return v;}
static double rDbl(const char*p){double v;printf("%s%s"ANSI_RESET,C_PMT,p);while(scanf("%lf",&v)!=1){clrIn();}clrIn();return v;}
static void pause(void){printf(ANSI_WHITE"Press Enter..."ANSI_RESET);getchar();}
static void divr(void){printf(ANSI_WHITE"----------------------------------------"ANSI_RESET"\n");}
static void hdr(const char*t){printf("\n"C_HDR"=== %s ==="ANSI_RESET"\n",t);}
static void ok(const char*m){printf(C_OK"[OK] %s"ANSI_RESET"\n",m);}
static void err(const char*m){printf(C_ERR"[ERR] %s"ANSI_RESET"\n",m);}
static void info(const char*m){printf(C_INFO"[*] %s"ANSI_RESET"\n",m);}
static void warn(const char*m){printf(C_WARN"[!] %s"ANSI_RESET"\n",m);}

static RentStatus hEns(HA*a,int mc){int nc=a->cap;House*p;while(nc<mc)nc*=2;if(nc==a->cap)return RENT_OK;p=(House*)realloc(a->d,sizeof(House)*nc);if(!p)return RENT_ERR_MEMORY;a->d=p;a->cap=nc;return RENT_OK;}
#define hInit(a) ((a)->d=(House*)malloc(sizeof(House)*INIT_CAP),(a)->d?((a)->n=0,(a)->cap=INIT_CAP,RENT_OK):RENT_ERR_MEMORY)
#define hGrow(a) hEns(a,(a)->n+1)
static RentStatus tEns(TenantArr*a,int mc){int nc=a->cap;Tenant*p;while(nc<mc)nc*=2;if(nc==a->cap)return RENT_OK;p=(Tenant*)realloc(a->d,sizeof(Tenant)*nc);if(!p)return RENT_ERR_MEMORY;a->d=p;a->cap=nc;return RENT_OK;}
#define tInit(a) ((a)->d=(Tenant*)malloc(sizeof(Tenant)*INIT_CAP),(a)->d?((a)->n=0,(a)->cap=INIT_CAP,RENT_OK):RENT_ERR_MEMORY)
#define tGrow(a) tEns(a,(a)->n+1)
static RentStatus rEns(RA*a,int mc){int nc=a->cap;Rental*p;while(nc<mc)nc*=2;if(nc==a->cap)return RENT_OK;p=(Rental*)realloc(a->d,sizeof(Rental)*nc);if(!p)return RENT_ERR_MEMORY;a->d=p;a->cap=nc;return RENT_OK;}
#define rInit(a) ((a)->d=(Rental*)malloc(sizeof(Rental)*INIT_CAP),(a)->d?((a)->n=0,(a)->cap=INIT_CAP,RENT_OK):RENT_ERR_MEMORY)
#define rGrow(a) rEns(a,(a)->n+1)

static RentStatus dbInit(DB*db){RentStatus s;if((s=hInit(&db->h))!=RENT_OK)return s;if((s=tInit(&db->t))!=RENT_OK)return s;if((s=rInit(&db->r))!=RENT_OK)return s;return RENT_OK;}
static void dbFree(DB*db){free(db->h.d);free(db->t.d);free(db->r.d);db->h.d=NULL;db->t.d=NULL;db->r.d=NULL;db->h.n=db->h.cap=db->t.n=db->t.cap=db->r.n=db->r.cap=0;}
static int hFind(DB*db,const char*c){int i;for(i=0;i<db->h.n;i++)if(strcmp(db->h.d[i].cert,c)==0)return i;return -1;}
static int tFind(DB*db,const char*id){int i;for(i=0;i<db->t.n;i++)if(strcmp(db->t.d[i].id,id)==0)return i;return -1;}
static int rFindCno(DB*db,const char*cn){int i;for(i=0;i<db->r.n;i++)if(strcmp(db->r.d[i].cno,cn)==0)return i;return -1;}
static int rFindActive(DB*db,const char*hc){int i;for(i=0;i<db->r.n;i++)if(strcmp(db->r.d[i].hcert,hc)==0&&db->r.d[i].active)return i;return -1;}
static int tActive(DB*db,const char*id){int i,c=0;for(i=0;i<db->r.n;i++)if(strcmp(db->r.d[i].tid,id)==0&&db->r.d[i].active)c++;return c;}

static RentStatus addHouse(DB*db,const char*addr,const char*cert,const char*lay,double area,const char*deco,double rent){House*h;if(!addr||!cert||!lay||!deco||!addr[0]||!cert[0]||area<=0||rent<0)return RENT_ERR_PARAM;if(hFind(db,cert)>=0)return RENT_ERR_DUPLICATE;if(hGrow(&db->h)!=RENT_OK)return RENT_ERR_MEMORY;h=&db->h.d[db->h.n];strncpy(h->addr,addr,MAX_STR-1);h->addr[MAX_STR-1]=0;strncpy(h->cert,cert,MAX_STR-1);h->cert[MAX_STR-1]=0;strncpy(h->layout,lay,MAX_STR-1);h->layout[MAX_STR-1]=0;strncpy(h->deco,deco,MAX_STR-1);h->deco[MAX_STR-1]=0;h->area=area;h->rent=rent;h->isRented=0;db->h.n++;return RENT_OK;}
static RentStatus delHouse(DB*db,const char*cert){int i,idx=hFind(db,cert);if(idx<0)return RENT_ERR_NOT_FOUND;if(db->h.d[idx].isRented||rFindActive(db,cert)>=0)return RENT_ERR_ACTIVE;for(i=idx;i<db->h.n-1;i++)db->h.d[i]=db->h.d[i+1];db->h.n--;return RENT_OK;}
static RentStatus addTenant(DB*db,const char*id,const char*nm,const char*wk,int yrs,const char*ph,double sub){Tenant*t;if(!id||!nm||!wk||!ph||!id[0]||!nm[0]||yrs<0||sub<0)return RENT_ERR_PARAM;if(tFind(db,id)>=0)return RENT_ERR_DUPLICATE;if(tGrow(&db->t)!=RENT_OK)return RENT_ERR_MEMORY;t=&db->t.d[db->t.n];strncpy(t->id,id,MAX_STR-1);t->id[MAX_STR-1]=0;strncpy(t->name,nm,MAX_STR-1);t->name[MAX_STR-1]=0;strncpy(t->work,wk,MAX_STR-1);t->work[MAX_STR-1]=0;strncpy(t->phone,ph,MAX_STR-1);t->phone[MAX_STR-1]=0;t->years=yrs;t->subsidy=sub;db->t.n++;return RENT_OK;}
static RentStatus delTenant(DB*db,const char*id){int i,idx=tFind(db,id);if(idx<0)return RENT_ERR_NOT_FOUND;if(tActive(db,id)>0)return RENT_ERR_ACTIVE;for(i=idx;i<db->t.n-1;i++)db->t.d[i]=db->t.d[i+1];db->t.n--;return RENT_OK;}
static RentStatus doRent(DB*db,const char*tid,const char*hc,const char*cin,const char*cno,const char*rmk){int ti,hi;Rental*r;if(!tid||!hc||!cin||!cno)return RENT_ERR_PARAM;ti=tFind(db,tid);if(ti<0)return RENT_ERR_NOT_FOUND;hi=hFind(db,hc);if(hi<0)return RENT_ERR_NOT_FOUND;if(db->h.d[hi].isRented||rFindActive(db,hc)>=0)return RENT_ERR_RENTED;if(rFindCno(db,cno)>=0)return RENT_ERR_CONTRACT;if(rGrow(&db->r)!=RENT_OK)return RENT_ERR_MEMORY;r=&db->r.d[db->r.n];strncpy(r->tid,tid,MAX_STR-1);r->tid[MAX_STR-1]=0;strncpy(r->hcert,hc,MAX_STR-1);r->hcert[MAX_STR-1]=0;strncpy(r->cin,cin,MAX_STR-1);r->cin[MAX_STR-1]=0;strncpy(r->cno,cno,MAX_STR-1);r->cno[MAX_STR-1]=0;if(rmk)strncpy(r->rmk,rmk,MAX_STR-1);else r->rmk[0]=0;r->rmk[MAX_STR-1]=0;r->active=1;db->r.n++;db->h.d[hi].isRented=1;return RENT_OK;}
static RentStatus doCheckout(DB*db,const char*hc){int hi,ri;if(!hc)return RENT_ERR_PARAM;hi=hFind(db,hc);if(hi<0)return RENT_ERR_NOT_FOUND;if(!db->h.d[hi].isRented)return RENT_ERR_NOT_RENTED;ri=rFindActive(db,hc);if(ri<0)return RENT_ERR_NOT_FOUND;db->r.d[ri].active=0;db->h.d[hi].isRented=0;return RENT_OK;}

static RentStatus dbSave(DB*db,const char*fn){FILE*fp=fopen(fn,"wb");if(!fp)return RENT_ERR_IO;if(fwrite(MAGIC,1,MAGIC_LEN,fp)!=(size_t)MAGIC_LEN){fclose(fp);return RENT_ERR_IO;}if(fwrite(&db->h.n,sizeof(int),1,fp)!=1){fclose(fp);return RENT_ERR_IO;}if(db->h.n>0&&fwrite(db->h.d,sizeof(House),(size_t)db->h.n,fp)!=(size_t)db->h.n){fclose(fp);return RENT_ERR_IO;}if(fwrite(&db->t.n,sizeof(int),1,fp)!=1){fclose(fp);return RENT_ERR_IO;}if(db->t.n>0&&fwrite(db->t.d,sizeof(Tenant),(size_t)db->t.n,fp)!=(size_t)db->t.n){fclose(fp);return RENT_ERR_IO;}if(fwrite(&db->r.n,sizeof(int),1,fp)!=1){fclose(fp);return RENT_ERR_IO;}if(db->r.n>0&&fwrite(db->r.d,sizeof(Rental),(size_t)db->r.n,fp)!=(size_t)db->r.n){fclose(fp);return RENT_ERR_IO;}fclose(fp);return RENT_OK;}
static RentStatus dbLoad(DB*db,const char*fn){FILE*fp;char mg[MAGIC_LEN+1]={0};int hn=0,tn=0,rn=0;fp=fopen(fn,"rb");if(!fp)return RENT_ERR_IO;if(fread(mg,1,MAGIC_LEN,fp)!=(size_t)MAGIC_LEN){fclose(fp);return RENT_ERR_MAGIC;}if(strcmp(mg,MAGIC)!=0){fclose(fp);return RENT_ERR_MAGIC;}if(fread(&hn,sizeof(int),1,fp)!=1||hn<0||hn>100000){fclose(fp);return RENT_ERR_IO;}if(hn>0){if(hEns(&db->h,hn)!=RENT_OK){fclose(fp);return RENT_ERR_MEMORY;}if(fread(db->h.d,sizeof(House),(size_t)hn,fp)!=(size_t)hn){fclose(fp);return RENT_ERR_IO;}db->h.n=hn;}if(fread(&tn,sizeof(int),1,fp)!=1||tn<0||tn>100000){fclose(fp);return RENT_ERR_IO;}if(tn>0){if(tEns(&db->t,tn)!=RENT_OK){fclose(fp);return RENT_ERR_MEMORY;}if(fread(db->t.d,sizeof(Tenant),(size_t)tn,fp)!=(size_t)tn){fclose(fp);return RENT_ERR_IO;}db->t.n=tn;}if(fread(&rn,sizeof(int),1,fp)!=1||rn<0||rn>100000){fclose(fp);return RENT_ERR_IO;}if(rn>0){if(rEns(&db->r,rn)!=RENT_OK){fclose(fp);return RENT_ERR_MEMORY;}if(fread(db->r.d,sizeof(Rental),(size_t)rn,fp)!=(size_t)rn){fclose(fp);return RENT_ERR_IO;}db->r.n=rn;}fclose(fp);return RENT_OK;}

static void listH(DB*db){int i;House*h;hdr("List Houses");if(db->h.n==0){warn("Empty");return;}divr();for(i=0;i<db->h.n;i++){h=&db->h.d[i];printf("%-18s %-8s %6.1fm2 %8.0fy %s\n",h->cert,h->layout,h->area,h->rent,h->isRented?"RENT":"FREE");}divr();}
static void listT(DB*db){int i;Tenant*t;hdr("List Tenants");if(db->t.n==0){warn("Empty");return;}divr();for(i=0;i<db->t.n;i++){t=&db->t.d[i];printf("%-20s %-8s %-12s %dy %s\n",t->id,t->name,t->work,t->years,t->phone);}divr();}
static void listR(DB*db){int i,c=0;Rental*r;hdr("List Rentals");divr();for(i=0;i<db->r.n;i++){r=&db->r.d[i];if(!r->active)continue;printf("%-14s %-20s %-18s %s\n",r->cno,r->tid,r->hcert,r->cin);c++;}divr();printf("Active: %d\n",c);}

static void mAddH(DB*db){char a[MAX_STR],c[MAX_STR],l[MAX_STR],d[MAX_STR];double ar,rn;RentStatus s;hdr("Add House");rStr("Addr:",a,MAX_STR);rStr("Cert:",c,MAX_STR);rStr("Layout:",l,MAX_STR);ar=rDbl("Area:");rStr("Decor:",d,MAX_STR);rn=rDbl("Rent:");s=addHouse(db,a,c,l,ar,d,rn);if(s==RENT_OK)ok("Added");else err("Fail");}
static void mAddT(DB*db){char id[MAX_STR],nm[MAX_STR],wk[MAX_STR],ph[MAX_STR];int yr;double sb;RentStatus s;hdr("Add Tenant");rStr("ID:",id,MAX_STR);rStr("Name:",nm,MAX_STR);rStr("Work:",wk,MAX_STR);yr=rInt("Years:");rStr("Phone:",ph,MAX_STR);sb=rDbl("Subsidy:");s=addTenant(db,id,nm,wk,yr,ph,sb);if(s==RENT_OK)ok("Added");else err("Fail");}
static void mRent(DB*db){char tid[MAX_STR],hc[MAX_STR],ci[MAX_STR],cn[MAX_STR],rm[MAX_STR];RentStatus s;hdr("Rent");rStr("Tenant ID:",tid,MAX_STR);rStr("Cert:",hc,MAX_STR);rStr("Check-in:",ci,MAX_STR);rStr("Contract:",cn,MAX_STR);rStr("Remark:",rm,MAX_STR);s=doRent(db,tid,hc,ci,cn,rm[0]?rm:NULL);if(s==RENT_OK)ok("Rented");else err("Fail");}
static void mOut(DB*db){char c[MAX_STR];RentStatus s;hdr("Checkout");rStr("Cert:",c,MAX_STR);s=doCheckout(db,c);if(s==RENT_OK)ok("Done");else err("Fail");}
static void mDelH(DB*db){char c[MAX_STR];RentStatus s;hdr("Delete House");rStr("Cert:",c,MAX_STR);s=delHouse(db,c);if(s==RENT_OK)ok("Deleted");else err("Fail");}
static void mDelT(DB*db){char id[MAX_STR];RentStatus s;hdr("Delete Tenant");rStr("ID:",id,MAX_STR);s=delTenant(db,id);if(s==RENT_OK)ok("Deleted");else err("Fail");}

static void menu(DB*db){int ch;do{printf("\n");divr();printf(C_HDR"    Rent System V2.0"ANSI_RESET"\n");divr();printf(ANSI_BOLD" 1."ANSI_RESET" Add House    "ANSI_BOLD" 2."ANSI_RESET" Add Tenant\n");printf(ANSI_BOLD" 3."ANSI_RESET" List Houses  "ANSI_BOLD" 4."ANSI_RESET" List Tenants\n");printf(ANSI_BOLD" 5."ANSI_RESET" Rent         "ANSI_BOLD" 6."ANSI_RESET" Checkout\n");printf(ANSI_BOLD" 7."ANSI_RESET" List Rentals "ANSI_BOLD" 8."ANSI_RESET" Del House\n");printf(ANSI_BOLD" 9."ANSI_RESET" Del Tenant   "ANSI_BOLD"10."ANSI_RESET" Save\n");printf(ANSI_BOLD"11."ANSI_RESET" Load         "ANSI_BOLD" 0."ANSI_RESET" Exit\n");divr();ch=rInt("Choice:");switch(ch){case 1:mAddH(db);pause();break;case 2:mAddT(db);pause();break;case 3:listH(db);pause();break;case 4:listT(db);pause();break;case 5:mRent(db);pause();break;case 6:mOut(db);pause();break;case 7:listR(db);pause();break;case 8:mDelH(db);pause();break;case 9:mDelT(db);pause();break;case 10:if(dbSave(db,DATAFILE)==RENT_OK)ok("Saved");else err("Save fail");pause();break;case 11:dbFree(db);dbInit(db);if(dbLoad(db,DATAFILE)==RENT_OK)ok("Loaded");else err("Load fail");pause();break;case 0:info("Saving...");dbSave(db,DATAFILE);ok("Bye");break;default:err("Bad");pause();break;}}while(ch!=0);}

static void TA(int c,const char*n){if(c){printf(C_OK"  [PASS]"ANSI_RESET" %s\n",n);gpass++;}else{printf(C_ERR"  [FAIL]"ANSI_RESET" %s\n",n);gfail++;}}
static void runTest(void){DB db,db2;RentStatus s;int i,ok,idx;char c[32],a[64],l[16];FILE*fp;char mg[7]={0};int mok=0;printf(C_HDR"=== Rental System V2.0 Self Test ==="ANSI_RESET"\n\n");dbInit(&db);s=dbInit(&db);TA(s==RENT_OK,"T01: DB Init");s=addHouse(&db,"A1","H001","2-1",75.5,"JD",4500);TA(s==RENT_OK,"T02: Add House 1");s=addHouse(&db,"A2","H002","3-1",120,"JZ",6800);TA(s==RENT_OK,"T03: Add House 2");s=addHouse(&db,"A3","H003","1-1",55,"JD",3800);TA(s==RENT_OK,"T04: Add House 3");s=addHouse(&db,"X","H001","X",60,"X",3000);TA(s==RENT_ERR_DUPLICATE,"T05: Dup House");TA(db.h.n==3,"T06: Count=3");idx=hFind(&db,"H002");TA(idx==1&&db.h.d[idx].area==120,"T07: Find");s=addTenant(&db,"111","Zhang","BABA",5,"138",1000);TA(s==RENT_OK,"T08: Add Tenant 1");s=addTenant(&db,"222","Li","Tencent",3,"139",800);TA(s==RENT_OK,"T09: Add Tenant 2");s=addTenant(&db,"111","Dup","X",1,"X",0);TA(s==RENT_ERR_DUPLICATE,"T10: Dup Tenant");TA(db.t.n==2,"T11: TCount=2");s=doRent(&db,"111","H001","2025-03-01","C001","Test");TA(s==RENT_OK,"T12: Rent OK");s=doRent(&db,"222","H001","2025-03-02","C002",NULL);TA(s==RENT_ERR_RENTED,"T13: Re-rent Blocked");idx=hFind(&db,"H001");TA(idx>=0&&db.h.d[idx].isRented==1,"T14: Rented Flag");s=doRent(&db,"222","H002","2025-03-05","C001",NULL);TA(s==RENT_ERR_CONTRACT,"T15: Dup Contract");s=doRent(&db,"222","H002","2025-03-05","C002",NULL);TA(s==RENT_OK,"T16: Rent 2");s=doCheckout(&db,"H001");TA(s==RENT_OK,"T17: Checkout OK");idx=hFind(&db,"H001");TA(idx>=0&&db.h.d[idx].isRented==0,"T18: Checkout Clear");s=dbSave(&db,"test.dat");TA(s==RENT_OK,"T19: Save");dbInit(&db2);s=dbLoad(&db2,"test.dat");TA(s==RENT_OK,"T20: Load");TA(db2.h.n==3&&db2.t.n==2&&db2.r.n==2,"T21: Integrity");fp=fopen("test.dat","rb");mok=0;if(fp){fread(mg,1,6,fp);fclose(fp);mok=(strcmp(mg,MAGIC)==0);}TA(mok,"T22: Magic RENTV2");s=addHouse(&db,NULL,"C","L",50,"D",2000);TA(s==RENT_ERR_PARAM,"T23: NULL Param");s=delHouse(&db,"H002");TA(s==RENT_ERR_ACTIVE,"T24: Active Del Blocked");ok=1;for(i=100;i<200;i++){sprintf(c,"T%04d",i);sprintf(a,"Addr%d",i);sprintf(l,"Lay");if(addHouse(&db,a,c,l,60.0+i,"JZ",2000.0+i*10)!=RENT_OK){ok=0;break;}}TA(ok&&db.h.n==103,"T25: Dyn Grow");s=delTenant(&db,"222");TA(s==RENT_ERR_ACTIVE,"T26: Active Tenant Del");s=delHouse(&db,"NONE");TA(s==RENT_ERR_NOT_FOUND,"T27: Not Found");idx=tFind(&db,"NONE");TA(idx==-1,"T28: Not Found -1");dbFree(&db);dbFree(&db2);remove("test.dat");printf("\n"C_HDR"========================================"ANSI_RESET"\n");printf("Result: "C_OK"%d passed"ANSI_RESET", "C_ERR"%d failed"ANSI_RESET", "C_INFO"%d total"ANSI_RESET"\n",gpass,gfail,gpass+gfail);printf(C_HDR"========================================"ANSI_RESET"\n");if(gfail==0)printf("\n"C_OK"[ALL PASS]"ANSI_RESET"\n");else printf("\n"C_ERR"[FAIL %d]"ANSI_RESET"\n",gfail);}

static void help(const char*p){printf(C_HDR"Rent System V2.0 - Help"ANSI_RESET"\n\nUsage: %s [options]\n\n  --help,-h  Help\n  --test,-t  Self test\n  (no args)  Menu\n",p);}

int main(int argc,char*argv[]){RentStatus ls;consoleInit();if(argc>1){if(strcmp(argv[1],"--help")==0||strcmp(argv[1],"-h")==0){help(argv[0]);return 0;}if(strcmp(argv[1],"--test")==0||strcmp(argv[1],"-t")==0){runTest();return gfail>0?1:0;}printf(C_ERR"Unknown: %s"ANSI_RESET"\n",argv[1]);return 1;}if(dbInit(&g_db)!=RENT_OK){err("DB init fail");return 1;}ls=dbLoad(&g_db,DATAFILE);if(ls==RENT_OK)ok("Save loaded");else info("No save");printf("\n"C_HDR"Welcome to Rent System V2.0!"ANSI_RESET"\n");menu(&g_db);dbFree(&g_db);return 0;}
