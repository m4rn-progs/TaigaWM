#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdbool.h>
#include <wayland-util.h>

struct Output {
    struct river_output_v1 *obj;
    struct river_layer_shell_output_v1 *layer_shell_output;
    bool removed;
    int32_t width;
    int32_t height;
    int32_t posx;
    int32_t posy;
    uint32_t tag_id;

    struct wl_list link; // WindowManager.outputs
};

extern const struct river_output_v1_listener river_output_listener;
void output_handle_removed(void *data, struct river_output_v1 *obj);
void output_handle_wl_output(void *data, struct river_output_v1 *obj,
                             uint32_t name);
void output_handle_position(void *data, struct river_output_v1 *obj, int32_t x,
                            int32_t y);
void output_handle_dimensions(void *data, struct river_output_v1 *obj,
                              int32_t width, int32_t height);
void output_maybe_destroy(struct Output *output);
struct Output *get_focused_output(void);
struct Output *get_output_at_position(int x, int y);
#endif
