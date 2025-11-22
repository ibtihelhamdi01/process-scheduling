#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../../include/algorithms.h"

#define MAX_ALGORITHMS 20


static AlgorithmPlugin dynamic_algorithms[MAX_ALGORITHMS];
static int dynamic_algorithm_count = 0;

static bool use_dynamic_plugins = false;


void scan_algorithms_directory(const char* directory_path) {
    DIR *dir;
    struct dirent *entry;
    
    printf("Scanning for algorithm plugins in: %s\n", directory_path);
    
    dir = opendir(directory_path);
    if (!dir) {
        printf("Warning: Cannot open directory: %s\n", directory_path);
        return;
    }
    
    while ((entry = readdir(dir)) != NULL && dynamic_algorithm_count < MAX_ALGORITHMS) {
        
        char *ext = strrchr(entry->d_name, '.');
        if (ext && strcmp(ext, ".so") == 0) {
            char lib_path[512];
            snprintf(lib_path, sizeof(lib_path), "%s/%s", directory_path, entry->d_name);
            
            printf("Found plugin: %s\n", lib_path);
            
            
            void *handle = dlopen(lib_path, RTLD_LAZY);
            if (!handle) {
                printf("Warning: Cannot load %s: %s\n", entry->d_name, dlerror());
                continue;
            }
            
            
            AlgorithmInfo* (*get_info)(void) = dlsym(handle, "get_algorithm_info_plugin");
            if (!get_info) {
                printf("Warning: Invalid plugin %s (missing get_algorithm_info_plugin)\n", entry->d_name);
                dlclose(handle);
                continue;
            }
            
            
            AlgorithmInfo* info = get_info();
            if (info) {
                dynamic_algorithms[dynamic_algorithm_count].handle = handle;
                dynamic_algorithms[dynamic_algorithm_count].execute = info->execute;
                strncpy(dynamic_algorithms[dynamic_algorithm_count].name, info->name, 
                       sizeof(dynamic_algorithms[dynamic_algorithm_count].name));
                strncpy(dynamic_algorithms[dynamic_algorithm_count].display_name, info->display_name, 
                       sizeof(dynamic_algorithms[dynamic_algorithm_count].display_name));
                dynamic_algorithms[dynamic_algorithm_count].id = info->id;
                dynamic_algorithms[dynamic_algorithm_count].requires_quantum = info->requires_quantum;
                
                printf("✓ Loaded: %s (%s)\n", info->name, info->display_name);
                dynamic_algorithm_count++;
            } else {
                printf("Warning: get_algorithm_info_plugin returned NULL for %s\n", entry->d_name);
                dlclose(handle);
            }
        }
    }
    
    closedir(dir);
}

void register_algorithms(void) {
    
    scan_algorithms_directory("./algorithms");
    
    if (dynamic_algorithm_count == 0) {
        printf("No algorithm plugins found in ./algorithms/\n");
        printf("Please build the algorithms with: make algorithms\n");
        use_dynamic_plugins = false;
    } else {
        use_dynamic_plugins = true;
        printf("Successfully loaded %d algorithm plugins\n", dynamic_algorithm_count);
    }
}

int get_algorithm_count(void) {
    return dynamic_algorithm_count;
}

AlgorithmInfo* get_algorithm_info(int index) {
    if (index >= 0 && index < dynamic_algorithm_count) {
        
        static AlgorithmInfo temp_info;
        temp_info.name = dynamic_algorithms[index].name;
        temp_info.display_name = dynamic_algorithms[index].display_name;
        temp_info.id = dynamic_algorithms[index].id;
        temp_info.execute = dynamic_algorithms[index].execute;
        temp_info.requires_quantum = dynamic_algorithms[index].requires_quantum;
        return &temp_info;
    }
    return NULL;
}

AlgorithmInfo* get_algorithm_by_id(Algorithm id) {
    for (int i = 0; i < dynamic_algorithm_count; i++) {
        AlgorithmInfo* algo = get_algorithm_info(i);
        if (algo->id == id) {
            return algo;
        }
    }
    return NULL;
}

AlgorithmPlugin* get_algorithm_plugin_by_index(int index) {
    if (index >= 0 && index < dynamic_algorithm_count) {
        return &dynamic_algorithms[index];
    }
    return NULL;
}

AlgorithmPlugin* get_algorithm_plugin_by_name(const char* name) {
    for (int i = 0; i < dynamic_algorithm_count; i++) {
        if (strcmp(dynamic_algorithms[i].name, name) == 0) {
            return &dynamic_algorithms[i];
        }
    }
    return NULL;
}

int get_algorithm_plugins_count(void) {
    return dynamic_algorithm_count;
}

bool algorithm_exists(Algorithm id) {
    return get_algorithm_by_id(id) != NULL;
}

bool using_dynamic_plugins(void) {
    return use_dynamic_plugins;
}

void free_algorithm_registry(void) {
    for (int i = 0; i < dynamic_algorithm_count; i++) {
        if (dynamic_algorithms[i].handle) {
            dlclose(dynamic_algorithms[i].handle);
        }
    }
    dynamic_algorithm_count = 0;
    use_dynamic_plugins = false;
}