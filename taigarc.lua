-- everything here is all that is supported in the current development state.

-- mods+othermods key action <optional command>
Keybinds = {
    "super return spawn alacritty",
    "super d spawn rofi -show drun",
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
    "alacritty",
}

-- key = value
Libinput = {
    tap_to_click = true,
    accel_profile = "flat",
}

-- key = value
Misc = {
    tearing = true,
}
