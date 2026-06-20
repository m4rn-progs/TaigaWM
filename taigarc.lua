function config_keybinds(mod)

    return {
        keyboard_binds = {
            {"space", mod, "spawn", "foot bash"},
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
function config_autostart()
        return {
                "foot",
        }
end
