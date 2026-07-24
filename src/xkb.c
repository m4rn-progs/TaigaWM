#include <river-xkb-bindings-v1-client-protocol.h>
#include <stdlib.h>
#include <string.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

#include "seat.h"
#include "xkb.h"

struct river_xkb_bindings_v1 *xkb_bindings_v1;

void xkb_binding_handle_pressed(void *data, struct river_xkb_binding_v1 *obj) {
    (void)obj;

    struct XkbBinding *binding = data;
    binding->seat->pending_action = binding->action;
    binding->seat->pending_cmd = binding->cmd;
}

void xkb_binding_handle_released(void *data, struct river_xkb_binding_v1 *obj) {
}

const struct river_xkb_binding_v1_listener river_xkb_binding_listener = {
    .pressed = xkb_binding_handle_pressed,
    .released = xkb_binding_handle_released,
};

void xkb_binding_destroy(struct XkbBinding *binding) {
    river_xkb_binding_v1_destroy(binding->obj);
    free(binding->cmd);
    wl_list_remove(&binding->link);
    free(binding);
}

void xkb_binding_create(struct Seat *seat, uint32_t mods, xkb_keysym_t keysym,
                        enum Action action, char *cmd) {
    struct XkbBinding *binding = calloc(1, sizeof(struct XkbBinding));
    binding->obj = river_xkb_bindings_v1_get_xkb_binding(
        xkb_bindings_v1, seat->obj, keysym, mods);
    binding->seat = seat;
    binding->action = action;
    if (cmd != NULL) {
        binding->cmd = strdup(cmd);
    }

    river_xkb_binding_v1_add_listener(binding->obj, &river_xkb_binding_listener,
                                      binding);
    river_xkb_binding_v1_enable(binding->obj);

    wl_list_insert(seat->xkb_bindings.prev, &binding->link);
}
