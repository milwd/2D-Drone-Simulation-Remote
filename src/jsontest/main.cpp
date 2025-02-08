#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cjson/cJSON.h"
#include <cstring>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

typedef struct {
    int num_obstacles;
    int num_targets;
    int mass;
    int visc_damp_coef;
    int obst_repl_coef;
    int radius;
    
    int domainnum;
    char topicobstacles[50];
    char topictargets[50];
    int discoveryport;
    char transmitterip[20];
    char receiverip[20];
} SimulationConfig;

void parseIP(const char* ip, int * octets) {
    char ip_copy[20];
    std::strcpy(ip_copy, ip);  // Make a copy of the IP string
    
    // Tokenize the IP address using '.' as the delimiter
    char* token = std::strtok(ip_copy, ".");
    int i = 0;
    while (token != nullptr && i < 4) {
        octets[i++] = std::stoi(token);  // Convert token to integer and store it
        token = std::strtok(nullptr, ".");  // Get next token
    }
}

void read_json(SimulationConfig *config, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Could not open file");
        return;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *data = (char *)malloc(length + 1);
    if (!data) {
        perror("Memory allocation failed for JSON config");
        fclose(file);
        return;
    }

    fread(data, 1, length, file);
    data[length] = '\0';
    fclose(file);

    cJSON *json = cJSON_Parse(data);
    if (!json) {
        printf("Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        free(data);
        return;
    }
    free(data);

    // Safely extract integer values
    cJSON *item;
    if ((item = cJSON_GetObjectItem(json, "num_obstacles"))) config->num_obstacles = item->valueint;
    if ((item = cJSON_GetObjectItem(json, "num_targets"))) config->num_targets = item->valueint;
    if ((item = cJSON_GetObjectItem(json, "mass"))) config->mass = item->valueint;
    if ((item = cJSON_GetObjectItem(json, "visc_damp_coef"))) config->visc_damp_coef = item->valueint;
    if ((item = cJSON_GetObjectItem(json, "obst_repl_coef"))) config->obst_repl_coef = item->valueint;
    if ((item = cJSON_GetObjectItem(json, "radius"))) config->radius = item->valueint;
    if ((item = cJSON_GetObjectItem(json, "domainnum"))) config->domainnum = item->valueint;
    if ((item = cJSON_GetObjectItem(json, "discoveryport"))) config->discoveryport = item->valueint;

    // Safely extract string values
    if ((item = cJSON_GetObjectItem(json, "topicobstacles")) && cJSON_IsString(item))
        strncpy(config->topicobstacles, item->valuestring, sizeof(config->topicobstacles) - 1);
    
    if ((item = cJSON_GetObjectItem(json, "topictargets")) && cJSON_IsString(item))
        strncpy(config->topictargets, item->valuestring, sizeof(config->topictargets) - 1);

    if ((item = cJSON_GetObjectItem(json, "transmitterip")) && cJSON_IsString(item))
        strncpy(config->transmitterip, item->valuestring, sizeof(config->transmitterip) - 1);

    if ((item = cJSON_GetObjectItem(json, "receiverip")) && cJSON_IsString(item))
        strncpy(config->receiverip, item->valuestring, sizeof(config->receiverip) - 1);

    cJSON_Delete(json);

    std::stringstream test(config->receiverip);
    std::string segment;
    std::vector<std::string> seglist;

    while(std::getline(test, segment, '.')){
        seglist.push_back(segment);
    }
    for (int i = 0; i < seglist.size(); i++){
        int num = std::stoi(seglist[i]);
        printf("%d\n", num);

    }
}

int main() {
    const char *filename = "config.json";
    SimulationConfig config = {0}; // Initialize to zero

    read_json(&config, filename);

    printf("Configuration Loaded:\n");
    printf("num_obstacles: %d\n", config.num_obstacles);
    printf("num_targets: %d\n", config.num_targets);
    printf("mass: %d\n", config.mass);
    printf("visc_damp_coef: %d\n", config.visc_damp_coef);
    printf("obst_repl_coef: %d\n", config.obst_repl_coef);
    printf("radius: %d\n", config.radius);
    printf("domainnum: %d\n", config.domainnum);
    printf("topicobstacles: %s\n", config.topicobstacles);
    printf("topictargets: %s\n", config.topictargets);
    printf("discoveryport: %d\n", config.discoveryport);
    printf("transmitterip: %s\n", config.transmitterip);
    printf("receiverip: %s\n", config.receiverip);

    return 0;
}
