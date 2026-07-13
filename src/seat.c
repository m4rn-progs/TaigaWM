#include <linux/input-event-codes.h>
#include <stdlib.h>
#include <unistd.h>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

#include "actions.h"
#include "config.h"
#include "output.h"
#include "seat.h"
#include "window.h"
#include "wm.h"
#include "xkb.h"

#include <river-layer-shell-v1-client-protocol.h>

void seat_handle_removed(void *data, struct river_seat_v1 *obj) {
    struct Seat *seat = data;
    seat->removed = true;
}

void seat_handle_pointer_enter(void *data, struct river_seat_v1 *obj,
                               struct river_window_v1 *river_window) {
    struct Seat *seat = data;
    seat->hovered = river_window_v1_get_user_data(river_window);
}

void seat_handle_pointer_leave(void *data, struct river_seat_v1 *obj) {
    struct Seat *seat = data;
    seat->hovered = NULL;
}

void seat_handle_window_interaction(void *data, struct river_seat_v1 *obj,
                                    struct river_window_v1 *river_window) {
    struct Seat *seat = data;
    seat->interacted = river_window_v1_get_user_data(river_window);
}

void seat_handle_op_delta(void *data, struct river_seat_v1 *obj, int32_t dx,
                          int32_t dy) {
    struct Seat *seat = data;
    seat->op_dx = dx;
    seat->op_dy = dy;
}

void seat_handle_op_release(void *data, struct river_seat_v1 *obj) {
    struct Seat *seat = data;
    seat->op_release = true;
}

void handle_focused_output_change(struct Seat *seat) {
    fprintf(stdout, "INFO: Changing default layer shell output.\n");
    river_layer_shell_output_v1_set_default(
        seat->focused_output->layer_shell_output);
}

void seat_handle_pointer_position(void *data, struct river_seat_v1 *obj,
                                  int32_t x, int32_t y) {
    struct Seat *seat = data;
    seat->cur_ptr_posx = x;
    seat->cur_ptr_posy = y;

    struct Output *new_output = get_focused_output();
    struct Output *old_output = seat->focused_output;

    if (new_output != old_output) {
        seat->focused_output = new_output;
        if (new_output != NULL) {
            fprintf(stdout, "INFO: Focused output changed.\n");
            handle_focused_output_change(seat);
        }
    }
}

// Ignored events
void seat_handle_wl_seat(void *data, struct river_seat_v1 *obj, uint32_t id) {}
void seat_handle_shell_surface_interaction(
    void *data, struct river_seat_v1 *obj,
    struct river_shell_surface_v1 *river_shell_surface) {}

const struct river_seat_v1_listener river_seat_listener = {
    .removed = seat_handle_removed,
    .wl_seat = seat_handle_wl_seat,
    .pointer_enter = seat_handle_pointer_enter,
    .pointer_leave = seat_handle_pointer_leave,
    .window_interaction = seat_handle_window_interaction,
    .shell_surface_interaction = seat_handle_shell_surface_interaction,
    .op_delta = seat_handle_op_delta,
    .op_release = seat_handle_op_release,
    .pointer_position = seat_handle_pointer_position,
};

void seat_maybe_destroy(struct Seat *seat) {
    if (!seat->removed) {
        return;
    }

    struct XkbBinding *xkb_binding, *xkb_binding_tmp;
    wl_list_for_each_safe(xkb_binding, xkb_binding_tmp, &seat->xkb_bindings,
                          link) {
        xkb_binding_destroy(xkb_binding);
    }

    struct PointerBinding *pointer_binding, *pointer_binding_tmp;
    wl_list_for_each_safe(pointer_binding, pointer_binding_tmp,
                          &seat->pointer_bindings, link) {
        pointer_binding_destroy(pointer_binding);
    }

    river_seat_v1_destroy(seat->obj);
    wl_list_remove(&seat->link);
    free(seat);
}

void seat_focus(struct Seat *seat, struct Window *window) {
    // Focus the top window (if any) when there is no explicit target.
    if (window == NULL && !wl_list_empty(&wm.windows)) {
        window = wl_container_of(wm.windows.prev, window, link);
    }

    if (seat->focused == window) {
        return;
    }

    if (window != NULL) {
        river_seat_v1_focus_window(seat->obj, window->obj);
        river_node_v1_place_top(window->node);
        wl_list_remove(&window->link);
        wl_list_insert(wm.windows.prev, &window->link);
    } else {
        river_seat_v1_clear_focus(seat->obj);
    }

    seat->focused = window;
}

