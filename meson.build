project('plain-json', 'c',
  version: '0.1.0',
  license: 'MIT',
  meson_version: '>=0.59.0',
  default_options: [
    'c_std=c99',
    'warning_level=1',
    'werror=true',
    'buildtype=debug',
    'b_sanitize=address,undefined',
    'b_ndebug=if-release'
  ])

plain_json_dep = declare_dependency(include_directories: [ 'plain-json' ] )

if not meson.is_subproject()
  cc = meson.get_compiler('c')
  cc_flags = [
    '-DPLAIN_JSON_DEBUG',
    '-gdwarf-5',
    '-fno-omit-frame-pointer',
    ]

  add_project_arguments(cc_flags, language: 'c')

  subdir('tools')
  subdir('test')
endif

