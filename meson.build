project(
  'ftl',
  'cpp',
  default_options : ['cpp_std=c++20']
)

add_global_arguments('-fconcepts-diagnostics-depth=3', language : 'cpp')

ftl_includes = include_directories('include')

ftl_dep = declare_dependency(
    include_directories: ftl_includes
)

# TODO: hide behind an option, disable if used
#       as a subproject
subdir('tests')
