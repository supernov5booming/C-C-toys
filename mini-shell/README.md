# shell.c 代码解析

这是一个功能完善的 Shell 程序实现，支持管道、输入/输出重定向、后台运行、信号处理、历史命令持久化和僵尸进程回收等功能。

***

## 头文件引用 (第1-9行)

```c
#include <stdio.h>      // 标准输入输出库，提供 printf、fgets、perror、fprintf 等函数
#include <stdlib.h>     // 标准库，提供 exit、getenv、free、strdup 等函数
#include <string.h>     // 字符串处理库，提供 strlen、strcmp、strtok、strcspn、strcpy、snprintf 等函数
#include <unistd.h>     // UNIX 标准库，提供 fork、execvp、chdir、dup2、getcwd 等系统调用
#include <sys/wait.h>   // 进程等待库，提供 wait、waitpid 等函数和 WNOHANG、WIFEXITED、WIFSIGNALED 等宏
#include <fcntl.h>      // 文件控制库，提供 open 函数和 O_RDONLY、O_WRONLY、O_CREAT、O_TRUNC、O_APPEND 等常量
#include <ctype.h>      // 字符类型库，提供 isspace 等函数
#include <signal.h>     // 信号处理库，提供 signal 函数和 SIGINT、SIGCHLD、SIGTSTP 等信号常量
#include <errno.h>      // 错误号库，提供 errno 变量和错误处理相关宏
```

***

## 宏定义 (第11-14行)

```c
#define MAX_ARGS 100        // 定义单个命令最大参数数量为 100
#define MAX_CMDS 10         // 定义管道中最大命令数量为 10
#define MAX_HISTORY 1000    // 定义历史命令最大存储数量为 1000
#define HISTORY_FILE ".shell_history"  // 定义历史命令保存文件名
```

***

## 数据结构定义 (第16-22行)

```c
typedef struct {
    char *args[MAX_ARGS];  // 命令参数数组，args[0] 是命令名，后面是参数，以 NULL 结尾
    int in_fd;             // 输入重定向文件描述符，默认为 0（标准输入）
    int out_fd;            // 输出重定向文件描述符，默认为 1（标准输出）
    int append;            // 追加重定向标志，1 表示追加模式，0 表示覆盖模式
    int background;        // 后台运行标志，1 表示后台运行（&），0 表示前台运行
} Command;
```

***

## 全局变量 (第24-25行)

```c
char *history[MAX_HISTORY];  // 历史命令存储数组，每个元素是一个命令字符串指针
int history_count = 0;       // 当前已存储的历史命令数量
```

***

## 历史命令功能

### load\_history 函数 - 加载历史命令 (第27-46行)

```c
void load_history() {
    char *home = getenv("HOME");  // 获取用户主目录环境变量
    char path[512];               // 存储历史文件完整路径
    if (home) {
        snprintf(path, sizeof(path), "%s/%s", home, HISTORY_FILE);  // 拼接完整路径
    } else {
        strcpy(path, HISTORY_FILE);  // 无 HOME 则使用当前目录
    }
    
    FILE *fp = fopen(path, "r");  // 以只读方式打开历史文件
    if (!fp) return;              // 文件不存在则直接返回
    
    char line[1024];              // 临时存储每行命令
    while (fgets(line, sizeof(line), fp) && history_count < MAX_HISTORY) {
        line[strcspn(line, "\n")] = '\0';  // 去除换行符
        history[history_count] = strdup(line);  // 复制字符串并存入数组
        history_count++;          // 计数器加一
    }
    fclose(fp);                   // 关闭文件
}
```

**功能说明**：程序启动时从 `~/.shell_history` 文件加载历史命令，实现跨会话的历史记录持久化。

***

### save\_history 函数 - 保存历史命令 (第48-64行)

```c
void save_history() {
    char *home = getenv("HOME");  // 获取用户主目录
    char path[512];               // 存储历史文件完整路径
    if (home) {
        snprintf(path, sizeof(path), "%s/%s", home, HISTORY_FILE);  // 拼接完整路径
    } else {
        strcpy(path, HISTORY_FILE);  // 无 HOME 则使用当前目录
    }
    
    FILE *fp = fopen(path, "w");  // 以写入方式打开文件（覆盖原有内容）
    if (!fp) return;              // 打开失败则返回
    
    for (int i = 0; i < history_count; i++) {
        fprintf(fp, "%s\n", history[i]);  // 逐行写入历史命令
    }
    fclose(fp);                   // 关闭文件
}
```

