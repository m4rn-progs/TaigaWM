# SPDX-FileCopyrightText: © 2026 Isaac Freund
# SPDX-License-Identifier: 0BSD

project(
  'taiga',
  'c',
  meson_version: '>= 1.8.0',
  version: '0.1',
  default_options: [
    'warning_level=3',
  ],
)

cc = meson.get_compiler('c')
add_project_arguments(cc.get_supported_arguments([
  '-D_POSIX_C_SOURCE=200809L',
	'-Wno-unused-parameter',
]), language: 'c')

wayland_client = dependency('wayland-client')
xkbcommon = dependency('xkbcommon')
libinput = dependency('libinput')
lua = dependency('lua')

wl_mod = import('wayland')
generated = wl_mod.scan_xml(
  'protocol/river-window-management-v1.xml',
  'protocol/river-xkb-bindings-v1.xml',
  'protocol/river-layer-shell-v1.xml',
  'protocol/river-libinput-config-v1.xml',
  'protocol/river-input-management-v1.xml',
)

inc = include_directories('include')

exe = executable(
  'taiga',
  'src/main.c',
  'src/output.c',
  'src/window.c',
  'src/xkb.c',
  'src/pointer.c',
  'src/seat.c',
  'src/wm.c',
  'src/layer_shell.c',
  'src/config.c',
  'src/libinput.c',
  'src/autostart.c',
  generated,
  dependencies: [
    wayland_client,
    xkbcommon,
    libinput,
    lua,
  ],
  include_directories: inc,
  install: true,
)
