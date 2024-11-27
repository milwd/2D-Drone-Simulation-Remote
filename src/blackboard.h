#ifndef BLACKBOARD_H
#define BLACKBOARD_H

#define SHM_KEY 12345
// #define SHM_SIZE 4000
#define SEM_NAME "/blackboard_sem"
#define PIPE_NAME "/tmp/keyboard_pipe"
#define PARAM_FILE "parameters.txt"

#define WIN_SIZE_X 60
#define WIN_SIZE_Y 20

#define M   1.0  // mass
#define K   1.0  // viscous damping coefficient
#define DT  0.001 // time step
#define R   5.0  // radius of the drone obstacle repulsion
#define ETA 5.0 // strength of the obstacle repulsion factor

typedef struct {
    int drone_x, drone_y;
    int score;
    int n_obstacles;
    int n_targets;
    int command_force_x, command_force_y;
    int obstacle_xs[100];
    int obstacle_ys[100];
    int target_xs[100];
    int target_ys[100];
} newBlackboard;

// typedef struct {
//     int drone_x, drone_y;
//     int target_x, target_y;
//     int obstacle_x, obstacle_y;
//     int score;
// } Blackboard;

#endif 
