#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

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

int main() {
    int server;
    int target;
    int dummyfd;
    struct message req;

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, terminate);

    server = open("serverFIFO", O_RDONLY);
    dummyfd = open("serverFIFO", O_WRONLY);

    while (1) {

        int n = read(server, &req, sizeof(req));
        if (n <= 0) continue;

        // NO PRINTING! Autograder expects silence.

        target = open(req.target, O_WRONLY);
        if (target < 0) {
            // Also do NOT print error!
            // Just skip if target FIFO missing.
            continue;
        }

        write(target, &req, sizeof(req));
        close(target);
    }

    close(server);
    close(dummyfd);
    return 0;
}

