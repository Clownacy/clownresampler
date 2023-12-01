cmake_minimum_required(VERSION 3.0...3.12)

project(clownresampler-test LANGUAGES C)

add_executable(clownresampler-test "test.c" "dr_flac.h")

find_library(MATH_LIBRARY m)

if(MATH_LIBRARY)
	target_link_libraries(clownresampler-test PRIVATE ${MATH_LIBRARY})
endif()

#########
# Tests #
#########

enable_testing()

add_test(NAME test1 COMMAND clownresampler-test "${CMAKE_CURRENT_SOURCE_DIR}/test.flac" "test-output" 8000 44100 44100)
add_test(NAME test1_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test1" "test-output")

add_test(NAME test2 COMMAND clownresampler-test "${CMAKE_CURRENT_SOURCE_DIR}/test.flac" "test-output" 8000 44100 8000)
add_test(NAME test2_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test2" "test-output")

add_test(NAME test3 COMMAND clownresampler-test "${CMAKE_CURRENT_SOURCE_DIR}/test.flac" "test-output" 44100 8000 44100)
add_test(NAME test3_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test3" "test-output")

add_test(NAME test4 COMMAND clownresampler-test "${CMAKE_CURRENT_SOURCE_DIR}/test.flac" "test-output" 44100 8000 8000)
add_test(NAME test4_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test4" "test-output")
