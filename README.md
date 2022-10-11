This repository will not be updated
===================================
(https://codeberg.org/ananas/ftl) will be the canonical repository
for future updates since I'm moving my active public repositories
there in time.

Freestanding template library
=============================
Provides freestanding-compatible template types for common use cases.

Note that nothing prevents you from using these in non-freestanding
scenarios either, they're just made to work in freestanding subset
as well.

Uses concepts and requires -statements, and doesn't shy away from
C++20 features in general, so your compiler needs to support those

Otherwise depends only on freestanding C++ headers, but some headers
may expect `memmove` and `memcpy` to be available via `string.h`

Types provided are not strictly drop-in-replacements to `std`, but quite close.
They do not make guarantees the standard does with iterator invalidation, etc.


Provides CMakeLists.txt and meson.build for easy integration.


You can also just copy the required header files if you find that easier,
the headers do somewhat depend on each other though, but the dependencies
are listed in the documentation of each header.


Memory and allocators
---------------------
Dynamically sized containers can use allocators.  On non-freestanding
environments, the default allocator is `std::allocator`.

In addition, there is a type `ftl::static_storage<size_t>` that can
be used instead of an allocator, instructing it to embed a static buffer
of requested size into the container type itself.  It is used as a default
on freestanding environments.


Array
-----
Defined in `array.hpp`, uses `utility.hpp` and `hash.hpp`

Similar to `std::array`, but can be multi-dimensional.  Does not use
allocators or use heap memory.


Ring buffer
-----------
Defined in `ring_buffer.hpp`, uses `utility.hpp` and `memory.hpp`

Very simple ring buffer with `push` and `pop`.  Resizes itself
if allocator is given when unread elements fill the entire
storage.  Has rudimentary iterator support as well.

If exceptions are enabled in compiler, `ring_buffer::pop()` will
throw `out_of_range` if trying to read from empty array.

Uses `ftl::static_storage<32>` as default "Allocator" on freestanding


Licence
-------
[MIT Licence](LICENCE.md)

The licence information is also contained at the *end* of the files,
so generally, as they are header-only, you are good as long as you don't
explicitly modify or remove the licence from those files no matter your use.

