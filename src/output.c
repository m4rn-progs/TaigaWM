#include <stdlib.h>
#include <river-window-management-v1-client-protocol.h>

#include "output.h"

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

void output_handle_wl_output(void *data, struct river_output_v1 *obj, uint32_t name) {}
void output_handle_position(void *data, struct river_output_v1 *obj, int32_t x, int32_t y) {}
void output_handle_dimensions(void *data, struct river_output_v1 *obj, int32_t width, int32_t height) {}
