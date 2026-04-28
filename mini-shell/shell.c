#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>

#define MAX_ARGS 100
#define MAX_CMDS 10
#define MAX_HISTORY 1000
#define HISTORY_FILE ".shell_history"

typedef struct {
    char *args[MAX_ARGS];
    int in_fd;
    int out_fd;
    int append;
    int background;
} Command;

char *history[MAX_HISTORY];
int history_count = 0;

void load_history() {
    char *home = getenv("HOME");
    char path[512];
    if (home) {
        snprintf(path, sizeof(path), "%s/%s", home, HISTORY_FILE);
    } else {
        strcpy(path, HISTORY_FILE);
    }
    
    FILE *fp = fopen(path, "r");
    if (!fp) return;
    
    char line[1024];
    while (fgets(line, sizeof(line), fp) && history_count < MAX_HISTORY) {
        line[strcspn(line, "\n")] = '\0';
        history[history_count] = strdup(line);
        history_count++;
    }
    fclose(fp);
}

void save_history() {
    char *home = getenv("HOME");
    char path[512];
    if (home) {
        snprintf(path, sizeof(path), "%s/%s", home, HISTORY_FILE);
    } else {
        strcpy(path, HISTORY_FILE);
    }
    
    FILE *fp = fopen(path, "w");
    if (!fp) return;
    
    for (int i = 0; i < history_count; i++) {
        fprintf(fp, "%s\n", history[i]);
    }
    fclose(fp);
}

void add_history(const char *cmd) {
    if (strlen(cmd) == 0) return;
    if (history_count < MAX_HISTORY) {
        history[history_count] = strdup(cmd);
        history_count++;
    }
}

void show_history() {
    int start = (history_count > 10) ? history_count - 10 : 0;
    for (int i = start; i < history_count; i++) {
        printf("%d %s\n", i + 1, history[i]);
    }
}

void sigchld_handler(int sig) {
    (void)sig;
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)) {
        } else if (WIFSIGNALED(status)) {
        }
    }
}

void sigint_handler(int sig) {
    (void)sig;
    printf("\nmyshell> ");
    fflush(stdout);
}

void setup_signals() {
    signal(SIGINT, sigint_handler);
    signal(SIGCHLD, sigchld_handler);
    signal(SIGTSTP, SIG_IGN);
}

void trim(char *s) {
    char *p = s;
    while (*p && isspace(*p)) p++;
    memmove(s, p, strlen(p) + 1);
    p = s + strlen(s) - 1;
    while (p >= s && isspace(*p)) *p-- = '\0';
}

void parse_single_cmd(Command *cmd) {
    cmd->in_fd = 0;
    cmd->out_fd = 1;
    cmd->append = 0;
    cmd->background = 0;

    int i = 0, j = 0;
    while (cmd->args[i]) {
        if (!strcmp(cmd->args[i], "<")) {
            cmd->in_fd = open(cmd->args[i+1], O_RDONLY);
            i += 2;
        } else if (!strcmp(cmd->args[i], ">")) {
            cmd->out_fd = open(cmd->args[i+1], O_WRONLY|O_CREAT|O_TRUNC, 0644);
            i += 2;
        } else if (!strcmp(cmd->args[i], ">>")) {
            cmd->out_fd = open(cmd->args[i+1], O_WRONLY|O_CREAT|O_APPEND, 0644);
            i += 2;
        } else if (!strcmp(cmd->args[i], "&")) {
            cmd->background = 1;
            i++;
        } else {
            cmd->args[j++] = cmd->args[i++];
        }
    }
    cmd->args[j] = NULL;
}

void exec_cmd(Command *cmd) {
    if (cmd->in_fd != 0) {
        dup2(cmd->in_fd, 0);
        close(cmd->in_fd);
    }
    if (cmd->out_fd != 1) {
        dup2(cmd->out_fd, 1);
        close(cmd->out_fd);
    }
    execvp(cmd->args[0], cmd->args);
    perror("exec failed");
    exit(1);
}

void exec_pipeline(Command cmds[], int cmd_cnt) {
    int i, fd[2], in_fd = 0;
    pid_t last_pid = -1;

    for (i = 0; i < cmd_cnt; i++) {
        pipe(fd);
        pid_t pid = fork();

        if (pid == 0) {
            signal(SIGINT, SIG_DFL);
            signal(SIGCHLD, SIG_DFL);
            dup2(in_fd, 0);
            if (i != cmd_cnt - 1) dup2(fd[1], 1);
            close(fd[0]); close(fd[1]);
            exec_cmd(&cmds[i]);
        }

        last_pid = pid;
        close(fd[1]);
        if (in_fd != 0) close(in_fd);
        in_fd = fd[0];
    }
    if (in_fd != 0) close(in_fd);

    if (!cmds[0].background) {
        int status;
        while (waitpid(-1, &status, 0) > 0);
    } else {
        printf("[后台运行] PID: %d\n", last_pid);
    }
}

int builtin_cd(char **args) {
    if (args[1] == NULL) {
        char *home = getenv("HOME");
        if (home == NULL) {
            fprintf(stderr, "cd: HOME not set\n");
            return 1;
        }
        if (chdir(home) != 0) {
            perror("cd");
            return 1;
        }
    } else {
        if (chdir(args[1]) != 0) {
            perror("cd");
            return 1;
        }
    }
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("当前目录: %s\n", cwd);
    }
    return 0;
}

int builtin_history(char **args) {
    (void)args;
    show_history();
    return 0;
}

int main() {
    char line[1024];
    
    load_history();
    setup_signals();
    
    while (1) {
        printf("myshell> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = '\0';
        trim(line);
        if (strlen(line) == 0) continue;

        add_history(line);

        Command cmds[MAX_CMDS] = {0};
        int cmd_cnt = 0;

        char *cmd_part = strtok(line, "|");
        while (cmd_part && cmd_cnt < MAX_CMDS) {
            trim(cmd_part);
            int idx = 0;
            cmds[cmd_cnt].args[idx++] = strtok(cmd_part, " ");
            while ((cmds[cmd_cnt].args[idx++] = strtok(NULL, " ")) != NULL);
            parse_single_cmd(&cmds[cmd_cnt]);
            cmd_cnt++;
            cmd_part = strtok(NULL, "|");
        }

        if (cmds[0].args[0] == NULL) continue;

        if (!strcmp(cmds[0].args[0], "exit")) {
            printf("bye~\n");
            break;
        }

        if (!strcmp(cmds[0].args[0], "cd")) {
            builtin_cd(cmds[0].args);
            continue;
        }

        if (!strcmp(cmds[0].args[0], "history")) {
            builtin_history(cmds[0].args);
            continue;
        }

        exec_pipeline(cmds, cmd_cnt);
    }
    
    save_history();
    for (int i = 0; i < history_count; i++) {
        free(history[i]);
    }
    
    return 0;
    
}
