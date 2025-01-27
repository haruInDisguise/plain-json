libtest_dep = dependency('libtest',
  fallback: ['libtest', 'libtest_dep'],
  required: true)

test_exe = executable('run_tests',
  dependencies: [ plain_json_dep, libtest_dep ],
  sources: ['test_unicode.c', 'test_main.c', 'test_number.c'])

