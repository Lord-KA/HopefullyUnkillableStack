# Hopefully Unkillable Stack

This is a generalized stack container header-only library in C with additional debug options like hash calculation, canaries and elem/ptr poison.

## Debug options that could be enabled with macro

| Debug flag/option              | description                                                                                                              | traits |
|--------------------------------|--------------------------------------------------------------------------------------------------------------------------|--------|
| `STACK_USE_POISON`             | enables poisoning (filling with predefined value) of stack data                                                          | [**SLOW**] |
| `STACK_USE_PTR_POISON`         | enables poisoning of structural pointers                                                                                 | |
| `STACK_USE_CANARY`             | enables canaries (arrays of predefined 64bit values on both sides of data) constance of which is checked                 | |
| `STACK_USE_STRUCT_HASH`        | enables hash calculation of structural values of the stack structure, like capacity, value count etc.                    | [**REQUIRES_SSE4.2**] |
| `STACK_USE_DATA_HASH`          | enables calculation of bitwise hash of data                                                                              | [**SLOW**] [**REQUIRES_SSE4.2**] |
| `STACK_USE_CAPACITY_SYS_CHECK` | enables system capacity correctness check (via `malloc_usable_size()` in unix or `_msize()` in windows)                  | [**OS_DEPENDENT**] |
| `STACK_USE_PTR_SYS_CHECK`      | enables system pointer correctness check                                                                                 | [**SLOW**] [**OS_DEPENDENT**] |
| `STACK_VERBOSE 2`              | sets the level of log verbosity from 0 to 2; if not defined eq. 0                                                        | |


## Building with some debug options
```bash
$ mkdir build
$ cd build
$ cmake .. -D CMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS} -D STACK_USE_PTR_POISON -D STACK_USE_DATA_HASH -D STACK_VERBOSE=2"
$ make
```


## Building with all debug options
```bash
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_BUILD_TYPE=FULL_DEBUG
$ make
```


## Building release version
```bash
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Release
$ make
```

## TODO
1. Improve logging
2. Add Graphviz to logs
3. Add crosscompile options to CMake config

## Done
1. Basic stack structure
2. Generalized container
3. Advanced logging
4. Canaries
5. Hashing
6. Poisoning
7. Sys ptr checks
8. Sys allocation size checks
9. Demo showing different debug reactions
10. Github CI config codegen
11. Github CI
12. GoogleTesting
13. Doxygen docs
14. Capibara ASCII art
15. Add c-style templates
