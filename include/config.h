#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <lua.h>

#include "seat.h"

int get_lua_table_by_name(lua_State *state, const char *name);
void homeify(char **tmp_buf, char *home, char *after_home);
void parse_get(char **save, char **str, const char *delim);
int parse_and_add_keybind(char *keybind_str, struct Seat *seat);
char **parse_keybinds(char *config_path, size_t *len_return);
char *locate_config(void);

#endif
