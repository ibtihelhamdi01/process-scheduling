#ifndef USEFUL_H
#define USEFUL_H

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include "../../../include/types.h"
#include "../../queues/fifo/queuef.h"
#include "../../gannt/format.h"

bool is_in_old_list(process p, process *old_process_list, int old_process_list_size);
int get_earliest_time(process *process_array, int process_array_size);
void sort_process_array_by_at(process *process_array, int process_array_size);
void free_process_array(process *arr);
int get_queue_size(proc_queue *q);
int find_process_by_name(process *process_array, int process_count, const char *name);
int get_next_arrival_time_in_queue(process *process_array, int process_count, int current_time, int *added);
ExecutedTask* ensure_task_capacity(ExecutedTask *tasks, int *capacity, int current_size);
void enqueue_new_arrivals(proc_queue **queues, process *process_array,int process_count, int current_time,int *added, int *wait_time);
#endif