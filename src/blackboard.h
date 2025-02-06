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

// INITIAL CONDITIONS
#define M   1     // mass
#define K   1     // viscous damping coefficient
#define ETA 15    // strength of the obstacle repulsion factor
#define R   8     // radius of the drone obstacle repulsion
// int mass = M;
// int visc_damp_coef = K;
// int obst_repl_coef = ETA;
// int radius = R;

// COMMON STRUCTURES and DATA STRUCTURES
typedef struct {
    int num_obstacles;
    int num_targets;
    int mass;
    int visc_damp_coef;
    int obst_repl_coef;
    int radius;
} SimulationConfig;

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
    Stats stats;
    Physix physix;
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