**功能说明**：程序退出时将所有历史命令保存到 `~/.shell_history` 文件。

***

### add\_history 函数 - 添加历史命令 (第66-72行)

```c
void add_history(const char *cmd) {
    if (strlen(cmd) == 0) return;  // 空命令不记录
    if (history_count < MAX_HISTORY) {
        history[history_count] = strdup(cmd);  // 复制命令字符串
        history_count++;          // 计数器加一
    }
}
```

**功能说明**：将用户输入的命令添加到历史记录数组。

***

### show\_history 函数 - 显示历史命令 (第74-79行)

```c
void show_history() {
    int start = (history_count > 10) ? history_count - 10 : 0;  // 只显示最近 10 条
    for (int i = start; i < history_count; i++) {
        printf("%d %s\n", i + 1, history[i]);  // 带序号输出
    }
}
```

**功能说明**：显示最近 10 条历史命令，每条命令前带有序号。

***

## 信号处理功能

### sigchld\_handler 函数 - SIGCHLD 信号处理 (第81-90行)

```c
void sigchld_handler(int sig) {
    (void)sig;                    // 忽略未使用的参数，避免编译警告
    int status;                   // 存储子进程退出状态
    pid_t pid;                    // 存储子进程 PID
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // WNOHANG: 非阻塞模式，没有子进程退出时立即返回 0
        // -1: 等待任意子进程
        // 循环回收所有已终止的子进程
        if (WIFEXITED(status)) {
            // 子进程正常退出（调用 exit 或从 main 返回）
        } else if (WIFSIGNALED(status)) {
            // 子进程被信号终止
        }
    }
}
```

**功能说明**：处理 SIGCHLD 信号，使用 `waitpid(WNOHANG)` 非阻塞地回收僵尸进程。这是防止僵尸进程积累的关键机制。

**僵尸进程原理**：

- 子进程终止后，父进程尚未调用 wait/waitpid 回收，子进程就变成僵尸进程
- 僵尸进程占用进程表项，大量僵尸进程会耗尽系统资源
- 通过 SIGCHLD 信号通知父进程子进程状态变化，及时回收

***

### sigint\_handler 函数 - SIGINT 信号处理 (第92-96行)

```c
void sigint_handler(int sig) {
    (void)sig;                    // 忽略未使用的参数
    printf("\nmyshell> ");        // 打印新的提示符
    fflush(stdout);               // 刷新输出缓冲区
}
```

**功能说明**：处理 SIGINT 信号（Ctrl+C），不退出 shell，而是打印新的提示符等待用户输入。

***

### setup\_signals 函数 - 信号注册 (第98-102行)

```c
void setup_signals() {
    signal(SIGINT, sigint_handler);   // 注册 SIGINT 处理函数（Ctrl+C）
    signal(SIGCHLD, sigchld_handler); // 注册 SIGCHLD 处理函数（子进程状态变化）
    signal(SIGTSTP, SIG_IGN);         // 忽略 SIGTSTP 信号（Ctrl+Z）
}
```

**功能说明**：程序初始化时注册所有信号处理函数。

**信号说明**：

| 信号      | 触发方式   | 默认行为 | 自定义行为      |
| ------- | ------ | ---- | ---------- |
| SIGINT  | Ctrl+C | 终止进程 | 打印新提示符，不退出 |
| SIGCHLD | 子进程终止  | 忽略   | 回收僵尸进程     |
| SIGTSTP | Ctrl+Z | 暂停进程 | 忽略         |

***

## 辅助函数

### trim 函数 - 去除字符串前后空格 (第104-110行)

```c
void trim(char *s) {
    char *p = s;                              // p 指针指向字符串开头
    while (*p && isspace(*p)) p++;            // 跳过前导空白字符
    memmove(s, p, strlen(p) + 1);             // 将非空白内容移到字符串开头
    p = s + strlen(s) - 1;                    // p 指向字符串最后一个字符
    while (p >= s && isspace(*p)) *p-- = '\0'; // 从末尾删除空白字符
}
```

