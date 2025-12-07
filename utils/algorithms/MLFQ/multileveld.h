#ifndef MULTILEVELD_H
#define MULTILEVELD_H
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "../../queues/priority/priority_queue.h"
#include "../../../include/types.h"
#include "../useful/useful.h"
#include "../../gannt/format.h"
#include "../../../include/algorithms.h"
#include "../../queues/fifo/queuef.h"

ExecutedTask *get_multileveld_output(process *process_array, int process_array_size, int *executed_tasks_size);
ExecutedTask *multileveld_wrapper(process *proc, int proc_count, int *executed_count, options ops);
AlgorithmInfo* get_algorithm_info_plugin(void);

#endif