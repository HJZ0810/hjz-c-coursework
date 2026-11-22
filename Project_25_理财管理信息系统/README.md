<div align="center">

#  家庭理财管理信息系统

###  V2.0 升级版  |  2025-2026学年第二学期C语言算法实习

![Language](https://img.shields.io/badge/Language-C99-blue?style=for-the-badge&logo=c&logoColor=white)
![Version](https://img.shields.io/badge/Version-V2.0-brightgreen?style=for-the-badge)
![Tests](https://img.shields.io/badge/Tests-26%20PASS-ff69b4?style=for-the-badge)
![Warnings](https://img.shields.io/badge/Warnings-0-orange?style=for-the-badge)

---

** 教师：刘坤朋**  ** 学号：424123888888**
** 班级：计算机类A班**
** 姓名：HuiJunzhuo**

---

*从链表到动态数组，从纯文本到彩色终端，从零自测到26项全自动化测试*

</div>

---

## V2.0 升级亮点（蓝色高亮特性）

| 特性 | 说明 |
|:---|:---|
| **UTF-8 BOM 编码** | 文件头带BOM，`SetConsoleOutputCP(65001)` 设置控制台UTF-8，彻底解决中文乱码 |
| **ANSI 彩色终端** | VT100虚拟终端序列，收入绿色、支出红色、标题青色、提示黄色 |
| **CLI 参数** | `-h/--help` 显示帮助、`-t/--test` 运行自测、无参数进入交互菜单 |
| **动态数组** | 替代V1.0的链表，AccountList + TransactionList，自动倍增扩容 |
| **统一错误码** | `enum FinStatus`：FIN_OK、FIN_ERR_MEMORY、FIN_ERR_FILE_*、FIN_ERR_NOT_FOUND 等10种 |
| **Magic Header** | 数据文件头 `"FIN V2"` 校验，防止读取非法/旧版文件 |
| **内置自测** | 26项自动化测试（≥15项要求），覆盖CRUD/统计/余额/文件I/O/错误路径 |
| **编译零警告** | `gcc -std=c99 -Wall -Wextra -O2` 严格编译零警告零错误 |

---

## 编译与运行

### 编译器
```
D:/bin/gcc.exe (MinGW GCC)
```

### Makefile
```makefile
CC = D:/bin/gcc.exe
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = finance.exe
```

### 命令
```bash
make               # 编译 finance.exe
./finance.exe -h   # 显示帮助信息
./finance.exe -t   # 运行内置自测 (26项)
./finance.exe      # 启动交互式菜单
make clean         # 清理生成文件
```

---

## 数据结构

### V2.0 动态数组模型

```c
typedef struct {
    int    id;
    char   name[64];
    double balance;
} Account;

typedef struct {
    int    id;
    int    account_id;
    int    type;          /* 1=收入, 0=支出 */
    double amount;
    char   category[64];
    char   date[64];
    char   desc[128];
} Transaction;

typedef struct {
    Account     *data;
    size_t       count;
    size_t       capacity;   /* 自动倍增: 4→8→16→32... */
} AccountList;
```

### 文件格式（finance.dat）

```
+------------------------+
| Magic "FIN V2" (6字节) |
+------------------------+
| next_acc_id (int)      |
| next_tx_id  (int)      |
| acc_cnt     (size_t)   |
| tx_cnt      (size_t)   |
+------------------------+
| Account[acc_cnt]       |
| Transaction[tx_cnt]    |
+------------------------+
```

### 错误码枚举

```c
typedef enum {
    FIN_OK = 0,
    FIN_ERR_MEMORY,
    FIN_ERR_FILE_OPEN,
    FIN_ERR_FILE_READ,
    FIN_ERR_FILE_WRITE,
    FIN_ERR_MAGIC,
    FIN_ERR_NOT_FOUND,
    FIN_ERR_INVALID_PARAM,
    FIN_ERR_DUPLICATE,
    FIN_ERR_IO
} FinStatus;
```

---

## 自测报告（26项全通过）

```
═══ 运行内置自测 ═══

  [PASS] T1  初始状态：账户数为0
  [PASS] T2  初始状态：交易数为0
  [PASS] T3  初始状态：下一个账户ID=1
  [PASS] T4  添加账户1成功
  [PASS] T5  添加账户2成功
  [PASS] T6  账户列表count=2
  [PASS] T7  查询账户1余额正确
  [PASS] T8  添加收入交易成功
  [PASS] T9  收入后账户余额=1500
  [PASS] T10 添加支出交易成功
  [PASS] T11 支出后账户余额=1300
  [PASS] T12 总收入统计正确
  [PASS] T13 总支出统计正确
  [PASS] T14 净结余=300
  [PASS] T15 删除交易成功并回滚余额
  [PASS] T16 收入笔数=1,支出笔数=0
  [PASS] T17 更新账户名成功
  [PASS] T18 账户名已更新
  [PASS] T19 保存到文件成功
  [PASS] T20 从文件加载成功
  [PASS] T21 加载后账户数一致
  [PASS] T22 加载后交易数一致
  [PASS] T23 Magic header验证
  [PASS] T24 加载后余额一致
  [PASS] T25 查询不存在账户返回NULL
  [PASS] T26 有交易的账户不可删除

  测试结果：26 通过, 0 失败
```

---

## 交互菜单

```
──────────────────────────────────────────────
  家庭理财管理系统 V2.0
──────────────────────────────────────────────
  1. 查看账户列表
  2. 添加账户
  3. 删除账户
  4. 查看交易记录
  5. 添加交易记录
  6. 删除交易记录
  7. 收支统计
  8. 保存数据
  0. 退出
──────────────────────────────────────────────
```

---

## 文件清单

| 文件 | 说明 |
|:---|:---|
| `main.c` | V2.0 主程序源码（UTF-8 BOM） |
| `Makefile` | GCC 构建脚本 |
| `finance.exe` | 编译生成的可执行文件 |
| `finance.dat` | 数据文件（Magic: FIN V2） |
| `gen_doc.py` | Word文档生成脚本（蓝色高亮V2特性） |
| `Project_25_理财管理信息系统.docx` | V2.0 文档（已生成） |
| `_V1_backup.docx` | V1.0 原文档备份 |
| `README.md` | 本文件 |

---

> ** 教师：刘坤朋**  ** 学号：424123888888**  ** 班级：计算机类A班**
> ** 姓名：HuiJunzhuo**  ** V2.0 升级版  |  2025-2026学年第二学期**
