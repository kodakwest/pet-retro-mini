#include "game_launcher.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void set_game(game_entry *game, const char *id, const char *title, const char *author,
                     const char *genre, int year, const char *file, const char *model,
                     unsigned int load_addr)
{
    memset(game, 0, sizeof(*game));
    strncpy(game->id, id, GAME_TEXT - 1);
    strncpy(game->title, title, GAME_TEXT - 1);
    strncpy(game->author, author, GAME_TEXT - 1);
    strncpy(game->genre, genre, GAME_TEXT - 1);
    strncpy(game->file, file, GAME_PATH - 1);
    strncpy(game->model, model, GAME_TEXT - 1);
    game->year = year;
    game->load_addr = load_addr;
}

void manifest_defaults(game_manifest *manifest)
{
    memset(manifest, 0, sizeof(*manifest));
    manifest->count = MAX_GAMES;
    set_game(&manifest->games[0], "zork1", "Zork I", "Infocom", "Adventure", 1980,
             "games/zork1/zork1.prg", "CBM 4032", 0x0401);
    set_game(&manifest->games[1], "miner", "Miner", "Cursor #19", "Arcade", 1980,
             "games/miner.prg", "PET 2001", 0x0401);
    set_game(&manifest->games[2], "lander", "Lunar Lander", "Public Domain", "Simulation", 1979,
             "games/lander.prg", "PET 2001", 0x0401);
    set_game(&manifest->games[3], "dungeon", "Dungeon", "Cursor #15", "RPG", 1980,
             "games/dungeon.prg", "CBM 4032", 0x0401);
    set_game(&manifest->games[4], "petinvaders", "PET Invaders", "mass:werk", "Arcade", 2002,
             "games/petinvaders.prg", "CBM 8032", 0x0401);
}

static int extract_string(const char *object, const char *key, char *out, size_t out_size)
{
    char pattern[64];
    const char *p;
    const char *start;
    const char *end;
    size_t len;

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    p = strstr(object, pattern);
    if (p == NULL) {
        return 0;
    }
    p = strchr(p + strlen(pattern), ':');
    if (p == NULL) {
        return 0;
    }
    start = strchr(p, '"');
    if (start == NULL) {
        return 0;
    }
    start++;
    end = strchr(start, '"');
    if (end == NULL) {
        return 0;
    }
    len = (size_t)(end - start);
    if (len >= out_size) {
        len = out_size - 1;
    }
    memcpy(out, start, len);
    out[len] = '\0';
    return 1;
}

static int extract_int(const char *object, const char *key, int *value)
{
    char pattern[64];
    const char *p;

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    p = strstr(object, pattern);
    if (p == NULL) {
        return 0;
    }
    p = strchr(p + strlen(pattern), ':');
    if (p == NULL) {
        return 0;
    }
    *value = atoi(p + 1);
    return 1;
}

void manifest_load(game_manifest *manifest, const char *path)
{
    FILE *file;
    char data[8192];
    size_t read_count;
    const char *cursor;
    int count = 0;

    manifest_defaults(manifest);
    file = fopen(path, "rb");
    if (file == NULL) {
        return;
    }
    read_count = fread(data, 1, sizeof(data) - 1, file);
    fclose(file);
    data[read_count] = '\0';

    cursor = data;
    while (count < MAX_GAMES) {
        const char *start = strchr(cursor, '{');
        const char *end;
        char load_addr[GAME_TEXT];
        int year = 0;

        if (start == NULL) {
            break;
        }
        end = strchr(start, '}');
        if (end == NULL) {
            break;
        }
        if (strstr(start, "\"id\"") != NULL && strstr(start, "\"title\"") != NULL) {
            game_entry parsed;
            memset(&parsed, 0, sizeof(parsed));
            extract_string(start, "id", parsed.id, sizeof(parsed.id));
            extract_string(start, "title", parsed.title, sizeof(parsed.title));
            extract_string(start, "author", parsed.author, sizeof(parsed.author));
            extract_string(start, "genre", parsed.genre, sizeof(parsed.genre));
            extract_string(start, "file", parsed.file, sizeof(parsed.file));
            extract_string(start, "model", parsed.model, sizeof(parsed.model));
            extract_int(start, "year", &year);
            parsed.year = year;
            if (extract_string(start, "load_addr", load_addr, sizeof(load_addr))) {
                parsed.load_addr = (unsigned int)strtoul(load_addr, NULL, 0);
            } else {
                parsed.load_addr = 0x0401;
            }
            manifest->games[count++] = parsed;
        }
        cursor = end + 1;
    }

    if (count > 0) {
        manifest->count = count;
    }
}

int prg_read_load_addr(const char *path, unsigned int *load_addr)
{
    FILE *file = fopen(path, "rb");
    unsigned char bytes[2];

    if (file == NULL) {
        return 0;
    }
    if (fread(bytes, 1, 2, file) != 2) {
        fclose(file);
        return 0;
    }
    fclose(file);
    *load_addr = (unsigned int)bytes[0] | ((unsigned int)bytes[1] << 8);
    return 1;
}

const char *path_basename(const char *path)
{
    const char *slash = strrchr(path, '/');
    const char *backslash = strrchr(path, '\\');
    const char *base = path;

    if (slash != NULL && slash + 1 > base) {
        base = slash + 1;
    }
    if (backslash != NULL && backslash + 1 > base) {
        base = backslash + 1;
    }
    return base;
}
