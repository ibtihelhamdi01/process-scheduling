#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <cjson/cJSON.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <stddef.h>
#include <math.h>
#include <dlfcn.h>
#include "./include/types.h"
#include "./include/algorithms.h"
#include "./utils/algorithms/useful/useful.h"
#include "./utils/queues/fifo/queuef.h"
#include "./utils/queues/priority/priority_queue.h"
#include "./utils/gannt/format.h"
#include "./utils/metrics/metrics.h"
#include "./utils/config/write/write.h"
#include "./utils/config/read/read.h"

void load_algorithm(Algorithm algo_id);
void show_message_box_(const gchar *message);
void close_settings_window(void);
void show_about_dialog(void);

const char *config_filename = "generated_config.cjson";
int config_file_size = 0;
int executed_tasks_size = 0;
ExecutedTask tasks[100];
process *proc_head = NULL;
GtkWidget *window, *drawing_area, *vbox, *dialog, *metrics_window, *metrics_table, *open_metrics, *settings_window, *view_settings, *max_exec_input, *max_proc_input, *max_arrival_input, *max_priority_input;
char *exec_range = "1-2";

char *max_proc_range = "1-10";
char *priority_range = "1-10";
char *arrival_range = "1-10";
cairo_surface_t *global_surface = NULL;
options ops;
bool is_metrics_open = false;

Algorithm current_algorithm = FIFO;

GtkWidget *header_box, *algo_btn_box, *control_btn_box;
GtkWidget **algo_buttons = NULL;
GtkWidget *status_bar;
GtkCssProvider *css_provider;

int algo_count = 0;

