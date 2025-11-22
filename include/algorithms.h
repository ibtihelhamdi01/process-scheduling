#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include <stdbool.h>
#include "types.h"




typedef struct {
    const char *name;
    const char *display_name;
    Algorithm id;
    ExecutedTask* (*execute)(process *proc, int proc_count, int *executed_count, options ops);
    bool requires_quantum;
} AlgorithmInfo;


typedef struct {
    char name[50];
    char display_name[100];
    Algorithm id;
    void* handle;  
    ExecutedTask* (*execute)(process *proc, int proc_count, int *executed_count, options ops);
    bool requires_quantum;
} AlgorithmPlugin;


void register_algorithms(void);
int get_algorithm_count(void);
AlgorithmInfo* get_algorithm_info(int index);
AlgorithmInfo* get_algorithm_by_id(Algorithm id);
bool algorithm_exists(Algorithm id);

void scan_algorithms_directory(const char* directory_path);
AlgorithmPlugin* get_algorithm_plugin_by_name(const char* name);
AlgorithmPlugin* get_algorithm_plugin_by_index(int index);
int get_algorithm_plugins_count(void);
void free_algorithm_registry(void);
bool using_dynamic_plugins(void);


AlgorithmInfo* get_algorithm_info_plugin(void);

#endif