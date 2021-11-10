#!/bin/sh

mkdir -p res/shaders
glslangValidator -V src/shaders/shader.vert -o res/shaders/shader.vert.spv
glslangValidator -V src/shaders/shader.frag -o res/shaders/shader.frag.spv
glslangValidator -V src/shaders/screen.frag -o res/shaders/screen.frag.spv
glslangValidator -V src/shaders/screen.vert -o res/shaders/screen.vert.spv
