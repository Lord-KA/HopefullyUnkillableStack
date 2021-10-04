from itertools import combinations


def printoption(arr):
    print("\t\n  ", end='')
    for elem in arr[:-1]:
        print(elem.replace('_', ''), end='-')
    print(arr[-1].replace('_', ''), end=":\n")

    print("""    runs-on: ubuntu-latest
    timeout-minutes: 10

    steps:
    - uses: actions/checkout@v2

    - name: Configure CMake
      run: cmake -B ${{github.workspace}}/build -DCMAKE_BUILD_TYPE=Sanitizer -D CMAKE_CXX_FLAGS= " ${CMAKE_CXX_FLAGS}""", end=' ')

    for elem in arr:
        print(" -D " + elem, end=' ')

    print(""""

    - name: Build
      run: cmake --build ${{github.workspace}}/build --config Sanitizer

    - name: Test
      working-directory: ${{github.workspace}}/build/
      run: ./stack-test""")


inp = open("gstack.h", "r")

inp = inp.read()

start = inp.find("#ifdef FULL_DEBUG")
end = inp.find("STACK_VERBOSE")

inp = inp[start:end]
inp = inp[inp.find('\n') + 1:inp.rfind('\n')].split("\n")

macro = []
for line in inp:
    macro.append(line.split(' ')[-1])


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

jobs:\n""")
for elem in combos:
    printoption(elem)
