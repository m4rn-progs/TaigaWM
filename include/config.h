#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>

void homeify(char **tmp_buf, char *home, char *after_home);
char **parse_keybinds(char *config_path, size_t *len_return);
char *locate_config(void);

#endif
