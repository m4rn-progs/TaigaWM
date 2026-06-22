local wm = require("wm")
local globals = require("globals")
local config = require("config")

local m = {}

m.Output = { mt = {}, listener = {} }
m.Output.mt.__index = m.Output

function m.Output:manage()
	if self.new then
		self.new = nil
		self.layer_shell_obj = globals.globals["river_layer_shell_v1"]:get_output(self.obj)
		if self == wm.outputs[1] then
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

function m.Output.create(obj)
	local output = {
		obj = obj,
		new = true,
	}
	setmetatable(output, m.Output.mt)
	obj:set_user_data(output)
	obj:add_listener(m.Output.listener)
	return output
end

function m.Output:maybe_destroy()
	if self.removed then
		self.obj:destroy()
	else
		return self
	end
end

function m.Output.listener:removed()
	self:get_user_data().removed = true
end

-- get output height and stuff
function m.Output.listener:dimensions(width, height)
	local output = self:get_user_data()
	output.width = width
	output.height = height
end

return m
