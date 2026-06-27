#include <wayland-util.h>
#include <river-window-management-v1-client-protocol.h>
#include <stdlib.h>

#include "window.h"
#include "seat.h"
#include "wm.h"
#include "output.h"

struct river_window_manager_v1 *window_manager_v1;
const struct river_window_v1_listener river_window_listener = {
	.closed = window_handle_closed,
	.dimensions_hint = window_handle_dimensions_hint,
	.dimensions = window_handle_dimensions,
	.app_id = window_handle_app_id,
	.title = window_handle_title,
	.parent = window_handle_parent,
	.decoration_hint = window_handle_decoration_hint,
	.pointer_move_requested = window_handle_pointer_move_requested,
	.pointer_resize_requested = window_handle_pointer_resize_requested,
	.show_window_menu_requested = window_handle_show_window_menu_requested,
	.maximize_requested = window_handle_maximize_requested,
	.unmaximize_requested = window_handle_unmaximize_requested,
	.fullscreen_requested = window_handle_fullscreen_requested,
	.exit_fullscreen_requested = window_handle_exit_fullscreen_requested,
	.minimize_requested = window_handle_minimize_requested,
	.unreliable_pid = window_handle_unreliable_pid,
	.presentation_hint = window_handle_presentation_hint,
	.identifier = window_handle_identifier,
};

void window_handle_closed(void *data, struct river_window_v1 *obj) {
	struct Window *window = data;
	window->closed = true;
}

void window_handle_dimensions(
		void *data, struct river_window_v1 *obj, int32_t width, int32_t height) {
	struct Window *window = data;
	window->width = width;
	window->height = height;
}

void window_handle_pointer_move_requested(
		void *data, struct river_window_v1 *obj, struct river_seat_v1 *river_seat) {
	struct Window *window = data;
	window->pointer_move_requested = river_seat_v1_get_user_data(river_seat);
}

void window_handle_pointer_resize_requested(
		void *data, struct river_window_v1 *obj, struct river_seat_v1 *river_seat, uint32_t edges) {
	struct Window *window = data;
	window->pointer_resize_requested = river_seat_v1_get_user_data(river_seat);
	window->pointer_resize_requested_edges = edges;
}

void window_handle_fullscreen_requested(void *data, struct river_window_v1 *obj, struct river_output_v1 *river_output) {
    struct Window *window = data;
    window->fullscreen = true;
    river_window_v1_inform_fullscreen(window->obj);
}

void window_handle_exit_fullscreen_requested(void *data, struct river_window_v1 *obj) {
    struct Window *window = data;
    window->fullscreen = false;
    river_window_v1_inform_not_fullscreen(window->obj);
}

void window_handle_maximize_requested(void *data, struct river_window_v1 *obj) {
    struct Window *window = data;

    window->maximized = true;
    window->oldx = window->x;
    window->oldy = window->y;

    river_window_v1_inform_maximized(window->obj);

    struct Output *output;
    wl_list_for_each(output, &wm.outputs, link) {
        river_window_v1_propose_dimensions(window->obj, output->width, output->height);
        break;
    }
    window_set_position(window, 0, 0);
}

void window_handle_unmaximize_requested(void *data, struct river_window_v1 *obj) {
    struct Window *window = data;
    window->maximized = false;
    river_window_v1_inform_unmaximized(window->obj);
    river_window_v1_propose_dimensions(window->obj, 0, 0);
    window_set_position(window, window->oldx, window->oldy);
}

// Ignored events
void window_handle_dimensions_hint(void *data, struct river_window_v1 *obj, int32_t min_width, int32_t min_height, int32_t max_width, int32_t max_height) {}
void window_handle_app_id(void *data, struct river_window_v1 *obj, const char *app_id) {}
void window_handle_title(void *data, struct river_window_v1 *obj, const char *title) {}
void window_handle_parent(void *data, struct river_window_v1 *obj, struct river_window_v1 *parent) {}
void window_handle_decoration_hint(void *data, struct river_window_v1 *obj, uint32_t hint) {}
void window_handle_show_window_menu_requested(void *data, struct river_window_v1 *obj, int32_t x, int32_t y) {}
void window_handle_minimize_requested(void *data, struct river_window_v1 *obj) {}
void window_handle_unreliable_pid(void *data, struct river_window_v1 *obj, int32_t unreliable_pid) {}
void window_handle_presentation_hint(void *data, struct river_window_v1 *obj, uint32_t hint) {}
void window_handle_identifier(void *data, struct river_window_v1 *obj, const char *identifier) {}


void window_maybe_destroy(struct Window *window) {
	if (!window->closed) {
		return;
	}

	struct Seat *seat;
	wl_list_for_each(seat, &wm.seats, link) {
		if (seat->focused == window) {
			seat->focused = NULL;
		}
		if (seat->op_window == window) {
			river_seat_v1_op_end(seat->obj);
			seat->op = SEAT_OP_NONE;
			seat->op_window = NULL;
		}
	}

	river_window_v1_destroy(window->obj);
	wl_list_remove(&window->link);
	free(window);
}

void window_set_position(struct Window *window, int32_t x, int32_t y) {
	river_node_v1_set_position(window->node, x, y);
	window->x = x;
	window->y = y;
}

void window_manage(struct Window *window) {
	if (window->new) {
		window->new = false;
		window_set_position(window, 0, 0);
		river_window_v1_propose_dimensions(window->obj, 0, 0);
	}
	if (window->pointer_move_requested != NULL) {
	    if (window->maximized) {
			window_handle_unmaximize_requested(window, window->obj);
		}
		seat_pointer_move(window->pointer_move_requested, window);
		window->pointer_move_requested = NULL;
	}
	if (window->pointer_resize_requested != NULL) {
        if (window->maximized) {
            window_handle_unmaximize_requested(window, window->obj);
        }
		seat_pointer_resize(window->pointer_resize_requested, window,
				window->pointer_resize_requested_edges);
		window->pointer_resize_requested = NULL;
	}
}
