#ifndef BLACKBOARD_H
#define BLACKBOARD_H

#define SHM_KEY 12346
#define SEM_NAME "/blackboard_sem"
#define PIPE_NAME "/tmp/keyboard_pipe"
#define PARAM_FILE "parameters.txt"

#define WIN_SIZE_X 60
#define WIN_SIZE_Y 20
#define RENDER_DELAY 100000 // microseconds

#define MAX_TARGETS 100
#define MAX_OBSTACLES 100

#define M   1.0     // mass
#define K   1.0     // viscous damping coefficient
#define DT  0.001   // time step
#define R   5.0     // radius of the drone obstacle repulsion
#define ETA 5.0     // strength of the obstacle repulsion factor

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
    int obstacle_xs[MAX_OBSTACLES];
    int obstacle_ys[MAX_OBSTACLES];
    int target_xs[MAX_TARGETS];
    int target_ys[MAX_TARGETS];
} newBlackboard;

#endif 
