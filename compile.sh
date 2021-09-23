#!/bin/sh

mkdir -p res/shaders
glslc src/shaders/shader.vert -o res/shaders/shader.vert.spv
glslc src/shaders/shader.frag -o res/shaders/shader.frag.spv
