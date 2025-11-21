#include "write.h" // Inclusion du header correspondant avec les déclarations

// Fonction pour modifier la valeur du quantum dans le fichier de configuration JSON
bool modify_quantum_val(const char* config_filename, int new_value)
{
    FILE *fp = fopen(config_filename, "r+"); // Ouvre le fichier en lecture/écriture
    if (fp == NULL) // Vérifie si le fichier est ouvert correctement
    {
        printf("Error: could not open file %s\n", config_filename);
        return false;
    }

    // Lecture complète du fichier dans un buffer
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

    // Parse le JSON
    cJSON *config_json = cJSON_Parse(config);
    if (config_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        free(config);
        return NULL;
    }

    // Accès à l'objet "options" et modification de "quantum"
    const cJSON *options_list = cJSON_GetObjectItemCaseSensitive(config_json, "options");
    cJSON *quantum_option = cJSON_GetObjectItemCaseSensitive(options_list, "quantum");
    cJSON_SetNumberValue(quantum_option, new_value);

    // Convertit le JSON en chaîne pour l'écriture
    char *string = cJSON_Print(config_json);
    if (string == NULL)
    {
        fprintf(stderr, "Failed to print config_file.\n");
    }

    cJSON_Delete(config_json); // Libère le JSON
    write_to_config(config_filename, string); // Écrit les changements dans le fichier

    return true;
}

// Fonction pour modifier les plages (ranges) dans le fichier de configuration JSON
bool modify_ranges(const char* config_filename, char *proc_range, char *exec_range, char *priority_range, char *arrival_range)
{
    FILE *fp = fopen(config_filename, "r+"); // Ouvre le fichier
    if (fp == NULL)
    {
        printf("Error: could not open file %s\n", config_filename);
        return false;
    }

    // Lecture du fichier
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

    // Parse du JSON
    cJSON *config_json = cJSON_Parse(config);
    if (config_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        free(config);
        return NULL;
    }

    // Accès à l'objet "options" et modification des ranges
    cJSON *options_list = cJSON_GetObjectItemCaseSensitive(config_json, "options");
    cJSON *max_exec_option = cJSON_GetObjectItemCaseSensitive(options_list, "max_exec");
    cJSON *max_proc_option = cJSON_GetObjectItemCaseSensitive(options_list, "max_proc");
    cJSON *max_priority_option = cJSON_GetObjectItemCaseSensitive(options_list, "max_priority");
    cJSON *max_arrival_option = cJSON_GetObjectItemCaseSensitive(options_list, "max_arrival");

    cJSON_SetValuestring(max_exec_option, exec_range);
    cJSON_SetValuestring(max_proc_option, proc_range);
    cJSON_SetValuestring(max_priority_option, priority_range);
    cJSON_SetValuestring(max_arrival_option, arrival_range);

    // Convertit le JSON en chaîne et écrit dans le fichier
    char *string = cJSON_Print(config_json);
    if (string == NULL)
    {
        fprintf(stderr, "Failed to print config_file.\n");
    }

    cJSON_Delete(config_json); // Libère le JSON
    write_to_config(config_filename, string); // Écrit les changements

    return true;
}

// Fonction pour transformer une chaîne "start-end" en deux entiers
void set_ranges(char range[100], int *range_start, int *range_end)
{
    *range_start = -1;
    *range_end = -1;
    char *token = strtok(range, "-"); // Sépare la chaîne avec '-'

    while (token != NULL)
    {
        if (*range_start == -1)
        {
            *range_start = atoi(token); // Premier nombre = début de la plage
        }
        else if (*range_end == -1)
        {
            *range_end = atoi(token); // Second nombre = fin de la plage
        }
        token = strtok(NULL, "-");
    }
}

// Fonction pour générer un tableau aléatoire de processus
int create_random_process_array(process processes[100], int max_proc_range_start, int max_proc_range_end,
                                int exec_range_start, int exec_range_end, int priority_range_start,
                                int priority_range_end, int arrival_range_start, int arrival_range_end)
{
    srand(time(NULL)); // Initialisation du générateur aléatoire
    int dim = rand() % (max_proc_range_end - max_proc_range_start + 1) + max_proc_range_start; // Nombre aléatoire de processus

    for (int i = 0; i < dim; i++)
    {
        processes[i].arrived_at = rand() % (arrival_range_end - arrival_range_start + 1) + arrival_range_start;
        processes[i].execution_time = rand() % (exec_range_end - exec_range_start + 1) + exec_range_start;
        processes[i].priority = rand() % (priority_range_end - priority_range_start + 1) + priority_range_start;
        snprintf(processes[i].name, sizeof(processes[i].name), "p%d", i + 1); // Nom du processus
    }

    return dim; // Retourne le nombre de processus générés
}

