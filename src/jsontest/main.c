#include <stdio.h>
#include <stdlib.h>
#include "cjson/cJSON.h"

typedef struct {
    int num_obstacles;
    int num_targets;
    int mass;
    int visc_damp_coef;
    int obst_repl_coef;
    int radius;
} SimulationConfig;


int main() {
    const char *filename = "config.json";
    SimulationConfig config;
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Could not open file");
        return -1;
    }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data = (char *)malloc(length + 1);
    if (!data) {
        perror("Memory allocation failed for JSON config");
        fclose(file);
        return -1;
    }
    fread(data, 1, length, file);
    data[length] = '\0';
    fclose(file);
    cJSON *json = cJSON_Parse(data);
    if (!json) {
        printf("Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        return 0;
    }
    free(data);
    config.num_obstacles = cJSON_GetObjectItem(json, "num_obstacles")->valueint;
    config.num_targets = cJSON_GetObjectItem(json, "num_targets")->valueint;
    config.mass = cJSON_GetObjectItem(json, "mass")->valueint;
    config.visc_damp_coef = cJSON_GetObjectItem(json, "visc_damp_coef")->valueint;
    config.obst_repl_coef = cJSON_GetObjectItem(json, "obst_repl_coef")->valueint;
    config.radius = cJSON_GetObjectItem(json, "radius")->valueint;
    cJSON_Delete(json);
    printf("Configuration Loaded:\n");
    printf("num_obstacles: %d\n", config.num_obstacles);
    printf("num_targets: %d\n", config.num_targets);
    printf("mass: %d\n", config.mass);
    printf("visc_damp_coef: %d\n", config.visc_damp_coef);
    printf("obst_repl_coef: %d\n", config.obst_repl_coef);
    printf("radius: %d\n", config.radius);
    return 0;
}
