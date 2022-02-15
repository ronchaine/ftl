# `ftl::ring_buffer`

``` cpp
template<
    typename ValueT,
    typename Storage, = FTL_DEFAULT_ALLOCATOR
> class ftl::ring_buffer;
```

`ftl::ring_buffer` implements a circular buffer, which can be used
with an allocator or given static storage amount.  When given an
allocator, similarly to `std::vector` it is expanded when required.

It fills most of the standard named requirements for `SequenceContainer`
and `ReversibleContainer`, though not completely (since its iterator
is not `DefaultConstructible`)

## Members
| Types             |                                                                   |
| -------           | -----------                                                       |
| `value_type`      | `T`                                                               |
| `size_type`       | `std::size_t`                                                     |
| `pointer`         | `Storage::pointer` (almost always `T*`)                           |
| `reference`       | `T&`                                                              |
| `const_reference` | `const T&`                                                        |
| `difference_type` | `std::ptrdiff_t`                                                  |
| `allocator_type`  | `Storage` or `void` in case of `ftl::static_storage` being used   |
| `iterator`        |  Iterator to value type                                           |
| `const_iterator`  |  `const` Iterator to value type                                   |

| Static values     |                                                                   |
| -------           | -----------                                                       |
| `is_dynamic`      | `true` if is statically allocated, `false` if using allocator     |

| Functions                     |                                                                               |
| -------                       | -----------                                                                   |
| iterators                     |                                                                               |
| `begin()`                     | return iterator to the  beginning of buffer                                   |
| `end()`                       | return iterator to the end of buffer                                          |
| modifiers                     |                                                                               |
| `push(T&&)`                   | add an element to the end of the array                                        |
| `push(const T&)`              |                                                                               |
| `push_overwrite(T&&)`         | add an element to the end of the array, overwriting the first instead of      |
| `push_overwrite(const T&)`    | resizing if the container is full                                             |
| `pop()`                       | read first element and destroy it                                             |
| `reserve(size_type)`          | for allocator-powered buffers, makes sure there is enough space for at least  |
|                               | given number of elements.  For static buffers, this is a no-op                |
| `clear()`                     | empties the array, leaving memory reserved                                    |
| `swap(ring_buffer&)`          | swaps ring buffer with another                                                |
| queries                       |                                                                               |
| `size()`                      | returns number of elements stored in the container                            |
| `capacity()`                  | returns number of elements the container has reserved memory for              |
| `is_empty()`                  | `true` if the container has no stored elements, otherwise `false`             |
| `is_full()`                   | `true` if adding elements to the container would need reallocation            |
| `is_contiguous()`             | `true` if all the elements in the array are stored contiguously in-order      |

## Example use

``` cpp
#include <ftl/ring_buffer.hpp>
#include <string>
#include <iostream>

int main()
{
    ftl::ring_buffer<std::string, ftl::static_storage<10>> history;

    std::string cmd;
    while(std::getline(std::cin, cmd)) {
        history.push_overwrite(cmd);
        std::cout << history.size() << "> " << std::flush;
    }

    for (auto& str : history)
        std::cout << str << "\n";
}
```
