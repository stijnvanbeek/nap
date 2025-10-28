#!/bin/sh

if [ "$(uname)" = "Darwin" ]; then
  echo Generating XCode solution in directory "xcode"...
  cmake -S . -B xcode -GXcode
elif [ "$(uname)" = "Linux" ]; then
  echo Generating build directory for default generator...
  cmake -S . -B build
else
  echo Generating MSVC solution in directory "msvc"...
  cmake -S . -B msvc -G"Visual Studio 17 2022"
fi