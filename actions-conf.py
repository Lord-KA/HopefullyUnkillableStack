#!/bin/python3

from itertools import combinations


def printoption(arr):
    if len(arr) == 0:
        return 0
    print("  ", end='', file=outp)
    for elem in arr[:-1]:
        print(elem.replace('_', ''), end='-', file=outp)
    print(arr[-1].replace('_', ''), end=":\n", file=outp)

    print("""    runs-on: ubuntu-latest
    timeout-minutes: 10

    steps:
    - uses: actions/checkout@v2

    - name: Configure CMake
      run: cmake -B ${{github.workspace}}/build -DCMAKE_BUILD_TYPE=Sanitizer -D CMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS}""", end=' ', file=outp)

    for elem in arr:
        print(" -D " + elem, end='', file=outp)

    print(""""

    - name: Build
      run: cmake --build ${{github.workspace}}/build --config Sanitizer

    - name: Test
      working-directory: ${{github.workspace}}/build/
      run: ./stack-test""", file=outp)


inp = open("gstack-header.h", "r")
outp = open(".github/workflows/cmake.yml", "w")

inp = inp.read()

start = inp.find("#ifdef FULL_DEBUG")
end = inp.find("STACK_VERBOSE")

inp = inp[start:end]
inp = inp[inp.find('\n') + 1:inp.rfind('\n')].split("\n")

macro = []
for line in inp:
    macro.append(line.split(' ')[-1])

for line in macro:
    print(line)

combos = []  # list(combinations(macro, len(macro)))
for i in range(1, len(macro)):
    combos += list(combinations(macro, i))


print("""name: CMake
on:
  push:
    branches: [ master ]
  pull_request:
    branches: [ master ]

env:
  BUILD_TYPE: Release

jobs:
  release-test:
    runs-on: ubuntu-latest
    timeout-minutes: 10

    steps:
    - uses: actions/checkout@v2

    - name: Configure CMake
      run: cmake -B ${{github.workspace}}/build -DCMAKE_BUILD_TYPE=Release

    - name: Build
      run: cmake --build ${{github.workspace}}/build --config Release

    - name: Test
      working-directory: ${{github.workspace}}/build/
      run: ./stack-test

""", file=outp)
for elem in combos:
    printoption(elem)
