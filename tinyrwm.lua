#! /usr/bin/env lua
-- SPDX-FileCopyrightText: © 2026 FireFly
-- SPDX-License-Identifier: 0BSD

local wau = require("wau")
local xkbcommon = require("tinyrwm.xkbcommon")
local posix = require("posix")

wau:require("tinyrwm.protocol.river-window-management-v1")
wau:require("tinyrwm.protocol.river-xkb-bindings-v1")

local globals = {}
local required_globals = {
    ["river_window_manager_v1"] = 4,
    ["river_xkb_bindings_v1"] = 1,
}

local Mods = wau.river_seat_v1.Modifiers

local xkb_bindings = {
    {"space", Mods.MOD4, "spawn-foot"},
    {"q", Mods.MOD4, "close"},
    {"n", Mods.MOD4, "focus-next"},
    {"Escape", Mods.MOD4, "exit"},
}

local pointer_bindings = {
    {"left", Mods.MOD4, "move"},
    {"right", Mods.MOD4, "resize"},
}

local wm = {
    outputs = {},
    seats = {},
    -- Windows are kept in rendering order; last window is topmost
    windows = {},
}

local function table_index_of(tbl, sought)
    for i, v in ipairs(tbl) do
        if v == sought then return i end
    end
    return 0
end

local function table_filter_inplace(tbl, pred)
    local removed = 0
    for i=1,#tbl do
        if pred(tbl[i]) then
            tbl[i - removed] = tbl[i]
        else
            removed = removed + 1
        end
        if removed > 0 then tbl[i] = nil end
    end
    return tbl
end


---- Output ---------------------------
local Output = { mt = {}, listener = {} }
Output.mt.__index = Output

function Output.create(obj)
    local output = { obj = obj }
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


---- Window ---------------------------
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
        self:set_position(0, 0)
        self.obj:propose_dimensions(0, 0)
    end

    local move = self.pointer_move_requested
    if move ~= nil then
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
function Window.listener:dimensions(width, height)
    local window = self:get_user_data()
    window.width = width
    window.height = height
end
function Window.listener:pointer_move_requested(seat)
    self:get_user_data().pointer_move_requested = {
        seat = seat:get_user_data(),
    }
end
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


---- Seat -----------------------------
local Seat = { mt = {}, listener = {} }
Seat.mt.__index = Seat

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

function Seat:action(action)
    if action == "spawn-foot" then
        if posix.unistd.fork() == 0 then
            posix.unistd.execp("foot", {})
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

function Seat:add_pointer_binding(button, mods, action)
    -- From /usr/include/linux/input-event-codes.h
    local button_code = ({ left = 0x110, right = 0x111 })[button]
    local obj = self.obj:get_pointer_binding(button_code, mods)
    local binding = { obj = obj }

    obj:add_listener {
        ["pressed"] = function (_)
            self.pending_action = action
        end,
    }
    obj:enable()
    table.insert(self.pointer_bindings, binding)
end

function Seat:add_xkb_binding(key, mods, action)
    local keysym = xkbcommon.keysym(key)
    local obj = globals["river_xkb_bindings_v1"]:get_xkb_binding(
                    self.obj, keysym, mods)
    local binding = { obj = obj }

    obj:add_listener {
        ["pressed"] = function (_)
            self.pending_action = action
        end,
    }
    obj:enable()
    table.insert(self.xkb_bindings, binding)
end

function Seat:manage()
    if self.new then
        self.new = nil

        for _, tbl in ipairs(xkb_bindings) do
            self:add_xkb_binding(table.unpack(tbl))
        end

        for _, tbl in ipairs(pointer_bindings) do
            self:add_pointer_binding(table.unpack(tbl))
        end
    end

    if self.focused and self.focused.closed then
        self.focused = nil
    end

    self:focus(self.interacted)
    self.interacted = nil

    if self.pending_action ~= nil then
        self:action(self.pending_action)
        self.pending_action = nil
    end

    if self.op and self.op.window then
        local op, window = self.op, self.op.window
        local window = self.op.window

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
                op.edges.left and (op.start.width - op.dx) or
                op.edges.right and (op.start.width + op.dx) or
                op.start.width
            )
            local height = math.max(
                1,
                op.edges.top and (op.start.height - op.dy) or
                op.edges.bottom and (op.start.height + op.dy) or
                op.start.height
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
            window:set_position(
                op.start.x + op.dx,
                op.start.y + op.dy
            )
        elseif self.op.type == "resize" then
            local x = op.edges.left
                      and (op.start.x + (op.start.width - window.width))
                      or op.start.x
            local y = op.edges.top
                      and (op.start.y + (op.start.height - window.height))
                      or op.start.y
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


---- wm -------------------------------
local function wm_manage()
    table_filter_inplace(wm.outputs, Output.maybe_destroy)
    table_filter_inplace(wm.windows, Window.maybe_destroy)
    table_filter_inplace(wm.seats, Seat.maybe_destroy)

    for _, window in ipairs(wm.windows) do
        window:manage()
    end

    for _, seat in ipairs(wm.seats) do
        seat:manage()
    end

    globals["river_window_manager_v1"]:manage_finish()
end

local function wm_render()
    for _, seat in ipairs(wm.seats) do
        seat:render()
    end

    globals["river_window_manager_v1"]:render_finish()
end

local wm_handlers = {
    ["unavailable"] = function (self)
        io.stderr:write("another window manager is already running\n")
        os.exit(1)
    end,
    ["finished"] = function (self)
        os.exit(0)
    end,
    ["manage_start"] = wm_manage,
    ["render_start"] = wm_render,
    ["output"] = function (self, obj)
        table.insert(wm.outputs, Output.create(obj))
    end,
    ["seat"] = function (self, obj)
        table.insert(wm.seats, Seat.create(obj))
    end,
    ["window"] = function (self, obj)
        table.insert(wm.windows, Window.create(obj))
    end,
}


---- Entry point ----------------------
display = wau.wl_display.connect()
assert(display, "Failed to connect to wayland compositor")

-- Ensure we exit nonzero if an event handler errors
local function handle_callback_error(proxy, name, func, err)
    io.stderr:write(("-- Error calling event handler for %s %q:")
                        :format(tostring(proxy), name))
    io.stderr:write(("%s\n"):format(tostring(err)))
    os.exit(1)
end
wau.wl_proxy.set_error_callback(handle_callback_error)

-- Avoid passing WAYLAND_DEBUG to our children
posix.stdlib.setenv("WAYLAND_DEBUG", nil)

-- Ensure children are automatically reaped
posix.signal.signal(posix.signal.SIGCHLD, posix.signal.SIG_IGN)

local registry = display:get_registry()
registry:add_listener {
    ["global"] = function (self, name, iface, version)
        local required_version = required_globals[iface]
        if required_version ~= nil then
            assert(required_version <= version,
                ("wayland compositor supported %s version too old (need %d, got %d)")
                    :format(iface, required_version, version))
            globals[iface] = self:bind(name, wau[iface], required_version)
        end
    end,
}

display:roundtrip()

for k in pairs(required_globals) do
    assert(globals[k] ~= nil, ("wayland compositor does not support %s"):format(k))
end

globals["river_window_manager_v1"]:add_listener(wm_handlers)

while display:dispatch() do end
