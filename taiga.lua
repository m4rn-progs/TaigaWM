#! /usr/bin/env lua
-- SPDX-FileCopyrightText: © 2026 FireFly
-- SPDX-License-Identifier: 0BSD

-- imports
local wau = require("wau")
local xkbcommon = require("taiga.xkbcommon")
local posix = require("posix")
local signal = require("posix.signal")
local inotify = require("inotify")

-- required protocols
wau:require("taiga.protocol.river-window-management-v1")
wau:require("taiga.protocol.river-xkb-bindings-v1")
wau:require("taiga.protocol.river-layer-shell-v1")
wau:require("taiga.protocol.river-libinput-config-v1")

-- TOPLEVEL VARIABLE DEF
local globals = {}
local required_globals = {
	["river_window_manager_v1"] = 4,
	["river_xkb_bindings_v1"] = 1,
	["river_layer_shell_v1"] = 1,
	["river_libinput_config_v1"] = 1,
}
local Mods = wau.river_seat_v1.Modifiers
local default_mod = Mods.MOD1
local libinput = wau.river_libinput_device_v1

NO_CONFIG = false
CONFIG_FILE_RELOAD = false
DEFAULT_KEYBINDS = {
	{ "Escape", default_mod, "exit" },
	{ "space", default_mod, "spawn", "foot" },
}

-- a small gentle reminder of the horseshit that goes on in xml world: case changes
-- wayland xml binders, when changing stuff from xml to programming languages like lua or c,
-- will default to turning snake_case variables into PascalCase variables to try to match language conventions
-- (even though languages like rust have snake_case by default its really dumb)
--
-- therefore, you will see accel_profile in the xml files, but I typed it AccelProfile (and everything else)
-- because I was trying to dodge the translations. If I didn't do so, lua would have assumed that i was trying to access a nil field
-- and the server would hang.
local default_input_config = {
	accel_profile = libinput.AccelProfile.ADAPTIVE,
	accel_speed = { 0.0 },
	natural_scroll = libinput.NaturalScrollState.DISABLED,
	left_handed = libinput.LeftHandedState.DISABLED,
}

CONFIG_KEYBINDS = {}
CONFIG_AUTOSTART = {}
local xkb_bindings = {}
local pointer_bindings = {}
local user_inputs = {}

-- autostart related functions
-- basically just loop through a table and exec each item in a fork
local function autostart(tbl)
	for _, item in ipairs(tbl) do
		if posix.unistd.fork() == 0 then
			posix.unistd.execp("/bin/sh", { "-c", item })
			os.exit(0)
		end
	end
end

-- config file related functions
-- just expand a tilde using gsub setting it to $HOME
local function expand_tilde(path)
	if not path then
		return path
	end
	local home = os.getenv("HOME")
	return path:gsub("^~", home)
end

-- check if a file exists
local function file_exists(path)
	path = expand_tilde(path)
	local stat = posix.stat(path)
	if stat then
		return true
	end
	return false
end

-- check 3 paths for the config file, else return nil
local function get_config_file()
	local paths = {
		"~/.config/taiga/taigarc.lua",
		"~/.taigarc.lua",
		"/etc/taiga/taigarc.lua",
	}
	-- just loop over the table and check if any exist, while expanding ~
	for _, p in ipairs(paths) do
		if file_exists(p) then
			print("Path: " .. p .. " exists.")
			return expand_tilde(p)
		end
	end
	return nil
end

-- open the config file
local function open_config()
	-- try to get a file, if we dont get one we fallback to cwd/taigarc.lua
	CONFIG_FILE_PATH = get_config_file()
	if CONFIG_FILE_PATH == nil then
		print("Using backup local config file.")
		CONFIG_FILE_PATH = posix.getcwd() .. "/taigarc.lua"
		-- if we still cant find a fall back, return nil
		if not file_exists(CONFIG_FILE_PATH) then
			print("WARNING! No config file found.")
			return nil
		end
	end

	-- else, we will just load it
	local chunk, err = loadfile(CONFIG_FILE_PATH, "t")
	if not chunk then
		error("load error: " .. tostring(err))
	end
	chunk()
	print("Successfully loaded config file.")
	return true
end

