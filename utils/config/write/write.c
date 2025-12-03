#include "write.h"

static char* remove_json_comments(const char* input) {
    if (!input) return NULL;
    
    size_t len = strlen(input);
    char* output = malloc(len + 1);
    if (!output) return NULL;
    
    char* out_ptr = output;
    const char* in_ptr = input;
    int in_single_line_comment = 0;
    int in_multi_line_comment = 0;
    int in_string = 0;
    char last_char = 0;
    
    while (*in_ptr) {
        if (!in_single_line_comment && !in_multi_line_comment) {
            if (*in_ptr == '"' && last_char != '\\') {
                in_string = !in_string;
            }
            
            if (!in_string) {
                if (*in_ptr == '/' && *(in_ptr + 1) == '/') {
                    in_single_line_comment = 1;
                    in_ptr += 2;
                    continue;
                }
                
                if (*in_ptr == '/' && *(in_ptr + 1) == '*') {
                    in_multi_line_comment = 1;
                    in_ptr += 2;
                    continue;
                }
            }
        }
        
        if (in_single_line_comment) {
            if (*in_ptr == '\n') {
                in_single_line_comment = 0;
                *out_ptr++ = *in_ptr;
            }
            in_ptr++;
            continue;
        }
        
        if (in_multi_line_comment) {
            if (*in_ptr == '*' && *(in_ptr + 1) == '/') {
                in_multi_line_comment = 0;
                in_ptr += 2;
                continue;
            }
            in_ptr++;
            continue;
        }
        
        *out_ptr++ = *in_ptr++;
        last_char = *(in_ptr - 1);
    }
    
    *out_ptr = '\0';
    return output;
}
bool modify_quantum_val(const char* config_filename, int new_value)
{
    FILE *fp = fopen(config_filename, "r+");

    if (fp == NULL)
    {
        printf("Error: could not open file %s\n", config_filename);
        return false;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *config = (char *)malloc(file_size + 1);
    if (config == NULL)
    {
        printf("Error: memory allocation failed\n");
        fclose(fp);
        return false;
    }

    fread(config, 1, file_size, fp);
    config[file_size] = '\0';

    fclose(fp);

   
    char* clean_json = remove_json_comments(config);
    free(config);  
    
    if (!clean_json) {
        return false;
    }
    
   
    cJSON *config_json = cJSON_Parse(clean_json);
    free(clean_json);  
    
    if (config_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return false;
    }

    const cJSON *options_list = NULL;
    cJSON *quantum_option = NULL;

    options_list = cJSON_GetObjectItemCaseSensitive(config_json, "options");
    if (options_list == NULL) {
        fprintf(stderr, "Error: 'options' object not found\n");
        cJSON_Delete(config_json);
        return false;
    }
    
    quantum_option = cJSON_GetObjectItemCaseSensitive(options_list, "quantum");
    if (quantum_option == NULL) {
        fprintf(stderr, "Error: 'quantum' field not found\n");
        cJSON_Delete(config_json);
        return false;
    }

    cJSON_SetNumberValue(quantum_option, new_value);

    char *string = cJSON_Print(config_json);
    if (string == NULL)
    {
        fprintf(stderr, "Failed to print config_file.\n");
        cJSON_Delete(config_json);
        return false;
    }

    cJSON_Delete(config_json);
    write_to_config(config_filename, string);
    free(string);

    return true;
}

bool modify_ranges(const char* config_filename, char *proc_range, char *exec_range, char *priority_range, char *arrival_range)
{
    FILE *fp = fopen(config_filename, "r+");

    if (fp == NULL)
    {
        printf("Error: could not open file %s\n", config_filename);
        return false;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *config = (char *)malloc(file_size + 1);
    if (config == NULL)
    {
        printf("Error: memory allocation failed\n");
        fclose(fp);
        return false;
    }

    fread(config, 1, file_size, fp);
    config[file_size] = '\0';

    fclose(fp);

    
    char* clean_json = remove_json_comments(config);
    free(config);  
    
    if (!clean_json) {
        return false;
    }
    
    
    cJSON *config_json = cJSON_Parse(clean_json);
    free(clean_json);  
    
    if (config_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return false;
    }

    const cJSON *options_list = NULL;
    cJSON *max_exec_option = NULL;
    cJSON *max_proc_option = NULL;
    cJSON *max_priority_option = NULL;
    cJSON *max_arrival_option = NULL;

    options_list = cJSON_GetObjectItemCaseSensitive(config_json, "options");
    if (options_list == NULL) {
        fprintf(stderr, "Error: 'options' object not found\n");
        cJSON_Delete(config_json);
        return false;
    }
    
    max_exec_option = cJSON_GetObjectItemCaseSensitive(options_list, "max_exec");
    max_proc_option = cJSON_GetObjectItemCaseSensitive(options_list, "max_proc");
    max_priority_option = cJSON_GetObjectItemCaseSensitive(options_list, "max_priority");
    max_arrival_option = cJSON_GetObjectItemCaseSensitive(options_list, "max_arrival");

    if (!max_exec_option || !max_proc_option || !max_priority_option || !max_arrival_option) {
        fprintf(stderr, "Error: one or more range fields not found\n");
        cJSON_Delete(config_json);
        return false;
    }

    cJSON_SetValuestring(max_exec_option, exec_range);
    cJSON_SetValuestring(max_proc_option, proc_range);
    cJSON_SetValuestring(max_priority_option, priority_range);
    cJSON_SetValuestring(max_arrival_option, arrival_range);
    

    char *string = cJSON_Print(config_json);
    if (string == NULL)
    {
        fprintf(stderr, "Failed to print config_file.\n");
        cJSON_Delete(config_json);
        return false;
    }

    cJSON_Delete(config_json);
    write_to_config(config_filename, string);
    free(string);

    return true;
}

void set_ranges(char range[100], int *range_start, int *range_end)
{
    *range_start = -1;
    *range_end = -1;
    char *token = strtok(range, "-");

    while (token != NULL)
    {
        if (*range_start == -1)
        {
            *range_start = atoi(token);
        }
        else if (*range_end == -1)
        {
            *range_end = atoi(token);
        }

        token = strtok(NULL, "-");
    }
}

int create_random_process_array(process processes[100], int max_proc_range_start, int max_proc_range_end, int exec_range_start, int exec_range_end, int priority_range_start, int priority_range_end, int arrival_range_start, int arrival_range_end)
{

    srand(time(NULL));
    int dim = rand() % (max_proc_range_end - max_proc_range_start + 1) + max_proc_range_start;
    for (int i = 0; i < dim; i++)
    {
        processes[i].arrived_at = rand() % (arrival_range_end - arrival_range_start + 1) + arrival_range_start;
        processes[i].execution_time = rand() % (exec_range_end - exec_range_start + 1) + exec_range_start;
        processes[i].priority = rand() % (priority_range_end - priority_range_start + 1) + priority_range_start;
        snprintf(processes[i].name, sizeof(processes[i].name), "p%d", i + 1);
    }

    return dim;
}

void write_to_config(const char* config_filename, const char *content)
{
    
    FILE *fp = fopen(config_filename, "r");
    if (fp == NULL) {
        
        FILE *fptr = fopen(config_filename, "w");
        if (fptr) {
            fprintf(fptr, "%s", content);
            fclose(fptr);
        }
        return;
    }
    
    
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char *original_content = malloc(file_size + 1);
    if (original_content == NULL) {
        fclose(fp);
        return;
    }
    
    fread(original_content, 1, file_size, fp);
    original_content[file_size] = '\0';
    fclose(fp);
    
  
    char *json_start = NULL;
    int in_comment = 0;
    int in_string = 0;
    char last_char = 0;
    
    for (char *ptr = original_content; *ptr; ptr++) {
        if (!in_comment && !in_string) {
            if (*ptr == '"' && last_char != '\\') {
                in_string = 1;
            } else if (*ptr == '/' && *(ptr + 1) == '/') {
                in_comment = 1;  
            } else if (*ptr == '/' && *(ptr + 1) == '*') {
                in_comment = 2;  
            } else if (*ptr == '{') {
                json_start = ptr;
                break;
            }
        } else if (in_comment == 1) {
            if (*ptr == '\n') {
                in_comment = 0;
            }
        } else if (in_comment == 2) {
            if (*ptr == '*' && *(ptr + 1) == '/') {
                in_comment = 0;
                ptr++;  
            }
        } else if (in_string) {
            if (*ptr == '"' && last_char != '\\') {
                in_string = 0;
            }
        }
        last_char = *ptr;
    }
    
   
    FILE *fptr = fopen(config_filename, "w");
    if (fptr) {
        
        if (json_start != NULL) {
            fwrite(original_content, 1, json_start - original_content, fptr);
        }
        
        
        fprintf(fptr, "%s", content);
        
        fclose(fptr);
    }
    
    free(original_content);
}

void generate_config_with_comments(const char* filename, options ops, char *max_proc_range, 
                                  char *exec_range, char *priority_range, char *arrival_range) {
    srand(time(NULL));
    
    process resolution_numbers[100];
    
    int max_proc_range_start, max_proc_range_end;
    int exec_range_start, exec_range_end;
    int priority_range_start, priority_range_end;
    int arrival_range_start, arrival_range_end;
    
    char tempString[100];
    strcpy(tempString, max_proc_range);
    set_ranges(tempString, &max_proc_range_start, &max_proc_range_end);
    
    strcpy(tempString, exec_range);
    set_ranges(tempString, &exec_range_start, &exec_range_end);
    
    strcpy(tempString, priority_range);
    set_ranges(tempString, &priority_range_start, &priority_range_end);
    strcpy(tempString, arrival_range);
    set_ranges(tempString, &arrival_range_start, &arrival_range_end);
    
    int dim = create_random_process_array(resolution_numbers, max_proc_range_start, max_proc_range_end, 
                                         exec_range_start, exec_range_end, 
                                         priority_range_start, priority_range_end, 
                                         arrival_range_start, arrival_range_end);
    
    FILE *f = fopen(filename, "w");
    if (!f) {
        printf("Error opening file %s for writing\n", filename);
        return;
    }
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[100];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
     

   
    fprintf(f, "// ================================================\n");
    fprintf(f, "// CONFIGURATION FILE - PROCESS SCHEDULER\n");
    fprintf(f, "// Format: JSONC (JSON with C-style comments)\n");
    fprintf(f, "// ================================================\n\n");
   
    fprintf(f, "// Global scheduler configuration options\n");
    fprintf(f, "{\n");
    fprintf(f, "  \"options\": {\n");
    fprintf(f, "    // Quantum time for Round Robin algorithm\n");
    fprintf(f, "    // Range: 1 to 10 time units\n");
    fprintf(f, "    \"quantum\": %d,\n\n", ops.quantum);
    
    fprintf(f, "    // Maximum number of processes to generate\n");
    fprintf(f, "    // Format: \"min-max\" (e.g., \"1-10\" for 1 to 10 processes)\n");
    fprintf(f, "    \"max_proc\": \"%s\",\n\n", max_proc_range);
    
    fprintf(f, "    // Execution time (burst time) range\n");
    fprintf(f, "    // Each process gets a random execution time in this range\n");
    fprintf(f, "    \"max_exec\": \"%s\",\n\n", exec_range);
    
    fprintf(f, "    // Priority range (1 = highest priority, 10 = lowest)\n");
    fprintf(f, "    \"max_priority\": \"%s\",\n\n", priority_range);
    
    fprintf(f, "    // Arrival time range\n");
    fprintf(f, "    // When processes arrive in the system\n");
    fprintf(f, "    \"max_arrival\": \"%s\"\n", arrival_range);
    fprintf(f, "  },\n\n");
    
    
    fprintf(f, "  // List of generated processes\n");
    fprintf(f, "  // Each process has 4 attributes:\n");
    fprintf(f, "  // - name: Process identifier (p1, p2, ...)\n");
    fprintf(f, "  // - arrived_at: Time when process arrives\n");
    fprintf(f, "  // - execution_time: CPU burst time required\n");
    fprintf(f, "  // - priority: Priority level (1-10)\n");
    fprintf(f, "  \"process\": [\n");
    
    for (int i = 0; i < dim; i++) {
        fprintf(f, "    {\n");
        fprintf(f, "      // Process %s\n", resolution_numbers[i].name);
        fprintf(f, "      \"name\": \"%s\",\n", resolution_numbers[i].name);
        fprintf(f, "      \"arrived_at\": %d,  // Arrival time\n", resolution_numbers[i].arrived_at);
        fprintf(f, "      \"execution_time\": %d,  // Burst time\n", resolution_numbers[i].execution_time);
        fprintf(f, "      \"priority\": %d  // Priority level\n", resolution_numbers[i].priority);
        
        if (i < dim - 1) {
            fprintf(f, "    },\n\n");
        } else {
            fprintf(f, "    }\n");
        }
    }
    
    fprintf(f, "  ]\n");
    fprintf(f, "}\n\n");
    
    
    fclose(f);
    
}

void generate_config_file(const char* config_filename, options ops, char *max_proc_range, char *exec_range, char *priority_range, char *arrival_range)
{

    generate_config_with_comments(config_filename, ops, max_proc_range, exec_range, priority_range, arrival_range);
} 