#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define _CRT_SECURE_NO_WARNINGS

#define COLOR_RESET     "\033[0m"
#define COLOR_RED       "\033[31m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_BLUE      "\033[34m"
#define COLOR_MAGENTA   "\033[35m"
#define COLOR_CYAN      "\033[36m"
#define COLOR_WHITE     "\033[37m"
#define COLOR_BOLD      "\033[1m"

#define MAX_NAME        64
#define MAX_DATE        24
#define INIT_CAPACITY   8
#define MAGIC_HEADER    "FOODV2"
#define MAGIC_LEN       6
#define DATA_FILE       "foodv2.dat"

#define CATEGORY_MAIN   1
#define CATEGORY_SNACK  2
#define CATEGORY_DRINK  3
#define CATEGORY_OTHER  4

typedef enum {
    FOOD_OK = 0,
    FOOD_ERR_MEMORY,
    FOOD_ERR_NOT_FOUND,
    FOOD_ERR_DUPLICATE,
    FOOD_ERR_INVALID_PARAM,
    FOOD_ERR_FILE_OPEN,
    FOOD_ERR_FILE_READ,
    FOOD_ERR_FILE_WRITE,
    FOOD_ERR_FILE_MAGIC,
    FOOD_ERR_FILE_VERSION,
    FOOD_ERR_EMPTY,
    FOOD_ERR_INVALID_PRICE,
    FOOD_ERR_INVALID_QUANTITY,
    FOOD_ERR_NOT_ENOUGH_STOCK,
    FOOD_ERR_ALREADY_SETTLED,
    FOOD_ERR_IO
} FoodStatus;

typedef struct {
    int     id;
    char    name[MAX_NAME];
    double  price;
    int     category;
    int     sales_count;
    int     stock;
} MenuItem;

typedef struct {
    int     order_id;
    char    customer[MAX_NAME];
    int     menu_id;
    char    item_name[MAX_NAME];
    double  unit_price;
    int     quantity;
    double  total_price;
    double  discount;
    double  final_price;
    char    cashier[MAX_NAME];
    char    date[MAX_DATE];
    int     is_vip;
    int     vip_level;
    int     settled;
} Order;

typedef struct {
    char    name[MAX_NAME];
    int     vip_level;
    double  total_spent;
    int     order_count;
} VIPCustomer;

typedef struct {
    char    name[MAX_NAME];
    double  total_sales;
    int     order_count;
} Cashier;

typedef struct {
    MenuItem     *menu;
    int           menu_count;
    int           menu_cap;
    Order        *orders;
    int           order_count;
    int           order_cap;
    VIPCustomer  *vips;
    int           vip_count;
    int           vip_cap;
    Cashier      *cashiers;
    int           cashier_count;
    int           cashier_cap;
    int           next_menu_id;
    int           next_order_id;
    char          current_date[MAX_DATE];
} FoodSystem;

static FoodSystem g_sys;
static int g_use_color = 1;
static int g_test_mode = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

static void safe_strncpy(char *dst, const char *src, size_t size)
{
    if (size == 0) return;
    snprintf(dst, size, "%s", src);
}

static void* array_grow(void *arr, int *cap, int count, size_t elem_size)
{
    int new_cap;
    void *new_arr;
    if (count < *cap) return arr;
    new_cap = (*cap == 0) ? INIT_CAPACITY : (*cap * 2);
    new_arr = realloc(arr, (size_t)new_cap * elem_size);
    if (!new_arr) return NULL;
    *cap = new_cap;
    return new_arr;
}

static void console_init(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        if (hOut != INVALID_HANDLE_VALUE && GetConsoleMode(hOut, &mode)) {
            mode |= 0x0004;
            SetConsoleMode(hOut, mode);
        }
    }
#endif
}

static void print_colored(const char *color, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if (g_use_color) {
        printf("%s", color);
        vprintf(fmt, args);
        printf("%s", COLOR_RESET);
    } else {
        vprintf(fmt, args);
    }
    va_end(args);
}

static void print_line(char ch, int len)
{
    int i;
    for (i = 0; i < len; i++) putchar(ch);
    putchar('\n');
}

static void print_title(const char *title)
{
    int len = 62;
    int title_len = (int)strlen(title);
    int pad = (len - title_len) / 2;
    int i;
    print_colored(COLOR_CYAN, "");
    print_line('=', len);
    for (i = 0; i < pad; i++) putchar(' ');
    print_colored(COLOR_BOLD COLOR_CYAN, "%s\n", title);
    print_line('=', len);
}

static void print_error(FoodStatus status)
{
    const char *msg = "Unknown Error";
    switch (status) {
        case FOOD_OK:              msg = "Success"; break;
        case FOOD_ERR_MEMORY:      msg = "Memory allocation failed"; break;
        case FOOD_ERR_NOT_FOUND:   msg = "Record not found"; break;
        case FOOD_ERR_DUPLICATE:   msg = "Record already exists"; break;
        case FOOD_ERR_INVALID_PARAM: msg = "Invalid parameter"; break;
        case FOOD_ERR_FILE_OPEN:   msg = "Cannot open file"; break;
        case FOOD_ERR_FILE_READ:   msg = "File read failed"; break;
        case FOOD_ERR_FILE_WRITE:  msg = "File write failed"; break;
        case FOOD_ERR_FILE_MAGIC:  msg = "Invalid file format"; break;
        case FOOD_ERR_FILE_VERSION: msg = "File version incompatible"; break;
        case FOOD_ERR_INVALID_PRICE: msg = "Price must be positive"; break;
        case FOOD_ERR_INVALID_QUANTITY: msg = "Quantity must be positive"; break;
        case FOOD_ERR_NOT_ENOUGH_STOCK: msg = "Insufficient stock"; break;
        case FOOD_ERR_ALREADY_SETTLED: msg = "Order already settled"; break;
        default: break;
    }
    print_colored(COLOR_RED, "  [ERROR] %s\n", msg);
}

static void pause_screen(void)
{
    int ch;
    if (g_test_mode) return;
    printf("\n  Press Enter to continue...");
    fflush(stdout);
    while ((ch = getchar()) != '\n' && ch != EOF) {}
}

static void clear_input_buffer(void)
{
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {}
}

static void food_get_current_date(char *buf, size_t size)
{
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buf, size, "%Y-%m-%d", tm_info);
}

