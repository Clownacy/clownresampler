get_property(CLOWNRESAMPLER_INCLUDE_GUARD GLOBAL PROPERTY CLOWNRESAMPLER_INCLUDE_GUARD)
if(NOT DEFINED CLOWNRESAMPLER_INCLUDE_GUARD)
set_property(GLOBAL PROPERTY CLOWNRESAMPLER_INCLUDE_GUARD ON)

cmake_minimum_required(VERSION 3.0...3.12)

project(clownresampler LANGUAGES C)

add_library(clownresampler STATIC "clownresampler.c" "clownresampler.h")

endif()
