project('c_pick', ['c'],default_options: ['warning_level=0','c_std=gnu99',],
  subproject_dir: 'submodules/c_deps/subprojects',
  version:'0.0.1')
fs = import('fs')
sudo_program = find_program('sudo', required : true)
git_program = find_program('git', required : true)
sh_program = find_program('sh', required : true)
bat_program = find_program('bat', required : true)
jq_program = find_program('jq', required : true)
yaml2json_program = find_program('yaml2json', required : true)
cc = meson.get_compiler('c')
pwd = meson.current_source_dir()
c_deps_dir = join_paths(meson.current_source_dir(),'submodules/c_deps')
if fs.is_dir('.git')
    git_rev_parse = run_command(git_program, 'rev-parse', '--short', '@', check: true)
    git_sha = git_rev_parse.stdout().strip()
else
    git_sha = ''
endif
version_info = configuration_data()
version_info.set('version', git_sha)
version_info.set('vcs_tag', git_sha)
subdir('meson')
inc = [include_directories(
  'submodules','submodules/c_deps',
  'submodules/c_deps/submodules/c_ansi',
  'submodules/c_deps/submodules/dbg.h',
  'submodules/c_deps/submodules/tiny-regex-c',
  'submodules/c_deps/submodules',
  'submodules/c_deps/submodules/c_vector/src',
  'submodules/c_deps/submodules/c_vector/include',
  'libpick',
  'libpick-test',
  '.',
)]
#############################################################################
subdir('submodules')
#############################################################################
subdir('libpick')
#############################################################################
subdir('libpick-test')
#############################################################################
