#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon.h>

#include "xkb.h"
#include "seat.h"

void homeify(char **tmp_buf, char *home, char *after_home) {
    size_t home_sz = strlen(home);
    size_t af_sz = strlen(after_home);

    *tmp_buf = malloc(home_sz + 1 + af_sz + 1);

    snprintf(*tmp_buf, home_sz + 1 + af_sz + 1, "%s/%s", home, after_home);
}

void parse_get(char **save, char **str, const char *delim) {
    char *token = strtok(*str, delim);
    *str = NULL;
    if (token == NULL) {
        save = NULL;
        return;
    }
    *save = malloc(strlen(token) + 1);
    strcpy(*save, token);
}

int parse_and_add_keybind(char *keybind_str, struct Seat *seat) {
    const char *delim = " ";
    char *mods = NULL;
    char *key = NULL;
    char *action = NULL;
    char *cmd = NULL;

    parse_get(&mods, &keybind_str, delim);
    if (mods == NULL) {
        fprintf(stderr, "ERROR! Missing modifiers.\n");
        return 1;
    }

    parse_get(&key, &keybind_str, delim);
    if (key == NULL) {
        fprintf(stderr, "ERROR! Missing key.\n");
        return 2;
    }

    parse_get(&action, &keybind_str, delim);
    if (action == NULL) {
        fprintf(stderr, "ERROR! Missing action.\n");
        return 3;
    }

    if (strcmp(action, "spawn") == 0) {
        parse_get(&cmd, &keybind_str, delim);
        if (cmd == NULL) {
            fprintf(stderr, "ERROR! Missing command.\n");
            return 4;
        }

        uint32_t mods_local;
        if (strcmp(mods, "super") == 0) {
            mods_local = RIVER_SEAT_V1_MODIFIERS_MOD4;
        } else if (strcmp(mods, "alt") == 0) {
            mods_local = RIVER_SEAT_V1_MODIFIERS_MOD1;
        } else {
            fprintf(stderr, "ERROR! Unimplemented mod.\n");
            return 5;
        }

        xkb_binding_create(seat, mods_local, xkb_keysym_from_name(key, XKB_KEYSYM_CASE_INSENSITIVE), ACTION_SPAWN_SH, cmd);
    }

    return 0;
}

char **parse_keybinds(char *config_path, size_t *len_return) {
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    if (luaL_dofile(L, config_path) != LUA_OK) {
        fprintf(stderr, "Failed to open lua config file.\n");
        return NULL;
    }

    lua_getglobal(L, "Keybinds");

    if (!lua_istable(L, -1)) {
        fprintf(stderr, "Failed to find 'Keybinds' section.\n");
        lua_close(L);
        return NULL;
    }

    size_t len = lua_rawlen(L, -1);
    size_t used = 0;

    char **keybinds_buf = malloc(len * sizeof(*keybinds_buf));
    for (size_t i = 0; i < len; i++) keybinds_buf[i] = NULL;

    for (size_t i = 1; i <= len; i++) {
        lua_rawgeti(L, -1, i);

        if (!lua_isstring(L, -1)) {
            fprintf(stderr, "Failed to read keybinds\n");
            lua_pop(L, 1);
            for (size_t j = 0; j < used; j++) free(keybinds_buf[j]);
            free(keybinds_buf);
            lua_close(L);
            return NULL;
        }

        const char *str = lua_tostring(L, -1);
        size_t sl = strlen(str);

        keybinds_buf[used] = malloc(sl + 1);
        memcpy(keybinds_buf[used], str, sl + 1);
        used++;

        lua_pop(L, 1);
    }

    *len_return = len;
    lua_close(L);
    return keybinds_buf;
}

char *locate_config(void) {
    // we will add home later.
    char *config_locations[] = {
        ".config/taiga/taigarc.lua",
        ".taigarc.lua",
        "/usr/share/taigarc.lua",
    };

    size_t config_locations_len = sizeof(config_locations) / sizeof(config_locations[0]);
    for (size_t i = 0 ; i < config_locations_len ; i++) {
        if (config_locations[i][0] != '/') {
            char *home = getenv("HOME");
            char *tmp_buf = NULL;
            homeify(&tmp_buf, home, config_locations[i]);
            FILE *tmp_file = fopen(tmp_buf, "r");
            if(tmp_file != NULL) {
                printf("Found a config\n");
                return tmp_buf;
            }
            free(tmp_buf);
        } else {
            FILE *tmp_file = fopen(config_locations[i], "r");
            if(tmp_file != NULL) {
                printf("Found a config\n");
                return config_locations[i];
            }
        }
    }
    return NULL;
}
