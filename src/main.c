#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <sys/inotify.h>
#include <errno.h>
#include <wayland-util.h>

#include "window.h"
#include "wm.h"
#include "xkb.h"
#include "config.h"
#include "autostart.h"

volatile sig_atomic_t config_changed = 0;

void setup_inotify(void) {
    if (fork() == 0) {
        int fd = inotify_init();
        char buf[1024];
        inotify_add_watch(fd, config_path, IN_MODIFY);

        while (read(fd, buf, sizeof(buf)) > 0) {
            kill(getppid(), SIGUSR1);
        }
        exit(0);
    }
}

void handle_config_change(int sig) {
    config_changed = 1;
}

int main(void) {
    load_config();
    struct sigaction sa = {0};
    sa.sa_handler = handle_config_change;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa, NULL);

    struct wl_display *display = wl_display_connect(NULL);
    if (display == NULL) {
        fprintf(stderr, "ERROR: failed to connect to Wayland server.\n");
        return 1;
    }

    unsetenv("WAYLAND_DEBUG");
    signal(SIGCHLD, SIG_IGN);

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    if (wl_display_roundtrip(display) < 0) {
        fprintf(stderr, "ERROR: roundtrip failed.\n");
        return 1;
    }

    if (window_manager_v1 == NULL || xkb_bindings_v1 == NULL) {
        fprintf(stderr, "ERROR: compositor is not river compatible.\n");
        return 1;
    }

    wm_init();
    setup_inotify();

    river_window_manager_v1_add_listener(window_manager_v1, &wm_listener, NULL);

    if (autostart_config.autostarts != NULL) {
        autostart(autostart_config.autostarts, autostart_config.autostarts_len);
    }

    while (true) {
        if (wl_display_dispatch(display) < 0) {
            if (errno == EINTR) {
                continue;
            }
            fprintf(stderr, "ERROR: dispatch failed\n");
            return 1;
        }

        if (config_changed) {
            fprintf(stdout, "INFO: config file changed, Reloading.\n");
            config_changed = 0;
            struct Seat *seat;
            wl_list_for_each(seat, &wm.seats, link) {
                seat->new = true;
            }
            setup_inotify();
        }
    }

    return 0;
}
