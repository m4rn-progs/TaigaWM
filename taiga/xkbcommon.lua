-- SPDX-FileCopyrightText: © 2026 FireFly
-- SPDX-License-Identifier: 0BSD

local ffi = require("cffi")

local M = {}

local raw = ffi.load("xkbcommon")

ffi.cdef [[
    typedef uint32_t xkb_keysym_t;

    enum xkb_keysym_flags {
        XKB_KEYSYM_NO_FLAGS = 0,
        XKB_KEYSYM_CASE_INSENSITIVE = (1 << 0)
    };

    xkb_keysym_t
    xkb_keysym_from_name(const char *name, enum xkb_keysym_flags flags);
]]

function M.keysym(name, flags)
    return raw.xkb_keysym_from_name(name, flags or 0)
end

return M
