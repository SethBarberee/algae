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
  'main.c',
]

executable(
  meson.project_name(),
  sources,
  include_directories: [algae_inc],
  dependencies: dependencies,
  install: true
)