char *concat(const char *s1, const char *s2) {
    char *result = malloc(strlen(s1) + strlen(s2) + 1);
    if (result == NULL) {
        printf("Error allocating memory");
        exit(-1);
    }
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

void on_generate_config_clicked(GtkWidget *button, gpointer user_data) {
    generate_config_file(config_filename, ops, max_proc_range, exec_range, priority_range, arrival_range);
    
    if (proc_head != NULL) {
        free(proc_head);
        proc_head = NULL;
    }
    
    proc_head = read_config_file(config_filename, &config_file_size, &ops);
    
    if (proc_head != NULL) {
        AlgorithmInfo *algo_info = get_algorithm_by_id(current_algorithm);
        if (algo_info != NULL) {
            load_algorithm(current_algorithm);
            show_message_box_("Config file generated and loaded successfully!");
        } else {
            if (algo_count > 0) {
                AlgorithmInfo *first_algo = get_algorithm_info(0);
                current_algorithm = first_algo->id;
                load_algorithm(current_algorithm);
                show_message_box_("Config generated! Algorithm reset to first available.");
            }
        }
    } else {
        show_message_box_("Error generating config file!");
    }
}

void save_to_png(GtkWidget *drawing_area, gpointer user_data) {
    gint width, height;
    gtk_window_get_size(GTK_WINDOW(window), &width, &height);
    GdkPixbuf *pixbuf = gdk_pixbuf_get_from_window(
        gtk_widget_get_window(drawing_area),
        0, 0,
        width, height);

    if (pixbuf) {
        gdk_pixbuf_save(pixbuf, "output.png", "png", NULL, NULL);
        g_object_unref(pixbuf);
        show_message_box_("Gantt chart exported as output.png");
    } else {
        show_message_box_("Error exporting chart!");
    }
}

void load_algorithm(Algorithm algo_id) {
    AlgorithmInfo *algo_info = get_algorithm_by_id(algo_id);
    if (algo_info == NULL) {
        printf("Algorithm %d not found in registry!\n", algo_id);
        
        if (algo_count > 0) {
            AlgorithmInfo *first_algo = get_algorithm_info(0);
            current_algorithm = first_algo->id;
            algo_info = first_algo;
            printf("Falling back to %s\n", first_algo->display_name);
        } else {
            show_message_box_("No algorithms available!");
            return;
        }
    }

    ExecutedTask *task = NULL;
    executed_tasks_size = 0;

    task = algo_info->execute(proc_head, config_file_size, &executed_tasks_size, ops);
    current_algorithm = algo_id;

    if (task != NULL) {
        for (int i = 0; i < executed_tasks_size; i++) {
            tasks[i] = task[i];
        }
        free(task);
    }

    char window_title[100];
    snprintf(window_title, sizeof(window_title), "Process Scheduler (%s)", algo_info->display_name);
    gtk_window_set_title(GTK_WINDOW(window), window_title);

    char status_text[100];
    snprintf(status_text, sizeof(status_text), 
             "Algorithm: %s | Processes: %d | Quantum: %d", 
             algo_info->display_name, config_file_size, ops.quantum);
    gtk_statusbar_push(GTK_STATUSBAR(status_bar), 0, status_text);

    gtk_widget_queue_draw(drawing_area);
}

GtkWidget *draw_metrics_table() {
    GtkWidget *table = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(table), 60);

    GtkWidget *label1 = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label1), "<b> Process Name</b>");
    gtk_widget_set_halign(label1, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(label1, GTK_ALIGN_FILL);
    gtk_grid_attach(GTK_GRID(table), label1, 0, 0, 1, 1);

    GtkWidget *label2 = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label2), "<b> Waiting Time</b>");
    gtk_widget_set_halign(label2, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(label2, GTK_ALIGN_FILL);
    gtk_grid_attach(GTK_GRID(table), label2, 1, 0, 1, 1);

    GtkWidget *label3 = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label3), "<b> Rotation Time </b>");
    gtk_widget_set_halign(label3, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(label3, GTK_ALIGN_FILL);
    gtk_grid_attach(GTK_GRID(table), label3, 2, 0, 1, 1);

    int row = 1;
    int total_rotation_time = 0;
    int total_waiting_time = 0;
    
    for (int i = 0; i < config_file_size; i++) {
        GtkWidget *label4 = gtk_label_new(proc_head[i].name);
        gtk_widget_set_halign(label4, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(label4, GTK_ALIGN_FILL);
        gtk_grid_attach(GTK_GRID(table), label4, 0, row, 1, 1);

        char waiting_string[20];
        int waiting_time = get_waiting_time(proc_head[i].name, tasks, executed_tasks_size, proc_head, config_file_size);
        total_waiting_time += waiting_time;
        sprintf(waiting_string, "%d", waiting_time);
        GtkWidget *label5 = gtk_label_new(waiting_string);
        gtk_widget_set_halign(label5, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(label5, GTK_ALIGN_FILL);
        gtk_grid_attach(GTK_GRID(table), label5, 1, row, 1, 1);

        char rotation_string[20];
        int rotation_time = get_rotation_time(proc_head[i].name, tasks, executed_tasks_size);
        total_rotation_time += rotation_time;
        sprintf(rotation_string, "%d", rotation_time);
        GtkWidget *label6 = gtk_label_new(rotation_string);
        gtk_widget_set_halign(label6, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(label6, GTK_ALIGN_FILL);
        gtk_grid_attach(GTK_GRID(table), label6, 2, row, 1, 1);

        row++;
    }
    
    gtk_window_set_default_size(GTK_WINDOW(metrics_window), 400, 10 * row);
    GtkWidget *total_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(total_label), "<b> Average: </b>");
    gtk_widget_set_halign(total_label, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(total_label, GTK_ALIGN_FILL);
    gtk_grid_attach(GTK_GRID(table), total_label, 0, row, 1, 1);

    char total_waiting_string[20];
    sprintf(total_waiting_string, "%.2f units", (float)total_waiting_time / (config_file_size));
    GtkWidget *total_waiting_label = gtk_label_new(total_waiting_string);
    gtk_widget_set_halign(total_waiting_label, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(total_waiting_label, GTK_ALIGN_FILL);
    gtk_grid_attach(GTK_GRID(table), total_waiting_label, 1, row, 1, 1);

    char total_rotation_string[20];
    sprintf(total_rotation_string, "%.2f units", (float)total_rotation_time / (config_file_size));
    GtkWidget *total_rotation_label = gtk_label_new(total_rotation_string);
    gtk_widget_set_halign(total_rotation_label, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(total_rotation_label, GTK_ALIGN_FILL);
    gtk_grid_attach(GTK_GRID(table), total_rotation_label, 2, row, 1, 1);

    return table;
}

gboolean on_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
    switch (GPOINTER_TO_INT(user_data)) {
    case SETTINGS_WINDOW:
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(view_settings), FALSE);
        break;
    case METRICS_WINDOW:
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(open_metrics), FALSE);
        break;
    case MAIN_WINDOW:
        show_about_dialog();
        break;
    }
    return FALSE;
}

bool match_regex(const char *str) {
    regex_t re;
    regmatch_t match[3];
    int ret;

    if (regcomp(&re, "([0-9]+)-([0-9]+)", REG_EXTENDED) != 0)
        return false;

    ret = regexec(&re, str, sizeof(match) / sizeof(match[0]), match, 0);
    regfree(&re);

    if (ret == 0) {
        char group1Str[64], group2Str[64];
        snprintf(group1Str, sizeof(group1Str), "%.*s", (int)(match[1].rm_eo - match[1].rm_so), str + match[1].rm_so);
        snprintf(group2Str, sizeof(group2Str), "%.*s", (int)(match[2].rm_eo - match[2].rm_so), str + match[2].rm_so);
        int start = strtol(group1Str, NULL, 10);
        int end = strtol(group2Str, NULL, 10);
        if (start < end)
            return true;
    }
    return false;
}

bool save_settings(GtkWidget *btn, gpointer user_data) {
    const char *max_exec_input_txt = gtk_entry_get_text(GTK_ENTRY(max_exec_input));
    const char *max_proc_input_txt = gtk_entry_get_text(GTK_ENTRY(max_proc_input));
    const char *max_priority_input_txt = gtk_entry_get_text(GTK_ENTRY(max_priority_input));
    const char *max_arrival_input_txt = gtk_entry_get_text(GTK_ENTRY(max_arrival_input));

    if (match_regex(max_exec_input_txt) && match_regex(max_proc_input_txt) && 
        match_regex(max_priority_input_txt) && match_regex(max_arrival_input_txt)) {
        
       
        char *new_exec_range = strdup(max_exec_input_txt);
        char *new_proc_range = strdup(max_proc_input_txt);
        char *new_priority_range = strdup(max_priority_input_txt);
        char *new_arrival_range = strdup(max_arrival_input_txt);
        
        if (!new_exec_range || !new_proc_range || !new_priority_range || !new_arrival_range) {
            show_message_box_("Memory allocation failed!");
            return FALSE;
        }
        
        
        free(exec_range); exec_range = new_exec_range;
        free(max_proc_range); max_proc_range = new_proc_range;
        free(priority_range); priority_range = new_priority_range;
        free(arrival_range); arrival_range = new_arrival_range;
        
        if (!modify_ranges(config_filename, max_proc_range, exec_range, priority_range, arrival_range)) {
            show_message_box_("Failed to modify ranges in config file!");
            return FALSE;
        }
        
        
        char *old_exec = exec_range;
        char *old_proc = max_proc_range;
        char *old_priority = priority_range;
        char *old_arrival = arrival_range;
        
        if (!load_settings(config_filename, &max_proc_range, &exec_range, &priority_range, &arrival_range)) {
            show_message_box_("Failed to reload settings!");
            return FALSE;
        }
        
        
        if (old_exec != exec_range && old_exec != NULL) free(old_exec);
        if (old_proc != max_proc_range && old_proc != NULL) free(old_proc);
        if (old_priority != priority_range && old_priority != NULL) free(old_priority);
        if (old_arrival != arrival_range && old_arrival != NULL) free(old_arrival);
        
        generate_config_file(config_filename, ops, max_proc_range, exec_range, priority_range, arrival_range);
        
        if (proc_head != NULL) {
            free(proc_head);
            proc_head = NULL;
        }
        
        proc_head = read_config_file(config_filename, &config_file_size, &ops);
        
        if (proc_head != NULL) {
            load_algorithm(current_algorithm);
            gtk_widget_queue_draw(drawing_area);
            show_message_box_("Settings saved successfully!");
            close_settings_window();
            return TRUE;
        } else {
            show_message_box_("There has been an issue generating file.");
            return FALSE;
        }
    } else {
        show_message_box_("Invalid input. Please follow start-end (start < end) format.");
    }
    return FALSE;
}

void on_slider_value_changed(GtkWidget *slider, gpointer user_data) {
    GtkLabel *label = GTK_LABEL(user_data);
    int value = gtk_range_get_value(GTK_RANGE(slider));
    gchar *label_text = g_strdup_printf("Current Quantum: %d", value);
    modify_quantum_val(config_filename, value);

    if (proc_head != NULL) {
        free(proc_head);
        proc_head = NULL;
    }
    
    proc_head = read_config_file(config_filename, &config_file_size, &ops);
    
    AlgorithmInfo *algo_info = get_algorithm_by_id(current_algorithm);
    if (algo_info != NULL && algo_info->requires_quantum) {
        load_algorithm(current_algorithm);
    }
        
    gtk_label_set_text(label, label_text);
    g_free(label_text);
}

void show_settings_window() {
    if (settings_window != NULL && GTK_IS_WINDOW(settings_window)) {
        gtk_window_present(GTK_WINDOW(settings_window));
        return;
    }

    settings_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(settings_window), "Settings");
    gtk_window_set_default_size(GTK_WINDOW(settings_window), 450, 500);
    gtk_window_set_resizable(GTK_WINDOW(settings_window), FALSE);
    gtk_window_set_transient_for(GTK_WINDOW(settings_window), GTK_WINDOW(window));
    
    g_signal_connect(settings_window, "destroy", G_CALLBACK(gtk_widget_destroyed), &settings_window);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(main_box), 15);
    gtk_container_add(GTK_CONTAINER(settings_window), main_box);

    GtkWidget *header_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(header_label), "<span size='x-large' weight='bold'>⚙️ Settings</span>");
    gtk_box_pack_start(GTK_BOX(main_box), header_label, FALSE, FALSE, 0);

    GtkWidget *rr_frame = gtk_frame_new("Round Robin Quantum");
    gtk_box_pack_start(GTK_BOX(main_box), rr_frame, FALSE, FALSE, 10);
    
    GtkWidget *rr_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(rr_box), 10);
    gtk_container_add(GTK_CONTAINER(rr_frame), rr_box);

    char *quantum_label_content = g_strdup_printf("Current Quantum: %d", ops.quantum);
    GtkWidget *quantum_label = gtk_label_new(quantum_label_content);
    g_free(quantum_label_content);
    gtk_box_pack_start(GTK_BOX(rr_box), quantum_label, FALSE, FALSE, 0);

    GtkWidget *slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 1, 10, 1);
    gtk_range_set_value(GTK_RANGE(slider), ops.quantum);
    gtk_box_pack_start(GTK_BOX(rr_box), slider, FALSE, FALSE, 0);
    g_signal_connect(slider, "value-changed", G_CALLBACK(on_slider_value_changed), quantum_label);

    GtkWidget *gen_frame = gtk_frame_new("Process Generation Settings");
    gtk_box_pack_start(GTK_BOX(main_box), gen_frame, TRUE, TRUE, 10);
    
    GtkWidget *gen_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(gen_box), 10);
    gtk_container_add(GTK_CONTAINER(gen_frame), gen_box);

    GtkWidget *proc_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *proc_label = gtk_label_new("Max processes:");
    gtk_widget_set_size_request(proc_label, 120, -1);
    max_proc_input = gtk_entry_new();
    if (max_proc_range != NULL) {
        gtk_entry_set_text(GTK_ENTRY(max_proc_input), max_proc_range);
    }
    gtk_entry_set_placeholder_text(GTK_ENTRY(max_proc_input), "Ex: 2-10");
    gtk_box_pack_start(GTK_BOX(proc_box), proc_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(proc_box), max_proc_input, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(gen_box), proc_box, FALSE, FALSE, 5);

    GtkWidget *exec_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *exec_label = gtk_label_new("Burst time limit:");
    gtk_widget_set_size_request(exec_label, 120, -1);
    max_exec_input = gtk_entry_new();
    if (exec_range != NULL) {
        gtk_entry_set_text(GTK_ENTRY(max_exec_input), exec_range);
    }
    gtk_entry_set_placeholder_text(GTK_ENTRY(max_exec_input), "Ex: 3-10");
    gtk_box_pack_start(GTK_BOX(exec_box), exec_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(exec_box), max_exec_input, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(gen_box), exec_box, FALSE, FALSE, 5);

    GtkWidget *priority_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *priority_label = gtk_label_new("Priority limit:");
    gtk_widget_set_size_request(priority_label, 120, -1);
    max_priority_input = gtk_entry_new();
    if (priority_range != NULL) {
        gtk_entry_set_text(GTK_ENTRY(max_priority_input), priority_range);
    }
    gtk_entry_set_placeholder_text(GTK_ENTRY(max_priority_input), "Ex: 3-10");
    gtk_box_pack_start(GTK_BOX(priority_box), priority_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(priority_box), max_priority_input, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(gen_box), priority_box, FALSE, FALSE, 5);

    GtkWidget *arrival_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *arrival_label = gtk_label_new("Arrival limit:");
    gtk_widget_set_size_request(arrival_label, 120, -1);
    max_arrival_input = gtk_entry_new();
    if (arrival_range != NULL) {
        gtk_entry_set_text(GTK_ENTRY(max_arrival_input), arrival_range);
    }
    gtk_entry_set_placeholder_text(GTK_ENTRY(max_arrival_input), "Ex: 3-10");
    gtk_box_pack_start(GTK_BOX(arrival_box), arrival_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(arrival_box), max_arrival_input, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(gen_box), arrival_box, FALSE, FALSE, 5);

    GtkWidget *save_btn = gtk_button_new_with_label("💾 Save & Generate");
    g_signal_connect(save_btn, "clicked", G_CALLBACK(save_settings), NULL);
    gtk_box_pack_start(GTK_BOX(main_box), save_btn, FALSE, FALSE, 10);

    gtk_widget_show_all(settings_window);
}

