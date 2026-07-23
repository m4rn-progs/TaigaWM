term_cmd = "kitty"
menu_cmd = "rofi -show drun"
cursor_name = "breeze_cursors"
cursor_size = 24

-- edit this to configure your monitors
-- note, if you have more than 1 output, specify ALL of them.
output_cmd = "wlr-randr --output DP-1 --mode 1920x1080@60 --scale 1.0"
wallpaper_cmd = "swaybg -m fill -i /usr/share/boreal-artwork/boreal-default-dark-wallpaper.jpg"

-- mods+othermods key action <optional command>
Keybinds = {
    "super return spawn " .. term_cmd,
    "super d spawn " .. menu_cmd,
    "super q killactive",
    "super+shift e exit",
    "none f11 fullscreen"
}

-- mods+othermods mouse_button action
Pointerbinds = {
    "alt left_click move",
    "alt right_click resize",
}

-- autostart cmds
Autostart = {
    output_cmd,
    wallpaper_cmd,
    term_cmd,
}

Xkb = {
    layout = "us",
    variant = ""
}
-- key = value
Libinput = {
    tap_to_click = true,
    accel_profile = "flat",
}

-- key = value
Misc = {
    tearing = false,
    xcursor_theme = cursor_name,
    xcursor_size = cursor_size,
    border_size = 5,
    -- colors are defined in HEX including alpha
    focused_border_color_hex = 0xFFFFFFFF,
    unfocused_border_color_hex = 0x000000FF
}
