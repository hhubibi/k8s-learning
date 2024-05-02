/**
namespace隔离例子
*/

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/mount.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <sys/wait.h>

#define STACK_SIZE (1024 * 1024)
static char container_stack[STACK_SIZE];
char* const container_args[] = {
    "/bin/bash",
    NULL
};

// 子进程将要执行的函数
int container_main(void *arg) {
    printf("Container - inside the container! PID=%d\n", getpid());
    mount("none", "/tmp", "tmpfs", 0, "");
    execv(container_args[0], container_args);
    printf("Something wrong!\n");
    return 1;
}

int main() {
    printf("Parent - start a container! PID=%d\n", getpid());

    // CLONE_NEWNS启用MOUNT namespace
    // CLONE_NEWPID启用PID namespace
    pid_t container_pid = clone(container_main, container_stack + STACK_SIZE, CLONE_NEWNS | CLONE_NEWPID | SIGCHLD, NULL);
    if (container_pid == -1) {
        perror("clone");
        exit(EXIT_FAILURE);
    }

    // 等待子进程结束
    waitpid(container_pid, NULL, 0);
    printf("Parent - container stopped\n");
    return 0;
}