#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "autostart.h"
#include "config.h"
#include "window.h"
#include "wm.h"
#include "xkb.h"

// A way to pass something to the sig handler
volatile sig_atomic_t config_changed = 0;

void setup_inotify(void) {
    // Fork and wait for config changed
    // On config change send sig handler and exit
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

// We do this so the main loop can check if the config was changed, in order to
// setup inotify again
void handle_config_change(int sig) { config_changed = 1; }

int main(void) {
    // Load the config as the first thing we do
    load_config();

    // Sig magic
    struct sigaction sa = {0};
    sa.sa_handler = handle_config_change;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa, NULL);

    // Connect to display
    struct wl_display *display = wl_display_connect(NULL);
    if (display == NULL) {
        fprintf(stderr, "ERROR: failed to connect to Wayland server.\n");
        return 1;
    }

    // Remove WAYLAND_DEBUG from our children
    unsetenv("WAYLAND_DEBUG");
    signal(SIGCHLD, SIG_IGN);

    // Registry setup
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    if (wl_display_roundtrip(display) < 0) {
        fprintf(stderr, "ERROR: roundtrip failed.\n");
        return 1;
    }

    // Check that the compositor is river compatible / is river
    if (window_manager_v1 == NULL || xkb_bindings_v1 == NULL) {
        fprintf(stderr, "ERROR: compositor is not river compatible.\n");
        return 1;
    }

    // Steup the wm struct, inotify, then setup the wm listener
    wm_init();
    setup_inotify();
    river_window_manager_v1_add_listener(window_manager_v1, &wm_listener, NULL);

    // Execute the autostarts in the main function, cuz it only runs once
    fprintf(stdout, "INFO: Executing autostarts.\n");
    if (autostart_config.autostarts != NULL) {
        autostart(autostart_config.autostarts, autostart_config.autostarts_len);
    }

    while (true) {
        // Dispatch and error handle
        if (wl_display_dispatch(display) < 0) {
            if (errno == EINTR) {
                continue;
            }
            fprintf(stderr, "ERROR: dispatch failed\n");
            return 1;
        }

        // If the config changes set seat_new for all, then setup inotify again
        if (config_changed) {
            fprintf(stdout, "INFO: config file changed, Reloading.\n");
            config_changed = 0;
            struct Seat *seat;
            wl_list_for_each(seat, &wm.seats, link) { seat->new = true; }
            setup_inotify();
        }

        // use this for debug
        // struct Output *output = get_focused_output();
        // fprintf(stdout, "Focused monitor: pos=%dx%d, dim=%dx%d\n",
        // output->posx, output->posy, output->width, output->height);
    }
    return 0;
}
