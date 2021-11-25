list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

project(rkadk)

install(DIRECTORY include/ DESTINATION include/rkadk
        FILES_MATCHING PATTERN "*.h")

message("RKMEDIA_HEADER_DIR = ${RKMEDIA_HEADER_DIR}")
message("ROCKIT_HEADER_DIR = ${ROCKIT_HEADER_DIR}")
include_directories(${RKMEDIA_HEADER_DIR})
include_directories(${ROCKIT_HEADER_DIR})
include_directories(${ROCKIT_HEADER_DIR}/mpi)
include_directories(include)

#-Wextra
add_definitions(-Wall
		-Wshadow
		-ffast-math
		-Wignored-qualifiers
		-Werror-implicit-function-declaration
		-fPIC)

add_subdirectory(src)
add_subdirectory(examples)