void seat_pointer_move(struct Seat *seat, struct Window *window) {
    seat_focus(seat, window);
    river_seat_v1_op_start_pointer(seat->obj);
    seat->op = SEAT_OP_MOVE;
    seat->op_window = window;
    seat->op_start_x = window->x;
    seat->op_start_y = window->y;
    seat->op_dx = 0;
    seat->op_dy = 0;
}

void seat_pointer_resize(struct Seat *seat, struct Window *window,
                         uint32_t edges) {
    seat_focus(seat, window);
    river_window_v1_inform_resize_start(window->obj);
    river_seat_v1_op_start_pointer(seat->obj);
    seat->op = SEAT_OP_RESIZE;
    seat->op_window = window;
    seat->op_edges = edges;
    seat->op_start_x = window->x;
    seat->op_start_y = window->y;
    seat->op_start_width = window->width;
    seat->op_start_height = window->height;
    seat->op_dx = 0;
    seat->op_dy = 0;
}

void seat_action(struct Seat *seat, enum Action action) {
    switch (action) {
    case ACTION_NONE:
        break;
    case ACTION_SPAWN_SH:
        if (fork() == 0) {
            execlp("/bin/sh", "/bin/sh", "-c", seat->pending_cmd, (char *)0);
        }
        break;
    case ACTION_CLOSE:
        if (seat->focused != NULL) {
            river_window_v1_close(seat->focused->obj);
        }
        break;
    case ACTION_FOCUS_NEXT:
        if (!wl_list_empty(&wm.windows)) {
            // Focus the bottom window
            struct Window *window =
                wl_container_of(wm.windows.next, window, link);
            seat_focus(seat, window);
        }
        break;
    case ACTION_MOVE:
        if (seat->op == SEAT_OP_NONE && seat->hovered != NULL) {
            seat_pointer_move(seat, seat->hovered);
        }
        break;
    case ACTION_RESIZE:
        if (seat->op == SEAT_OP_NONE && seat->hovered != NULL) {
            seat_pointer_resize(seat, seat->hovered,
                                RIVER_WINDOW_V1_EDGES_BOTTOM |
                                    RIVER_WINDOW_V1_EDGES_RIGHT);
        }
        break;
    case ACTION_EXIT:
        river_window_manager_v1_exit_session(window_manager_v1);
        break;
    case ACTION_FULLSCREEN:
        if (seat->focused->fullscreen) {
            fprintf(stdout, "INFO: Leaving fullscreen\n");
            seat_fullscreen_unrequest(seat->focused);
        } else {
            fprintf(stdout, "INFO: Entering fullscreen\n");
            seat_fullscreen_request(seat->focused, get_focused_output());
        }
        break;
    }
}

void fallback_pointerbinds(struct Seat *seat) {
    // Set the fallback pointer binds

    fprintf(stderr, "WARNING: falling back to sane default pointer binds.\n");
    const uint32_t super = RIVER_SEAT_V1_MODIFIERS_MOD4;
    pointer_binding_create(seat, super, BTN_LEFT, ACTION_MOVE);
    pointer_binding_create(seat, super, BTN_RIGHT, ACTION_RESIZE);
}

void fallback_keybinds(struct Seat *seat) {
    // Set the fallback key binds

    fprintf(stderr, "WARNING: falling back to sane default keybinds.\n");
    const uint32_t super = RIVER_SEAT_V1_MODIFIERS_MOD4;
    xkb_binding_create(seat, super, XKB_KEY_Return, ACTION_SPAWN_SH, "foot");
    xkb_binding_create(seat, super, XKB_KEY_d, ACTION_SPAWN_SH,
                       "rofi -show drun");
    xkb_binding_create(seat, super, XKB_KEY_q, ACTION_CLOSE, NULL);
    xkb_binding_create(seat, super, XKB_KEY_n, ACTION_FOCUS_NEXT, NULL);
    xkb_binding_create(seat, super, XKB_KEY_Escape, ACTION_EXIT, NULL);
}