-- setup lua inofity to watch for a file change
local function watch_config_changes()
	if posix.unistd.fork() == 0 then
		local parent_pid = posix.unistd.getppid()
		local handle = inotify.init()
		local config_dir = CONFIG_FILE_PATH:match("(.*)/")
		local config_name = CONFIG_FILE_PATH:match(".*/(.*)")
		local _ = handle:addwatch(config_dir, inotify.IN_CLOSE_WRITE | inotify.IN_MOVED_TO)
		for ev in handle:events() do
			if ev.name == config_name then
				print("config file changed, reloading")
				posix.signal.kill(parent_pid, signal.SIGUSR1)
			end
		end
		os.exit(0)
	end
end

-- ENTRY POINT 2.0
TAIGARC = open_config()

-- if the config file doesnt exist set the flag so we dont watch it
if TAIGARC == nil then
	NO_CONFIG = true
end

-- setup inotify to run this all the time

if not NO_CONFIG then
	watch_config_changes()
	-- config exists just read it like normal and do stuff
	xkb_bindings = CONFIG_KEYBINDS().keyboard_binds or DEFAULT_KEYBINDS
	pointer_bindings = CONFIG_KEYBINDS().mouse_binds or {}
	local custom_inputs = {}
	if type(CONFIG_LIBINPUT) == "function" then
		custom_inputs = CONFIG_LIBINPUT(libinput) or {}
	end
	user_inputs = {
		accel_profile = custom_inputs.accel_profile or default_input_config.accel_profile,
		accel_speed = custom_inputs.accel_speed or default_input_config.accel_speed,
		natural_scroll = custom_inputs.natural_scroll or default_input_config.natural_scroll,
		left_handed = custom_inputs.left_handed or default_input_config.left_handed,
	}

	local autostart_tbl = CONFIG_AUTOSTART()
	autostart(autostart_tbl)
else
	-- if no config, set to defaults
	xkb_bindings = DEFAULT_KEYBINDS
	pointer_bindings = { {} }
	user_inputs = default_input_config
end
-- autostart

local wm = {
	outputs = {},
	seats = {},
	-- Windows are kept in rendering order; last window is topmost
	windows = {},
	layers = { outputs = {}, seats = {} },
}

local function table_index_of(tbl, sought)
	for i, v in ipairs(tbl) do
		if v == sought then
			return i
		end
	end
	return 0
end

local function table_filter_inplace(tbl, pred)
	local removed = 0
	for i = 1, #tbl do
		if pred(tbl[i]) then
			tbl[i - removed] = tbl[i]
		else
			removed = removed + 1
		end
		if removed > 0 then
			tbl[i] = nil
		end
	end
	return tbl
end

---- Output ---------------------------
local Output = { mt = {}, listener = {} }
Output.mt.__index = Output

function Output:manage()
	if self.new then
		self.new = nil
		self.layer_shell_obj = globals["river_layer_shell_v1"]:get_output(self.obj)
		if self == wm.outputs[1] then
			self.layer_shell_obj:set_default()
        end
        self.obj:set_presentation_mode(1)
	end
end
function Output.create(obj)
	local output = {
		obj = obj,
		new = true,
	}
	setmetatable(output, Output.mt)
	obj:set_user_data(output)
	obj:add_listener(Output.listener)
	return output
end

function Output:maybe_destroy()
	if self.removed then
		self.obj:destroy()
	else
		return self
	end
end

function Output.listener:removed()
	self:get_user_data().removed = true
end

-- get output height and stuff
function Output.listener:dimensions(width, height)
	local output = self:get_user_data()
	output.width = width
	output.height = height
end

-- WINDOW SECTION
local Window = { mt = {}, listener = {} }
Window.mt.__index = Window

function Window.create(obj)
	local window = {
		obj = obj,
		node = obj:get_node(),
		new = true,
	}
	setmetatable(window, Window.mt)
	obj:set_user_data(window)
	obj:add_listener(Window.listener)
	return window
end

function Window:maybe_destroy()
	if self.closed then
		self.obj:destroy()
		self.node:destroy()
	else
		return self
	end
end

function Window:manage()
	if self.new then
		self.new = nil
        local half_width = wm.outputs[1].width // 4
        local half_height = wm.outputs[1].height // 4
		self:set_position(half_width, half_height)
		self.obj:propose_dimensions(0, 0)
		self.obj:set_capabilities(14)
	end

	local move = self.pointer_move_requested
	if move ~= nil then
		if IS_MAXIMIZED then
			IS_MAXIMIZED = false
			self.obj:inform_unmaximized()
			self.obj:propose_dimensions(0, 0)
		end
		self.pointer_move_requested = nil
		move.seat:pointer_move(self)
	end

	local resize = self.pointer_resize_requested
	if resize ~= nil then
		self.pointer_resize_requested = nil
		resize.seat:pointer_resize(self, resize.edges)
	end