void show_metrics_window() {
    if (metrics_window != NULL && GTK_IS_WINDOW(metrics_window)) {
        gtk_window_present(GTK_WINDOW(metrics_window));
        return;
    }

    metrics_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(metrics_window), "Performance Metrics");
    gtk_window_set_default_size(GTK_WINDOW(metrics_window), 500, 400);
    gtk_window_set_resizable(GTK_WINDOW(metrics_window), FALSE);
    gtk_window_set_transient_for(GTK_WINDOW(metrics_window), GTK_WINDOW(window));
    
    g_signal_connect(metrics_window, "destroy", G_CALLBACK(gtk_widget_destroyed), &metrics_window);
    
    metrics_table = draw_metrics_table();
    gtk_container_add(GTK_CONTAINER(metrics_window), metrics_table);
    gtk_widget_show_all(metrics_window);
    is_metrics_open = true;
}

void close_settings_window() {
    gtk_window_close(GTK_WINDOW(settings_window));
}

void close_metrics_window() {
    gtk_window_close(GTK_WINDOW(metrics_window));
}

void update_metrics_window() {
    if (metrics_window == NULL || !GTK_IS_WINDOW(metrics_window))
        return;

    GtkWidget *child = gtk_bin_get_child(GTK_BIN(metrics_window));
    if (child != NULL) {
        gtk_container_remove(GTK_CONTAINER(metrics_window), child);
    }

    metrics_table = draw_metrics_table();
    gtk_container_add(GTK_CONTAINER(metrics_window), metrics_table);
    gtk_widget_show_all(metrics_window);
}

