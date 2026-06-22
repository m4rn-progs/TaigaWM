-- imports
local posix = require("posix")

-- local imports
local xkbcommon = require("..include.xkbcommon")
local table_helpers = require("table_helpers")
local globals = require("globals")
local config = require("config")
local wau = require("wau")
wau:require("..include.protocol.river-layer-shell-v1")

local m = {}
-- define the seat
m.Seat = { mt = {}, listener = {} }
m.Seat.mt.__index = m.Seat

-- create the seat
function m.Seat.create(obj)
	local seat = {
		obj = obj,
		new = true,
		xkb_bindings = {},
		pointer_bindings = {},
	}
	setmetatable(seat, m.Seat.mt)
	obj:set_user_data(seat)
	obj:add_listener(m.Seat.listener)
	return seat
end

-- seat focus request
function m.Seat:focus(window_local)
    local wm = require("wm")
	if window_local == nil and #wm.wm.windows > 0 then
		-- Fall back to topmost window
	    window_local = wm.wm.windows[#wm.wm.windows]
	end

	if window_local then
		if self.focused ~= window_local then
			self.obj:focus_window(window_local.obj)
			self.focused = window_local
			-- Move to top
			local i = table_helpers.table_index_of(wm.wm.windows, window_local)
			table.remove(wm.wm.windows, i)
			table.insert(wm.wm.windows, window_local)
			window_local.node:place_top()
		end
	else
		self.obj:clear_focus()
		self.focused = nil
	end
end

-- seat pointer move request
function m.Seat:pointer_move(window_local)
	if self.op == nil then
		self:focus(window_local)
		self.obj:op_start_pointer()
		self.op = {
			type = "move",
			window = window_local,
			start = { x = window_local.x, y = window_local.y },
			dx = 0,
			dy = 0,
		}
	end
end

-- seat pointer resize request
function m.Seat:pointer_resize(window_local, edges)
	if self.op == nil then
		self:focus(window_local)
		window_local.obj:inform_resize_start()
		self.obj:op_start_pointer()
		self.op = {
			type = "resize",
			window = window_local,
			edges = edges,
			start = {
				x = window_local.x,
				y = window_local.y,
				width = window_local.width,
				height = window_local.height,
			},
			dx = 0,
			dy = 0,
		}
	end
end

-- seat action
function m.Seat:action(action)
    local wm = require("wm")
	-- if the action passed == spawn then just use just fork and exec self.arg
	if action == "spawn" then
		if posix.unistd.fork() == 0 then
			if self.arg ~= nil then
				posix.unistd.execp("/bin/sh", { "-c", self.arg })
				os.exit(0)
			end
		end
	elseif action == "close" then
		if self.focused ~= nil then
			self.focused.obj:close()
		end
	elseif action == "focus-next" then
		self:focus(wm.wm.windows[1])
	elseif action == "move" then
		if self.hovered ~= nil then
			self:pointer_move(self.hovered)
		end
	elseif action == "resize" then
		if self.hovered ~= nil then
			self:pointer_resize(self.hovered, { bottom = true, right = true })
        end
	elseif action == "exit" then
		globals.globals["river_window_manager_v1"]:exit_session()
	else
		print("ERROR: m.Seat:action: unimplemented", action)
	end
end

-- seat add pointer binding
function m.Seat:add_pointer_binding(button, mods, action)
	-- From /usr/include/linux/input-event-codes.h
	local button_code = ({ left = 0x110, right = 0x111 })[button]
	local obj = self.obj:get_pointer_binding(button_code, mods)
	local binding = { obj = obj }

	obj:add_listener({
		["pressed"] = function(_)
			self.pending_action = action
		end,
	})
	obj:enable()
	table.insert(self.pointer_bindings, binding)
end

-- seat add xkb_binding
function m.Seat:add_xkb_binding(key, mods, action, arg)
	local keysym = xkbcommon.keysym(key)
	local obj = globals.globals["river_xkb_bindings_v1"]:get_xkb_binding(self.obj, keysym, mods)
	local binding = { obj = obj }

	obj:add_listener({
		["pressed"] = function(_)
			self.pending_action = action
			-- on add keybinding, add another value called arg to the listener
			self.arg = arg
		end,
	})
	obj:enable()
	table.insert(self.xkb_bindings, binding)
end

-- seat manage
function m.Seat:manage()
    local wm = require("wm")
	if self.new then
		self.new = nil
		for _, tbl in ipairs(config.xkb_bindings) do
			self:add_xkb_binding(table.unpack(tbl))
		end

		for _, tbl in ipairs(config.pointer_bindings) do
			self:add_pointer_binding(table.unpack(tbl))
        end

        if not config.config_reload then
            self.layer_shell_seat = globals.globals["river_layer_shell_v1"]:get_seat(self.obj)
            self.layer_shell_seat:add_listener({
                ["focus_none"] = function(_)
                    --rofi closed and dropped "focus-none", so we need to catch it and take away focus from it
                    self.focused = nil
                end,
            })
        end
    end
    -- we do this because when config file is reloaded it tries to call this code again but it cant because
    -- a seat already exists so we check that we arent reloading first

	if self.focused and self.focused.closed then
		self.focused = nil
		if #wm.wm.windows == 0 then
			self:focus(nil)
		end
	end

	if self.interacted then
		self:focus(self.interacted)
		self.interacted = nil
	elseif #wm.wm.windows > 0 then
		local topmost = wm.wm.windows[#wm.wm.windows]
		if self.focused ~= topmost then
			self:focus(topmost)
		end
	end

	if self.pending_action then
		self:action(self.pending_action)
		self.pending_action = nil
	end

	if self.op and self.op.window then
		local op, window_local = self.op, self.op.window

		if window_local.closed then
			self.obj:op_end()
			self.op = nil
		elseif self.op_release then
			if op.type == "resize" then
				window_local.obj:inform_resize_end()
			end
			self.obj:op_end()
			self.op = nil
		elseif op.type == "resize" then
			local width = math.max(
				1,
				op.edges.left and (op.start.width - op.dx)
					or op.edges.right and (op.start.width + op.dx)
					or op.start.width
			)
			local height = math.max(
				1,
				op.edges.top and (op.start.height - op.dy)
					or op.edges.bottom and (op.start.height + op.dy)
					or op.start.height
			)
			window_local.obj:propose_dimensions(width, height)
		end
	end

	self.op_release = nil
end

function m.Seat:render()
	if self.op and self.op.window then
		local op, window_local = self.op, self.op.window
		if self.op.type == "move" then
			window_local:set_position(op.start.x + op.dx, op.start.y + op.dy)
		elseif self.op.type == "resize" then
			local x = op.edges.left and (op.start.x + (op.start.width - window_local.width)) or op.start.x
			local y = op.edges.top and (op.start.y + (op.start.height - window_local.height)) or op.start.y
			window_local:set_position(x, y)
		end
	end
end

function m.Seat:maybe_destroy()
	if self.removed then
		for _, binding in ipairs(self.xkb_bindings) do
			binding.obj:destroy()
		end
		for _, binding in ipairs(self.pointer_bindings) do
			binding.obj:destroy()
		end
		self.obj:destroy()
	else
		return self
	end
end

function m.Seat.listener.removed(self)
	self:get_user_data().removed = true
end
function m.Seat.listener.pointer_enter(self, window_local)
	self:get_user_data().hovered = window_local:get_user_data()
end
function m.Seat.listener.pointer_leave(self)
	self:get_user_data().hovered = nil
end
function m.Seat.listener.window_interaction(self, window_local)
	self:get_user_data().interacted = window_local:get_user_data()
end
function m.Seat.listener.op_delta(self, dx, dy)
	local seat = self:get_user_data()
	seat.op.dx = dx
	seat.op.dy = dy
end
function m.Seat.listener.op_release(self)
	self:get_user_data().op_release = true
end

return m
