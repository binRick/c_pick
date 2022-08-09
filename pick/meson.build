pick_srcs = [
'compat-pledge.c',
'compat-reallocarray.c',
'compat-strtonum.c',
'libpick.c',
'libpick.h',
'pty.c',
]
pick_link_flags = []
pick_c_args = []
pick_deps = [
  c_stringfn_dep,
  c_vector_dep,
  curses_dep,
  tiny_regex_c_dep,
  c_string_buffer_dep,
  c_stringfn_dep,
  c_fsio_dep,  
]
pick_dirs = [
  '.',
  inc,
]
pick_lib = static_library('pick',
  pick_srcs,
  dependencies: pick_deps,
  c_args: pick_c_args,
  include_directories: pick_dirs,
)
pick_dep = declare_dependency(
  include_directories: pick_dirs,
  link_with: pick_lib,
  link_args: pick_link_flags,
)
