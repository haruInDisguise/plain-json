project('plain-json', 'c',
  version: '0.1.0',
  license: 'MIT',
  meson_version: '>=0.59.0',
  default_options: [
    'c_std=c99',
    'warning_level=3',
    'werror=true',
    'buildtype=release',
    'b_ndebug=if-release'
  ])

plain_json_dep = declare_dependency(include_directories: [ 'plain-json' ] )

if not meson.is_subproject()
  if get_option('buildtype') == 'debug'
    cc_flags = [
      '-DPLAIN_JSON_DEBUG',
      '-fsanitize=address,undefined',
      ]

    add_project_arguments(cc_flags, language: 'c')
    add_project_link_arguments([ '-fsanitize=address,undefined'], language: 'c')
  endif

  subdir('tools')
  subdir('test')
endif

