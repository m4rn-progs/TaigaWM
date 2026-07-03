#ifndef LIBINPUT_H
#define LIBINPUT_H

#include <river-libinput-config-v1-client-protocol.h>
#include <stdint.h>

extern struct river_libinput_config_v1 *river_libinput_config;

extern const struct river_libinput_config_v1_listener
    river_libinput_config_listener;
extern const struct river_libinput_device_v1_listener
    river_libinput_device_listener;

void river_libinput_config_handle_libinput_device(
    void *data, struct river_libinput_config_v1 *config,
    struct river_libinput_device_v1 *device_id);
void river_libinput_config_handle_finished(
    void *data, struct river_libinput_config_v1 *config);
void river_libinput_device_handle_accel_profiles_support(
    void *data, struct river_libinput_device_v1 *device, uint32_t profiles);

void river_libinput_device_handle_removed(
    void *data, struct river_libinput_device_v1 *device);
void river_libinput_device_handle_input_device(
    void *data, struct river_libinput_device_v1 *device,
    struct river_input_device_v1 *input_device);
void river_libinput_device_handle_send_events_support(
    void *data, struct river_libinput_device_v1 *device, uint32_t modes);
void river_libinput_device_handle_send_events_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t mode);
void river_libinput_device_handle_send_events_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t mode);
void river_libinput_device_handle_tap_support(
    void *data, struct river_libinput_device_v1 *device, int32_t finger_count);
void river_libinput_device_handle_tap_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t state);
void river_libinput_device_handle_tap_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t state);
void river_libinput_device_handle_tap_button_map_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t button_map);
void river_libinput_device_handle_tap_button_map_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t button_map);
void river_libinput_device_handle_drag_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t state);
void river_libinput_device_handle_drag_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t state);
void river_libinput_device_handle_drag_lock_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t state);
void river_libinput_device_handle_drag_lock_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t state);
void river_libinput_device_handle_three_finger_drag_support(
    void *data, struct river_libinput_device_v1 *device, int32_t finger_count);
void river_libinput_device_handle_three_finger_drag_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t state);
void river_libinput_device_handle_three_finger_drag_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t state);
void river_libinput_device_handle_calibration_matrix_support(
    void *data, struct river_libinput_device_v1 *device, int32_t supported);
void river_libinput_device_handle_calibration_matrix_default(
    void *data, struct river_libinput_device_v1 *device,
    struct wl_array *matrix);
void river_libinput_device_handle_calibration_matrix_current(
    void *data, struct river_libinput_device_v1 *device,
    struct wl_array *matrix);
void river_libinput_device_handle_accel_profiles_support(
    void *data, struct river_libinput_device_v1 *device, uint32_t profiles);
void river_libinput_device_handle_accel_profile_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t profile);
void river_libinput_device_handle_accel_profile_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t profile);
void river_libinput_device_handle_accel_speed_default(
    void *data, struct river_libinput_device_v1 *device,
    struct wl_array *speed);
void river_libinput_device_handle_accel_speed_current(
    void *data, struct river_libinput_device_v1 *device,
    struct wl_array *speed);
void river_libinput_device_handle_natural_scroll_support(
    void *data, struct river_libinput_device_v1 *device, int32_t supported);
void river_libinput_device_handle_natural_scroll_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t state);
void river_libinput_device_handle_natural_scroll_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t state);
void river_libinput_device_handle_left_handed_support(
    void *data, struct river_libinput_device_v1 *device, int32_t supported);
void river_libinput_device_handle_left_handed_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t state);
void river_libinput_device_handle_left_handed_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t state);
void river_libinput_device_handle_click_method_support(
    void *data, struct river_libinput_device_v1 *device, uint32_t methods);
void river_libinput_device_handle_click_method_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t method);
void river_libinput_device_handle_click_method_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t method);
void river_libinput_device_handle_clickfinger_button_map_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t button_map);
void river_libinput_device_handle_clickfinger_button_map_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t button_map);
void river_libinput_device_handle_middle_emulation_support(
    void *data, struct river_libinput_device_v1 *device, int32_t supported);
void river_libinput_device_handle_middle_emulation_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t state);
void river_libinput_device_handle_middle_emulation_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t state);
void river_libinput_device_handle_scroll_method_support(
    void *data, struct river_libinput_device_v1 *device, uint32_t methods);
void river_libinput_device_handle_scroll_method_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t method);
void river_libinput_device_handle_scroll_method_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t method);
void river_libinput_device_handle_scroll_button_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t button);
void river_libinput_device_handle_scroll_button_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t button);
void river_libinput_device_handle_scroll_button_lock_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t state);
void river_libinput_device_handle_scroll_button_lock_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t state);
void river_libinput_device_handle_dwt_support(
    void *data, struct river_libinput_device_v1 *device, int32_t supported);
void river_libinput_device_handle_dwt_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t state);
void river_libinput_device_handle_dwt_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t state);
void river_libinput_device_handle_dwtp_support(
    void *data, struct river_libinput_device_v1 *device, int32_t supported);
void river_libinput_device_handle_dwtp_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t state);
void river_libinput_device_handle_dwtp_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t state);
void river_libinput_device_handle_rotation_support(
    void *data, struct river_libinput_device_v1 *device, int32_t supported);
void river_libinput_device_handle_rotation_default(
    void *data, struct river_libinput_device_v1 *device, uint32_t angle);
void river_libinput_device_handle_rotation_current(
    void *data, struct river_libinput_device_v1 *device, uint32_t angle);
void river_libinput_device_handle_done(void *data,
                                       struct river_libinput_device_v1 *device);

#endif
