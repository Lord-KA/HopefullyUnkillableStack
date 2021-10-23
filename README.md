# Hopefully Unkillable Stack

This is a generalized stack container lib with additional debug options like hash calculation, canaries and elem/ptr poison.

## Debug options that could be enabled with macro

1. STACK_USE_POISON             - enables poisoning (filling with predefined value) of elemen of stack [**SLOW**]
2. STACK_USE_PTR_POISON         - enables poisoning of structural pointers
3. STACK_USE_CANARY             - enables canaries (arrays of predefined ULL values on both sides of data) constance of which is checked
4. STACK_USE_STRUCT_HASH        - enables calculation of hash of valuable values (like capacity, len etc.) inside stack structure, for now requires SSE4.2
5. STACK_USE_DATA_HASH          - enables calculation of bitwise hash of data, for now requires SSE4.2 [**SLOW**]
6. STACK_USE_CAPACITY_SYS_CHECK - enables system capacity correctness check (via malloc_usable_size in unix or _msize in windows) 
7. STACK_USE_PTR_SYS_CHECK      - enables system ptr correctness check (os dependent) [**SLOW**]
8. STACK_VERBOSE 2              - sets the level of log verbosity from 0 to 2; if not defined eq. 0


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
1. Add c-style templates
2. Add crosscompile options for cmake
3. Add graphviz logs
4. Improve logging
5. Align clarifications in debug options listing

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
