libpick_multiple_test_srcs = [
  'libpick-multiple-test.c',
]
libpick_include_dirs = [
  '.',
  inc,
]
libpick_include_cargs = [
  ]
libpick_multiple_test_deps = [
  c_greatest_dep,
  c_vector_dep,
  tiny_regex_c_dep,
  c_string_buffer_dep,
  c_stringfn_dep,
  c_fsio_dep,
  libpick_dep,
  debug_memory_dep,
]
if get_option('enable-binaries')
  libpick_multiple_test_exec = executable('pick-multiple-test',
     libpick_multiple_test_srcs,
     dependencies: libpick_multiple_test_deps,
     include_directories: libpick_include_dirs,
     c_args: libpick_include_cargs,
  )
  if get_option('enable-binary-tests')
    #test('libpick-multiple-test', passh_program, args: [libpick_multiple_test_exec])
  endif
endif