static FoodStatus food_init(FoodSystem *sys)
{
    memset(sys, 0, sizeof(FoodSystem));
    sys->next_menu_id = 1;
    sys->next_order_id = 1001;
    food_get_current_date(sys->current_date, sizeof(sys->current_date));
    return FOOD_OK;
}

static void food_shutdown(FoodSystem *sys)
{
    free(sys->menu);
    free(sys->orders);
    free(sys->vips);
    free(sys->cashiers);
    memset(sys, 0, sizeof(FoodSystem));
}

static double vip_get_discount(int level)
{
    switch (level) {
        case 3: return 0.80;
        case 2: return 0.90;
        default: return 1.00;
    }
}

static const char* vip_level_name(int level)
{
    switch (level) {
        case 3: return "Gold";
        case 2: return "Silver";
        default: return "Regular";
    }
}

static void vip_upgrade(VIPCustomer *vip)
{
    if (vip->total_spent >= 500.0 && vip->vip_level < 3) {
        vip->vip_level = 3;
    } else if (vip->total_spent >= 200.0 && vip->vip_level < 2) {
        vip->vip_level = 2;
    }
}

static FoodStatus vip_find_or_create(FoodSystem *sys, const char *name, VIPCustomer **out)
{
    int i;
    VIPCustomer *new_vips;
    for (i = 0; i < sys->vip_count; i++) {
        if (strcmp(sys->vips[i].name, name) == 0) {
            if (out) *out = &sys->vips[i];
            return FOOD_OK;
        }
    }
    new_vips = (VIPCustomer*)array_grow(sys->vips, &sys->vip_cap, sys->vip_count, sizeof(VIPCustomer));
    if (!new_vips) return FOOD_ERR_MEMORY;
    sys->vips = new_vips;
    memset(&sys->vips[sys->vip_count], 0, sizeof(VIPCustomer));
    safe_strncpy(sys->vips[sys->vip_count].name, name, MAX_NAME);
    sys->vips[sys->vip_count].vip_level = 1;
    sys->vips[sys->vip_count].total_spent = 0;
    sys->vips[sys->vip_count].order_count = 0;
    if (out) *out = &sys->vips[sys->vip_count];
    sys->vip_count++;
    return FOOD_OK;
}

static FoodStatus cashier_find_or_create(FoodSystem *sys, const char *name, Cashier **out)
{
    int i;
    Cashier *new_cashiers;
    for (i = 0; i < sys->cashier_count; i++) {
        if (strcmp(sys->cashiers[i].name, name) == 0) {
            if (out) *out = &sys->cashiers[i];
            return FOOD_OK;
        }
    }
    new_cashiers = (Cashier*)array_grow(sys->cashiers, &sys->cashier_cap, sys->cashier_count, sizeof(Cashier));
    if (!new_cashiers) return FOOD_ERR_MEMORY;
    sys->cashiers = new_cashiers;
    memset(&sys->cashiers[sys->cashier_count], 0, sizeof(Cashier));
    safe_strncpy(sys->cashiers[sys->cashier_count].name, name, MAX_NAME);
    sys->cashiers[sys->cashier_count].total_sales = 0;
    sys->cashiers[sys->cashier_count].order_count = 0;
    if (out) *out = &sys->cashiers[sys->cashier_count];
    sys->cashier_count++;
    return FOOD_OK;
}

static FoodStatus menu_add(FoodSystem *sys, const char *name, double price, int category, int stock, int *out_id)
{
    MenuItem *new_menu;
    int i;
    if (!name || price <= 0 || category < 1 || category > 4 || stock < 0) {
        return FOOD_ERR_INVALID_PARAM;
    }
    for (i = 0; i < sys->menu_count; i++) {
        if (strcmp(sys->menu[i].name, name) == 0) {
            return FOOD_ERR_DUPLICATE;
        }
    }
    new_menu = (MenuItem*)array_grow(sys->menu, &sys->menu_cap, sys->menu_count, sizeof(MenuItem));
    if (!new_menu) return FOOD_ERR_MEMORY;
    sys->menu = new_menu;
    memset(&sys->menu[sys->menu_count], 0, sizeof(MenuItem));
    sys->menu[sys->menu_count].id = sys->next_menu_id++;
    safe_strncpy(sys->menu[sys->menu_count].name, name, MAX_NAME);
    sys->menu[sys->menu_count].price = price;
    sys->menu[sys->menu_count].category = category;
    sys->menu[sys->menu_count].sales_count = 0;
    sys->menu[sys->menu_count].stock = stock;
    if (out_id) *out_id = sys->menu[sys->menu_count].id;
    sys->menu_count++;
    return FOOD_OK;
}

static FoodStatus menu_find_by_id(FoodSystem *sys, int id, MenuItem **out)
{
    int i;
    for (i = 0; i < sys->menu_count; i++) {
        if (sys->menu[i].id == id) {
            if (out) *out = &sys->menu[i];
            return FOOD_OK;
        }
    }
    return FOOD_ERR_NOT_FOUND;
}

static FoodStatus menu_find_by_name(FoodSystem *sys, const char *name, MenuItem **out)
{
    int i;
    for (i = 0; i < sys->menu_count; i++) {
        if (strcmp(sys->menu[i].name, name) == 0) {
            if (out) *out = &sys->menu[i];
            return FOOD_OK;
        }
    }
    return FOOD_ERR_NOT_FOUND;
}

static FoodStatus menu_update(FoodSystem *sys, int id, const char *name, double price, int category, int stock)
{
    MenuItem *item = NULL;
    FoodStatus st = menu_find_by_id(sys, id, &item);
    if (st != FOOD_OK) return st;
    if (name && name[0] != '\0') {
        safe_strncpy(item->name, name, MAX_NAME);
    }
    if (price > 0) item->price = price;
    if (category >= 1 && category <= 4) item->category = category;
    if (stock >= 0) item->stock = stock;
    return FOOD_OK;
}

static FoodStatus menu_delete(FoodSystem *sys, int id)
{
    int i;
    for (i = 0; i < sys->menu_count; i++) {
        if (sys->menu[i].id == id) {
            if (i < sys->menu_count - 1) {
                memmove(&sys->menu[i], &sys->menu[i + 1],
                        (size_t)(sys->menu_count - i - 1) * sizeof(MenuItem));
            }
            sys->menu_count--;
            return FOOD_OK;
        }
    }
    return FOOD_ERR_NOT_FOUND;
}

