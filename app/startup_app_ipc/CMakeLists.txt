cmake_minimum_required(VERSION 3.8)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(startup_app_ipc)

include(FindPkgConfig)
pkg_check_modules (JSON-C REQUIRED IMPORTED_TARGET json-c)
find_package(MiniMedia REQUIRED)

include_directories(
	${PROJECT_SOURCE_DIR}/./)

link_directories(
	${PROJECT_SOURCE_DIR}/./lib)

aux_source_directory(./ SRCS)
add_executable(${PROJECT_NAME}
	${SRCS})

option(SANITIZER_STATIC "compile with sanitizer (static library linker)" OFF)
if(SANITIZER_STATIC)
	add_definitions(-fsanitize=address -static-libasan -g -ggdb -gdwarf -funwind-tables -rdynamic -O0)
	add_definitions(-fno-stack-protector -fno-omit-frame-pointer -fsanitize-recover=address)
	# add_definitions(-fsanitize=undefined)
	target_link_libraries(${PROJECT_NAME} libasan.a dl m rt)
endif()

option(SANITIZER_DYNAMIC "compile with sanitizer (dynamic library linker)" OFF)
if(SANITIZER_DYNAMIC)
	# NOTE: copy libasan.so with manual operation
	add_definitions(-fsanitize=address -g -ggdb -gdwarf -funwind-tables -rdynamic -O0)
	add_definitions(-fno-stack-protector -fno-omit-frame-pointer -fsanitize-recover=address)
	# add_definitions(-fsanitize=undefined)
	target_link_libraries(${PROJECT_NAME} asan)
endif()

target_link_libraries(${PROJECT_NAME}
	PkgConfig::JSON-C
	MiniMedia::MiniMedia
	pthread
	IPCProtocol
	)

install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION bin)

