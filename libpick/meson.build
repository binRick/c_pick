libpick_srcs = [
'compat-pledge.c',
'compat-reallocarray.c',
'compat-strtonum.c',
'libpick.c',
'libpick.h',
'pty.c',
]
libpick_link_flags = []
libpick_c_args = []
libpick_deps = [
  c_stringfn_dep,
  c_vector_dep,
  curses_dep,
  tiny_regex_c_dep,
  c_string_buffer_dep,
  c_stringfn_dep,
  c_fsio_dep,  
]
libpick_dirs = [
  '.',
  inc,
]
libpick_lib = library('libpick',
  libpick_srcs,
  dependencies: libpick_deps,
  c_args: libpick_c_args,
  include_directories: libpick_dirs,
)
libpick_dep = declare_dependency(
  include_directories: libpick_dirs,
  link_with: libpick_lib,
  link_args: libpick_link_flags,
)