static void menu_show_all(FoodSystem *sys)
{
    const char *cat_names[] = {"", "Main", "Snack", "Drink", "Other"};
    int i;
    print_line('-', 70);
    print_colored(COLOR_BOLD, "  %-4s %-18s %-10s %-8s %-8s %-8s\n",
           "ID", "Name", "Price", "Cat", "Sales", "Stock");
    print_line('-', 70);
    for (i = 0; i < sys->menu_count; i++) {
        MenuItem *m = &sys->menu[i];
        printf("  %-4d %-18s %-10.2f %-8s %-8d %-8d\n",
               m->id, m->name, m->price,
               (m->category >= 1 && m->category <= 4) ? cat_names[m->category] : "?",
               m->sales_count, m->stock);
    }
    print_line('-', 70);
    printf("  Total: %d items\n", sys->menu_count);
}

static FoodStatus order_create(FoodSystem *sys, const char *customer, int menu_id, int quantity,
                        const char *cashier_name, int is_vip, int *out_order_id)
{
    MenuItem *item = NULL;
    Order *new_orders;
    Order *ord;
    VIPCustomer *vip = NULL;
    double discount = 1.0;
    FoodStatus st;

    if (!customer || quantity <= 0 || !cashier_name) return FOOD_ERR_INVALID_PARAM;

    st = menu_find_by_id(sys, menu_id, &item);
    if (st != FOOD_OK) return st;

    if (item->stock < quantity) return FOOD_ERR_NOT_ENOUGH_STOCK;

    st = cashier_find_or_create(sys, cashier_name, NULL);
    if (st != FOOD_OK) return st;

    if (is_vip) {
        st = vip_find_or_create(sys, customer, &vip);
        if (st != FOOD_OK) return st;
        discount = vip_get_discount(vip->vip_level);
    }

    new_orders = (Order*)array_grow(sys->orders, &sys->order_cap, sys->order_count, sizeof(Order));
    if (!new_orders) return FOOD_ERR_MEMORY;
    sys->orders = new_orders;
    ord = &sys->orders[sys->order_count];
    memset(ord, 0, sizeof(Order));

    ord->order_id = sys->next_order_id++;
    safe_strncpy(ord->customer, customer, MAX_NAME);
    ord->menu_id = menu_id;
    safe_strncpy(ord->item_name, item->name, MAX_NAME);
    ord->unit_price = item->price;
    ord->quantity = quantity;
    ord->total_price = item->price * quantity;
    ord->discount = discount;
    ord->final_price = ord->total_price * discount;
    safe_strncpy(ord->cashier, cashier_name, MAX_NAME);
    safe_strncpy(ord->date, sys->current_date, MAX_DATE);
    ord->is_vip = is_vip;
    ord->vip_level = vip ? vip->vip_level : 0;
    ord->settled = 0;

    item->stock -= quantity;
    item->sales_count += quantity;

    if (out_order_id) *out_order_id = ord->order_id;
    sys->order_count++;
    return FOOD_OK;
}

static FoodStatus order_find(FoodSystem *sys, int order_id, Order **out)
{
    int i;
    for (i = 0; i < sys->order_count; i++) {
        if (sys->orders[i].order_id == order_id) {
            if (out) *out = &sys->orders[i];
            return FOOD_OK;
        }
    }
    return FOOD_ERR_NOT_FOUND;
}

static FoodStatus order_settle(FoodSystem *sys, int order_id)
{
    Order *ord = NULL;
    VIPCustomer *vip = NULL;
    Cashier *cashier = NULL;
    FoodStatus st;

    st = order_find(sys, order_id, &ord);
    if (st != FOOD_OK) return st;
    if (ord->settled) return FOOD_ERR_ALREADY_SETTLED;

    ord->settled = 1;

    st = cashier_find_or_create(sys, ord->cashier, &cashier);
    if (st == FOOD_OK) {
        cashier->total_sales += ord->final_price;
        cashier->order_count++;
    }

    if (ord->is_vip) {
        st = vip_find_or_create(sys, ord->customer, &vip);
        if (st == FOOD_OK) {
            vip->total_spent += ord->final_price;
            vip->order_count++;
            vip_upgrade(vip);
        }
    }

    return FOOD_OK;
}

static FoodStatus order_delete(FoodSystem *sys, int order_id)
{
    int i;
    for (i = 0; i < sys->order_count; i++) {
        if (sys->orders[i].order_id == order_id) {
            if (i < sys->order_count - 1) {
                memmove(&sys->orders[i], &sys->orders[i + 1],
                        (size_t)(sys->order_count - i - 1) * sizeof(Order));
            }
            sys->order_count--;
            return FOOD_OK;
        }
    }
    return FOOD_ERR_NOT_FOUND;
}

static void order_show_all(FoodSystem *sys)
{
    int i;
    print_line('-', 80);
    print_colored(COLOR_BOLD, "  %-6s %-12s %-16s %-6s %-8s %-8s %s\n",
           "Order", "Customer", "Item", "Qty", "Original", "Final", "Status");
    print_line('-', 80);
    for (i = 0; i < sys->order_count; i++) {
        Order *o = &sys->orders[i];
        printf("  %-6d %-12s %-16s %-6d %-8.2f %-8.2f ",
               o->order_id, o->customer, o->item_name,
               o->quantity, o->total_price, o->final_price);
        if (o->settled) {
            print_colored(COLOR_GREEN, "Paid\n");
        } else {
            print_colored(COLOR_YELLOW, "Unpaid\n");
        }
    }
    print_line('-', 80);
    printf("  Total: %d orders\n", sys->order_count);
}

static void order_show_unsettled(FoodSystem *sys)
{
    int i, count = 0;
    print_line('-', 80);
    print_colored(COLOR_BOLD COLOR_YELLOW, "  ===== Unpaid Orders =====\n");
    print_line('-', 80);
    for (i = 0; i < sys->order_count; i++) {
        Order *o = &sys->orders[i];
        if (!o->settled) {
            printf("  [%d] %s - %s x%d  Final: %.2f  VIP:%s\n",
                   o->order_id, o->customer, o->item_name,
                   o->quantity, o->final_price,
                   o->is_vip ? vip_level_name(o->vip_level) : "No");
            count++;
        }
    }
    if (count == 0) {
        print_colored(COLOR_GREEN, "  No unpaid orders\n");
    }
    print_line('-', 80);
}

