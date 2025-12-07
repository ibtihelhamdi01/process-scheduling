#include "useful.h"


bool is_in_old_list(process p, process *old_process_list, int old_process_list_size)
{
    if (old_process_list != NULL)
    {
        for (int j = 0; j < old_process_list_size; j++)
        {
            if (strncmp(p.name, old_process_list[j].name, sizeof(p.name)) == 0)
            {
                return true;
            }
        }
    }
    return false;
}

int get_earliest_time(process *process_array, int process_array_size)
{
    if (process_array_size == 0)
        return -1;
    if (process_array_size == 1)
        return process_array[0].arrived_at;

    int min_value = get_earliest_time(process_array, process_array_size - 1);
    if (min_value > process_array[process_array_size - 1].arrived_at)
    {
        return process_array[process_array_size - 1].arrived_at;
    }
    else
    {
        return min_value;
    }
}

void sort_process_array_by_at(process *process_array, int process_array_size)
{
    for (int i = 0; i < process_array_size - 1; i++)
    {
        for (int j = 0; j < process_array_size - i - 1; j++)
        {
            if (process_array[j].arrived_at > process_array[j + 1].arrived_at)
            {
                process temp = process_array[j];
                process_array[j] = process_array[j + 1];
                process_array[j + 1] = temp;
            }
        }
    }
}

void free_process_array(process *arr)
{
    free(arr);
}


int get_queue_size(proc_queue *q) {
    if (q == NULL || is_queue_empty(q)) {
        return 0;
    }

    int count = 0;
    proc_in_queue *current = q->head;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    return count;
}

int find_process_by_name(process *process_array, int process_count, const char *name) {
    if (process_array == NULL || name == NULL) {
        return -1;
    }

    for (int i = 0; i < process_count; ++i) {
        if (strcmp(process_array[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}


int get_next_arrival_time_in_queue(process *process_array, int process_count,
                                    int current_time, int *added) {
    if (process_array == NULL || added == NULL) {
        return INT_MAX;
    }

    int next_arrival = INT_MAX;
    for (int i = 0; i < process_count; ++i) {
        if (!added[i] && process_array[i].arrived_at > current_time) {
            if (process_array[i].arrived_at < next_arrival) {
                next_arrival = process_array[i].arrived_at;
            }
        }
    }
    return next_arrival;
}

ExecutedTask* ensure_task_capacity(ExecutedTask *tasks, int *capacity, int current_size) {
    if (tasks == NULL || capacity == NULL) {
        return NULL;
    }

    if (current_size >= *capacity) {
        *capacity *= 2;
        ExecutedTask *new_tasks = realloc(tasks, sizeof(ExecutedTask) * (*capacity));
        if (new_tasks == NULL) {
            fprintf(stderr, "ERREUR: échec de réallocation mémoire pour les tâches\n");
            free(tasks);
            exit(1);
        }
        return new_tasks;
    }
    return tasks;
}
