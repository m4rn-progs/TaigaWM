#ifndef WINDOW_H
#define WINDOW_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-util.h>

#include "output.h"

#include <river-window-management-v1-client-protocol.h>

struct Window {
    struct river_window_v1 *obj;
    struct river_node_v1 *node;

    bool new;
    bool closed;
    bool fullscreen;
    bool maximized;

    int32_t x;
    int32_t y;
    int32_t oldx;
    int32_t oldy;

    int32_t width;
    int32_t height;

    struct Seat *pointer_move_requested;
    struct Seat *pointer_resize_requested;
    uint32_t pointer_resize_requested_edges;

    struct wl_list link; // WindowManager.windows
};

extern const struct river_window_v1_listener river_window_listener;
extern struct river_window_manager_v1 *window_manager_v1;

void window_handle_closed(void *data, struct river_window_v1 *obj);
void window_handle_dimensions(void *data, struct river_window_v1 *obj,
                              int32_t width, int32_t height);
void window_handle_pointer_move_requested(void *data,
                                          struct river_window_v1 *obj,
                                          struct river_seat_v1 *river_seat);
void window_handle_pointer_resize_requested(void *data,
                                            struct river_window_v1 *obj,
                                            struct river_seat_v1 *river_seat,
                                            uint32_t edges);
// Ignored events
void window_handle_dimensions_hint(void *data, struct river_window_v1 *obj,
                                   int32_t min_width, int32_t min_height,
                                   int32_t max_width, int32_t max_height);
void window_handle_app_id(void *data, struct river_window_v1 *obj,
                          const char *app_id);
void window_handle_title(void *data, struct river_window_v1 *obj,
                         const char *title);
void window_handle_parent(void *data, struct river_window_v1 *obj,
                          struct river_window_v1 *parent);
void window_handle_decoration_hint(void *data, struct river_window_v1 *obj,
                                   uint32_t hint);
void window_handle_show_window_menu_requested(void *data,
                                              struct river_window_v1 *obj,
                                              int32_t x, int32_t y);
void window_handle_maximize_requested(void *data, struct river_window_v1 *obj);
void window_handle_unmaximize_requested(void *data,
                                        struct river_window_v1 *obj);
void seat_enter_fullscreen(struct Window *window, struct Output *output);
void seat_exit_fullscreen(struct Window *window);
void window_handle_fullscreen_requested(void *data, struct river_window_v1 *obj,
                                        struct river_output_v1 *river_output);
void window_handle_exit_fullscreen_requested(void *data,
                                             struct river_window_v1 *obj);
void window_handle_minimize_requested(void *data, struct river_window_v1 *obj);
void window_handle_unreliable_pid(void *data, struct river_window_v1 *obj,
                                  int32_t unreliable_pid);
void window_handle_presentation_hint(void *data, struct river_window_v1 *obj,
                                     uint32_t hint);
void window_handle_identifier(void *data, struct river_window_v1 *obj,
                              const char *identifier);
void window_maybe_destroy(struct Window *window);
void window_set_position(struct Window *window, int32_t x, int32_t y);
void set_borders(struct Window *window);
void window_manage(struct Window *window);

#endif
