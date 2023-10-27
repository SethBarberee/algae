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

wlroots_version = ['>=0.17.0', '<0.18.0']
wlroots_proj = subproject(
	'wlroots',
	default_options: ['examples=false'],
	required: false,
        version: wlroots_version,
)

if wlroots_proj.found()
  wlroots       = wlroots_proj.get_variable('wlroots')
else
  wlroots       = dependency('wlroots', version: wlroots_version)
endif

wayland_server = dependency('wayland-server')
wayland_protos = dependency('wayland-protocols')
xkbcommon      = dependency('xkbcommon')
pixman         = dependency('pixman-1')
drm_full        = dependency('libdrm')
drm             = drm_full.partial_dependency(compile_args: true, includes: true)

algae_inc = include_directories('include')

subdir('include')
subdir('protocols')
subdir('src')
