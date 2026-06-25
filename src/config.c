#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void homeify(char **tmp_buf, char *home, char *after_home) {
    size_t home_sz = strlen(home);
    size_t af_sz = strlen(after_home);

    *tmp_buf = malloc(home_sz + 1 + af_sz + 1);

    snprintf(*tmp_buf, home_sz + 1 + af_sz + 1, "%s/%s", home, after_home);
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
