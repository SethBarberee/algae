dependencies = [
  drm,
  pixman,
  server_protos,
  wayland_server,
  wayland_protos,
  wlroots,
  xkbcommon,
]

sources = [
  'algae_keyboard.c',
  'algae_view.c',
  'xdg_activation.c',
  'xdg_surface.c',
  'main.c',
]

executable(
  meson.project_name(),
  sources,
  include_directories: [algae_inc],
  dependencies: dependencies,
  install: true
)

