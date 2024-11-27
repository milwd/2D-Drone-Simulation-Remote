#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#define WATCHDOG_TIMEOUT 10


void timeout_handler(int signum) {
    printf("Watchdog timeout! Shutting down system...\n");
    exit(0);
}

int main() {
    signal(SIGALRM, timeout_handler);
    alarm(WATCHDOG_TIMEOUT);

    while (1) {
        printf("Watchdog is active...\n");
        sleep(5);
    }

    return 0;
}
