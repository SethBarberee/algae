project(
  'algae',
  'c',
  version: '0.0.1',
  license: 'None',
)

add_project_arguments([
  '-DWLR_USE_UNSTABLE',
  ], language: 'c')


cc = meson.get_compiler('c')

wlroots_proj = subproject(
	'wlroots',
	default_options: ['examples=false'],
	required: false,
)

if wlroots_proj.found()
  wlroots       = wlroots_proj.get_variable('wlroots')
else
  wlroots       = dependency('wlroots', version: '>= 0.12.0')
endif

wayland_server = dependency('wayland-server')
wayland_protos = dependency('wayland-protocols')
xkbcommon      = dependency('xkbcommon')
pixman         = dependency('pixman-1')
drm_full        = dependency('libdrm')
drm             = drm_full.partial_dependency(compile_args: true, includes: true)

subdir('protocols')

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
  dependencies: dependencies,
  install: true
)
