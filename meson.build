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
  subdir('tests')
endif
