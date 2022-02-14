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
  'ring_buffer/ring_buffer_static.cpp',
  'ring_buffer/ring_buffer_allocated.cpp',
  'ring_buffer/ring_buffer_common.cpp',
]

ringbuffer_tests = executable(
  'test_ring_buffer',
  test_runner_source,
  ringbuffer_test_sources,
  dependencies: [ftl_dep]
)

test('array', array_tests)
test('ring buffer', ringbuffer_tests)
