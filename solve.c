#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>

#define BUFFER_SIZE 32

int main() {
    int pipe_fd[2];
    pid_t pid;
    char message[BUFFER_SIZE];
    sem_t *sem_parent, *sem_child;

    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    sem_unlink("/parent");
    sem_parent = sem_open("/parent", O_CREAT | O_EXCL, 0644, 1);

    sem_unlink("/child");
    sem_child = sem_open("/child", O_CREAT | O_EXCL, 0644, 0);

    if (sem_parent == SEM_FAILED || sem_child == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Дочерний
        close(pipe_fd[1]);

        // 10 сообщений
        for (int i = 0; i < 10; i++) {
            sem_wait(sem_child);
            read(pipe_fd[0], message, BUFFER_SIZE);
            printf("Потомок получил: %s\n", message);
            sem_post(sem_parent);
        }

        close(pipe_fd[0]);
        sem_close(sem_child);
        sem_close(sem_parent);
        sem_unlink("/child");
        sem_unlink("/parent");
    } else {
        // Родитель
        close(pipe_fd[0]);

        // 10 сообщений
        for (int i = 0; i < 10; i++) {
            sem_wait(sem_parent);
            snprintf(message, BUFFER_SIZE, "Сообщение %d", i + 1);
            write(pipe_fd[1], message, BUFFER_SIZE);
            printf("Родитель отправил: %s\n", message);
            sem_post(sem_child);
        }

        close(pipe_fd[1]);
        wait(NULL);
        sem_close(sem_child);
        sem_close(sem_parent);
        sem_unlink("/child");
        sem_unlink("/parent");
    }

    return 0;
}