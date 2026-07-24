-- This config file is written in lua. Any valid lua is valid config syntax
-- https://lua.org/pil/contents.html

-- toplevel variables, not needed but makes the config cleaner
-- key = value
term_cmd = "kitty"
menu_cmd = "rofi -show drun"
cursor_name = "breeze_cursors"
cursor_size = 24

-- edit this to configure your monitors
-- note, if you have more than 1 output, specify ALL of them.
-- if you fail to specify all of them, the layout might not work properly
output_cmd = "wlr-randr --output DP-1 --mode 1920x1080@60 --scale 1.0"

-- edit this to configure your wallpaper
wallpaper_cmd = "swaybg -m fill -i /usr/share/boreal-artwork/boreal-default-dark-wallpaper.jpg"

-- Keybind config
-- <modifier>+<other modifier> <key> <action> <command>

-- all actions
-- <spawn, kill_active, exit, focus_next, fullscreen, maximize, focus_mon_next, focus_mon_prev>
-- <tag, tag_inc, tag_dec, win_tag, win_tag_inc, win_tag_dec>
-- all actions
Keybinds = {
    -- spawn action
    -- <modifier>+<other modifier> <key> <action> <command>
    -- example 'super return spawn foot'
    -- example 'super+shift e spawn wlogout'
    "super return spawn " .. term_cmd,
    "super d spawn " .. menu_cmd,

    -- general actions
    -- <modifier>+<other modifier> <key> <action>
    "super q kill_active",
    "super+shift e exit",
    "alt tab focus_next",

    -- note: specify <none> for the <modifier> to disable needing a modifier
    "none f11 fullscreen",
    "none f10 maximize",

    -- switch outputs
    -- <modifier>+<other modifier> <key> <action>
    "super b focus_mon_next",
    "super+shift b focus_mon_prev",

    -- tag management
    -- <modifier>+<other modifier> <key> <action> <tag num>
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
    "super period tag_inc",
    "super comma tag_dec",

    -- window tag management
    -- <modifier>+<other modifier> <key> <action> <tag num>
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
    "super+shift period win_tag_inc",
    "super+shift comma win_tag_dec",
}

-- Pointer binds config
-- <modifier>+<other modifier> <mouse_button> <action>
-- example 'super left_click move'
-- all actions: <move, resize>
Pointerbinds = {
    "super left_click move",
    "super right_click resize",
}

-- Autostart config
-- Any valid string
Autostart = {
    output_cmd,
    wallpaper_cmd,
    term_cmd,
}

-- Xkb / keyboard layout config
Xkb = {
    layout = "us", -- layout name <string> <any valid layout>
    variant = "" -- variant name <string> <leave empty, any valid variant>
}

-- Input config
Input = {
    tap_to_click = true, -- enable tap to click <boolean>
    accel_profile = "flat", -- accel profile <string> <flat, adaptive, none>
    repeat_rate = 40; -- repeat rate keys-per-second <int>
    repeat_delay = 250; -- repeat delay in ms <int>
}

-- Misc config
Misc = {
    tearing = false, -- enable tearing <boolean>
    xcursor_theme = cursor_name, -- cursor theme name <string> <any>
    xcursor_size = cursor_size, -- cursor size <int>
    border_size = 5, -- border size <int>
    client_side_decorations = false, -- enable client side decorations <boolean>

    -- colors are defined in HEX including alpha
    focused_border_color_hex = 0xFFFFFFFF, -- focused border color <hex + alpha>
    unfocused_border_color_hex = 0x000000FF -- unfocused border color <hex + alpha>
}