// Fonction pour écrire une chaîne dans le fichier de configuration
void write_to_config(const char* config_filename, const char *content)
{
    FILE *fptr = fopen(config_filename, "w");
    fprintf(fptr, "%s", content);
    fclose(fptr);
}

// Fonction pour générer un fichier de configuration complet aléatoire
void *generate_config_file(const char* config_filename, options ops, char *max_proc_range, char *exec_range,
                           char *priority_range, char *arrival_range)
{
    srand(time(NULL));
    process resolution_numbers[100]; // Tableau temporaire de processus

    int max_proc_range_start, max_proc_range_end;
    int exec_range_start, exec_range_end;
    int priority_range_start, priority_range_end;
    int arrival_range_start, arrival_range_end;

    char tempString[100];

    // Transforme les chaînes "start-end" en entiers
    strcpy(tempString, max_proc_range);
    set_ranges(tempString, &max_proc_range_start, &max_proc_range_end);

    strcpy(tempString, exec_range);
    set_ranges(tempString, &exec_range_start, &exec_range_end);

    strcpy(tempString, priority_range);
    set_ranges(tempString, &priority_range_start, &priority_range_end);

    strcpy(tempString, arrival_range);
    set_ranges(tempString, &arrival_range_start, &arrival_range_end);

    // Génère les processus aléatoires
    int dim = create_random_process_array(resolution_numbers, max_proc_range_start, max_proc_range_end,
                                          exec_range_start, exec_range_end, priority_range_start,
                                          priority_range_end, arrival_range_start, arrival_range_end);

    char *string = NULL;

    cJSON *options = NULL;
    cJSON *process = NULL;
    cJSON *nested_process = NULL;

    cJSON *quantum = NULL;

    cJSON *max_exec_option = NULL;
    cJSON *max_proc_option = NULL;
    cJSON *max_priority_option = NULL;
    cJSON *max_arrival_option = NULL;

    // Création de l'objet JSON principal
    cJSON *config_file = cJSON_CreateObject();
    if (config_file == NULL) goto end;

    // Création de l'objet options
    options = cJSON_CreateObject();
    if (options == NULL) goto end;
    cJSON_AddItemToObject(config_file, "options", options);

    // Création du tableau "process"
    process = cJSON_CreateArray();
    if (process == NULL) goto end;
    cJSON_AddItemToObject(config_file, "process", process);

    // Ajoute le quantum
    quantum = cJSON_CreateNumber(ops.quantum);
    if (quantum == NULL) goto end;
    cJSON_AddItemToObject(options, "quantum", quantum);

    // Ajoute les ranges sous forme de chaînes
    max_proc_option = cJSON_CreateString(max_proc_range);
    if (max_proc_option == NULL) goto end;
    cJSON_AddItemToObject(options, "max_proc", max_proc_option);

    max_exec_option = cJSON_CreateString(exec_range);
    if (max_exec_option == NULL) goto end;
    cJSON_AddItemToObject(options, "max_exec", max_exec_option);

    max_priority_option = cJSON_CreateString(priority_range);
    if (max_priority_option == NULL) goto end;
    cJSON_AddItemToObject(options, "max_priority", max_priority_option);

    max_arrival_option = cJSON_CreateString(arrival_range);
    if (max_arrival_option == NULL) goto end;
    cJSON_AddItemToObject(options, "max_arrival", max_arrival_option);

    // Ajoute tous les processus générés dans le tableau JSON
    for (size_t index = 0; index < dim; ++index)
    {
        nested_process = cJSON_CreateObject();
        if (nested_process == NULL) goto end;
        cJSON_AddItemToArray(process, nested_process);

        cJSON_AddItemToObject(nested_process, "arrived_at", cJSON_CreateNumber(resolution_numbers[index].arrived_at));
        cJSON_AddItemToObject(nested_process, "execution_time", cJSON_CreateNumber(resolution_numbers[index].execution_time));
        cJSON_AddItemToObject(nested_process, "priority", cJSON_CreateNumber(resolution_numbers[index].priority));
        cJSON_AddItemToObject(nested_process, "name", cJSON_CreateString(resolution_numbers[index].name));
    }

    // Convertit tout en chaîne JSON
    string = cJSON_Print(config_file);
    if (string == NULL)
    {
        fprintf(stderr, "Failed to print config_file.\n");
    }

end:
    cJSON_Delete(config_file); // Libère la mémoire
    write_to_config(config_filename, string); // Écrit le fichier
}
