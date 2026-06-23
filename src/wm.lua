local table_helpers = require("table_helpers")
local output = require("output")
local window = require("window")
local seat = require("seat")
local globals = require("globals")

local m = {}

m.wm = {
	outputs = {},
	seats = {},
	windows = {},
	layers = { outputs = {}, seats = {} },
    heads = {}
}

local function wm_manage()
	table_helpers.table_filter_inplace(m.wm.outputs, output.Output.maybe_destroy)
	table_helpers.table_filter_inplace(m.wm.windows, window.Window.maybe_destroy)
	table_helpers.table_filter_inplace(m.wm.seats, seat.Seat.maybe_destroy)

	for _, local_output in ipairs(m.wm.outputs) do
		local_output:manage()
	end

	for _, local_window in ipairs(m.wm.windows) do
		local_window:manage()
	end

	for _, seats in ipairs(m.wm.seats) do
		seats:manage()
	end

	globals.globals["river_window_manager_v1"]:manage_finish()
end

-- wm render
local function wm_render()
	for _, seat_local in ipairs(m.wm.seats) do
		seat_local:render()
	end

	globals.globals["river_window_manager_v1"]:render_finish()
end

-- wm handlers
m.wm_handlers = {
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
		table.insert(m.wm.outputs, output.Output.create(obj))
	end,
	["seat"] = function(self, obj)
		table.insert(m.wm.seats, seat.Seat.create(obj))
	end,
	["window"] = function(self, obj)
		table.insert(m.wm.windows, window.Window.create(obj))
	end,
}

return m
