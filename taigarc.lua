local wau = require("wau")
local Mods = wau.river_seat_v1.Modifiers
local mod = Mods.MOD1

function config_keybinds()
	return {
		keyboard_binds = {
			{ "Return", mod, "spawn", "foot" },
			{ "q", mod, "close" },
			{ "n", mod, "focus-next" },
			{ "Escape", mod, "exit" },
		},

		mouse_binds = {
			{ "left", mod, "move" },
			{ "right", mod, "resize" },
		},
	}
end
function config_autostart()
	return {
        "foot",
	}
end
