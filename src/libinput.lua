-- imports
local wau = require("wau")

-- local imports
local config = require("config")

-- wau
wau:require("..include.protocol.river-libinput-config-v1")

local m = {}

local libinput = wau.river_libinput_device_v1
m.device_handlers = {
	["accel_profiles_support"] = function(self, profiles)
		if profiles > 0 then
			print("INFO: Libinput pointer device detected.")

			local res = self:set_accel_profile(config.user_inputs.accel_profile)

			res:add_listener({
				["success"] = function() end,
				["unsupported"] = function() end,
				["invalid"] = function() end,
			})
		end
	end,

	["tap_support"] = function(self, finger_count)
		if finger_count > 0 then
			print("INFO: Libinput trackpad detected. Overriding pointer config.")

			local res_accel = self:set_accel_profile(config.user_inputs.accel_profile)
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

m.libinput_handlers = {
	["libinput_device"] = function(self, device)
		print("INFO: New libinput device detected.")
		device:add_listener(m.device_handlers)
	end,
}

return m
