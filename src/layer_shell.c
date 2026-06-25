#include <wayland-util.h>
#include <stdio.h>
#include <river-layer-shell-v1-client-protocol.h>

#include "layer_shell.h"
#include "seat.h"
#include "wm.h"

struct river_layer_shell_v1 *layer_shell;

const struct river_layer_shell_seat_v1_listener layer_shell_seat_listener = {
    .focus_exclusive = layer_shell_handle_focus_exclusive,
    .focus_non_exclusive = layer_shell_handle_focus_non_exclusive,
    .focus_none = layer_shell_handle_focus_none,
};

void layer_shell_handle_focus_none(void *data, struct river_layer_shell_seat_v1 *seat) {
    struct Seat *s, *s_tmp;
    wl_list_for_each_safe(s, s_tmp, &wm.seats, link) {
        s->focused = NULL;
    }
}

void layer_shell_handle_focus_exclusive(void *data, struct river_layer_shell_seat_v1 *seat) {}
void layer_shell_handle_focus_non_exclusive(void *data, struct river_layer_shell_seat_v1 *seat) {}
