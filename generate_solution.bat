#!/bin/sh

echo Generating MSVC solution in directory "vs"...
cmake -S . -B vs -G"Visual Studio 16 2019"