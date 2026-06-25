#ifndef LAYER_SHELL_H
#define LAYER_SHELL_H

#include <river-layer-shell-v1-client-protocol.h>


extern struct river_layer_shell_v1 *layer_shell;
extern const struct river_layer_shell_seat_v1_listener layer_shell_seat_listener;

void layer_shell_handle_focus_exclusive(void *data, struct river_layer_shell_seat_v1 *seat);
void layer_shell_handle_focus_non_exclusive(void *data, struct river_layer_shell_seat_v1 *seat);
void layer_shell_handle_focus_none(void *data, struct river_layer_shell_seat_v1 *seat);

#endif
