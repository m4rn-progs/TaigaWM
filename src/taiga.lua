#! /usr/bin/env lua
-- SPDX-FileCopyrightText: © 2026 FireFly
-- SPDX-License-Identifier: 0BSD

-- imports
local wau = require("wau")
local posix = require("posix")
local signal = require("posix.signal")

-- local imports
local config = require("config")
local wm = require("wm")
local globals = require("globals")
local libinput = require("libinput")

-- required protocols
wau:require("..include.protocol.river-xkb-bindings-v1")

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
posix.stdlib.setenv("WAYLAND_DEBUG", nil)
posix.signal.signal(posix.signal.SIGCHLD, posix.signal.SIG_IGN)

-- Registry
local registry = DISPLAY:get_registry()
registry:add_listener({
	["global"] = function(self, name, iface, version)
		local required_version = globals.required_globals[iface]
		if required_version ~= nil then
			assert(
				required_version <= version,
				("wayland compositor supported %s version too old (need %d, got %d)"):format(
					iface,
					required_version,
					version
				)
			)
			globals.globals[iface] = self:bind(name, wau[iface], required_version)
		end
	end,
})

DISPLAY:roundtrip()

for k in pairs(globals.required_globals) do
	assert(globals.globals[k] ~= nil, ("wayland compositor does not support %s"):format(k))
end

globals.globals["river_window_manager_v1"]:add_listener(wm.wm_handlers)
globals.globals["river_libinput_config_v1"]:add_listener(libinput.libinput_handlers)

signal.signal(signal.SIGUSR1, function()
	config.init()

	for _, seat_local in ipairs(wm.wm.seats) do
		config.config_reload = true
		seat_local.new = true
		for _, binding in ipairs(seat_local.xkb_bindings) do
			binding.obj:destroy()
		end
		for _, binding in ipairs(seat_local.pointer_bindings) do
			binding.obj:destroy()
		end
		seat_local.xkb_bindings = {}
		seat_local.pointer_bindings = {}
	end

	config.watch_config_changes()
end)

-- init the config and start the compositor main loop
config.init()

while DISPLAY:dispatch() do
end