void on_algo_button_clicked(GtkWidget *button, gpointer user_data) {
    Algorithm selected_algo = GPOINTER_TO_INT(user_data);
    
    if (!algorithm_exists(selected_algo)) {
        printf("Algorithm %d is not available\n", selected_algo);
        return;
    }
    
    for (int i = 0; i < algo_count; i++) {
        gtk_style_context_remove_class(gtk_widget_get_style_context(algo_buttons[i]), "active-algo");
    }
    
    gtk_style_context_add_class(gtk_widget_get_style_context(button), "active-algo");
    
    load_algorithm(selected_algo);
    update_metrics_window();
}

static gboolean on_draw_event_modern(GtkWidget *widget, cairo_t *cr, gpointer data) {
    gint width = gtk_widget_get_allocated_width(widget);
    gint height = gtk_widget_get_allocated_height(widget);
    
    cairo_set_source_rgb(cr, 0.95, 0.95, 0.95);
    cairo_paint(cr);
    
    if (executed_tasks_size == 0) return FALSE;
    
    double bar_height = height / (double)(executed_tasks_size + 1);
    double bar_width = width / (double)tasks[executed_tasks_size - 1].finish;
    
    cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
    cairo_set_line_width(cr, 2.0);
    cairo_move_to(cr, 0, height - bar_height);
    cairo_line_to(cr, width, height - bar_height);
    cairo_stroke(cr);

    for (int i = 0; i < executed_tasks_size; i++) {
        double x = tasks[i].start * bar_width;
        double y = i * bar_height;
        double task_width = (tasks[i].finish - tasks[i].start) * bar_width;

        cairo_pattern_t *pat = cairo_pattern_create_linear(x, y, x + task_width, y + bar_height);
        cairo_pattern_add_color_stop_rgba(pat, 0, tasks[i].color[0], tasks[i].color[1], tasks[i].color[2], 0.9);
        cairo_pattern_add_color_stop_rgba(pat, 1, tasks[i].color[0], tasks[i].color[1], tasks[i].color[2], 0.7);
        
        cairo_set_source(cr, pat);
        cairo_rectangle(cr, x, y, task_width, bar_height);
        cairo_fill_preserve(cr);
        cairo_pattern_destroy(pat);

        cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);

        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 10.0);

        cairo_text_extents_t extents;
        cairo_text_extents(cr, tasks[i].label, &extents);

        double text_x = x + (task_width - extents.width) / 2;
        double text_y = y + (bar_height + extents.height) / 2;

        cairo_move_to(cr, text_x, text_y);
        cairo_show_text(cr, tasks[i].label);

        cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
        cairo_set_font_size(cr, 8.0);
        
        char start_time_str[10];
        snprintf(start_time_str, sizeof(start_time_str), "%d", tasks[i].start);
        cairo_move_to(cr, tasks[i].start * bar_width, height - bar_height / 2);
        cairo_show_text(cr, start_time_str);
        
        char end_time_str[10];
        snprintf(end_time_str, sizeof(end_time_str), "%d", tasks[i].finish);
        cairo_move_to(cr, tasks[i].finish * bar_width, height - bar_height / 2);
        cairo_show_text(cr, end_time_str);
    }

    return FALSE;
}

