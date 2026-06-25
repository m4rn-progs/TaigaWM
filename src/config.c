#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

void homeify(char **tmp_buf, char *home, char *after_home) {
    size_t home_sz = strlen(home);
    size_t af_sz = strlen(after_home);

    *tmp_buf = malloc(home_sz + 1 + af_sz + 1);

    snprintf(*tmp_buf, home_sz + 1 + af_sz + 1, "%s/%s", home, after_home);
}

char **parse_keybinds(char *config_path, size_t *len_return) {
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    if (luaL_dofile(L, config_path) != LUA_OK) {
        fprintf(stderr, "Failed to open lua config file.\n");
        return NULL;
    }

    lua_getglobal(L, "Keybinds");

    if (! lua_istable(L, -1)) {
        fprintf(stderr, "Failed to find 'Keybinds' section.\n");
        return NULL;
    }

    size_t used = 0;
    size_t len = lua_rawlen(L, -1);

    // malloc then initialize
    char **keybinds_buf = malloc(len *  sizeof(**keybinds_buf));
    for (size_t i = 0 ; i <= len ; i++) keybinds_buf[i] = NULL;

    for (size_t i = 1; i <= len; i++) {
        lua_rawgeti(L, -1, i);

        if (!lua_isstring(L, -1)) {
            fprintf(stderr, "Failed to read keybinds");

            lua_pop(L, 1);
            for (size_t j = 0; j < used; j++) free(keybinds_buf[j]);
            free(keybinds_buf);
            return NULL;
        }

        // get the string + len then malloc
        const char *str = lua_tostring(L, -1);
        size_t sl = strlen(str);
        keybinds_buf[used] = malloc(sl + 1);

        // tried to use strcpy, but didnt work, have to use memcpy
        memcpy(keybinds_buf[used], str, sl + 1); // copies null terminator
        used++;

        lua_pop(L, 1); // pop value
    }

    *len_return = len;
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
