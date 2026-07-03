#include <stdlib.h>
#include <unistd.h>

#include "autostart.h"

void autostart(char **autostart_list, size_t autostart_list_sz) {
    for (size_t i = 0; i < autostart_list_sz; i++) {
        if (fork() == 0) {
            fprintf(stdout, "INFO: executing: %s\n", autostart_list[i]);
            execl("/bin/sh", "/bin/sh", "-c", autostart_list[i], NULL);
        }
        free(autostart_list[i]);
    }
    free(autostart_list);
}
