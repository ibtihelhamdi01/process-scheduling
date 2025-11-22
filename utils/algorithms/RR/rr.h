#ifndef ROUND_ROBIN_H
#define ROUND_ROBIN_H
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "../../queues/fifo/queuef.h"
#include "../../../include/types.h"
#include "../useful/useful.h"
#include "../../gannt/format.h"
#include "../../../include/algorithms.h"

ExecutedTask *get_round_robin_output(int quantum, process *process_array, int process_array_size, int *tasks_size);
#endif