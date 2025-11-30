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
ExecutedTask *get_multileveld_output( process *process_array, int process_array_size, int *executed_tasks_size);
#endif