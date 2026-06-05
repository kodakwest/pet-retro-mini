#ifndef PET_CONFIG_H
#define PET_CONFIG_H

#define MAX_RECENT_FILES 5
#define MAX_PATH_TEXT 260

typedef struct app_config {
    int model_index;
    int crt_enabled;
    int speed_index;
    char recent_files[MAX_RECENT_FILES][MAX_PATH_TEXT];
    int recent_count;
} app_config;

void config_defaults(app_config *config);
void config_load(app_config *config, const char *path);
void config_save(const app_config *config, const char *path);
void config_add_recent(app_config *config, const char *path);

#endif
