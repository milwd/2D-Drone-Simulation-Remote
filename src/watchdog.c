#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include "blackboard.h"


typedef struct {
    const char *pipe_name;
    int fd;
    time_t last_heartbeat;
} ComponentMonitor;


int main() {  // watchdog works and receives signals neglecting the state machine of the whole process. others have been implemented to send heartbeats whatever the state is. 

    logger("Big brother Watchdog process is watching...");

    ComponentMonitor components[] = {
        {PIPE_BLACKBOARD,   open(PIPE_BLACKBOARD,   O_RDONLY | O_NONBLOCK), time(NULL)},
        {PIPE_DYNAMICS,     open(PIPE_DYNAMICS,     O_RDONLY | O_NONBLOCK), time(NULL)},
        {PIPE_KEYBOARD,     open(PIPE_KEYBOARD,     O_RDONLY | O_NONBLOCK), time(NULL)},
        {PIPE_WINDOW,       open(PIPE_WINDOW,       O_RDONLY | O_NONBLOCK), time(NULL)},
        {PIPE_OBSTACLE,     open(PIPE_OBSTACLE,     O_RDONLY | O_NONBLOCK), time(NULL)}, 
        {PIPE_TARGET,       open(PIPE_TARGET,       O_RDONLY | O_NONBLOCK), time(NULL)}
    };

    fd_set readfds;
    struct timeval timeout;

    while (1) {
        FD_ZERO(&readfds);
        int max_fd = 0;
        time_t now = time(NULL);

        for (int i = 0; i < 6; i++) {
            if (components[i].fd > 0) {
                FD_SET(components[i].fd, &readfds);
                if (components[i].fd > max_fd)
                    max_fd = components[i].fd;
            }
        }
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        if (select(max_fd + 1, &readfds, NULL, NULL, &timeout) < 0) {
            perror("select failed");
            break;
        }
        for (int i = 0; i < 6; i++) {
            if (FD_ISSET(components[i].fd, &readfds)) {
                int len;
                char buffer[16];
                while ((len = read(components[i].fd, buffer, sizeof(buffer) - 1)) > 0) {
                    components[i].last_heartbeat = now;  // Update timestamp only with the latest read
                    printf("Watchdog received: %s from %s\n", buffer, components[i].pipe_name);
                }
            }
            if (difftime(now, components[i].last_heartbeat) > TIMEOUT_SECONDS) {
                fprintf(stderr, "Watchdog ALERT: No heartbeat from %s!\n", components[i].pipe_name);
                for (int i = 0; i < 6; i++) {
                    if (components[i].fd > 0) {
                        close(components[i].fd);
                    }
                }
                exit(EXIT_FAILURE);
            }
        }
        sleep(1);
    }
    for (int i = 0; i < 6; i++) {
        if (components[i].fd > 0) {
            close(components[i].fd);
        }
    }

    return 0;
}
