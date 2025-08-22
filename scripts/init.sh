#!/usr/bin/env bash

conan install . --build missing -s compiler.cppstd=20 -s build_type=Debug
conan install . --build missing -s compiler.cppstd=20 -s build_type=Release
cmake --preset conan-debug
cmake --preset conan-release
