-- SPDX-FileCopyrightText: © 2026 FireFly
-- SPDX-License-Identifier: 0BSD

package = "taiga"
version = "dev-1"
rockspec_format = "3.0"

source = {
    url = "git+https://codeberg.org/river/",
}

description = {
    summary = "Minimal example window manager for river",
    homepage = "https://codeberg.org/river/taiga",
    license = "0BSD",
}

dependencies = {
    "lua == 5.4",
    "cffi-lua >= 0.2.4",
    "xml2lua",
    "firefly/wau",
    "luaposix",
    "inotify"
}

external_dependencies = {
    -- runtime dependencies: ensure these are in your LD_LIBRARY_PATH
 -- XKBCOMMON = { library = "libxkbcommon.so" },
 -- WAYLAND = { library = "libwayland-client.so" },
}

build = {
    type = "command",
    build_command = [[
        for f in include/protocol/*.xml; do
            wau-scanner <$f >${f%%.xml}.lua
        done
    ]],
    install_command = [[
        # mimic build.type == "builtin" behaviour
        install -Dm644 include/xkbcommon.lua $(LUADIR)/include/xkbcommon.lua
        install -Dm644 -t $(LUADIR)/include/protocol include/protocol/*.lua
        install -m 755 src/taiga.lua $(BINDIR)/taiga
        install -Dm644 -t $(LUADIR)/ src/*.lua
    ]],
}