**功能说明**：去除字符串首尾的空白字符（空格、制表符、换行等）。

***

### parse\_single\_cmd 函数 - 解析单个命令 (第112-137行)

```c
void parse_single_cmd(Command *cmd) {
    cmd->in_fd = 0;       // 初始化输入文件描述符为标准输入
    cmd->out_fd = 1;      // 初始化输出文件描述符为标准输出
    cmd->append = 0;      // 初始化追加标志为 0
    cmd->background = 0;  // 初始化后台运行标志为 0

    int i = 0, j = 0;     // i 是读取索引，j 是写入索引
    while (cmd->args[i]) {  // 遍历所有参数
        if (!strcmp(cmd->args[i], "<")) {  // 输入重定向 <
            cmd->in_fd = open(cmd->args[i+1], O_RDONLY);  // 以只读方式打开文件
            i += 2;  // 跳过 < 和文件名
        } else if (!strcmp(cmd->args[i], ">")) {  // 输出覆盖重定向 >
            cmd->out_fd = open(cmd->args[i+1], O_WRONLY|O_CREAT|O_TRUNC, 0644);
            // O_WRONLY: 只写模式
            // O_CREAT: 文件不存在则创建
            // O_TRUNC: 文件存在则截断（清空）
            // 0644: 文件权限（用户读写，组读，其他读）
            i += 2;  // 跳过 > 和文件名
        } else if (!strcmp(cmd->args[i], ">>")) {  // 输出追加重定向 >>
            cmd->out_fd = open(cmd->args[i+1], O_WRONLY|O_CREAT|O_APPEND, 0644);
            // O_APPEND: 追加模式，写入内容添加到文件末尾
            i += 2;  // 跳过 >> 和文件名
        } else if (!strcmp(cmd->args[i], "&")) {  // 后台运行 &
            cmd->background = 1;  // 设置后台运行标志
            i++;  // 跳过 &
        } else {
            cmd->args[j++] = cmd->args[i++];  // 保留普通参数，移除重定向符号
        }
    }
    cmd->args[j] = NULL;  // 参数数组以 NULL 结尾
}
```

**功能说明**：解析单个命令，处理输入/输出重定向和后台运行标志，并从参数数组中移除这些特殊符号。

***

### exec\_cmd 函数 - 执行单个命令 (第139-151行)

```c
void exec_cmd(Command *cmd) {
    if (cmd->in_fd != 0) {          // 如果有输入重定向
        dup2(cmd->in_fd, 0);        // 将文件描述符复制到标准输入
        close(cmd->in_fd);          // 关闭原文件描述符
    }
    if (cmd->out_fd != 1) {         // 如果有输出重定向
        dup2(cmd->out_fd, 1);       // 将文件描述符复制到标准输出
        close(cmd->out_fd);         // 关闭原文件描述符
    }
    execvp(cmd->args[0], cmd->args);  // 执行命令，在 PATH 中搜索可执行文件
    perror("exec failed");          // 如果 execvp 返回，说明执行失败
    exit(1);                        // 退出子进程
}
```

**功能说明**：在子进程中执行命令，先设置好输入/输出重定向，然后调用 execvp 执行命令。

**dup2 原理**：

- `dup2(oldfd, newfd)` 将 oldfd 复制到 newfd
- 如果 newfd 已打开，先关闭它
- 这样可以将文件重定向到标准输入/输出

***

### exec\_pipeline 函数 - 执行管道命令 (第153-183行)