void show_message_box_(const gchar *message) {
    GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                               flags,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_OK,
                                               "%s", message);

    g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_widget_destroy), dialog);
    gtk_widget_show(dialog);
}

void apply_css_styles() {
    css_provider = gtk_css_provider_new();
    const char *css = 
        "window {"
        "    background: #2c5177ff;"
        "}"
        ""
        ".header-box {"
        "    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);"
        "    color: blue;"
        "    padding: 15px;"
        "    border-radius: 8px;"
        "    margin: 10px;"
        "}"
        ""
        ".algo-button {"
        "    background: #6c757d;"
        "    color: red;"
        "    border: none;"
        "    border-radius: 6px;"
        "    padding: 10px 15px;"
        "    margin: 5px;"
        "    font-weight: bold;"
        "}"
        ""
        ".algo-button:hover {"
        "    background: #5a6268;"
        "}"
        ""
        ".active-algo {"
        "    background: #007bff;"
        "    box-shadow: 0 4px 8px rgba(0,123,255,0.3);"
        "}"
        ""
        ".control-button {"
        "    background: #28a745;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 6px;"
        "    padding: 10px 20px;"
        "    margin: 5px;"
        "    font-weight: bold;"
        "}"
        ""
        ".control-button:hover {"
        "    background: #44ac5bff;"
        "}"
        ""
        "frame {"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 8px;"
        "}"
        ""
        "frame > label {"
        "    color: #495057;"
        "    font-weight: bold;"
        "}";

    gtk_css_provider_load_from_data(css_provider, css, -1, NULL);
    GdkScreen *screen = gdk_screen_get_default();
    gtk_style_context_add_provider_for_screen(screen,
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

GtkWidget* create_modern_header() {
    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_name(header, "header-box");
    
    GtkWidget *title_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title_label), 
        "<span size='xx-large' weight='bold'>Process Scheduler</span>\n"
        "<span size='small'>Visualize and compare CPU scheduling algorithms</span>");
    gtk_label_set_justify(GTK_LABEL(title_label), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(header), title_label, TRUE, TRUE, 0);
    
    return header;
}

