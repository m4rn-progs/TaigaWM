# TaigaWM

Small, custom tiling window manager written in Lua for the BorealOS project.

## Project status
Active development — incomplete.

## Short description
TaigaWM is a minimal, dependency-light tiling window manager for X11, configured entirely via a single Lua file (`taiga.lua`). It focuses on a tiny, auditable codebase with sensible defaults and easy extensibility for power users.

## During-development focus
- Core tiling and basic layouts implemented.
- Improving layouts, per-workspace rules, and multi-monitor support.
- Hardening configuration parsing, adding tests, and setting up CI.
- Expanding documentation and example configs.

## Known limitations
- Limited layout options compared to mature tiling WMs.
- Basic multi-monitor handling.
- Sparse documentation; configuration may require reading source.

## Minimal example (taiga.lua)
```lua
return {
  modkey = "Mod4",
  layouts = { "tile", "floating" },
  keys = {
    { {"Mod4"}, "Return", function() os.execute("xterm &") end },
    { {"Mod4","Shift"}, "q", function() os.execute("pkill taiga") end }
  }
}