static void stats_show(FoodSystem *sys)
{
    int i;
    double total_revenue = 0;
    int settled_count = 0;
    int total_items_sold = 0;
    MenuItem *best_seller = NULL;
    Cashier *top_cashier = NULL;
    VIPCustomer *top_vip = NULL;

    print_title("Statistics");

    for (i = 0; i < sys->order_count; i++) {
        if (sys->orders[i].settled) {
            total_revenue += sys->orders[i].final_price;
            settled_count++;
        }
    }

    for (i = 0; i < sys->menu_count; i++) {
        total_items_sold += sys->menu[i].sales_count;
        if (!best_seller || sys->menu[i].sales_count > best_seller->sales_count) {
            best_seller = &sys->menu[i];
        }
    }

    for (i = 0; i < sys->cashier_count; i++) {
        if (!top_cashier || sys->cashiers[i].total_sales > top_cashier->total_sales) {
            top_cashier = &sys->cashiers[i];
        }
    }

    for (i = 0; i < sys->vip_count; i++) {
        if (!top_vip || sys->vips[i].total_spent > top_vip->total_spent) {
            top_vip = &sys->vips[i];
        }
    }

    printf("\n");
    print_colored(COLOR_CYAN, "  +-------------------------------------+\n");
    printf("  ");
    print_colored(COLOR_CYAN, "|");
    print_colored(COLOR_BOLD, "  Menu items:    %-22d", sys->menu_count);
    print_colored(COLOR_CYAN, "|\n");
    printf("  ");
    print_colored(COLOR_CYAN, "|");
    print_colored(COLOR_BOLD, "  Total orders:  %-22d", sys->order_count);
    print_colored(COLOR_CYAN, "|\n");
    printf("  ");
    print_colored(COLOR_CYAN, "|");
    print_colored(COLOR_BOLD COLOR_GREEN, "  Paid orders:   %-22d", settled_count);
    print_colored(COLOR_CYAN, "|\n");
    printf("  ");
    print_colored(COLOR_CYAN, "|");
    print_colored(COLOR_BOLD COLOR_YELLOW, "  Revenue:       Y%-21.2f", total_revenue);
    print_colored(COLOR_CYAN, "|\n");
    printf("  ");
    print_colored(COLOR_CYAN, "|");
    print_colored(COLOR_BOLD, "  Items sold:    %-22d", total_items_sold);
    print_colored(COLOR_CYAN, "|\n");
    print_colored(COLOR_CYAN, "  +-------------------------------------+\n");

    printf("\n");
    if (best_seller && best_seller->sales_count > 0) {
        print_colored(COLOR_GREEN, "  * Best seller: %s (Sales: %d)\n", best_seller->name, best_seller->sales_count);
    }
    if (top_cashier && top_cashier->order_count > 0) {
        print_colored(COLOR_MAGENTA, "  * Top cashier: %s (Sales: Y%.2f, Orders: %d)\n",
               top_cashier->name, top_cashier->total_sales, top_cashier->order_count);
    }
    if (top_vip && top_vip->total_spent > 0) {
        print_colored(COLOR_BLUE, "  * Top VIP: %s (%s, Spent: Y%.2f)\n",
               top_vip->name, vip_level_name(top_vip->vip_level), top_vip->total_spent);
    }
    printf("\n");

    if (sys->cashier_count > 0) {
        print_colored(COLOR_BOLD, "  Cashier Performance:\n");
        print_line('-', 50);
        for (i = 0; i < sys->cashier_count; i++) {
            printf("  %d. %-12s Orders: %-4d  Sales: Y%.2f\n",
                   i + 1, sys->cashiers[i].name,
                   sys->cashiers[i].order_count, sys->cashiers[i].total_sales);
        }
        print_line('-', 50);
    }
}

static FoodStatus food_save(FoodSystem *sys, const char *filename)
{
    FILE *fp;
    int version = 2;

    fp = fopen(filename, "wb");
    if (!fp) return FOOD_ERR_FILE_OPEN;

    if (fwrite(MAGIC_HEADER, 1, MAGIC_LEN, fp) != MAGIC_LEN) {
        fclose(fp); return FOOD_ERR_FILE_WRITE;
    }
    if (fwrite(&version, sizeof(int), 1, fp) != 1) { fclose(fp); return FOOD_ERR_FILE_WRITE; }
    if (fwrite(&sys->next_menu_id, sizeof(int), 1, fp) != 1) { fclose(fp); return FOOD_ERR_FILE_WRITE; }
    if (fwrite(&sys->next_order_id, sizeof(int), 1, fp) != 1) { fclose(fp); return FOOD_ERR_FILE_WRITE; }

    if (fwrite(&sys->menu_count, sizeof(int), 1, fp) != 1) { fclose(fp); return FOOD_ERR_FILE_WRITE; }
    if (sys->menu_count > 0) {
        if (fwrite(sys->menu, sizeof(MenuItem), (size_t)sys->menu_count, fp) != (size_t)sys->menu_count) {
            fclose(fp); return FOOD_ERR_FILE_WRITE;
        }
    }
    if (fwrite(&sys->order_count, sizeof(int), 1, fp) != 1) { fclose(fp); return FOOD_ERR_FILE_WRITE; }
    if (sys->order_count > 0) {
        if (fwrite(sys->orders, sizeof(Order), (size_t)sys->order_count, fp) != (size_t)sys->order_count) {
            fclose(fp); return FOOD_ERR_FILE_WRITE;
        }
    }
    if (fwrite(&sys->vip_count, sizeof(int), 1, fp) != 1) { fclose(fp); return FOOD_ERR_FILE_WRITE; }
    if (sys->vip_count > 0) {
        if (fwrite(sys->vips, sizeof(VIPCustomer), (size_t)sys->vip_count, fp) != (size_t)sys->vip_count) {
            fclose(fp); return FOOD_ERR_FILE_WRITE;
        }
    }
    if (fwrite(&sys->cashier_count, sizeof(int), 1, fp) != 1) { fclose(fp); return FOOD_ERR_FILE_WRITE; }
    if (sys->cashier_count > 0) {
        if (fwrite(sys->cashiers, sizeof(Cashier), (size_t)sys->cashier_count, fp) != (size_t)sys->cashier_count) {
            fclose(fp); return FOOD_ERR_FILE_WRITE;
        }
    }
    fclose(fp);
    return FOOD_OK;
}