GtkWidget* create_algo_panel() {
    GtkWidget *frame = gtk_frame_new("Scheduling Algorithms");
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(box), 10);
    gtk_container_add(GTK_CONTAINER(frame), box);
    
    if (algo_buttons != NULL) {
        free(algo_buttons);
        algo_buttons = NULL;
    }
    
    algo_count = get_algorithm_count();
    algo_buttons = malloc(sizeof(GtkWidget*) * algo_count);
    
    printf("Found %d algorithms in registry\n", algo_count);
    
    for (int i = 0; i < algo_count; i++) {
        AlgorithmInfo *info = get_algorithm_info(i);
        if (info != NULL) {
            char button_label[100];
            snprintf(button_label, sizeof(button_label), " %s", info->display_name);
            
            algo_buttons[i] = gtk_button_new_with_label(button_label);
            gtk_widget_set_name(algo_buttons[i], "algo-button");
            g_signal_connect(algo_buttons[i], "clicked", G_CALLBACK(on_algo_button_clicked), 
                            GINT_TO_POINTER(info->id));
            gtk_box_pack_start(GTK_BOX(box), algo_buttons[i], FALSE, FALSE, 0);
            
            printf("UI: Registered algorithm: %s (%s)\n", info->name, info->display_name);
        }
    }
    
    if (algo_count > 0) {
        gtk_style_context_add_class(gtk_widget_get_style_context(algo_buttons[0]), "active-algo");
        current_algorithm = get_algorithm_info(0)->id;
    } else {
        printf("Warning: No algorithms available!\n");
    }
    
    return frame;
}

