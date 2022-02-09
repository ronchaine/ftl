test_runner_source = [
  'test_runner.cpp'
]

array_test_sources = [
  'array/array.cpp'
]

array_tests = executable(
  'test_array',
  test_runner_source,
  array_test_sources,
  dependencies: [ftl_dep]
)

ringbuffer_test_sources = [
  'ring_buffer/ring_buffer.cpp'
]

ringbuffer_tests = executable(
  'test_ring_buffer',
  test_runner_source,
  ringbuffer_test_sources,
  dependencies: [ftl_dep]
)

