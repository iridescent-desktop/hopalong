project('hopalong', 'c',
  version: '0.1',
  meson_version: '>=0.50')


add_project_arguments('-DWLR_USE_UNSTABLE', language: 'c')


wayland_server = dependency('wayland-server')
wayland_client = dependency('wayland-client')
wayland_cursor = dependency('wayland-cursor')
wayland_egl    = dependency('wayland-egl')
wayland_protos = dependency('wayland-protocols', version: '>=1.14')
pixman         = dependency('pixman-1')
cairo          = dependency('cairo')
pango          = dependency('pango')
pangocairo     = dependency('pangocairo')
wlroots        = dependency('wlroots')
xkbcommon      = dependency('xkbcommon')


subdir('src')