GtkWidget* create_control_panel() {
    GtkWidget *frame = gtk_frame_new("Controls");
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(box), 10);
    gtk_container_add(GTK_CONTAINER(frame), box);
    
    GtkWidget *gen_btn = gtk_button_new_with_label("🔄 Generate Config");
    GtkWidget *metrics_btn = gtk_button_new_with_label("📊 Show Metrics");
    GtkWidget *settings_btn = gtk_button_new_with_label("⚙️ Settings");
    GtkWidget *export_btn = gtk_button_new_with_label("💾 Export PNG");
    
    gtk_widget_set_name(gen_btn, "control-button");
    gtk_widget_set_name(metrics_btn, "control-button");
    gtk_widget_set_name(settings_btn, "control-button");
    gtk_widget_set_name(export_btn, "control-button");
    
    g_signal_connect(gen_btn, "clicked", G_CALLBACK(on_generate_config_clicked), NULL);
    g_signal_connect_swapped(metrics_btn, "clicked", G_CALLBACK(show_metrics_window), NULL);
    g_signal_connect_swapped(settings_btn, "clicked", G_CALLBACK(show_settings_window), NULL);
    g_signal_connect(export_btn, "clicked", G_CALLBACK(save_to_png), drawing_area);
    
    gtk_box_pack_start(GTK_BOX(box), gen_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), metrics_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), settings_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), export_btn, FALSE, FALSE, 0);
    
    return frame;
}

