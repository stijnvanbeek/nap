---
name: build-cmake-target
description: |
  Build a target in a cmake project in the current folder.
  
  Triggers when the user types:
  build <target>
---

## Steps

1. (Re)generate the cmake project:
'''
cmake -S . -B build
'''
2. Build the target
cmake --build build --target <target>
