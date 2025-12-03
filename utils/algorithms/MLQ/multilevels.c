#include "multilevels.h"

#define MLFQ_QUANTUM 2  

ExecutedTask *get_multilevel_static_output(process *process_array, int process_count, int *tasks_size)
{
    *tasks_size = 0;

    if (process_count == 0)
        return NULL;

    int max_priority = 0;
    int min_priority = 1000;
    for (int i = 0; i < process_count; i++) {
        if (process_array[i].priority > max_priority)
            max_priority = process_array[i].priority;
        if (process_array[i].priority < min_priority)
            min_priority = process_array[i].priority;
    }

    int levels = (max_priority - min_priority) + 1;

    proc_queue **queues = malloc(sizeof(proc_queue *) * levels);
    for (int i = 0; i < levels; i++) {
        queues[i] = malloc(sizeof(proc_queue));
        create_queue(queues[i]);
    }

    sort_process_array_by_at(process_array, process_count);

    ExecutedTask *tasks = malloc(sizeof(ExecutedTask) * 10000);
    int current_time = get_earliest_time(process_array, process_count);

    int *remaining = malloc(sizeof(int) * process_count);
    for (int i = 0; i < process_count; i++)
        remaining[i] = process_array[i].execution_time;

    int *added = calloc(process_count, sizeof(int));

    int finished = 0;
    int iteration = 0;

    while (finished < process_count)
    {
        iteration++;
        if (iteration > 5000) {
            break;
        }

        for (int i = 0; i < process_count; i++) {
            if (!added[i] && process_array[i].arrived_at <= current_time) {
                int q_idx = process_array[i].priority - min_priority;
                add_to_queue(queues[q_idx], process_array[i]);
                added[i] = 1;
            }
        }

        int queue_idx = -1;
        for (int p = 0; p < levels; p++) {
            if (!is_queue_empty(queues[p])) {
                queue_idx = p;
                break;
            }
        }

        if (queue_idx == -1) {
            current_time++;
            continue;
        }

        int will_be_alone = 0;
        proc_queue *temp_queue = malloc(sizeof(proc_queue));
        create_queue(temp_queue);
        int process_count_in_queue = 0;
        
        while (!is_queue_empty(queues[queue_idx])) {
            process temp = remove_from_queue(queues[queue_idx]);
            add_to_queue(temp_queue, temp);
            process_count_in_queue++;
        }
        
        while (!is_queue_empty(temp_queue)) {
            process temp = remove_from_queue(temp_queue);
            add_to_queue(queues[queue_idx], temp);
        }
        free(temp_queue);
        
        will_be_alone = (process_count_in_queue == 1);
        
        process running = remove_from_queue(queues[queue_idx]);

        int idx = -1;
        for (int i = 0; i < process_count; i++) {
            if (strcmp(process_array[i].name, running.name) == 0) {
                idx = i;
                break;
            }
        }

        if (idx == -1) {
            current_time++;
            continue;
        }

        int quantum_left = MLFQ_QUANTUM;
        int slice_start = current_time;
        int was_preempted = 0;
        int process_completed = 0;

        if (will_be_alone) {
            while (remaining[idx] > 0) {
                current_time++;
                remaining[idx]--;

                add_to_executed_tasks(
                    tasks,
                    tasks_size,
                    get_task(slice_start, current_time, running.arrived_at, running.name)
                );
                slice_start = current_time;

                if (remaining[idx] == 0) {
                    finished++;
                    process_completed = 1;
                    break;
                }

                for (int i = 0; i < process_count; i++) {
                    if (!added[i] && process_array[i].arrived_at <= current_time) {
                        int new_q_idx = process_array[i].priority - min_priority;
                        add_to_queue(queues[new_q_idx], process_array[i]);
                        added[i] = 1;
                        
                        if (new_q_idx < queue_idx) {
                            was_preempted = 1;
                            if (remaining[idx] > 0) {
                                add_to_queue(queues[queue_idx], running);
                            }
                            goto NEXT_ITERATION;
                        }
                    }
                }
            }
        } else {
            while (quantum_left > 0 && remaining[idx] > 0) {
                current_time++;
                remaining[idx]--;
                quantum_left--;

                add_to_executed_tasks(
                    tasks,
                    tasks_size,
                    get_task(slice_start, current_time, running.arrived_at, running.name)
                );
                slice_start = current_time;

                if (remaining[idx] == 0) {
                    finished++;
                    process_completed = 1;
                    break;
                }

                for (int i = 0; i < process_count; i++) {
                    if (!added[i] && process_array[i].arrived_at <= current_time) {
                        int new_q_idx = process_array[i].priority - min_priority;
                        add_to_queue(queues[new_q_idx], process_array[i]);
                        added[i] = 1;
                    }
                }

                for (int p = 0; p < queue_idx; p++) {
                    if (!is_queue_empty(queues[p])) {
                        was_preempted = 1;
                        if (remaining[idx] > 0) {
                            add_to_queue(queues[queue_idx], running);
                        }
                        goto NEXT_ITERATION;
                    }
                }
            }
        }

        if (!was_preempted && !process_completed) {
            add_to_queue(queues[queue_idx], running);
        }

NEXT_ITERATION:
        continue;
    }
    
    tasks = format_executed_tasks(tasks, tasks_size, process_array, process_count);

    free(remaining);
    free(added);
    for (int i = 0; i < levels; i++) {
        free(queues[i]);
    }
    free(queues);

    return tasks;
}

ExecutedTask *multilevel_static_wrapper(process *proc, int proc_count, int *executed_count, options ops)
{
    return get_multilevel_static_output(proc, proc_count, executed_count);
}

AlgorithmInfo* get_algorithm_info_plugin(void) {
    static AlgorithmInfo info = {
        .name = "MLQ_STATIC",
        .display_name = "Multilevel Queue scheduling",
        .id = MULTILEVELS,
        .execute = multilevel_static_wrapper,
        .requires_quantum = false
    };
    return &info;
}