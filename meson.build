project(
  'alage',
  'c',
  version: '0.0.1',
  license: 'None',
)

add_project_arguments([
  '-DWLR_USE_UNSTABLE',
  ], language: 'c')

# TODO finish this

wlroots_version = ['>=0.9.0', '<0.10.0']
wlroots_proj = subproject(
	'wlroots',
	default_options: ['examples=false'],
	required: false,
	version: wlroots_version,
)