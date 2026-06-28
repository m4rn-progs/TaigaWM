#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <lua.h>

#include "seat.h"

int get_lua_table_by_name(lua_State *state, const char *name);
void homeify(char **tmp_buf, char *home, char *after_home);
void parse_get(char **save, char **str, const char *delim);
int parse_and_add_keybind(char *keybind_str, struct Seat *seat);
lua_State *lua_open_table(const char *config_path, const char *table_name);
bool get_bool_from_var_from_table(const char *config_path, const char *table_name, const char *var_name);
const char *get_string_from_var_from_table(const char *config_path, const char *table_name, const char *var_name);
char **get_list_of_strings_from_lua_table(const char *config_path, size_t *len_return, const char *table_name);
char *locate_config(void);

#endif