end

function Window:set_position(x, y)
	self.node:set_position(x, y)
	self.x = x
	self.y = y
end

function Window.listener:closed()
	self:get_user_data().closed = true
end

-- dimensions_hint handling
function Window.listener:dimensions_hint(width, height)
	local window = self:get_user_data()
	window.width = width
	window.height = height
end

-- Maximize request handling
function Window.listener:maximize_requested()
	local window = self:get_user_data()
	IS_MAXIMIZED = true
	window.obj:inform_maximized()
	window.obj:propose_dimensions(wm.outputs[1].width, wm.outputs[1].height)
	window:set_position(0, 0)
end

-- unmaximize request
function Window.listener:unmaximize_requested()
	local window = self:get_user_data()
	IS_MAXIMIZED = false
	window.obj:inform_unmaximized()
	window.obj:propose_dimensions(0, 0)
end

-- window dimensions request
function Window.listener:dimensions(width, height)
	local window = self:get_user_data()
	window.width = width
	window.height = height
end

-- pointer move requiest
function Window.listener:pointer_move_requested(seat)
	self:get_user_data().pointer_move_requested = {
		seat = seat:get_user_data(),
	}
end

-- pointer resize request
function Window.listener:pointer_resize_requested(seat, edges)
	local Edges = wau.river_window_v1.Edges
	self:get_user_data().pointer_resize_requested = {
		seat = seat:get_user_data(),
		edges = {
			left = (edges & Edges.LEFT) ~= 0,
			right = (edges & Edges.RIGHT) ~= 0,
			top = (edges & Edges.TOP) ~= 0,
			bottom = (edges & Edges.BOTTOM) ~= 0,
		},
	}
end

-- SEAT SECTION --
-- define the seat
local Seat = { mt = {}, listener = {} }
Seat.mt.__index = Seat

-- create the seat
function Seat.create(obj)
	local seat = {
		obj = obj,
		new = true,
		xkb_bindings = {},
		pointer_bindings = {},
	}
	setmetatable(seat, Seat.mt)
	obj:set_user_data(seat)
	obj:add_listener(Seat.listener)
	return seat
end