void show_about_dialog() {
    GtkWidget *dialog = gtk_about_dialog_new();
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), "Process Scheduler");
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), "2.0");
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), 
        "A modern visual tool for simulating and comparing CPU scheduling algorithms");

    const gchar *authors[] = {
    "Saoussen ben farhat", 
    "Ibtihel hamdi",
    "Oussema ayari",
    "nada mechergui", 
    "khalil laouini",
    NULL  
    };
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog), authors);
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), "https://github.com/ibtihelhamdi01/process-scheduling");

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    exec_range = strdup("1-2");
    max_proc_range = strdup("1-10");
    priority_range = strdup("1-10");
    arrival_range = strdup("1-10");
    
    if (!exec_range || !max_proc_range || !priority_range || !arrival_range) {
        printf("Memory allocation failed\n");
        exit(1);
    }

    if (argc < 2) {
        printf("Usage: %s [config_file|G]\n", argv[0]);
        printf("  G - Generate new config file with comments (.cjson format) and EXIT\n");
        printf("  filename.cjson - Load existing config file and open GUI\n");
        printf("\nExamples:\n");
        printf("  %s G                    # Generate config and exit\n", argv[0]);
        printf("  %s generated_config.cjson # Load config and open GUI\n", argv[0]);
        
        free(exec_range);
        free(max_proc_range);
        free(priority_range);
        free(arrival_range);
        exit(0);
    }

    ops.quantum = 3;

    if (strcmp(argv[1], "G") == 0 || strcmp(argv[1], "g") == 0) {
      
        const char* filename = "generated_config.cjson";
        generate_config_with_comments(filename, ops, max_proc_range, exec_range, priority_range, arrival_range);
        
       
        free(exec_range);
        free(max_proc_range);
        free(priority_range);
        free(arrival_range);
        exit(0);
    }
    
   
    config_filename = argv[1];
    proc_head = read_config_file(config_filename, &config_file_size, &ops);
    if (proc_head == NULL) {
        printf("Failed to parse config file: %s\n", config_filename);
        free(exec_range);
        free(max_proc_range);
        free(priority_range);
        free(arrival_range);
        exit(0);
    }
    
    
    if (!load_settings(config_filename, &max_proc_range, &exec_range, &priority_range, &arrival_range)) {
        printf("Failed to load settings from config file. Using defaults.\n");
    }

    gtk_init(&argc, &argv);
    
    register_algorithms();
    
    if (using_dynamic_plugins()) {
        printf("Using dynamic algorithm plugins\n");
    } else {
        printf("Using built-in algorithms (fallback)\n");
    }
    
    algo_count = get_algorithm_count();
    if (algo_count == 0) {
        printf("Error: No scheduling algorithms registered!\n");
        printf("Check that algorithm implementation files are included in the build.\n");
        
        free(exec_range);
        free(max_proc_range);
        free(priority_range);
        free(arrival_range);
        return -1;
    }
    
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Process Scheduler");
    gtk_window_set_default_size(GTK_WINDOW(window), 1200, 800);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

    apply_css_styles();

    GtkWidget *main_grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), main_grid);

    header_box = create_modern_header();
    gtk_grid_attach(GTK_GRID(main_grid), header_box, 0, 0, 2, 1);

    GtkWidget *sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_size_request(sidebar, 300, -1);
    gtk_grid_attach(GTK_GRID(main_grid), sidebar, 0, 1, 1, 1);

    GtkWidget *algo_panel = create_algo_panel();
    gtk_box_pack_start(GTK_BOX(sidebar), algo_panel, FALSE, FALSE, 0);

    GtkWidget *control_panel = create_control_panel();
    gtk_box_pack_start(GTK_BOX(sidebar), control_panel, FALSE, FALSE, 0);

    drawing_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(drawing_area, TRUE);
    gtk_widget_set_vexpand(drawing_area, TRUE);
    g_signal_connect(G_OBJECT(drawing_area), "draw", G_CALLBACK(on_draw_event_modern), NULL);
    gtk_grid_attach(GTK_GRID(main_grid), drawing_area, 1, 1, 1, 1);

    status_bar = gtk_statusbar_new();
    gtk_grid_attach(GTK_GRID(main_grid), status_bar, 0, 2, 2, 1);

    load_algorithm(current_algorithm);

    gtk_widget_show_all(window);
    g_signal_connect(window, "delete-event", G_CALLBACK(on_delete_event), GINT_TO_POINTER(MAIN_WINDOW));

    gtk_main();
    
    
    if (proc_head != NULL) {
        free(proc_head);
    }
    
    if (algo_buttons != NULL) {
        free(algo_buttons);
    }
    
    if (css_provider != NULL) {
        g_object_unref(css_provider);
    }
    
    free(exec_range);
    free(max_proc_range);
    free(priority_range);
    free(arrival_range);
      
    free_algorithm_registry();
    return 0;
}