static FoodStatus food_load(FoodSystem *sys, const char *filename)
{
    FILE *fp;
    char magic[MAGIC_LEN + 1] = {0};
    int version = 0;
    int count;

    fp = fopen(filename, "rb");
    if (!fp) return FOOD_ERR_FILE_OPEN;

    if (fread(magic, 1, MAGIC_LEN, fp) != MAGIC_LEN) { fclose(fp); return FOOD_ERR_FILE_MAGIC; }
    if (memcmp(magic, MAGIC_HEADER, MAGIC_LEN) != 0) { fclose(fp); return FOOD_ERR_FILE_MAGIC; }
    if (fread(&version, sizeof(int), 1, fp) != 1) { fclose(fp); return FOOD_ERR_FILE_READ; }
    if (version != 2) { fclose(fp); return FOOD_ERR_FILE_VERSION; }
    if (fread(&sys->next_menu_id, sizeof(int), 1, fp) != 1) { fclose(fp); return FOOD_ERR_FILE_READ; }
    if (fread(&sys->next_order_id, sizeof(int), 1, fp) != 1) { fclose(fp); return FOOD_ERR_FILE_READ; }

    if (fread(&count, sizeof(int), 1, fp) != 1) { fclose(fp); return FOOD_ERR_FILE_READ; }
    if (count > 0) {
        sys->menu = (MenuItem*)calloc((size_t)count, sizeof(MenuItem));
        if (!sys->menu) { fclose(fp); return FOOD_ERR_MEMORY; }
        if (fread(sys->menu, sizeof(MenuItem), (size_t)count, fp) != (size_t)count) {
            fclose(fp); return FOOD_ERR_FILE_READ;
        }
        sys->menu_count = count;
        sys->menu_cap = count;
    }
    if (fread(&count, sizeof(int), 1, fp) != 1) { fclose(fp); return FOOD_ERR_FILE_READ; }
    if (count > 0) {
        sys->orders = (Order*)calloc((size_t)count, sizeof(Order));
        if (!sys->orders) { fclose(fp); return FOOD_ERR_MEMORY; }
        if (fread(sys->orders, sizeof(Order), (size_t)count, fp) != (size_t)count) {
            fclose(fp); return FOOD_ERR_FILE_READ;
        }
        sys->order_count = count;
        sys->order_cap = count;
    }
    if (fread(&count, sizeof(int), 1, fp) != 1) { fclose(fp); return FOOD_ERR_FILE_READ; }
    if (count > 0) {
        sys->vips = (VIPCustomer*)calloc((size_t)count, sizeof(VIPCustomer));
        if (!sys->vips) { fclose(fp); return FOOD_ERR_MEMORY; }
        if (fread(sys->vips, sizeof(VIPCustomer), (size_t)count, fp) != (size_t)count) {
            fclose(fp); return FOOD_ERR_FILE_READ;
        }
        sys->vip_count = count;
        sys->vip_cap = count;
    }
    if (fread(&count, sizeof(int), 1, fp) != 1) { fclose(fp); return FOOD_ERR_FILE_READ; }
    if (count > 0) {
        sys->cashiers = (Cashier*)calloc((size_t)count, sizeof(Cashier));
        if (!sys->cashiers) { fclose(fp); return FOOD_ERR_MEMORY; }
        if (fread(sys->cashiers, sizeof(Cashier), (size_t)count, fp) != (size_t)count) {
            fclose(fp); return FOOD_ERR_FILE_READ;
        }
        sys->cashier_count = count;
        sys->cashier_cap = count;
    }
    fclose(fp);
    return FOOD_OK;
}

static void test_assert(int condition, const char *test_name)
{
    if (condition) {
        g_tests_passed++;
        print_colored(COLOR_GREEN, "  [PASS] %s\n", test_name);
    } else {
        g_tests_failed++;
        print_colored(COLOR_RED, "  [FAIL] %s\n", test_name);
    }
}

static void test_summary(void)
{
    int total = g_tests_passed + g_tests_failed;
    print_line('=', 60);
    print_colored(COLOR_BOLD, "  Test Results\n");
    print_line('-', 60);
    print_colored(COLOR_GREEN, "  Passed: %d\n", g_tests_passed);
    print_colored(COLOR_RED, "  Failed: %d\n", g_tests_failed);
    print_colored(COLOR_CYAN, "  Total:  %d\n", total);
    print_line('=', 60);
    if (g_tests_failed == 0) {
        print_colored(COLOR_BOLD COLOR_GREEN, "\n  All tests passed!\n\n");
    } else {
        print_colored(COLOR_BOLD COLOR_RED, "\n  Some tests failed!\n\n");
    }
}

