local posix = require("posix")
local signal = require("posix.signal")
local inotify = require("inotify")
local wau = require("wau")

wau:require("..include.protocol.river-window-management-v1")
wau:require("..include.protocol.river-libinput-config-v1")

local autostart = require("autostart")
local m = {}

-- config file related functions
-- just expand a titlde using gsub setting it to $HOME
function m.expand_tilde(path)
	if not path then
		return path
	end
	local home = os.getenv("HOME")
	return path:gsub("^~", home)
end

-- check if a file exists
function m.file_exists(path)
	path = m.expand_tilde(path)
	local stat = posix.stat(path)
	if stat then
		return true
	end
	return false
end

-- check 3 paths for the config file, else return nil
function m.get_config_file()
	local paths = {
		"~/.config/taiga/taigarc.lua",
		"~/.taigarc.lua",
		"/etc/taiga/taigarc.lua",
	}
	-- just loop over the table and check if any exist, while expanding ~
	for _, p in ipairs(paths) do
		if m.file_exists(p) then
			print("INFO: Using config file: " .. p .. ".")
			return m.expand_tilde(p)
		end
	end
	return nil
end

-- open the config file
function m.open_config()
	-- try to get a file, if we dont get one we fallback to cwd/taigarc.lua
	CONFIG_FILE_PATH = m.get_config_file()
	if CONFIG_FILE_PATH == nil then
		print("INFO: Using backup config file.")
		CONFIG_FILE_PATH = posix.getcwd() .. "/taigarc.lua"
		-- if we still cant find a fall back, return nil
		if not m.file_exists(CONFIG_FILE_PATH) then
			print("WARNING: No config file found.")
			return nil
		end
	end

	-- else, we will just load it
	local chunk, err = loadfile(CONFIG_FILE_PATH, "t")
	if not chunk then
		error("ERROR: " .. tostring(err))
	end
	chunk()
	print("INFO: Successfully loaded config file.")
	return true
end

-- setup lua inofity to watch for a file change
function m.watch_config_changes()
	if posix.unistd.fork() == 0 then
		local parent_pid = posix.unistd.getppid()
		local handle = inotify.init()
		local config_dir = CONFIG_FILE_PATH:match("(.*)/")
		local config_name = CONFIG_FILE_PATH:match(".*/(.*)")
		local _ = handle:addwatch(config_dir, inotify.IN_CLOSE_WRITE | inotify.IN_MOVED_TO)
		for ev in handle:events() do
			if ev.name == config_name then
				print("INFO: Config file changed, reloading.")
				posix.signal.kill(parent_pid, signal.SIGUSR1)
			end
		end
		os.exit(0)
	end
end

-- config variables
Mods = wau.river_seat_v1.Modifiers
local libinput = wau.river_libinput_device_v1

m.default_mod = Mods.MOD1

m.default_keybinds = {
	{ "Escape", m.default_mod, "exit" },
	{ "space", m.default_mod, "spawn", "foot" },
}
m.default_misc_settings = {
	vsync = true,
}
m.default_input_config = {
	accel_profile = libinput.AccelProfile.ADAPTIVE,
	accel_speed = { 0.0 },
	natural_scroll = libinput.NaturalScrollState.DISABLED,
	left_handed = libinput.LeftHandedState.DISABLED,
}

m.no_config = false
m.config_reload = false
CONFIG_KEYBINDS = {}
CONFIG_AUTOSTART = {}
CONFIG_MISC = {}
m.xkb_bindings = {}
m.pointer_bindings = {}
m.user_inputs = {}
m.misc_config = {}

function m.init()
	m.taigarc = m.open_config()
	if m.taigarc == nil then
		m.no_config = true
	end

	if not m.no_config then
		m.watch_config_changes()

		m.xkb_bindings = CONFIG_KEYBINDS().keyboard_binds
		m.pointer_bindings = CONFIG_KEYBINDS().mouse_binds
		m.misc_config = CONFIG_MISC()
		m.custom_inputs = CONFIG_LIBINPUT(libinput)

		m.user_inputs = {
			accel_profile = m.custom_inputs.accel_profile,
			accel_speed = m.custom_inputs.accel_speed,
			natural_scroll = m.custom_inputs.natural_scroll,
			left_handed = m.custom_inputs.left_handed,
		}

		local autostart_tbl = CONFIG_AUTOSTART()
		autostart.autostart(autostart_tbl)
	else
		m.xkb_bindings = m.default_keybinds
		m.pointer_bindings = { {} }
		m.user_inputs = m.default_input_config
		m.misc_config = m.default_misc_settings
	end
end

return m
