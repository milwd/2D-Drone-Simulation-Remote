#ifndef BLACKBOARD_H
#define BLACKBOARD_H

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define SHM_NAME    "/blackboard_shm"
#define SEM_NAME    "/blackboard_sem"
#define JSON_PATH   "config.json"
#define PIPE_BLACKBOARD "/tmp/blackboard_pipe"
#define PIPE_DYNAMICS   "/tmp/dynamics_pipe"
#define PIPE_KEYBOARD   "/tmp/keyboard_pipe"
#define PIPE_WINDOW     "/tmp/window_pipe"
#define PIPE_OBSTACLE   "/tmp/obstacle_pipe"
#define PIPE_TARGET     "/tmp/target_pipe"
#define PIPE_IPCHECK    "/tmp/ipcheck_pipe"

extern pthread_mutex_t logger_mutex;
static FILE *log_file = NULL;
pthread_mutex_t logger_mutex = PTHREAD_MUTEX_INITIALIZER;

#define NUMBER_OF_PROCESSES 7
#define MAX_MSG_LENGTH 256

// SIMULATION HYPERPARAMETERS
#define WIN_SIZE_X 200      // max width for 1080p
#define WIN_SIZE_Y 50       // max height for 1080p
#define RENDER_DELAY 100000 // microseconds
#define RETRY_DELAY 50000   // for opening pipes
#define MAX_RETRIES 20      // for opening pipes
#define TIMEOUT_SECONDS 10  // for watchdog

#define EPSILON 0.0001      // small value to avoid division by zero
#define DT  0.001           // time step
#define MAX_OBJECTS 100     // max number of obstacles and targets
#define BLACKBOARD_CHECK_DELAY      5  // update blackboard every 1 second
#define OBSTACLE_GENERATION_DELAY   4  // generate obstacles every 4 seconds
#define TARGET_GENERATION_DELAY     6  // generate targets every 6 seconds
#define WATCHDOG_HEARTBEAT_DELAY    1  // check heartbeat every 1 second


// SHARED STRUCTURES and DATA STRUCTURES
typedef struct {
    int hit_obstacles;
    int hit_targets;
    double time_elapsed;
    double distance_traveled;
} Stats;

typedef struct {
    double mass;
    double visc_damp_coef;
    double obst_repl_coef;
    double radius;
} Physix;

typedef struct {
    int domainnum;
    char topicobstacles[50];
    char topictargets[50];
    int discoveryport;
    char transmitterip[20];  // either this ip is used (mode 2)
    char receiverip[20];     // or this ip is used (mode 3)
} ConfigDDS;

typedef struct {
    Stats stats;
    Physix physix;
    ConfigDDS configdds;
    int state;
    double score;
    int n_obstacles;
    int n_targets;
    int drone_x, drone_y; 
    int command_force_x, command_force_y;
    int obstacle_xs[MAX_OBJECTS];
    int obstacle_ys[MAX_OBJECTS];
    int target_xs[MAX_OBJECTS];
    int target_ys[MAX_OBJECTS];
    int max_height;
    int max_width;
} newBlackboard;

// COMMON FUNCTIONS
static inline void logger(const char *format, ...) {
    if (!log_file) {
        log_file = fopen("./logs/simulation.log", "a");
        if (!log_file) {
            perror("Unable to open log file");
            return;
        }
    }
    pthread_mutex_lock(&logger_mutex);
    time_t now = time(NULL);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(log_file, "[%s] ", time_str);
    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);
    fprintf(log_file, "\n");
    fflush(log_file);
    pthread_mutex_unlock(&logger_mutex);
}

int open_watchdog_pipe(const char *pipe_name) {
    int fd, retries = 0;
    while ((fd = open(pipe_name, O_RDWR | O_NONBLOCK)) < 0 && retries < MAX_RETRIES) {
        if (errno == ENOENT) {  // FIFO does not exist yet
            usleep(RETRY_DELAY);  // Wait and retry
            retries++;
        } else {
            perror("Failed to open watchdog pipe");
            return -1;
        }
    }
    return fd; 
}

void send_heartbeat(int fd) {
    if (write(fd, "HB", 2) < 0){
        perror("Failed to write to watchdog pipe");
    };
}

#endif 