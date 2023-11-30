cmake_minimum_required(VERSION 3.24)

project(clownresampler_examples LANGUAGES C)

add_executable(low-level "low-level.c")

set_target_properties(low-level PROPERTIES
	C_STANDARD 90
	C_STANDARD_REQUIRED NO
	C_STANDARD_EXTENSIONS YES
)

find_library(MATH_LIBRARY m)

if(MATH_LIBRARY)
	target_link_libraries(low-level PRIVATE ${MATH_LIBRARY})
endif()

add_executable(high-level "high-level.c")

set_target_properties(high-level PROPERTIES
	C_STANDARD 90
	C_STANDARD_REQUIRED NO
	C_STANDARD_EXTENSIONS YES
)

if(MATH_LIBRARY)
	target_link_libraries(high-level PRIVATE ${MATH_LIBRARY})
endif()
