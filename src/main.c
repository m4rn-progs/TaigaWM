#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

#include "window.h"
#include "wm.h"
#include "xkb.h"
#include "config.h"

int main(void) {
	struct wl_display *display = wl_display_connect(NULL);
	if (display == NULL) {
		fprintf(stderr, "Failed to connect to Wayland server.\n");
		return 1;
	}

	unsetenv("WAYLAND_DEBUG");
	signal(SIGCHLD, SIG_IGN);

	struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, NULL);
	if (wl_display_roundtrip(display) < 0) {
		fprintf(stderr, "Roundtrip failed.\n");
		return 1;
	}

	if (window_manager_v1 == NULL || xkb_bindings_v1 == NULL) {
		fprintf(stderr, "Compositor is not river compatible.\n");
		return 1;
	}

	wm_init();
	char *config_path = locate_config();
	if (config_path != NULL) {
	    printf("Found config at: %s\n", config_path);
	} else {
	    fprintf(stderr, "Failed to find config file.\n");
	}

	river_window_manager_v1_add_listener(window_manager_v1, &wm_listener, NULL);

	while (true) {
		if (wl_display_dispatch(display) < 0) {
			fprintf(stderr, "dispatch failed\n");
			return 1;
		}
	}

	return 0;
}