```c
void exec_pipeline(Command cmds[], int cmd_cnt) {
    int i, fd[2], in_fd = 0;      // fd[2] 用于管道，in_fd 保存上一个管道的读端
    pid_t last_pid = -1;          // 记录最后一个子进程的 PID

    for (i = 0; i < cmd_cnt; i++) {  // 遍历每个命令
        pipe(fd);                 // 创建管道，fd[0] 为读端，fd[1] 为写端
        pid_t pid = fork();       // 创建子进程

        if (pid == 0) {           // 子进程
            signal(SIGINT, SIG_DFL);   // 恢复 SIGINT 默认行为（可被 Ctrl+C 终止）
            signal(SIGCHLD, SIG_DFL);  // 恢复 SIGCHLD 默认行为
            dup2(in_fd, 0);       // 将上一个管道的读端（或标准输入）复制到标准输入
            if (i != cmd_cnt - 1) dup2(fd[1], 1);  // 非最后一个命令，将管道写端复制到标准输出
            close(fd[0]); close(fd[1]);  // 关闭管道两端
            exec_cmd(&cmds[i]);   // 执行命令
        }

        last_pid = pid;           // 记录子进程 PID
        close(fd[1]);             // 父进程关闭管道写端
        if (in_fd != 0) close(in_fd);  // 关闭上一个管道读端
        in_fd = fd[0];            // 保存管道读端供下一个命令使用
    }
    if (in_fd != 0) close(in_fd); // 关闭最后一个管道读端

    if (!cmds[0].background) {    // 如果不是后台运行
        int status;
        while (waitpid(-1, &status, 0) > 0);  // 阻塞等待所有子进程结束
    } else {
        printf("[后台运行] PID: %d\n", last_pid);  // 打印后台进程 PID
    }
}
```

**功能说明**：执行管道中的所有命令。通过创建多个子进程和管道，将前一个命令的输出连接到后一个命令的输入。

**管道原理图**：

```
命令1          命令2          命令3
  │             │             │
stdout ──► pipe1 ──► stdout ──► pipe2 ──► stdout
              │                    │
            stdin               stdin
```

***

## 内置命令

### builtin\_cd 函数 - cd 命令实现 (第185-207行)

```c
int builtin_cd(char **args) {
    if (args[1] == NULL) {        // 无参数，切换到 HOME 目录
        char *home = getenv("HOME");  // 获取 HOME 环境变量
        if (home == NULL) {
            fprintf(stderr, "cd: HOME not set\n");  // HOME 未设置时报错
            return 1;
        }
        if (chdir(home) != 0) {   // 切换目录
            perror("cd");         // 切换失败打印错误
            return 1;
        }
    } else {                      // 有参数，切换到指定目录
        if (chdir(args[1]) != 0) {
            perror("cd");
            return 1;
        }
    }
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("当前目录: %s\n", cwd);  // 打印当前工作目录
    }
    return 0;                     // 返回 0 表示成功
}
```

**功能说明**：实现 cd 内置命令，支持无参数切换到 HOME 目录，切换成功后显示当前目录。

***

### builtin\_history 函数 - history 命令实现 (第209-213行)

```c
int builtin_history(char **args) {
    (void)args;                   // 忽略未使用的参数
    show_history();               // 显示历史命令
    return 0;
}
```

**功能说明**：实现 history 内置命令，显示最近 10 条历史命令。

***

## main 函数 - 主程序入口 (第215-272行)

```c
int main() {
    char line[1024];              // 存储用户输入的命令行
    
    load_history();               // 启动时加载历史命令
    setup_signals();              // 注册信号处理函数
    
    while (1) {                   // 主循环
        printf("myshell> ");      // 打印提示符
        fflush(stdout);           // 刷新输出缓冲区

        if (!fgets(line, sizeof(line), stdin)) break;  // 读取一行输入，失败则退出
        line[strcspn(line, "\n")] = '\0';  // 去除末尾的换行符
        trim(line);               // 去除首尾空白
        if (strlen(line) == 0) continue;  // 空行则跳过

        add_history(line);        // 将命令添加到历史记录

        Command cmds[MAX_CMDS] = {0};  // 初始化命令数组
        int cmd_cnt = 0;          // 命令计数

        char *cmd_part = strtok(line, "|");  // 按管道符分割第一个命令
        while (cmd_part && cmd_cnt < MAX_CMDS) {  // 遍历每个命令部分
            trim(cmd_part);       // 去除命令首尾空白
            int idx = 0;          // 参数索引
            cmds[cmd_cnt].args[idx++] = strtok(cmd_part, " ");  // 按空格分割第一个参数
            while ((cmds[cmd_cnt].args[idx++] = strtok(NULL, " ")) != NULL);  // 继续分割剩余参数
            parse_single_cmd(&cmds[cmd_cnt]);  // 解析单个命令
            cmd_cnt++;            // 命令计数加一
            cmd_part = strtok(NULL, "|");  // 获取下一个命令部分
        }

        if (cmds[0].args[0] == NULL) continue;  // 空命令跳过

        // 内置命令 exit
        if (!strcmp(cmds[0].args[0], "exit")) {
            printf("bye~\n");     // 打印告别信息
            break;                // 退出循环
        }

        // 内置命令 cd
        if (!strcmp(cmds[0].args[0], "cd")) {
            builtin_cd(cmds[0].args);  // 执行 cd 命令
            continue;             // 继续下一轮循环
        }

        // 内置命令 history
        if (!strcmp(cmds[0].args[0], "history")) {
            builtin_history(cmds[0].args);  // 执行 history 命令
            continue;             // 继续下一轮循环
        }

        exec_pipeline(cmds, cmd_cnt);  // 执行管道命令
    }
    
    save_history();               // 退出时保存历史命令
    for (int i = 0; i < history_count; i++) {
        free(history[i]);         // 释放历史命令内存
    }
    
    return 0;                     // 程序正常退出
}
```

