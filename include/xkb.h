#ifndef XKB_H
#define XKB_H

#include <wayland-util.h>

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "actions.h"

struct XkbBinding {
	struct river_xkb_binding_v1 *obj;
	struct Seat *seat;
	enum Action action;
	char *cmd;
	struct wl_list link;
};

extern struct river_xkb_bindings_v1 *xkb_bindings_v1;
void xkb_binding_handle_pressed(void *data, struct river_xkb_binding_v1 *obj);
void xkb_binding_handle_released(void *data, struct river_xkb_binding_v1 *obj);
void xkb_binding_destroy(struct XkbBinding *binding);
void xkb_binding_create(struct Seat *seat, uint32_t mods, xkb_keysym_t keysym, enum Action action, char *cmd);

#endif
