#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon.h>

#include "xkb.h"
#include "seat.h"

// open a lua table, just for slighly cleaner code
int get_lua_table_by_name(lua_State *state, const char *name) {
    lua_getglobal(state, name);
    if (!lua_istable(state, -1)) {
        return 1;
    }
    return 0;
}

// add home + str to tmp buf
void homeify(char **tmp_buf, char *home, char *after_home) {
    size_t home_sz = strlen(home);
    size_t af_sz = strlen(after_home);

    *tmp_buf = malloc(home_sz + 1 + af_sz + 1);

    snprintf(*tmp_buf, home_sz + 1 + af_sz + 1, "%s/%s", home, after_home);
}

//basically a strtok loop
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

// huge function to parse, and .... add keybinds. duh.
int parse_and_add_keybind(char *keybind_str, struct Seat *seat) {
    const char *delim = " ";
    char *mods = NULL;
    char *key = NULL;
    char *action = NULL;
    char *cmd = NULL;

    // save keybind str for later
    char *save = malloc(strlen(keybind_str));
    memcpy(save, keybind_str, strlen(keybind_str) + 1);

    // get mods str
    parse_get(&mods, &keybind_str, delim);
    if (mods == NULL) {
        fprintf(stderr, "ERROR! Missing modifiers.\n");
        return 1;
    }

    // get key str
    parse_get(&key, &keybind_str, delim);
    if (key == NULL) {
        fprintf(stderr, "ERROR! Missing key.\n");
        return 2;
    }

    // get action str
    parse_get(&action, &keybind_str, delim);
    if (action == NULL) {
        fprintf(stderr, "ERROR! Missing action.\n");
        return 3;
    }

    // get the cmd str
    parse_get(&cmd, &keybind_str, delim);

    // variable init
    char *token;
    size_t total;
    char *final_cmd;
    final_cmd = malloc(1);
    final_cmd[0] = '\0';
    total = 0;

    // skip the first 4 tokens cuz we are restarting
    token = strtok(save, " ");
    token = strtok(NULL, " ");
    token = strtok(NULL, " ");
    token = strtok(NULL, " ");

    // loop over all delims and basically add the rest of the cmds to final_cmd
    while(token != NULL) {
        if (total == 0) {
            final_cmd = realloc(final_cmd, strlen(token + 1));
            strcpy(final_cmd, token);
            total = strlen(token);
        } else {
            final_cmd = realloc(final_cmd, total + 1 + strlen(token) + 1);
            strcat(final_cmd, " ");
            strcat(final_cmd, token);
            total = strlen(final_cmd);
        }
        token = strtok(NULL, " ");
    }

    // free the temp stuff
    free(save);

    // add all the mods together if there are any if not, no mod will be used
    uint32_t mods_local = RIVER_SEAT_V1_MODIFIERS_NONE;
    token = strtok(mods, "+");
    while(token != NULL) {
        if (strcmp(token, "super") == 0) {
            mods_local = mods_local | RIVER_SEAT_V1_MODIFIERS_MOD4;
        } else if (strcmp(token, "ctrl") == 0) {
            mods_local = mods_local | RIVER_SEAT_V1_MODIFIERS_CTRL;
        } else if (strcmp(token, "alt") == 0) {
            mods_local = mods_local | RIVER_SEAT_V1_MODIFIERS_MOD1;
        } else if (strcmp(token, "shift") == 0) {
            mods_local = mods_local | RIVER_SEAT_V1_MODIFIERS_SHIFT;
        } else {
            fprintf(stderr, "EXTREME WARNING! No modifer selected, you probably DON'T want that.\n");
        }
        token = strtok(NULL, "+");
    }

    // check the action to decide what to do
    if (strcmp(action, "spawn") == 0) {
        if (cmd == NULL) {
            fprintf(stderr, "ERROR! Missing command.\n");
            return 4;
        }
        xkb_binding_create(seat, mods_local, xkb_keysym_from_name(key, XKB_KEYSYM_CASE_INSENSITIVE), ACTION_SPAWN_SH, strdup(final_cmd));
        free(final_cmd);
    } else if (strcmp(action, "killactive") == 0) {
        xkb_binding_create(seat, mods_local, xkb_keysym_from_name(key, XKB_KEYSYM_CASE_INSENSITIVE), ACTION_CLOSE, NULL);
    }

    return 0;
}

char **get_list_of_strings_from_lua_table(const char *config_path, size_t *len_return, const char *table_name) {
    if (config_path == NULL) {
        return NULL;
    }

    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    if (luaL_dofile(L, config_path) != LUA_OK) {
        fprintf(stderr, "Failed to open lua config file.\n");
        return NULL;
    }

    if (get_lua_table_by_name(L, table_name)) {
        fprintf(stderr, "Failed to find '%s' section.\n", table_name);
        lua_close(L);
        return NULL;
    }

    size_t len = lua_rawlen(L, -1);
    size_t used = 0;
    char **strings_buf = malloc(len * sizeof(*strings_buf));
    for (size_t i = 0; i < len; i++) strings_buf[i] = NULL;

    for (size_t i = 1; i <= len; i++) {
        lua_rawgeti(L, -1, i);

        if (!lua_isstring(L, -1)) {
            fprintf(stderr, "Failed to read '%s' section.\n", table_name);
            lua_pop(L, 1);
            for (size_t j = 0; j < used; j++) free(strings_buf[j]);
            free(strings_buf);
            lua_close(L);
            return NULL;
        }

        const char *str = lua_tostring(L, -1);
        size_t sl = strlen(str);

        strings_buf[used] = malloc(sl + 1);
        memcpy(strings_buf[used], str, sl + 1);
        used++;

        lua_pop(L, 1);
    }

    *len_return = len;
    lua_close(L);
    return strings_buf;

}

// locate the config file
char *locate_config(void) {
    // we will add home later.
    char *config_locations[] = {
        ".config/taiga/taigarc.lua",
        ".taigarc.lua",
        "/usr/share/taigarc.lua",
    };

    // get len
    size_t config_locations_len = sizeof(config_locations) / sizeof(config_locations[0]);

    // for every possible config location
    for (size_t i = 0 ; i < config_locations_len ; i++) {
        // if it doesnt start with / we want to add home to it
        if (config_locations[i][0] != '/') {
            // proper string
            char *home = getenv("HOME");
            char *tmp_buf = NULL;
            homeify(&tmp_buf, home, config_locations[i]);

            // try open it
            FILE *tmp_file = fopen(tmp_buf, "r");
            if(tmp_file != NULL) {
                fclose(tmp_file);
                return tmp_buf;
            }

            //clean
            free(tmp_buf);
        } else {
            // try open it
            FILE *tmp_file = fopen(config_locations[i], "r");
            if(tmp_file != NULL) {
                fclose(tmp_file);
                return config_locations[i];
            }
        }
    }

    fprintf(stdout, "Trying to load fallback config ./taigarc.lua\n");

    // get cwd and prepend it to file name
    char cwd[1024];
    char full_path[2048];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        return NULL;
    }
    snprintf(full_path, sizeof(full_path), "%s/%s", cwd, "taigarc.lua");

    // try open it
    FILE *tmp_file = fopen(full_path, "r");
    if(tmp_file != NULL) {
        fclose(tmp_file);
        return strdup(full_path);
    }

    return NULL;
}
