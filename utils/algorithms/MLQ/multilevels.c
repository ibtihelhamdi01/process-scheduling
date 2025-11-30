#include "multilevels.h"

#define MLFQ_QUANTUM 2  
ExecutedTask *get_multilevel_static_output(process *process_array, int process_count, int *tasks_size)
{
    *tasks_size = 0;

    if (process_count == 0)
        return NULL;

   
    int max_priority = 0;
    for (int i = 0; i < process_count; i++)
        if (process_array[i].priority > max_priority)
            max_priority = process_array[i].priority;

    int levels = max_priority + 1;

    proc_queue **queues = malloc(sizeof(proc_queue *) * levels);
    for (int i = 0; i < levels; i++) {
        queues[i] = malloc(sizeof(proc_queue));
        create_queue(queues[i]);
    }


    sort_process_array_by_at(process_array, process_count);

    ExecutedTask *tasks = malloc(sizeof(ExecutedTask) * 2000);
    int current_time = get_earliest_time(process_array, process_count);

    
    int *remaining = malloc(sizeof(int) * process_count);
    for (int i = 0; i < process_count; i++)
        remaining[i] = process_array[i].execution_time;

    
    int *added = calloc(process_count, sizeof(int));

    int finished = 0;

    
    while (finished < process_count)
    {
        
        for (int i = 0; i < process_count; i++) {
            if (!added[i] && process_array[i].arrived_at <= current_time) {
                add_to_queue(queues[process_array[i].priority], process_array[i]);
                added[i] = 1;
            }
        }

        
        int lvl = -1;
        for (int p = 0; p < levels; p++) {
            if (!is_queue_empty(queues[p])) {
                lvl = p;
                break;
            }
        }

        
        if (lvl == -1) {
            current_time++;
            continue;
        }

        
        process running = remove_from_queue(queues[lvl]);

       
        int idx = -1;
        for (int i = 0; i < process_count; i++)
            if (strcmp(process_array[i].name, running.name) == 0)
                idx = i;

        int quantum_left = MLFQ_QUANTUM;
        int slice_start = current_time;
        int was_preempted = 0;

        
        int is_alone_in_queue = 1;
        if (!is_queue_empty(queues[lvl])) {
            is_alone_in_queue = 0;
        }

       
        if (is_alone_in_queue) {
            
            while (remaining[idx] > 0) {
                
                current_time++;
                remaining[idx]--;

                
                add_to_executed_tasks(
                    tasks,
                    tasks_size,
                    get_task(slice_start, current_time, running.arrived_at, running.name)
                );
                slice_start = current_time;

               
                for (int i = 0; i < process_count; i++) {
                    if (!added[i] && process_array[i].arrived_at <= current_time) {
                        add_to_queue(queues[process_array[i].priority], process_array[i]);
                        added[i] = 1;
                        
                       
                        if (process_array[i].priority < lvl) {
                            was_preempted = 1;
                            if (remaining[idx] > 0) {
                                add_to_queue(queues[lvl], running); 
                            }
                            goto NEXT_ITERATION;
                        }
                    }
                }
            }
            finished++; 
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

               
                for (int i = 0; i < process_count; i++) {
                    if (!added[i] && process_array[i].arrived_at <= current_time) {
                        add_to_queue(queues[process_array[i].priority], process_array[i]);
                        added[i] = 1;
                    }
                }

               
                for (int p = 0; p < lvl; p++) {
                    if (!is_queue_empty(queues[p])) {
                        
                        was_preempted = 1;
                        if (remaining[idx] > 0) {
                            add_to_queue(queues[lvl], running);  
                        }
                        goto NEXT_ITERATION;
                    }
                }
            }

            
            if (!was_preempted) {
                if (remaining[idx] == 0) {
                    finished++; 
                } else {
                    
                    add_to_queue(queues[lvl], running);
                    
                }
            }
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