static void run_self_tests(void)
{
    FoodSystem ts;
    FoodStatus st;
    int id1 = 0, id2 = 0, id3 = 0, oid1 = 0, oid2 = 0;
    MenuItem *m = NULL;
    Order *o = NULL;
    VIPCustomer *vip = NULL;
    const char *test_file = "test_foodv2.dat";
    FILE *fp;

    g_test_mode = 1;
    g_tests_passed = 0;
    g_tests_failed = 0;

    print_colored(COLOR_BOLD COLOR_CYAN, "\n");
    print_line('=', 60);
    print_colored(COLOR_BOLD, "        Fast Food Ordering System V2.0 - Self Tests\n");
    print_line('=', 60);
    printf("\n");

    food_init(&ts);
    test_assert(ts.menu_count == 0 && ts.order_count == 0, "1. System initialization");

    st = menu_add(&ts, "Hamburger", 18.0, CATEGORY_MAIN, 50, &id1);
    test_assert(st == FOOD_OK && id1 == 1, "2. Add main dish");

    st = menu_add(&ts, "French Fries", 8.0, CATEGORY_SNACK, 100, &id2);
    test_assert(st == FOOD_OK && id2 == 2, "3. Add snack");

    st = menu_add(&ts, "Cola", 6.0, CATEGORY_DRINK, 200, &id3);
    test_assert(st == FOOD_OK && id3 == 3 && ts.menu_count == 3, "4. Add drink");

    st = menu_add(&ts, "Hamburger", 20.0, CATEGORY_MAIN, 30, NULL);
    test_assert(st == FOOD_ERR_DUPLICATE, "5. Reject duplicate name");

    st = menu_add(&ts, "Test", -5.0, CATEGORY_MAIN, 10, NULL);
    test_assert(st == FOOD_ERR_INVALID_PARAM, "6. Reject invalid price");

    st = menu_find_by_id(&ts, id1, &m);
    test_assert(st == FOOD_OK && m != NULL && strcmp(m->name, "Hamburger") == 0, "7. Find by ID");

    st = menu_find_by_name(&ts, "French Fries", &m);
    test_assert(st == FOOD_OK && m != NULL && m->price == 8.0, "8. Find by name");

    st = menu_update(&ts, id1, NULL, 20.0, 0, 60);
    test_assert(st == FOOD_OK, "9a. Update price and stock");
    st = menu_find_by_id(&ts, id1, &m);
    test_assert(st == FOOD_OK && m != NULL && m->price == 20.0 && m->stock == 60, "9b. Verify updated values");

    st = order_create(&ts, "Zhang San", id1, 2, "Cashier A", 0, &oid1);
    test_assert(st == FOOD_OK && oid1 == 1001 && ts.order_count == 1, "10. Create regular order");

    st = order_create(&ts, "Li Si", id2, 3, "Cashier A", 1, &oid2);
    test_assert(st == FOOD_OK && oid2 == 1002 && ts.vip_count == 1, "11. Create VIP order");

    st = menu_find_by_id(&ts, id1, &m);
    test_assert(st == FOOD_OK && m != NULL && m->stock == 58, "12. Stock deduction after order");

    st = order_find(&ts, oid2, &o);
    test_assert(st == FOOD_OK && o != NULL && o->discount == 1.0 && o->final_price == o->total_price, "13. New VIP no discount");

    st = order_settle(&ts, oid1);
    test_assert(st == FOOD_OK, "14a. Order settle call");
    st = order_find(&ts, oid1, &o);
    test_assert(st == FOOD_OK && o != NULL && o->settled == 1, "14b. Verify order settled");

    test_assert(ts.cashier_count == 1 && ts.cashiers[0].order_count == 1, "15. Cashier performance tracking");

    st = order_settle(&ts, oid1);
    test_assert(st == FOOD_ERR_ALREADY_SETTLED, "16. Reject double settlement");

    st = order_create(&ts, "Wang Wu", id3, 9999, "Cashier B", 0, NULL);
    test_assert(st == FOOD_ERR_NOT_ENOUGH_STOCK, "17. Reject insufficient stock");

    st = order_create(&ts, "Zhao Liu", 9999, 1, "Cashier B", 0, NULL);
    test_assert(st == FOOD_ERR_NOT_FOUND, "18. Reject non-existent menu item");

    st = menu_add(&ts, "ToDelete", 5.0, CATEGORY_OTHER, 10, NULL);
    st = menu_delete(&ts, 4);
    test_assert(st == FOOD_OK && ts.menu_count == 3, "19. Delete menu item");

    st = menu_delete(&ts, 999);
    test_assert(st == FOOD_ERR_NOT_FOUND, "20. Delete non-existent item error");

    st = food_save(&ts, test_file);
    test_assert(st == FOOD_OK, "21. Save to file (FOODV2 magic)");

    {
        FoodSystem ts2;
        food_init(&ts2);
        st = food_load(&ts2, test_file);
        test_assert(st == FOOD_OK && ts2.menu_count == 3 && ts2.order_count == 2, "22. Load from file");
        food_shutdown(&ts2);
    }

    fp = fopen("bad_magic.dat", "wb");
    fwrite("BADXXX", 1, 6, fp);
    fclose(fp);
    {
        FoodSystem ts3;
        food_init(&ts3);
        st = food_load(&ts3, "bad_magic.dat");
        test_assert(st == FOOD_ERR_FILE_MAGIC, "23. Reject invalid magic number");
        food_shutdown(&ts3);
    }
    remove("bad_magic.dat");

    st = vip_find_or_create(&ts, "Li Si", &vip);
    vip->total_spent = 600.0;
    vip_upgrade(vip);
    test_assert(st == FOOD_OK && vip->vip_level == 3, "24. VIP upgrade to Gold after 500+ spending");

    test_assert(vip_get_discount(3) == 0.80 && vip_get_discount(2) == 0.90 && vip_get_discount(1) == 1.00,
               "25. VIP discount rates correct");

    remove(test_file);
    food_shutdown(&ts);
    test_summary();
    g_test_mode = 0;
}

static void checkout_ui(FoodSystem *sys);
static void vip_management_ui(FoodSystem *sys);
static void order_management_ui(FoodSystem *sys);
static void menu_management_ui(FoodSystem *sys);

static void vip_management_ui(FoodSystem *sys)
{
    int i;
    print_title("VIP Management");
    if (sys->vip_count == 0) {
        print_colored(COLOR_YELLOW, "  No VIP customers\n");
    } else {
        print_line('-', 60);
        print_colored(COLOR_BOLD, "  %-12s %-10s %-12s %s\n", "Name", "Level", "Total Spent", "Orders");
        print_line('-', 60);
        for (i = 0; i < sys->vip_count; i++) {
            VIPCustomer *v = &sys->vips[i];
            printf("  %-12s %-10s Y%-11.2f %d\n",
                   v->name, vip_level_name(v->vip_level),
                   v->total_spent, v->order_count);
        }
        print_line('-', 60);
    }
    pause_screen();
}

