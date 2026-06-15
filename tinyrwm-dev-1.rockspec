-- SPDX-FileCopyrightText: © 2026 FireFly
-- SPDX-License-Identifier: 0BSD

package = "tinyrwm"
version = "dev-1"
rockspec_format = "3.0"

source = {
    url = "git+https://codeberg.org/river/tinyrwm",
}

description = {
    summary = "Minimal example window manager for river",
    homepage = "https://codeberg.org/river/tinyrwm",
    license = "0BSD",
}

dependencies = {
    "lua == 5.4",
    "cffi-lua >= 0.2.4",
    "firefly/wau",
    "luaposix",
}

external_dependencies = {
    -- runtime dependencies: ensure these are in your LD_LIBRARY_PATH
 -- XKBCOMMON = { library = "libxkbcommon.so" },
 -- WAYLAND = { library = "libwayland-client.so" },
}

build = {
    type = "command",
    build_command = [[
        for f in tinyrwm/protocol/*.xml; do
            wau-scanner <$f >${f%%.xml}.lua
        done
    ]],
    install_command = [[
        # mimic build.type == "builtin" behaviour
        install -Dm644 tinyrwm/xkbcommon.lua $(LUADIR)/tinyrwm/xkbcommon.lua
        install -Dm644 -t $(LUADIR)/tinyrwm/protocol tinyrwm/protocol/*.lua
        install -Dm755 tinyrwm.lua $(BINDIR)/tinyrwm
    ]],
}
