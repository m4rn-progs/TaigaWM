function get_keybinds(mod)

    return {
        keyboard_binds = {
            {"space", mod, "spawn", "kitty /usr/bin/bash"},
            {"q", mod, "close"},
            {"n", mod, "focus-next"},
            {"Escape", mod, "exit"},
        },

        mouse_binds = {
            {"left", mod, "move"},
            {"right", mod, "resize"},
        }
    }

end