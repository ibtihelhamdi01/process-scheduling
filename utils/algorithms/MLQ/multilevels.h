#ifndef MULTILEVELS_H
#define MULTILEVELS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../queues/priority/priority_queue.h"
#include "../../../include/types.h"
#include "../useful/useful.h"
#include "../../gannt/format.h"
#include "../../../include/algorithms.h"
#include "../../queues/fifo/queuef.h"

ExecutedTask *get_multilevel_static_output(process *process_array, int process_count, int *tasks_size);
#endif 