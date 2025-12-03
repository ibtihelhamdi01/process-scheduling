#include "read.h"

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
bool load_settings(const char *filename, char **proc_range, char **exec_range, char **priority_range, char **arrival_range) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error: could not open file %s\n", filename);
        return false;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *config = (char *)malloc(file_size + 1);
    if (config == NULL) {
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
    
    if (config_json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return false;
    }

    const cJSON *options_node = cJSON_GetObjectItemCaseSensitive(config_json, "options");
    
    const cJSON *max_proc_node = cJSON_GetObjectItemCaseSensitive(options_node, "max_proc");
    const cJSON *max_exec_node = cJSON_GetObjectItemCaseSensitive(options_node, "max_exec");
    const cJSON *max_priority_node = cJSON_GetObjectItemCaseSensitive(options_node, "max_priority");
    const cJSON *max_arrival_node = cJSON_GetObjectItemCaseSensitive(options_node, "max_arrival");
    
    if (max_proc_node && max_proc_node->valuestring) {
        *proc_range = strdup(max_proc_node->valuestring);
    }
    if (max_exec_node && max_exec_node->valuestring) {
        *exec_range = strdup(max_exec_node->valuestring);
    }
    if (max_priority_node && max_priority_node->valuestring) {
        *priority_range = strdup(max_priority_node->valuestring);
    }
    if (max_arrival_node && max_arrival_node->valuestring) {
        *arrival_range = strdup(max_arrival_node->valuestring);
    }

    cJSON_Delete(config_json);
    return true;
}

process *read_config_file(const char *filename, int *config_file_size, options *ops) {
    *config_file_size = 0;
    process *process_array = malloc(12 * sizeof(process));
    if (process_array == NULL) {
        printf("Error: memory allocation failed\n");
        return NULL;
    }

    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error: could not open file %s\n", filename);
        free(process_array);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *config = (char *)malloc(file_size + 1);
    if (config == NULL) {
        printf("Error: memory allocation failed\n");
        fclose(fp);
        free(process_array);
        return NULL;
    }

    fread(config, 1, file_size, fp);
    config[file_size] = '\0';
    fclose(fp);
    
    
    char* clean_json = remove_json_comments(config);
    free(config);  
    
    if (!clean_json) {
        free(process_array);
        return NULL;
    }
    
    
    cJSON *config_json = cJSON_Parse(clean_json);
    free(clean_json);  
    
    if (config_json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        free(process_array);
        return NULL;
    }

    const cJSON *nested_process = NULL;
    const cJSON *process_list = NULL;
    const cJSON *options_node = NULL;
    const cJSON *quantum_node = NULL;
    int status = 0;

    options_node = cJSON_GetObjectItemCaseSensitive(config_json, "options");
    if (options_node) {
        quantum_node = cJSON_GetObjectItemCaseSensitive(options_node, "quantum");
        if (quantum_node && cJSON_IsNumber(quantum_node)) {
            ops->quantum = quantum_node->valueint;
        }
    }

    process_list = cJSON_GetObjectItemCaseSensitive(config_json, "process");
    if (process_list && cJSON_IsArray(process_list)) {
        cJSON_ArrayForEach(nested_process, process_list) {
            if (*config_file_size >= 12) break; 
            
            cJSON *arrived_at = cJSON_GetObjectItemCaseSensitive(nested_process, "arrived_at");
            cJSON *execution_time = cJSON_GetObjectItemCaseSensitive(nested_process, "execution_time");
            cJSON *priority = cJSON_GetObjectItemCaseSensitive(nested_process, "priority");
            cJSON *name = cJSON_GetObjectItemCaseSensitive(nested_process, "name");

            if (cJSON_IsNumber(arrived_at) && cJSON_IsNumber(execution_time) && 
                cJSON_IsNumber(priority) && cJSON_IsString(name)) {
                process proc;
                proc.arrived_at = arrived_at->valueint;
                proc.execution_time = execution_time->valueint;
                proc.priority = priority->valueint;
                strncpy(proc.name, name->valuestring, sizeof(proc.name) - 1);
                proc.name[sizeof(proc.name) - 1] = '\0';
                
                process_array[*config_file_size] = proc;
                (*config_file_size)++;
                status = 1;
            }
        }
    }

    cJSON_Delete(config_json);
    
    if (status == 0 && *config_file_size == 0) {
        free(process_array);
        return NULL;
    }
    
    return process_array;
}