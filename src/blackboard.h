#ifndef BLACKBOARD_H
#define BLACKBOARD_H

#include <pthread.h>
#include <stdarg.h>

extern pthread_mutex_t logger_mutex;
#define SHM_NAME "/blackboard_shm"
#define SEM_NAME "/blackboard_sem"
#define PARAM_FILE "parameters.txt"
// #define PIPE_NAME "/tmp/drone_pipe"

static FILE *log_file = NULL;
pthread_mutex_t logger_mutex = PTHREAD_MUTEX_INITIALIZER;

#define NUMBER_OF_PROCESSES 5
#define MAX_MSG_LENGTH 256

#define WIN_SIZE_X 200
#define WIN_SIZE_Y 50
#define RENDER_DELAY 100000 // microseconds

#define MAX_OBJECTS 100  

#define M   1     // mass
#define K   1     // viscous damping coefficient
#define ETA 15     // strength of the obstacle repulsion factor
#define R   8     // radius of the drone obstacle repulsion
int mass = M;
int visc_damp_coef = K;
int obst_repl_coef = ETA;
int radius = R;

#define EPSILON 0.0001  // small value to avoid division by zero
#define DT  0.001   // time step

typedef struct {
    int hit_obstacles;
    int hit_targets;
    double time_elapsed;
    double distance_traveled;
} Stats;

typedef struct {
    Stats stats;
    double score;
    int n_obstacles;
    int n_targets;
    int drone_x, drone_y; 
    int command_force_x, command_force_y;
    int obstacle_xs[MAX_OBJECTS];
    int obstacle_ys[MAX_OBJECTS];
    int target_xs[MAX_OBJECTS];
    int target_ys[MAX_OBJECTS];
    int state;
    int max_height;
    int max_width;
} newBlackboard;

static inline void logger(const char *format, ...) {
    if (!log_file) {
        log_file = fopen("simulation.log", "a");
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

#endif 