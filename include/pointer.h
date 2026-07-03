#ifndef POINTER_H
#define POINTER_H

#include <river-xkb-bindings-v1-client-protocol.h>
#include <wayland-util.h>

#include "actions.h"

struct PointerBinding {
    struct river_pointer_binding_v1 *obj;
    struct Seat *seat;
    enum Action action;
    struct wl_list link;
};

extern const struct river_pointer_binding_v1_listener
    river_pointer_binding_listener;

void pointer_binding_handle_pressed(void *data,
                                    struct river_pointer_binding_v1 *obj);
void pointer_binding_handle_released(void *data,
                                     struct river_pointer_binding_v1 *obj);
void pointer_binding_destroy(struct PointerBinding *binding);
void pointer_binding_create(struct Seat *seat, uint32_t mods, uint32_t button,
                            enum Action action);

#endif
