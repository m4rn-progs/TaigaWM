-- toplevel variables, not needed but makes the config cleaner
term_cmd = "kitty"
menu_cmd = "rofi -show drun"
cursor_name = "breeze_cursors"
cursor_size = 24

-- edit this to configure your monitors
-- note, if you have more than 1 output, specify ALL of them.
-- if you fail to specify all of them, the layout might not work properly
output_cmd = "wlr-randr --output DP-1 --mode 1920x1080@60 --scale 1.0"
wallpaper_cmd = "swaybg -m fill -i /usr/share/boreal-artwork/boreal-default-dark-wallpaper.jpg"

-- mods+othermods key action <optional command>
Keybinds = {
    "super return spawn " .. term_cmd,
    "super d spawn " .. menu_cmd,
    "super q killactive",
    "super+shift e exit",
    "none f11 fullscreen",

    "super m tag_inc",
    "super+shift m win_tag_inc",
    "super n tag_dec",
    "super+shift n win_tag_dec",

    "super 1 tag 0",
    "super 2 tag 1",
    "super 3 tag 2",
    "super 4 tag 3",
    "super 5 tag 4",
    "super 6 tag 5",
    "super 7 tag 6",
    "super 8 tag 7",
    "super 9 tag 8",
    "super 0 tag 9",
    "super+shift 1 win_tag 0",
    "super+shift 2 win_tag 1",
    "super+shift 3 win_tag 2",
    "super+shift 4 win_tag 3",
    "super+shift 5 win_tag 4",
    "super+shift 6 win_tag 5",
    "super+shift 7 win_tag 6",
    "super+shift 8 win_tag 7",
    "super+shift 9 win_tag 8",
    "super+shift 0 win_tag 9",
}

-- mods+othermods mouse_button action
Pointerbinds = {
    "super left_click move",
    "super right_click resize",
}

-- autostart cmds
Autostart = {
    output_cmd,
    wallpaper_cmd,
    term_cmd,
}

-- key = value
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
