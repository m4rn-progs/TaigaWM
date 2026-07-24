#ifndef INPUT_H
#define INPUT_H

#include <river-input-management-v1-client-protocol.h>

struct RiverInputDevice {
    struct river_input_device_v1 *device;
    char *name;
    uint32_t type;

    struct wl_list link;
};

extern struct river_input_manager_v1 *river_input_manager;
extern const struct river_input_manager_v1_listener
    river_input_manager_listener;
extern const struct river_input_device_v1_listener river_input_device_listener;

void river_input_device_handle_removed(void *data,
                                       struct river_input_device_v1 *device);
void river_input_device_handle_type(void *data,
                                    struct river_input_device_v1 *device,
                                    uint32_t type);
void river_input_device_handle_name(void *data,
                                    struct river_input_device_v1 *device,
                                    const char *name);
void river_input_device_handle_done(void *data,
                                    struct river_input_device_v1 *device);

void river_input_manager_handle_finished(
    void *data, struct river_input_manager_v1 *input_manager);
void river_input_manager_handle_input_device(
    void *data, struct river_input_manager_v1 *river_input_manager_v1,
    struct river_input_device_v1 *input_device);
#endif
