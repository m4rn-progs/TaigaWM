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
#include "config.h"


struct KeybindConfig keybind_config = {0};
struct AutostartConfig autostart_config = {0};
struct LibinputConfig libinput_config = {0};
struct MiscConfig misc_config = {0};

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


// huge function to parse, and .... add keybinds. duh.
// m4rn-progs note here,
// I have decided to rewrite this to use pre-allocated memory. Since we'll
// be calling this function in seat.c for each bit of config we have,
// I don't want to keep calling malloc like 50 times per line and for each " " char.
// Therefore, pre-allocate memory via array and operate in that array, and just destroy the array
int parse_and_add_keybind(const char *keybind_str, struct Seat *seat) {
    // pre-alloc that array
    size_t len = strlen(keybind_str);
    
    char buf[len + 1];
    strcpy(buf, keybind_str);

    char *saveptr1, *saveptr2;
    //start tokenizing

    // get mod str
    char *mods = strtok_r(buf, " ", &saveptr1);
    if (!mods) { 
        fprintf(stderr, "ERROR: missing modifiers.\n"); 
        return 1; 
    }

    //get key str
    char *key = strtok_r(NULL, " ", &saveptr1);
    if (!key) { 
        fprintf(stderr, "ERROR: missing key.\n"); 
        return 2; 
    }
    
    //get action str
    char *action = strtok_r(NULL, " ", &saveptr1);
    if (!action) { 
        fprintf(stderr, "ERROR: missing action.\n"); 
        return 3; 
    }
    
    // prepare a buffer that will soon enough get copied to the heap
    char final_cmd_buf[len + 1];
    size_t final_cmd_len = 0;
    
    // prepare the cmd_token that will be in that buffer
    char *cmd_tok = strtok_r(NULL, " ", &saveptr1);

     // "loop over all delims and basically add the rest of the cmds to final_cmd" - zoey, 2026
    while (cmd_tok != NULL) {
        size_t tok_len = strlen(cmd_tok);
        
        if (final_cmd_len > 0) {
            final_cmd_buf[final_cmd_len++] = ' ';
        }
        
        memcpy(final_cmd_buf + final_cmd_len, cmd_tok, tok_len);
        final_cmd_len += tok_len;
        
        cmd_tok = strtok_r(NULL, " ", &saveptr1);
    }
    final_cmd_buf[final_cmd_len] = '\0';

    uint32_t mods_local = RIVER_SEAT_V1_MODIFIERS_NONE;
    char *mod_tok = strtok_r(mods, "+", &saveptr2);
    
    // add all the mods together if there are any if not, no mod will be used
    while (mod_tok != NULL) {
        if (strcmp(mod_tok, "super") == 0) {
            mods_local |= RIVER_SEAT_V1_MODIFIERS_MOD4;
        } else if (strcmp(mod_tok, "ctrl") == 0) {
            mods_local |= RIVER_SEAT_V1_MODIFIERS_CTRL;
        } else if (strcmp(mod_tok, "alt") == 0) {
            mods_local |= RIVER_SEAT_V1_MODIFIERS_MOD1;
        } else if (strcmp(mod_tok, "shift") == 0) {
            mods_local |= RIVER_SEAT_V1_MODIFIERS_SHIFT;
        } else {
            fprintf(stderr, "EXTREME WARNING: no modifer selected, you probably DON'T want that.\n");
        }
        mod_tok = strtok_r(NULL, "+", &saveptr2);
    }
    
    // check the action to decide what to do
    if (strcmp(action, "spawn") == 0) {
        if (final_cmd_len == 0) { 
            fprintf(stderr, "ERROR: missing command.\n"); 
            return 4; 
        }
        // look mom the buffer got copied to heap
        xkb_binding_create(seat, mods_local, xkb_keysym_from_name(key, XKB_KEYSYM_CASE_INSENSITIVE), ACTION_SPAWN_SH, strdup(final_cmd_buf));
    } else if (strcmp(action, "killactive") == 0) {
        xkb_binding_create(seat, mods_local, xkb_keysym_from_name(key, XKB_KEYSYM_CASE_INSENSITIVE), ACTION_CLOSE, NULL);
    } else if (strcmp(action, "exit") == 0) {
        xkb_binding_create(seat, mods_local, xkb_keysym_from_name(key, XKB_KEYSYM_CASE_INSENSITIVE), ACTION_EXIT, NULL);
    }

    return 0;
}

