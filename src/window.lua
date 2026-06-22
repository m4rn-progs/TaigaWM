-- imports
local wau = require("wau")

-- local imports

-- wau
wau:require("..include.protocol.river-window-management-v1")

local m = {}

m.Window = { mt = {}, listener = {} }
m.Window.mt.__index = m.Window

function m.Window.create(obj)
	local window = {
		obj = obj,
		node = obj:get_node(),
		new = true,
	}
	setmetatable(window, m.Window.mt)
	obj:set_user_data(window)
	obj:add_listener(m.Window.listener)
	return window
end

function m.Window:maybe_destroy()
	if self.closed then
		self.obj:destroy()
		self.node:destroy()
	else
		return self
	end
end

function m.Window:manage()
	if self.new then
		self.new = nil

		self.obj:propose_dimensions(0, 0)
		self.obj:set_capabilities(14)
		self:set_position(0, 0)
		NOT_SPAWNED = true
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

function m.Window:set_position(x, y)
	self.node:set_position(x, y)
	self.x = x
	self.y = y
end

function m.Window.listener:closed()
	self:get_user_data().closed = true
end

-- dimensions_hint handling
function m.Window.listener:dimensions_hint(width, height)
	local window = self:get_user_data()
	window.width = width
	window.height = height
end

-- fullscreen request handling
function m.Window.listener:fullscreen_requested()
	local wm = require("wm")
	local window = self:get_user_data()
	window.obj:fullscreen(wm.wm.outputs[1].obj)
	window.obj:inform_fullscreen()
end

function m.Window.listener:exit_fullscreen_requested()
	local window = self:get_user_data()
	window.obj:exit_fullscreen()
	window.obj:inform_not_fullscreen()
end
-- Maximize request handling
function m.Window.listener:maximize_requested()
	local wm = require("wm")
	local window = self:get_user_data()
	IS_MAXIMIZED = true
	window.obj:inform_maximized()
	window.obj:propose_dimensions(wm.wm.outputs[1].width, wm.wm.outputs[1].height)
	window:set_position(0, 0)
end

-- unmaximize request
function m.Window.listener:unmaximize_requested()
	local window = self:get_user_data()
	IS_MAXIMIZED = false
	window.obj:inform_unmaximized()
	window.obj:propose_dimensions(0, 0)
end

-- window dimensions request
function m.Window.listener:dimensions(width, height)
	local window = self:get_user_data()
	window.width = width
	window.height = height
end

-- pointer move requiest
function m.Window.listener:pointer_move_requested(seat)
	self:get_user_data().pointer_move_requested = {
		seat = seat:get_user_data(),
	}
end

-- pointer resize request
function m.Window.listener:pointer_resize_requested(seat, edges)
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

return m
