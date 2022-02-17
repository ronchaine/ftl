project(
  'ftl',
  'cpp',
  default_options : ['cpp_std=c++20']
)

ftl_includes = include_directories('include')

ftl_dep = declare_dependency(
    include_directories: ftl_includes
)

if meson.is_subproject() == false
  add_project_arguments(['-pedantic', '-Werror', '-Wno-missing-braces'], language : 'cpp')
  subdir('tests')
endif
