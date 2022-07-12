cmake_minimum_required(VERSION 2.8.12)
project(clownresampler)
add_library(libclownresampler INTERFACE)
set_target_properties(libclownresampler PROPERTIES PUBLIC_HEADER "clownresampler.h")
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(inc_dest $ENV{COMMONFILES})
else()
  set(inc_dest /usr/local/include)
endif()

install(TARGETS libclownresampler
	PUBLIC_HEADER DESTINATION ${inc_dest})