lua_State *lua_open_table(const char *config_path, const char *table_name) {
    if (config_path == NULL) {
        return NULL;
    }

    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    if (luaL_dofile(L, config_path) != LUA_OK) {
        fprintf(stderr, "ERROR: failed to open lua config file.\n");
        return NULL;
    }

    if (get_lua_table_by_name(L, table_name)) {
        fprintf(stderr, "ERROR: failed to find '%s' section.\n", table_name);
        lua_close(L);
        return NULL;
    }

    return L;

}

bool get_bool_from_var_from_table(const char *config_path, const char *table_name, const char *var_name) {
    lua_State *L;
    if((L = lua_open_table(config_path, table_name)) == NULL) {
        return NULL;
    }

    lua_getfield(L, -1, var_name);
    int luatype = lua_type(L, -1);

    if (luatype == LUA_TBOOLEAN){
        bool b = lua_toboolean(L, -1);

        lua_close(L);
        return b;
    } else {
        lua_close(L);
        return NULL;
    }
}

char *get_string_from_var_from_table(const char *config_path, const char *table_name, const char *var_name) {
    lua_State *L;
    if((L = lua_open_table(config_path, table_name)) == NULL) {
        return NULL;
    }

    lua_getfield(L, -1, var_name);
    int luatype = lua_type(L, -1);

    if (luatype == LUA_TSTRING){
        const char *s = lua_tostring(L, -1);

        lua_close(L);
        return (char *)s;
    } else {
        lua_close(L);
        return NULL;
    }
}

char **get_list_of_strings_from_lua_table(const char *config_path, size_t *len_return, const char *table_name) {
    lua_State *L;
    if((L = lua_open_table(config_path, table_name)) == NULL) {
        return NULL;
    }

    size_t len = lua_rawlen(L, -1);
    size_t used = 0;
    char **strings_buf = malloc(len * sizeof(*strings_buf));
    for (size_t i = 0; i < len; i++) strings_buf[i] = NULL;

    for (size_t i = 1; i <= len; i++) {
        lua_rawgeti(L, -1, i);

        if (!lua_isstring(L, -1)) {
            fprintf(stderr, "ERROR: failed to read '%s' section.\n", table_name);
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
            if (home == NULL) {
                fprintf(stderr, "WARNING: HOME environment variable not set. Skipping %s\n", config_locations[i]);
                continue;
            }

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

    fprintf(stdout, "INFO: trying to load fallback config ./taigarc.lua\n");

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

int load_config(void) {
    const char *config_path = locate_config();
    if (config_path == NULL) {
        fprintf(stderr, "ERROR: failed to open a config file.");
        return 1;
    }

    // keybinds
    size_t binds_len;
    char **binds = get_list_of_strings_from_lua_table(config_path, &binds_len, "Keybinds");
    keybind_config.keybinds = binds;
    keybind_config.keybinds_len = binds_len;

    // autostart
    size_t autostart_len;
    char **autostart = get_list_of_strings_from_lua_table(config_path, &autostart_len, "Autostart");
    autostart_config.autostarts = autostart;
    autostart_config.autostarts_len = autostart_len;

    // libinput
    char *accel_profile = get_string_from_var_from_table(config_path, "Libinput", "accel_profile");

    // have to use strdup or no workie
    libinput_config.accel_profile = strdup(accel_profile);
    
    // misc
    bool tearing = get_bool_from_var_from_table(config_path, "Misc", "tearing");
    misc_config.tearing = tearing;

    return 0;
}