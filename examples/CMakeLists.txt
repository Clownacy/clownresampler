cmake_minimum_required(VERSION 3.24)

project(clownresampler_examples LANGUAGES C)

add_executable(low-level "low-level.c")

set_target_properties(low-level PROPERTIES
	C_STANDARD 90
	C_STANDARD_REQUIRED NO
	C_STANDARD_EXTENSIONS YES
)

add_executable(high-level "high-level.c")

set_target_properties(high-level PROPERTIES
	C_STANDARD 90
	C_STANDARD_REQUIRED NO
	C_STANDARD_EXTENSIONS YES
)
