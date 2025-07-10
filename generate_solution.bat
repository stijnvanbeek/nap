#!/bin/sh

echo Generating MSVC solution in directory "msvc"...
cmake -S . -B msvc -G"Visual Studio 16 2019"