static void checkout_ui(FoodSystem *sys)
{
    char customer[MAX_NAME], cashier[MAX_NAME];
    int menu_id, qty, is_vip, oid, confirm;
    FoodStatus st;
    MenuItem *mi = NULL;
    Order *ord = NULL;
    VIPCustomer *v = NULL;

    print_title("Order and Checkout");
    menu_show_all(sys);

    printf("  Customer name: "); scanf("%63s", customer);
    printf("  Is VIP? (1=Yes 0=No): "); scanf("%d", &is_vip);
    printf("  Cashier: "); scanf("%63s", cashier);
    printf("  Menu ID: "); scanf("%d", &menu_id);
    printf("  Quantity: "); scanf("%d", &qty);

    st = order_create(sys, customer, menu_id, qty, cashier, is_vip, &oid);
    if (st != FOOD_OK) {
        print_error(st);
        pause_screen();
        return;
    }

    {
        FoodStatus st_a = order_find(sys, oid, &ord);
        FoodStatus st_b = menu_find_by_id(sys, menu_id, &mi);
        if (st_a != FOOD_OK || st_b != FOOD_OK || ord == NULL || mi == NULL) {
            print_error(FOOD_ERR_IO);
            pause_screen();
            return;
        }
    }

    print_colored(COLOR_GREEN, "\n  ===== Receipt =====\n");
    print_line('-', 40);
    printf("  Order: %d\n", oid);
    printf("  Customer: %s", customer);
    if (is_vip) {
        printf(" (%s VIP)", vip_level_name(ord->vip_level));
    }
    printf("\n");
    printf("  Item: %s x%d\n", mi->name, qty);
    printf("  Price: Y%.2f\n", mi->price);
    printf("  Total: Y%.2f\n", ord->total_price);
    if (ord->discount < 1.0) {
        print_colored(COLOR_YELLOW, "  Discount: %.0f%%\n", (1.0 - ord->discount) * 100);
    }
    print_colored(COLOR_BOLD COLOR_GREEN, "  Due: Y%.2f\n", ord->final_price);
    print_line('-', 40);

    printf("\n  Confirm checkout? (1=Yes 0=Hold): ");
    scanf("%d", &confirm);
    if (confirm == 1) {
        st = order_settle(sys, oid);
        if (st == FOOD_OK) {
            print_colored(COLOR_GREEN, "  Checkout successful!\n");
            if (is_vip) {
                vip_find_or_create(sys, customer, &v);
                if (v != NULL && v->vip_level > ord->vip_level) {
                    print_colored(COLOR_BOLD COLOR_YELLOW,
                           "  Congratulations! Upgraded to %s VIP!\n", vip_level_name(v->vip_level));
                }
            }
            food_save(sys, DATA_FILE);
        } else {
            print_error(st);
        }
    } else {
        print_colored(COLOR_YELLOW, "  Order placed on hold\n");
    }
    pause_screen();
}

static void menu_management_ui(FoodSystem *sys)
{
    int choice;
    while (1) {
        char name[MAX_NAME];
        double price;
        int cat, stock, id, qc;
        FoodStatus st;
        MenuItem *mi = NULL;

        print_title("Menu Management");
        printf("  "); print_colored(COLOR_GREEN, "[1]"); printf(" View all items\n");
        printf("  "); print_colored(COLOR_GREEN, "[2]"); printf(" Add new item\n");
        printf("  "); print_colored(COLOR_GREEN, "[3]"); printf(" Update item\n");
        printf("  "); print_colored(COLOR_GREEN, "[4]"); printf(" Delete item\n");
        printf("  "); print_colored(COLOR_GREEN, "[5]"); printf(" Search item\n");
        printf("  "); print_colored(COLOR_RED, "[0]"); printf(" Back to main menu\n");
        print_line('-', 60);
        printf("  Select: ");

        if (scanf("%d", &choice) != 1) { clear_input_buffer(); continue; }

        switch (choice) {
            case 1:
                menu_show_all(sys);
                pause_screen();
                break;
            case 2:
                print_title("Add New Item");
                printf("  Name: "); scanf("%63s", name);
                printf("  Price: "); scanf("%lf", &price);
                printf("  Category (1=Main 2=Snack 3=Drink 4=Other): "); scanf("%d", &cat);
                printf("  Stock: "); scanf("%d", &stock);
                st = menu_add(sys, name, price, cat, stock, &id);
                if (st == FOOD_OK) {
                    print_colored(COLOR_GREEN, "  Item added! ID: %d\n", id);
                } else {
                    print_error(st);
                }
                pause_screen();
                break;
            case 3:
                print_title("Update Item");
                menu_show_all(sys);
                printf("  Item ID to update: "); scanf("%d", &id);
                {
                    double new_price = 0;
                    int new_cat = 0, new_stock = -1;
                    printf("  New price (0=skip): "); scanf("%lf", &new_price);
                    printf("  New category (0=skip): "); scanf("%d", &new_cat);
                    printf("  New stock (-1=skip): "); scanf("%d", &new_stock);
                    st = menu_update(sys, id, NULL, new_price, new_cat, new_stock);
                    if (st == FOOD_OK) {
                        print_colored(COLOR_GREEN, "  Item updated!\n");
                    } else {
                        print_error(st);
                    }
                }
                pause_screen();
                break;
            case 4:
                print_title("Delete Item");
                menu_show_all(sys);
                printf("  Item ID to delete: "); scanf("%d", &id);
                st = menu_delete(sys, id);
                if (st == FOOD_OK) {
                    print_colored(COLOR_GREEN, "  Item deleted!\n");
                } else {
                    print_error(st);
                }
                pause_screen();
                break;
            case 5:
                print_title("Search Item");
                printf("  1-By ID  2-By name: ");
                scanf("%d", &qc);
                if (qc == 1) {
                    printf("  Enter ID: "); scanf("%d", &id);
                    st = menu_find_by_id(sys, id, &mi);
                    if (st == FOOD_OK && mi != NULL) {
                        print_line('-', 50);
                        printf("  ID: %d\n  Name: %s\n  Price: Y%.2f\n  Category: %d\n  Sales: %d\n  Stock: %d\n",
                               mi->id, mi->name, mi->price, mi->category, mi->sales_count, mi->stock);
                        print_line('-', 50);
                    } else {
                        print_error(st);
                    }
                } else {
                    printf("  Enter name: "); scanf("%63s", name);
                    st = menu_find_by_name(sys, name, &mi);
                    if (st == FOOD_OK && mi != NULL) {
                        print_line('-', 50);
                        printf("  ID: %d\n  Name: %s\n  Price: Y%.2f\n  Category: %d\n  Sales: %d\n  Stock: %d\n",
                               mi->id, mi->name, mi->price, mi->category, mi->sales_count, mi->stock);
                        print_line('-', 50);
                    } else {
                        print_error(st);
                    }
                }
                pause_screen();
                break;
            case 0:
                return;
            default:
                print_colored(COLOR_RED, "  Invalid option!\n");
                pause_screen();
        }
    }
}

static void order_management_ui(FoodSystem *sys)
{
    int choice;
    while (1) {
        int oid;
        FoodStatus st;
        print_title("Order Management");
        printf("  "); print_colored(COLOR_GREEN, "[1]"); printf(" View all orders\n");
        printf("  "); print_colored(COLOR_GREEN, "[2]"); printf(" Create new order\n");
        printf("  "); print_colored(COLOR_GREEN, "[3]"); printf(" Delete order\n");
        printf("  "); print_colored(COLOR_RED, "[0]"); printf(" Back to main menu\n");
        print_line('-', 60);
        printf("  Select: ");
        if (scanf("%d", &choice) != 1) { clear_input_buffer(); continue; }
        switch (choice) {
            case 1:
                order_show_all(sys);
                pause_screen();
                break;
            case 2:
                checkout_ui(sys);
                break;
            case 3:
                print_title("Delete Order");
                order_show_all(sys);
                printf("  Order ID to delete: "); scanf("%d", &oid);
                st = order_delete(sys, oid);
                if (st == FOOD_OK) {
                    print_colored(COLOR_GREEN, "  Order deleted!\n");
                } else {
                    print_error(st);
                }
                pause_screen();
                break;
            case 0:
                return;
            default:
                print_colored(COLOR_RED, "  Invalid option!\n");
                pause_screen();
        }
    }
}

