term_cmd = "kitty"
menu_cmd = "rofi -show drun"
cursor_name = "breeze_cursors"
cursor_size = 24

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
    term_cmd,
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
}
