#include "multileveld.h"

#define BASE_QUANTUM 2
#define AGING_THRESHOLD 5
#define INITIAL_TASK_CAP 2000
#define MAX_SIMULATION_TIME 100000

static void enqueue_new_arrivals(proc_queue **queues, process *process_array,
                                  int process_count, int current_time,
                                  int *added, int *wait_time) {
    for (int i = 0; i < process_count; ++i) {
        if (!added[i] && process_array[i].arrived_at == current_time) {
            add_to_queue(queues[process_array[i].priority], process_array[i]);
            added[i] = 1;
            wait_time[i] = 0;
        }
    }
}

ExecutedTask *get_multileveld_output(process *process_array, int process_count, int *tasks_size)
{
    *tasks_size = 0;
    if (process_count == 0) return NULL;

    // Déterminer le nombre de niveaux basé sur la priorité maximale
    int max_priority = 0;
    for (int i = 0; i < process_count; ++i)
        if (process_array[i].priority > max_priority)
            max_priority = process_array[i].priority;
    int levels = max_priority + 1;

    // Allouer les files FIFO (une par niveau de priorité)
    proc_queue **queues = malloc(sizeof(proc_queue *) * levels);
    if (queues == NULL) {
        fprintf(stderr, "ERREUR: échec d'allocation mémoire pour les files\n");
        return NULL;
    }

    for (int i = 0; i < levels; ++i) {
        queues[i] = malloc(sizeof(proc_queue));
        if (queues[i] == NULL) {
            fprintf(stderr, "ERREUR: échec d'allocation mémoire pour la file %d\n", i);
            for (int j = 0; j < i; ++j) free(queues[j]);
            free(queues);
            return NULL;
        }
        create_queue(queues[i]);
    }

    sort_process_array_by_at(process_array, process_count);

    ExecutedTask *tasks = malloc(sizeof(ExecutedTask) * INITIAL_TASK_CAP);
    if (tasks == NULL) {
        fprintf(stderr, "ERREUR: échec d'allocation mémoire pour les tâches\n");
        for (int i = 0; i < levels; ++i) free(queues[i]);
        free(queues);
        return NULL;
    }

    int tasks_capacity = INITIAL_TASK_CAP;
    int current_time = get_earliest_time(process_array, process_count);

    int *remaining = malloc(sizeof(int) * process_count);
    int *added = calloc(process_count, sizeof(int));
    int *wait_time = calloc(process_count, sizeof(int));

    if (remaining == NULL || added == NULL || wait_time == NULL) {
        fprintf(stderr, "ERREUR: échec d'allocation mémoire\n");
        free(remaining);
        free(added);
        free(wait_time);
        free(tasks);
        for (int i = 0; i < levels; ++i) free(queues[i]);
        free(queues);
        return NULL;
    }

    for (int i = 0; i < process_count; ++i)
        remaining[i] = process_array[i].execution_time;

    int finished = 0;

    while (finished < process_count) {

        if (current_time > MAX_SIMULATION_TIME) {
            fprintf(stderr, "ERREUR: Boucle infinie détectée (temps > %d)\n", MAX_SIMULATION_TIME);
            break;
        }

        enqueue_new_arrivals(queues, process_array, process_count, current_time, added, wait_time);

        for (int p = 1; p < levels; ++p) {
            int original_size = get_queue_size(queues[p]);  // ✅ De useful.c

            for (int k = 0; k < original_size; ++k) {
                if (is_queue_empty(queues[p])) break;

                process tmp = remove_from_queue(queues[p]);
                int idx_tmp = find_process_by_name(process_array, process_count, tmp.name);  // ✅ De useful.c

                if (idx_tmp >= 0 && wait_time[idx_tmp] >= AGING_THRESHOLD) {
                    // Promotion vers niveau supérieur
                    int target_level = (p > 0) ? (p - 1) : 0;
                    add_to_queue(queues[target_level], tmp);
                    wait_time[idx_tmp] = 0;
                } else {
                    // Pas assez attendu, remettre dans la même file
                    add_to_queue(queues[p], tmp);
                }
            }
        }

        int lvl = -1;
        for (int p = 0; p < levels; ++p) {
            if (!is_queue_empty(queues[p])) {
                lvl = p;
                break;
            }
        }

        if (lvl == -1) {
            int next_arrival = get_next_arrival_time_in_queue(process_array, process_count, current_time, added);  // ✅ De useful.c

            if (next_arrival < INT_MAX) {
                current_time = next_arrival;
            } else {
                current_time++;
            }
            continue;
        }

        process running = remove_from_queue(queues[lvl]);
        int idx = find_process_by_name(process_array, process_count, running.name);  // ✅ De useful.c
        if (idx < 0) continue;

        int quantum = BASE_QUANTUM << lvl;  // 2, 4, 8, 16...
        int quantum_left = quantum;
        int slice_start = current_time;

        int preempted = 0;

        while (quantum_left > 0 && remaining[idx] > 0 && !preempted) {
            current_time++;
            remaining[idx]--;
            quantum_left--;

            for (int j = 0; j < process_count; ++j) {
                if (j != idx && added[j] && remaining[j] > 0) {
                    wait_time[j]++;
                }
            }

            enqueue_new_arrivals(queues, process_array, process_count, current_time, added, wait_time);

            for (int p = 0; p < lvl; ++p) {
                if (!is_queue_empty(queues[p])) {
                    preempted = 1;
                    break;
                }
            }
        }

        tasks = ensure_task_capacity(tasks, &tasks_capacity, *tasks_size);

        add_to_executed_tasks(tasks, tasks_size, get_task(slice_start, current_time, running.arrived_at, running.name));

        if (remaining[idx] == 0) {
            finished++;
            wait_time[idx] = 0;
        } else if (preempted) {
            add_to_queue(queues[lvl], running);
            wait_time[idx] = 0;
        } else if (quantum_left == 0) {
            int target_level = (lvl + 1 < levels) ? (lvl + 1) : lvl;
            add_to_queue(queues[target_level], running);
            wait_time[idx] = 0;
        } else {
            add_to_queue(queues[lvl], running);
            wait_time[idx] = 0;
        }
    }

    tasks = format_executed_tasks(tasks, tasks_size, process_array, process_count);

    free(remaining);
    free(added);
    free(wait_time);

    for (int i = 0; i < levels; ++i) {
        while (!is_queue_empty(queues[i])) {
            remove_from_queue(queues[i]);
        }
        free(queues[i]);
    }
    free(queues);

    return tasks;
}

ExecutedTask *multileveld_wrapper(process *proc, int proc_count, int *executed_count, options ops)
{
    return get_multileveld_output(proc, proc_count, executed_count);
}

AlgorithmInfo* get_algorithm_info_plugin(void) {
    static AlgorithmInfo info = {
        .name = "MLFQ-DYN",
        .display_name = "Multi-Level Feedback Queue (Dynamic priority)",
        .id = MULTILEVELD,
        .execute = multileveld_wrapper,
        .requires_quantum = false
    };
    return &info;
}