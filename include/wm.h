#ifndef WM_H
#define WM_H

#include <wayland-util.h>
#include <river-window-management-v1-client-protocol.h>
#include <wayland-client-protocol.h>
#include <stdbool.h>

struct WindowManager {
	struct wl_list outputs; // Output
	struct wl_list windows; // Window
	struct wl_list seats; // Seat
	bool layer_shell_has_default_output;
};

extern struct WindowManager wm;

extern const struct wl_registry_listener registry_listener;
extern const struct river_window_manager_v1_listener wm_listener;

void wm_handle_unavailable(void *data, struct river_window_manager_v1 *obj);
void wm_handle_finished(void *data, struct river_window_manager_v1 *obj);
void wm_handle_manage_start(void *data, struct river_window_manager_v1 *obj);
void wm_handle_render_start(void *data, struct river_window_manager_v1 *obj);
void wm_handle_window(void *data, struct river_window_manager_v1 *obj, struct river_window_v1 *river_window);
void wm_handle_output(void *data, struct river_window_manager_v1 *obj, struct river_output_v1 *river_output);
void wm_handle_seat(void *data, struct river_window_manager_v1 *obj, struct river_seat_v1 *river_seat);
void wm_handle_session_locked(void *data, struct river_window_manager_v1 *obj);
void wm_handle_session_unlocked(void *data, struct river_window_manager_v1 *obj);
void wm_init(void);
void handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
void handle_global_remove(void *data, struct wl_registry *registry, uint32_t name);

#endif
