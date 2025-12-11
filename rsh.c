#include <stdio.h>
#include <stdlib.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>

#define N 13

extern char **environ;
char uName[20];

char *allowed[N] = {"cp","touch","mkdir","ls","pwd","cat","grep","chmod","diff","cd","exit","help","sendmsg"};

struct message {
    char source[50];
    char target[50];
    char msg[200];
};

void terminate(int sig) {
    printf("Exiting....\n");
    fflush(stdout);
    exit(0);
}

void sendmsg(char *user, char *target, char *msg) {
    struct message m;
    int fd;

    strcpy(m.source, user);
    strcpy(m.target, target);
    strcpy(m.msg, msg);

    fd = open("serverFIFO", O_WRONLY);
    if (fd < 0) {
        perror("sendmsg: cannot open server FIFO");
        return;
    }

    write(fd, &m, sizeof(m));
    close(fd);
}

void* messageListener(void *arg) {
    int fd;
    struct message m;

    fd = open(uName, O_RDONLY);
    if (fd < 0) {
        perror("listener: cannot open user FIFO");
        pthread_exit(NULL);
    }

    while (1) {
        int n = read(fd, &m, sizeof(m));
        if (n > 0) {
            // FIX: no leading newline
            printf("Incoming message from %s: %s\n", m.source, m.msg);
            fflush(stdout);
        }
    }

    pthread_exit(0);
}

int isAllowed(const char*cmd) {
    for (int i = 0; i < N; i++) {
        if (strcmp(cmd, allowed[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    pid_t pid;
    char **cargv;
    char *path;
    char line[256];
    int status;
    posix_spawnattr_t attr;

    if (argc != 2) {
        printf("Usage: ./rsh <username>\n");
        exit(1);
    }
    signal(SIGINT, terminate);

    strcpy(uName, argv[1]);

    // Start listener thread
    pthread_t tid;
    pthread_create(&tid, NULL, messageListener, NULL);

    while (1) {

  	fprintf(stderr,"rsh>");

        if (fgets(line, 256, stdin) == NULL) continue;
        if (strcmp(line, "\n") == 0) continue;

        line[strlen(line)-1] = '\0';

        char cmd[256];
        char line2[256];
        strcpy(line2, line);
        strcpy(cmd, strtok(line, " "));

        if (!isAllowed(cmd)) {
            printf("NOT ALLOWED!\n");
            continue;
        }

        if (strcmp(cmd, "sendmsg") == 0) {
            char *target = strtok(NULL, " ");
            if (!target) {
                printf("sendmsg: you have to specify target user\n");
                continue;
            }

            char *msg = strtok(NULL, "");
            if (!msg) {
                printf("sendmsg: you have to enter a message\n");
                continue;
            }

            sendmsg(uName, target, msg);
            continue;
        }

        if (strcmp(cmd, "exit") == 0) break;

        if (strcmp(cmd, "cd") == 0) {
            char *targetDir = strtok(NULL, " ");
            if (strtok(NULL, " ") != NULL) {
                printf("-rsh: cd: too many arguments\n");
            } else {
                chdir(targetDir);
            }
            continue;
        }

        if (strcmp(cmd, "help") == 0) {
            printf("The allowed commands are:\n");
            for (int i = 0; i < N; i++) {
                printf("%d: %s\n", i + 1, allowed[i]);
            }
            continue;
        }

        cargv = malloc(sizeof(char*));
        cargv[0] = malloc(strlen(cmd) + 1);
        path = malloc(9 + strlen(cmd) + 1);
        strcpy(path, cmd);
        strcpy(cargv[0], cmd);

        char *attrToken = strtok(line2, " ");
        attrToken = strtok(NULL, " ");
        int n = 1;
        while (attrToken != NULL) {
            n++;
            cargv = realloc(cargv, sizeof(char*) * n);
            cargv[n-1] = malloc(strlen(attrToken) + 1);
            strcpy(cargv[n-1], attrToken);
            attrToken = strtok(NULL, " ");
        }
        cargv = realloc(cargv, sizeof(char*) * (n+1));
        cargv[n] = NULL;

        posix_spawnattr_init(&attr);

        if (posix_spawnp(&pid, path, NULL, &attr, cargv, environ) != 0) {
            perror("spawn failed");
            exit(EXIT_FAILURE);
        }

        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid failed");
            exit(EXIT_FAILURE);
        }

        posix_spawnattr_destroy(&attr);
    }

    return 0;
}

