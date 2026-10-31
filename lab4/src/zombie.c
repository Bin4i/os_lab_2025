#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Ошибка при вызове fork");
        return 1;
    }

    if (pid == 0) {
        printf("Дочерний процесс (PID: %d) запущен...\n", getpid());
        printf("Дочерний процесс сейчас завершится.\n");
        exit(0);
    } else {
        printf("Родительский процесс (PID: %d) создал дочерний (PID: %d).\n", getpid(), pid);
        printf("Родитель спит 30 секунд. Проверьте список процессов!\n");
        printf("Выполните команду: ps aux | grep zombie\n");
        sleep(30); 
        printf("Родитель проснулся и сейчас очистит зомби с помощью wait().\n");
        wait(NULL); 
        printf("Зомби-процесс очищен. Родитель завершает работу.\n");
    }

    return 0;
}
