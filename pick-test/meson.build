pick_test_srcs = [
  'pick-test.c',
]
pick_include_dirs = [
  '.',
  inc,
]
pick_include_cargs = [
  ]
pick_test_deps = [
  c_greatest_dep,
  c_vector_dep,
  tiny_regex_c_dep,
  c_string_buffer_dep,
  c_stringfn_dep,
  c_fsio_dep,
  pick_dep,
  debug_memory_dep,
]
if get_option('enable-binaries')
  pick_test_exec = executable('pick-test',
     pick_test_srcs,
     dependencies: pick_test_deps,
     include_directories: pick_include_dirs,
     c_args: pick_include_cargs,
  )
  if get_option('enable-binary-tests')
    #test('pick-test', passh_program, args: [pick_test_exec])
  endif
endif