-- seat focus request
function Seat:focus(window)
	if window == nil and #wm.windows > 0 then
		-- Fall back to topmost window
		window = wm.windows[#wm.windows]
	end

	if window then
		if self.focused ~= window then
			self.obj:focus_window(window.obj)
			self.focused = window
			-- Move to top
			local i = table_index_of(wm.windows, window)
			table.remove(wm.windows, i)
			table.insert(wm.windows, window)
			window.node:place_top()
		end
	else
		self.obj:clear_focus()
		self.focused = nil
	end
end

-- seat pointer move request
function Seat:pointer_move(window)
	if self.op == nil then
		self:focus(window)
		self.obj:op_start_pointer()
		self.op = {
			type = "move",
			window = window,
			start = { x = window.x, y = window.y },
			dx = 0,
			dy = 0,
		}
	end
end

-- seat pointer resize request
function Seat:pointer_resize(window, edges)
	if self.op == nil then
		self:focus(window)
		window.obj:inform_resize_start()
		self.obj:op_start_pointer()
		self.op = {
			type = "resize",
			window = window,
			edges = edges,
			start = {
				x = window.x,
				y = window.y,
				width = window.width,
				height = window.height,
			},
			dx = 0,
			dy = 0,
		}
	end
end

-- seat action
function Seat:action(action)
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
		self:focus(wm.windows[1])
	elseif action == "move" then
		if self.hovered ~= nil then
			self:pointer_move(self.hovered)
		end
	elseif action == "resize" then
		if self.hovered ~= nil then
			self:pointer_resize(self.hovered, { bottom = true, right = true })
		end
	elseif action == "exit" then
		globals["river_window_manager_v1"]:exit_session()
	else
		print("Seat:action: unimplemented", action)
	end
end

-- seat add pointer binding
function Seat:add_pointer_binding(button, mods, action)
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
function Seat:add_xkb_binding(key, mods, action, arg)
	local keysym = xkbcommon.keysym(key)
	local obj = globals["river_xkb_bindings_v1"]:get_xkb_binding(self.obj, keysym, mods)
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
function Seat:manage()
	if self.new then
		self.new = nil

		for _, tbl in ipairs(xkb_bindings) do
			-- the table passed contains arg and action
			-- since we pass a table, as many values as the table has can be acpeted in the keybind section
			self:add_xkb_binding(table.unpack(tbl))
		end

		for _, tbl in ipairs(pointer_bindings) do
			self:add_pointer_binding(table.unpack(tbl))
		end
    end
    -- we do this because when config file is reloaded it tries to call this code again but it cant because
    -- a seat already exists so we check that we arent reloading first
    if not CONFIG_FILE_RELOAD and self.new then
        self.new = nil
		self.layer_shell_seat = globals["river_layer_shell_v1"]:get_seat(self.obj)
		self.layer_shell_seat:add_listener({
			["focus_none"] = function(_)
				--rofi closed and dropped "focus-none", so we need to catch it and take away focus from it
				self.focused = nil
			end,
		})
    end

	if self.focused and self.focused.closed then
		self.focused = nil
		if #wm.windows == 0 then
			self:focus(nil)
		end
	end

	if self.interacted then
		self:focus(self.interacted)
		self.interacted = nil
	elseif #wm.windows > 0 then
		local topmost = wm.windows[#wm.windows]
		if self.focused ~= topmost then
			self:focus(topmost)
		end
	end

	if self.pending_action then
		self:action(self.pending_action)
		self.pending_action = nil
	end

	if self.op and self.op.window then
		local op, window = self.op, self.op.window

		if window.closed then
			self.obj:op_end()
			self.op = nil
		elseif self.op_release then
			if op.type == "resize" then
				window.obj:inform_resize_end()
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
			window.obj:propose_dimensions(width, height)
		end
	end

	self.op_release = nil
end

function Seat:render()
	if self.op and self.op.window then
		local op, window = self.op, self.op.window
		if self.op.type == "move" then
			window:set_position(op.start.x + op.dx, op.start.y + op.dy)
		elseif self.op.type == "resize" then
			local x = op.edges.left and (op.start.x + (op.start.width - window.width)) or op.start.x
			local y = op.edges.top and (op.start.y + (op.start.height - window.height)) or op.start.y
			window:set_position(x, y)
		end
	end
end

function Seat:maybe_destroy()
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

function Seat.listener.removed(self)
	self:get_user_data().removed = true
end
function Seat.listener.pointer_enter(self, window)
	self:get_user_data().hovered = window:get_user_data()
end
function Seat.listener.pointer_leave(self)
	self:get_user_data().hovered = nil
end
function Seat.listener.window_interaction(self, window)
	self:get_user_data().interacted = window:get_user_data()
end
function Seat.listener.op_delta(self, dx, dy)
	local seat = self:get_user_data()
	seat.op.dx = dx
	seat.op.dy = dy
end
function Seat.listener.op_release(self)
	self:get_user_data().op_release = true
end

-- LIBINPUT SECTION --
-- the entire thing that happened inside this local device_handler block was
-- that I was trying to play a guessing game of "keyboard, trackpad or mouse".
--
-- the idea was that I'll first check for accel profiles.
-- if that exists, it is either a mouse or a trackpad.
-- if it returns tap support, its a trackpad since there is no physical way that a mouse can have tap support.
-- a keyboard would have failed both checks since accel profiles and tap support aren't a thing on keyboards,
-- so it doesn't recieve a bunch of signals that would have failed and caused the wayland display to seize up
local device_handlers = {
	["accel_profiles_support"] = function(self, profiles)
		if profiles > 0 then
			print("[Libinput] Pointer device detected.")

			local res = self:set_accel_profile(user_inputs.accel_profile)

			res:add_listener({
				["success"] = function() end,
				["unsupported"] = function() end,
				["invalid"] = function() end,
			})
		end
	end,

	["tap_support"] = function(self, finger_count)
		if finger_count > 0 then
			print("[Libinput] Trackpad detected. Overriding pointer config.")

			local res_accel = self:set_accel_profile(user_inputs.accel_profile)
			res_accel:add_listener({
				["success"] = function() end,
				["unsupported"] = function() end,
				["invalid"] = function() end,
			})

			local res_tap = self:set_tap(libinput.TapState.ENABLED)
			res_tap:add_listener({
				["success"] = function() end,
				["unsupported"] = function() end,
				["invalid"] = function() end,
			})
		end
	end,
}

local libinput_handlers = {
	["libinput_device"] = function(self, device)
		print("A new input device was detected!")
		device:add_listener(device_handlers)
	end,
}

-- WM SECTION --
-- wm manage
local function wm_manage()
	table_filter_inplace(wm.outputs, Output.maybe_destroy)
	table_filter_inplace(wm.windows, Window.maybe_destroy)
	table_filter_inplace(wm.seats, Seat.maybe_destroy)

	for _, output in ipairs(wm.outputs) do
		output:manage()
	end

	for _, window in ipairs(wm.windows) do
		window:manage()
	end

	for _, seat in ipairs(wm.seats) do
		seat:manage()
	end

	globals["river_window_manager_v1"]:manage_finish()
end

-- wm render
local function wm_render()
	for _, seat in ipairs(wm.seats) do
		seat:render()
	end

	globals["river_window_manager_v1"]:render_finish()
end

-- wm handlers
local wm_handlers = {
	["unavailable"] = function(self)
		io.stderr:write("another window manager is already running\n")
		os.exit(1)
	end,
	["finished"] = function(self)
		os.exit(0)
	end,
	["manage_start"] = wm_manage,
	["render_start"] = wm_render,
	["output"] = function(self, obj)
		table.insert(wm.outputs, Output.create(obj))
	end,
	["seat"] = function(self, obj)
		table.insert(wm.seats, Seat.create(obj))
	end,
	["window"] = function(self, obj)
		table.insert(wm.windows, Window.create(obj))
	end,
}

-- ENTRY POINT --
DISPLAY = wau.wl_display.connect()
assert(DISPLAY, "Failed to connect to wayland compositor")

-- Ensure we exit nonzero if an event handler errors
local function handle_callback_error(proxy, name, func, err)
	io.stderr:write(("-- Error calling event handler for %s %q:"):format(tostring(proxy), name))
	io.stderr:write(("%s\n"):format(tostring(err)))
	os.exit(1)
end
wau.wl_proxy.set_error_callback(handle_callback_error)

-- Avoid passing WAYLAND_DEBUG to our children
posix.stdlib.setenv("WAYLAND_DEBUG", nil)

-- Ensure children are automatically reaped
posix.signal.signal(posix.signal.SIGCHLD, posix.signal.SIG_IGN)

-- Registry
local registry = DISPLAY:get_registry()
registry:add_listener({
	["global"] = function(self, name, iface, version)
		local required_version = required_globals[iface]
		if required_version ~= nil then
			assert(
				required_version <= version,
				("wayland compositor supported %s version too old (need %d, got %d)"):format(
					iface,
					required_version,
					version
				)
			)
			globals[iface] = self:bind(name, wau[iface], required_version)
		end
	end,
})

DISPLAY:roundtrip()

-- assert that globals exist
for k in pairs(required_globals) do
	assert(globals[k] ~= nil, ("wayland compositor does not support %s"):format(k))
end

-- add listeners
globals["river_window_manager_v1"]:add_listener(wm_handlers)
-- had to add the if statement because if libinput fails (no mouse or smth) or the mouse thingy fails we can still boot the wm
if globals["river_libinput_config_v1"] then
	globals["river_libinput_config_v1"]:add_listener(libinput_handlers)
end

signal.signal(signal.SIGUSR1, function()
	TAIGARC = open_config()
	xkb_bindings = CONFIG_KEYBINDS().keyboard_binds or {}
	pointer_bindings = CONFIG_KEYBINDS().mouse_binds or {}

	-- reset everything
	for _, seat in ipairs(wm.seats) do
		-- next time seat:manage is called, with seat.new = true it will re-set it up
        CONFIG_FILE_RELOAD = true
		seat.new = true
		-- reset bindings in each seat aswell
		for _, binding in ipairs(seat.xkb_bindings) do
			binding.obj:destroy()
		end
		for _, binding in ipairs(seat.pointer_bindings) do
			binding.obj:destroy()
		end
		-- after destroying the objs, reset them to an empty table
		seat.xkb_bindings = {}
		seat.pointer_bindings = {}
	end

	-- start watching for changes again
	watch_config_changes()
end)

while DISPLAY:dispatch() do
end