**功能说明**：主循环读取用户输入，解析命令，处理内置命令（exit、cd、history），然后执行管道命令。程序启动时加载历史命令，退出时保存历史命令并释放内存。

***

## 功能总结

| 功能     | 语法             | 示例                      |
| ------ | -------------- | ----------------------- |
| 基本命令执行 | `command args` | `ls -la`                |
| 管道     | `cmd1 \| cmd2` | `ls \| grep .c`         |
| 输入重定向  | `< file`       | `cat < input.txt`       |
| 输出覆盖   | `> file`       | `ls > output.txt`       |
| 输出追加   | `>> file`      | `echo hello >> log.txt` |
| 后台运行   | `&`            | `sleep 10 &`            |
| 切换目录   | `cd [dir]`     | `cd /home`              |
| 历史命令   | `history`      | `history`               |
| 退出     | `exit`         | `exit`                  |

***

## 新增功能详解

### 1. 信号处理机制

```
用户按 Ctrl+C
      ↓
触发 SIGINT 信号
      ↓
sigint_handler 被调用
      ↓
打印新提示符，继续等待输入
（shell 不会退出）
```

### 2. 僵尸进程回收机制

```
子进程终止
      ↓
内核发送 SIGCHLD 信号给父进程
      ↓
sigchld_handler 被调用
      ↓
waitpid(WNOHANG) 非阻塞回收
      ↓
僵尸进程被清除
```

### 3. 历史命令持久化

```
程序启动
      ↓
load_history() 从 ~/.shell_history 加载
      ↓
用户输入命令
      ↓
add_history() 添加到内存数组
      ↓
程序退出
      ↓
save_history() 保存到 ~/.shell_history
```

***

## 执行流程图

```
程序启动
    ↓
加载历史命令 + 注册信号处理
    ↓
┌─────────────────────────────┐
│         主循环              │
│  ┌───────────────────────┐  │
│  │ 打印提示符            │  │
│  │ 读取用户输入          │  │
│  │ 添加到历史记录        │  │
│  │ 按 | 分割命令         │  │
│  │ 解析重定向和后台标志  │  │
│  │ 检查内置命令          │  │
│  │   ├─ exit → 退出      │  │
│  │   ├─ cd → 切换目录    │  │
│  │   └─ history → 显示   │  │
│  │ 执行管道命令          │  │
│  └───────────────────────┘  │
└─────────────────────────────┘
    ↓
保存历史命令 + 释放内存
    ↓
程序退出
```

***

## 编译与运行

```bash
# 编译
gcc -o myshell shell.c

# 运行
./myshell
```

***

## 注意事项

1. **历史文件位置**：保存在 `~/.shell_history`（用户主目录下）
2. **信号处理**：Ctrl+C 不会退出 shell，使用 `exit` 命令退出
3. **僵尸进程**：通过 SIGCHLD 信号自动回收，无需手动处理
4. **后台进程**：使用 `&` 后台运行时，会打印子进程 PID

