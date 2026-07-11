#include <river-window-management-v1-client-protocol.h>
#include <stdio.h>
#include <stdlib.h>

#include "output.h"
#include "seat.h"
#include "wm.h"

const struct river_output_v1_listener river_output_listener = {
    .removed = output_handle_removed,
    .wl_output = output_handle_wl_output,
    .position = output_handle_position,
    .dimensions = output_handle_dimensions,
};

void output_handle_removed(void *data, struct river_output_v1 *obj) {
    struct Output *output = data;
    output->removed = true;
}

void output_maybe_destroy(struct Output *output) {
    if (!output->removed) {
        return;
    }
    river_output_v1_destroy(output->obj);
    wl_list_remove(&output->link);
    free(output);
}

void output_handle_dimensions(void *data, struct river_output_v1 *obj,
                              int32_t width, int32_t height) {
    fprintf(stdout, "INFO: Output dimensions = %dx%d\n", width, height);
    struct Output *output = data;
    output->width = width;
    output->height = height;
}

void output_handle_wl_output(void *data, struct river_output_v1 *obj,
                             uint32_t name) {
    printf("new wloutput\n");
}
void output_handle_position(void *data, struct river_output_v1 *obj, int32_t x,
                            int32_t y) {
    fprintf(stdout, "INFO: Output position = %dx%d\n", x, y);
    struct Output *output = data;
    output->posx = x;
    output->posy = y;
}

struct Output *get_focused_output(void) {
    if (wl_list_empty(&wm.seats)) {
        return NULL;
    }

    struct Seat *seat = wl_container_of(wm.seats.next, seat, link);

    struct Output *output;
    wl_list_for_each(output, &wm.outputs, link) {
        int32_t within_x = seat->cur_ptr_posx >= output->posx &&
                           seat->cur_ptr_posx < output->posx + output->width;
        int32_t within_y = seat->cur_ptr_posy >= output->posy &&
                           seat->cur_ptr_posy < output->posy + output->height;

        if (within_x && within_y) {
            return output;
        }
    }

    return NULL;
}
