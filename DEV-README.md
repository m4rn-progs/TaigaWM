# Coding style rules

- In the case of importing things do this

```lua
-- imports
local x = import("some global module")

-- local imports
local y = import("some local module that you wrote")

-- wau
wau:require("some protocol")

```

- Use 4 space indent with expand tab.
- Declare global variables or toplevel variables at the top of the files UNDER imports.
- Try to seperate code into modular files.
- Try to use as little "global" variables as possible.

# Notes

In the case that you have to require the wm module do NOT import it at the toplevel
only import it within function scope or everything will break

Example:
```lua
function m.Output:manage()
    local wm = require("wm")
	if self.new then
		self.new = nil
		self.layer_shell_obj = globals.globals["river_layer_shell_v1"]:get_output(self.obj)
		if self == wm.wm.outputs[1] then
			self.layer_shell_obj:set_default()
        end

        -- presentation mode
        if not config.misc_config.vsync then
            print("INFO: Vsync disabled.")
            self.obj:set_presentation_mode(1)
        else
            print("INFO: Vsync enabled.")
            self.obj:set_presentation_mode(0)
        end

	end
end
```

