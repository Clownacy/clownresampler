cmake_minimum_required(VERSION 3.0...3.12)

project(clownresampler-test LANGUAGES C)

find_library(MATH_LIBRARY m)

add_executable(test-high-level "test-high-level.c" "dr_flac.h")

if(MATH_LIBRARY)
	target_link_libraries(test-high-level PRIVATE ${MATH_LIBRARY})
endif()

add_executable(test-low-level "test-low-level.c" "dr_flac.h")

if(MATH_LIBRARY)
	target_link_libraries(test-low-level PRIVATE ${MATH_LIBRARY})
endif()

#########
# Tests #
#########

enable_testing()

add_test(NAME high-test1 COMMAND test-high-level "${CMAKE_CURRENT_SOURCE_DIR}/test.flac" "test-output" 8000 44100 44100)
add_test(NAME high-test1_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test1" "test-output")

add_test(NAME high-test2 COMMAND test-high-level "${CMAKE_CURRENT_SOURCE_DIR}/test.flac" "test-output" 8000 44100 8000)
add_test(NAME high-test2_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test2" "test-output")

add_test(NAME high-test3 COMMAND test-high-level "${CMAKE_CURRENT_SOURCE_DIR}/test.flac" "test-output" 44100 8000 44100)
add_test(NAME high-test3_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test3" "test-output")

add_test(NAME high-test4 COMMAND test-high-level "${CMAKE_CURRENT_SOURCE_DIR}/test.flac" "test-output" 44100 8000 8000)
add_test(NAME high-test4_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test4" "test-output")

add_test(NAME low-test1 COMMAND test-low-level "${CMAKE_CURRENT_SOURCE_DIR}/test.flac" "test-output" 8000 44100 44100)
add_test(NAME low-test1_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test1" "test-output")

add_test(NAME low-test2 COMMAND test-low-level "${CMAKE_CURRENT_SOURCE_DIR}/test.flac" "test-output" 8000 44100 8000)
add_test(NAME low-test2_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test2" "test-output")

add_test(NAME low-test3 COMMAND test-low-level "${CMAKE_CURRENT_SOURCE_DIR}/test.flac" "test-output" 44100 8000 44100)
add_test(NAME low-test3_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test3" "test-output")

add_test(NAME low-test4 COMMAND test-low-level "${CMAKE_CURRENT_SOURCE_DIR}/test.flac" "test-output" 44100 8000 8000)
add_test(NAME low-test4_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test4" "test-output")
