#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void trim_newline(char *text)
{
    size_t len = strlen(text);
    while (len > 0 && (text[len - 1] == '\n' || text[len - 1] == '\r')) {
        text[--len] = '\0';
    }
}

void config_defaults(app_config *config)
{
    memset(config, 0, sizeof(*config));
    config->model_index = 1;
    config->crt_enabled = 1;
    config->speed_index = 0;
}

void config_load(app_config *config, const char *path)
{
    FILE *file;
    char line[640];

    config_defaults(config);
    file = fopen(path, "r");
    if (file == NULL) {
        return;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        trim_newline(line);
        if (strncmp(line, "model_index=", 12) == 0) {
            config->model_index = atoi(line + 12);
        } else if (strncmp(line, "crt_enabled=", 12) == 0) {
            config->crt_enabled = atoi(line + 12) ? 1 : 0;
        } else if (strncmp(line, "speed_index=", 12) == 0) {
            config->speed_index = atoi(line + 12);
        } else if (strncmp(line, "recent", 6) == 0) {
            char *equals = strchr(line, '=');
            if (equals != NULL && config->recent_count < MAX_RECENT_FILES) {
                strncpy(config->recent_files[config->recent_count], equals + 1, MAX_PATH_TEXT - 1);
                config->recent_files[config->recent_count][MAX_PATH_TEXT - 1] = '\0';
                config->recent_count++;
            }
        }
    }

    if (config->model_index < 0 || config->model_index > 2) {
        config->model_index = 1;
    }
    if (config->speed_index < 0 || config->speed_index > 2) {
        config->speed_index = 0;
    }

    fclose(file);
}

void config_save(const app_config *config, const char *path)
{
    FILE *file = fopen(path, "w");
    int i;

    if (file == NULL) {
        return;
    }

    fprintf(file, "[pet-retro-mini]\n");
    fprintf(file, "model_index=%d\n", config->model_index);
    fprintf(file, "crt_enabled=%d\n", config->crt_enabled ? 1 : 0);
    fprintf(file, "speed_index=%d\n", config->speed_index);
    for (i = 0; i < config->recent_count && i < MAX_RECENT_FILES; i++) {
        fprintf(file, "recent%d=%s\n", i, config->recent_files[i]);
    }

    fclose(file);
}

void config_add_recent(app_config *config, const char *path)
{
    int i;
    int existing = -1;

    for (i = 0; i < config->recent_count; i++) {
        if (strcmp(config->recent_files[i], path) == 0) {
            existing = i;
            break;
        }
    }

    if (existing >= 0) {
        for (i = existing; i > 0; i--) {
            strncpy(config->recent_files[i], config->recent_files[i - 1], MAX_PATH_TEXT);
        }
    } else {
        if (config->recent_count < MAX_RECENT_FILES) {
            config->recent_count++;
        }
        for (i = config->recent_count - 1; i > 0; i--) {
            strncpy(config->recent_files[i], config->recent_files[i - 1], MAX_PATH_TEXT);
        }
    }

    strncpy(config->recent_files[0], path, MAX_PATH_TEXT - 1);
    config->recent_files[0][MAX_PATH_TEXT - 1] = '\0';
}
