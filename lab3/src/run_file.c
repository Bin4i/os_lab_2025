#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

int main() {
    pid_t pid = fork();

    if (pid == -1) {
        perror("Fork failed");
        return 1;
    }

    if (pid == 0) {
        execl("./sequential_min_max", "sequential_min_max", "42", "100", NULL);
        perror("exec failed");
        return 1;
    } else {
        wait(NULL);
        printf("Дочерний процесс sequential_min_max успешно завершен.\n");
    }

    return 0;
}