void seat_handle_new(struct Seat *seat) {
    seat->new = false;

    // destroy all keybinds first if they exist
    struct XkbBinding *xkb_binding, *xkb_binding_tmp;
    wl_list_for_each_safe(xkb_binding, xkb_binding_tmp, &seat->xkb_bindings,
                          link) {
        xkb_binding_destroy(xkb_binding);
    }

    struct PointerBinding *pointer_binding, *pointer_binding_tmp;
    wl_list_for_each_safe(pointer_binding, pointer_binding_tmp,
                          &seat->pointer_bindings, link) {
        pointer_binding_destroy(pointer_binding);
    }

    load_config();
    // set pointer bindings
    if (keybind_config.keybinds != NULL) {
        for (size_t i = 0; i < keybind_config.keybinds_len; i++) {
            fprintf(stdout, "INFO: Adding keybind str: %s\n",
                    keybind_config.keybinds[i]);
            parse_and_add_keybind(keybind_config.keybinds[i], seat);
            free(keybind_config.keybinds[i]);
        }
        free(keybind_config.keybinds); // free the array of char*
        keybind_config.keybinds = NULL;
    } else {
        fallback_keybinds(seat);
    }

    if (pointer_config.pointerbinds != NULL) {
        for (size_t i = 0; i < pointer_config.pointerbinds_len; i++) {
            parse_and_add_pointerbind(pointer_config.pointerbinds[i], seat);
            free(pointer_config.pointerbinds[i]);
        }
        free(pointer_config.pointerbinds); // free the array of char*
        pointer_config.pointerbinds = NULL;
    } else {
        fallback_pointerbinds(seat);
    }
}

void seat_manage(struct Seat *seat) {
    if (seat->new) {
        seat_handle_new(seat);
    }

    // If no window was interacted with in the current manage sequence,
    // intentionally pass NULL to ensure the window on top has focus.
    // This is necessary to handle new windows for example.
    seat_focus(seat, seat->interacted);
    seat->interacted = NULL;

    seat_action(seat, seat->pending_action);
    seat->pending_action = ACTION_NONE;

    switch (seat->op) {
    case SEAT_OP_NONE:
        break;
    case SEAT_OP_MOVE:
        if (seat->op_release) {
            river_seat_v1_op_end(seat->obj);
            seat->op = SEAT_OP_NONE;
            seat->op_window = NULL;
            break;
        }
        break;
    case SEAT_OP_RESIZE:
        if (seat->op_release) {
            river_window_v1_inform_resize_end(seat->op_window->obj);
            river_seat_v1_op_end(seat->obj);
            seat->op = SEAT_OP_NONE;
            seat->op_window = NULL;
            break;
        }
        int32_t width = seat->op_start_width;
        int32_t height = seat->op_start_height;
        if ((seat->op_edges & RIVER_WINDOW_V1_EDGES_LEFT) != 0) {
            width -= seat->op_dx;
        }
        if ((seat->op_edges & RIVER_WINDOW_V1_EDGES_RIGHT) != 0) {
            width += seat->op_dx;
        }
        if ((seat->op_edges & RIVER_WINDOW_V1_EDGES_TOP) != 0) {
            height -= seat->op_dy;
        }
        if ((seat->op_edges & RIVER_WINDOW_V1_EDGES_BOTTOM) != 0) {
            height += seat->op_dy;
        }
        river_window_v1_propose_dimensions(seat->op_window->obj,
                                           width > 1 ? width : 1,
                                           height > 1 ? height : 1);
        break;
    }
    seat->op_release = false;
}

void seat_render(struct Seat *seat) {
    // Move and resize stuff, river did this for us.
    switch (seat->op) {
    case SEAT_OP_NONE:
        break;
    case SEAT_OP_MOVE:
        window_set_position(seat->op_window, seat->op_start_x + seat->op_dx,
                            seat->op_start_y + seat->op_dy);
        break;
    case SEAT_OP_RESIZE:;
        int32_t x = seat->op_start_x;
        int32_t y = seat->op_start_y;
        if ((seat->op_edges & RIVER_WINDOW_V1_EDGES_LEFT) != 0) {
            x += seat->op_start_width - seat->op_window->width;
        }
        if ((seat->op_edges & RIVER_WINDOW_V1_EDGES_TOP) != 0) {
            y += seat->op_start_height - seat->op_window->height;
        }
        window_set_position(seat->op_window, x, y);
        break;
    }
}
