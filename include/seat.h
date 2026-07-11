#ifndef SEAT_H
#define SEAT_H

#include <river-window-management-v1-client-protocol.h>
#include <stdbool.h>
#include <stdint.h>
#include <wayland-util.h>

#include "actions.h"
#include "pointer.h"

enum SeatOp {
    SEAT_OP_NONE,
    SEAT_OP_MOVE,
    SEAT_OP_RESIZE,
};

struct Seat {
    struct river_seat_v1 *obj;
    bool new;
    bool removed;

    struct Window *focused;
    struct Window *hovered;
    struct Window *interacted;

    struct wl_list xkb_bindings;     // XkbBinding
    struct wl_list pointer_bindings; // PointerBinding
    struct river_layer_shell_seat_v1 *layer_shell_seat;
    struct Output *focused_output;
    enum Action pending_action;
    char *pending_cmd;
    bool no_autostart;

    enum SeatOp op;
    // For SEAT_OP_MOVE and SEAT_OP_RESIZE
    struct Window *op_window;
    int32_t op_start_x, op_start_y;
    int32_t op_dx, op_dy;
    bool op_release;
    // For SEAT_OP_RESIZE only
    int32_t op_start_width, op_start_height;
    uint32_t op_edges;

    int32_t cur_ptr_posx;
    int32_t cur_ptr_posy;
    struct wl_list link; // WindowManager.seats
};

extern const struct river_seat_v1_listener river_seat_listener;

void fallback_config(struct Seat *seat);
void seat_pointer_move(struct Seat *seat, struct Window *window);
void seat_pointer_resize(struct Seat *seat, struct Window *window,
                         uint32_t edges);
void seat_handle_new(struct Seat *seat);
void seat_handle_removed(void *data, struct river_seat_v1 *obj);
void seat_handle_pointer_enter(void *data, struct river_seat_v1 *obj,
                               struct river_window_v1 *river_window);
void seat_handle_pointer_leave(void *data, struct river_seat_v1 *obj);
void seat_handle_window_interaction(void *data, struct river_seat_v1 *obj,
                                    struct river_window_v1 *river_window);
void seat_handle_op_delta(void *data, struct river_seat_v1 *obj, int32_t dx,
                          int32_t dy);
void seat_handle_op_release(void *data, struct river_seat_v1 *obj);
void seat_handle_wl_seat(void *data, struct river_seat_v1 *obj, uint32_t id);
void seat_handle_shell_surface_interaction(
    void *data, struct river_seat_v1 *obj,
    struct river_shell_surface_v1 *river_shell_surface);
void handle_focused_output_change(struct Seat *seat);
void seat_handle_pointer_position(void *data, struct river_seat_v1 *obj,
                                  int32_t x, int32_t y);
void seat_maybe_destroy(struct Seat *seat);
void seat_focus(struct Seat *seat, struct Window *window);
void seat_pointer_move(struct Seat *seat, struct Window *window);
void seat_pointer_resize(struct Seat *seat, struct Window *window,
                         uint32_t edges);
void seat_action(struct Seat *seat, enum Action action);
void seat_manage(struct Seat *seat);
void seat_render(struct Seat *seat);

#endif
