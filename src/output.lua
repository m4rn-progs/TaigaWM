local globals = require("globals")
local config = require("config")

local m = {}

m.head_listener = {
    ["name"] = function(obj, name)
        print("A monitor chose a name")
        obj:get_user_data().name = name
    end,
    ["description"] = function(obj, description)
        print("A monitor chose a description")
        obj:get_user_data().desc = description
    end,
    ["mode"] = function(obj, mode)
        print("a new mode was given")
        table.insert(obj:get_user_data().mode_objs, mode)
        mode:add_listener({
            ["size"] = function(_, width, height)
                print('in here')
                obj:get_user_data().width = width
                obj:get_user_data().height = height
            end
        })
    end
}

m.head_manager_listener = {
    ["head"] = function(_, head_obj)
        print("A new monitor is connected")

        local wm = require("wm")

        local head_local = {
            obj = head_obj,
            name = "Unknown",
            desc = "Unknown",
            mode_objs = {},
            width = 0,
            height = 0,
        }

        head_obj:set_user_data(head_local)
        head_obj:add_listener(m.head_listener)
        table.insert(wm.wm.heads, head_local)
    end,
    ["done"] = function(manager, serial)
        local wm = require("wm")
        print("Current connected monitor info: ")

        for i, head_local in ipairs(wm.wm.heads) do
            print(string.format("Head [%d]: name = %s, desc = %s, serial = %d, mode = %dx%d", i, head_local.name, head_local.desc, serial, head_local.width, head_local.height))
        end

        local zwlr_output_config = manager:create_configuration(serial)

        zwlr_output_config:add_listener({
            ["succeeded"] = function(obj) print("Config Succeeded!"); obj:destroy() end,
            ["failed"] = function(obj)
                print("COMPOSITOR REJECTED CONFIGURATION")
                obj:destroy()
            end
        })

        for _, head_local in ipairs(wm.wm.heads) do
            local zwlr_head_config = zwlr_output_config:enable_head(head_local.obj)
            zwlr_head_config:set_scale(1024)
        end

        zwlr_output_config:apply()
    end
}

m.Output = { mt = {}, listener = {} }
m.Output.mt.__index = m.Output

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