static void interactive_menu(FoodSystem *sys)
{
    int choice;
    FoodStatus st;
    int oid;

    print_colored(COLOR_BOLD COLOR_CYAN, "\n  Fast Food Ordering System V2.0\n\n");

    while (1) {
        print_title("Main Menu");
        printf("  "); print_colored(COLOR_GREEN, "[1]"); printf(" Menu Management\n");
        printf("  "); print_colored(COLOR_GREEN, "[2]"); printf(" Order Management\n");
        printf("  "); print_colored(COLOR_GREEN, "[3]"); printf(" Order / Checkout\n");
        printf("  "); print_colored(COLOR_GREEN, "[4]"); printf(" View Unpaid Orders\n");
        printf("  "); print_colored(COLOR_GREEN, "[5]"); printf(" VIP Management\n");
        printf("  "); print_colored(COLOR_GREEN, "[6]"); printf(" Statistics\n");
        printf("  "); print_colored(COLOR_GREEN, "[7]"); printf(" Save Data\n");
        printf("  "); print_colored(COLOR_RED, "[0]"); printf(" Save and Exit\n");
        print_line('-', 60);
        printf("  Date: %s\n", sys->current_date);
        printf("  Menu: %d | Orders: %d\n", sys->menu_count, sys->order_count);
        print_line('-', 60);
        printf("  Select: ");
        if (scanf("%d", &choice) != 1) { clear_input_buffer(); continue; }

        switch (choice) {
            case 1: menu_management_ui(sys); break;
            case 2: order_management_ui(sys); break;
            case 3: checkout_ui(sys); break;
            case 4:
                order_show_unsettled(sys);
                printf("\n  Enter order ID to settle (0=back): ");
                scanf("%d", &oid);
                if (oid > 0) {
                    st = order_settle(sys, oid);
                    if (st == FOOD_OK) {
                        print_colored(COLOR_GREEN, "  Order %d settled!\n", oid);
                        food_save(sys, DATA_FILE);
                    } else {
                        print_error(st);
                    }
                }
                pause_screen();
                break;
            case 5: vip_management_ui(sys); break;
            case 6: stats_show(sys); pause_screen(); break;
            case 7:
                st = food_save(sys, DATA_FILE);
                if (st == FOOD_OK) {
                    print_colored(COLOR_GREEN, "  Data saved to %s\n", DATA_FILE);
                } else {
                    print_error(st);
                }
                pause_screen();
                break;
            case 0:
                st = food_save(sys, DATA_FILE);
                if (st == FOOD_OK) {
                    print_colored(COLOR_GREEN, "  Data saved. Thank you!\n");
                }
                return;
            default:
                print_colored(COLOR_RED, "  Invalid option!\n");
                pause_screen();
        }
    }
}

static void print_help(const char *prog)
{
    printf("Fast Food Ordering System V2.0 - Usage\n");
    printf("======================================\n\n");
    printf("Usage: %s [options]\n\n", prog);
    printf("Options:\n");
    printf("  -h, --help      Show this help\n");
    printf("  -t, --test      Run built-in self tests\n");
    printf("  (no args)       Start interactive menu\n\n");
    printf("Features:\n");
    printf("  UTF-8 support and ANSI color terminal\n");
    printf("  Menu CRUD management\n");
    printf("  Order/Checkout/VIP discount\n");
    printf("  Dynamic arrays and FOODV2 file persistence\n");
    printf("  25 built-in tests\n");
    printf("  Statistics, Cashier performance, VIP management\n\n");
    printf("Data file: %s\n", DATA_FILE);
}

int main(int argc, char *argv[])
{
    FoodStatus st;
    int i;
    int run_tests = 0;
    int show_help = 0;

    console_init();
    food_init(&g_sys);

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            show_help = 1;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--test") == 0) {
            run_tests = 1;
        }
    }

    if (show_help) {
        print_help(argv[0]);
        food_shutdown(&g_sys);
        return 0;
    }

    if (run_tests) {
        g_use_color = 1;
        run_self_tests();
        food_shutdown(&g_sys);
        return g_tests_failed == 0 ? 0 : 1;
    }

    st = food_load(&g_sys, DATA_FILE);
    if (st == FOOD_OK) {
        print_colored(COLOR_GREEN, "  Loaded data from %s\n", DATA_FILE);
    } else if (st != FOOD_ERR_FILE_OPEN) {
        print_error(st);
    } else {
        print_colored(COLOR_YELLOW, "  No data file found, initializing default menu\n");
        menu_add(&g_sys, "Hamburger", 18.0, CATEGORY_MAIN, 50, NULL);
        menu_add(&g_sys, "French Fries (L)", 10.0, CATEGORY_SNACK, 80, NULL);
        menu_add(&g_sys, "French Fries (M)", 8.0, CATEGORY_SNACK, 100, NULL);
        menu_add(&g_sys, "Cola (L)", 8.0, CATEGORY_DRINK, 120, NULL);
        menu_add(&g_sys, "Cola (M)", 6.0, CATEGORY_DRINK, 150, NULL);
        menu_add(&g_sys, "Fried Chicken", 12.0, CATEGORY_SNACK, 60, NULL);
        menu_add(&g_sys, "Chicken Burger", 15.0, CATEGORY_MAIN, 40, NULL);
        menu_add(&g_sys, "Ice Cream", 6.0, CATEGORY_OTHER, 50, NULL);
        menu_add(&g_sys, "Orange Juice", 9.0, CATEGORY_DRINK, 70, NULL);
        menu_add(&g_sys, "Fish Burger", 7.0, CATEGORY_SNACK, 45, NULL);
        print_colored(COLOR_GREEN, "  Initialized default menu (10 items)\n");
    }

    interactive_menu(&g_sys);
    food_shutdown(&g_sys);
    return 0;
}
