#ifndef BLACKBOARD_H
#define BLACKBOARD_H

#define SHM_KEY 12345
// #define SHM_SIZE 4000
#define SEM_NAME "/blackboard_sem"
#define PIPE_NAME "/tmp/keyboard_pipe"
#define PARAM_FILE "parameters.txt"

#define WIN_SIZE_X 60
#define WIN_SIZE_Y 20

typedef struct {
    int drone_x, drone_y;
    int score;
    int n_obstacles;
    int n_targets;
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
