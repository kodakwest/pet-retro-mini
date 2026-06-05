#ifndef PET_GAME_LAUNCHER_H
#define PET_GAME_LAUNCHER_H

#define MAX_GAMES 5
#define GAME_TEXT 64
#define GAME_PATH 260

typedef struct game_entry {
    char id[GAME_TEXT];
    char title[GAME_TEXT];
    char author[GAME_TEXT];
    char genre[GAME_TEXT];
    int year;
    char file[GAME_PATH];
    char model[GAME_TEXT];
    unsigned int load_addr;
} game_entry;

typedef struct game_manifest {
    game_entry games[MAX_GAMES];
    int count;
} game_manifest;

void manifest_defaults(game_manifest *manifest);
void manifest_load(game_manifest *manifest, const char *path);
int prg_read_load_addr(const char *path, unsigned int *load_addr);
const char *path_basename(const char *path);

#endif
