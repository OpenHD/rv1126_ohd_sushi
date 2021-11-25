
if(NOT CMAKE_BUILD_TYPE)
    # default to Release build for GCC builds
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif()

cmake_minimum_required(VERSION 2.8)

#project name
PROJECT(rkisp_api)

#set(ENABLE_ASAN true)

# ----------------------------------------------------------------------------
# SET THE COMPILER PATH
# ----------------------------------------------------------------------------
if(BUILDROOT_BUILD_PROJECT)
	# Buildroot will set compiler when building
else()
#	SET(CMAKE_C_COMPILER "${PROJECT_SOURCE_DIR}/../../../puma-prebuilts/prebuilts/gcc/linux-x86/arm/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc")
#	SET(CMAKE_CXX_COMPILER "${PROJECT_SOURCE_DIR}/../../../puma-prebuilts/prebuilts/gcc/linux-x86/arm/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++")
endif()

# ----------------------------------------------------------------------------
# ADD SYMBOL TABLE FOR GDB
# ----------------------------------------------------------------------------
# SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -ggdb")
# SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -ggdb")

# ----------------------------------------------------------------------------
# set property to classify library kinds
# ----------------------------------------------------------------------------
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")

#set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

#set ADD_RK_AIQ
set(ADD_RK_AIQ true)

#SET RK_AIQ SOURCE DIR
set(RK_AIQ_SOURCE_DIR ${PROJECT_SOURCE_DIR}/..)
message("rkaiq src path" ${RK_AIQ_SOURCE_DIR})

if(ENABLE_ASAN)
add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
SET(LINK_ASAN asan)
endif()

if(ADD_RK_AIQ)
add_compile_options(-std=c++11)
add_compile_options(-DADD_RK_AIQ)
add_compile_options(-O0)
# add_compile_options(-v)
SET(RK_AIQ_LIB_DIR ${RK_AIQ_SOURCE_DIR}/build/linux/all_lib/${CMAKE_BUILD_TYPE})
message("rkaiq lib path" ${RK_AIQ_LIB_DIR})
SET(RK_AIQ_INCLUDE_DIR
${RK_AIQ_SOURCE_DIR}/include/uAPI
${RK_AIQ_SOURCE_DIR}/include/xcore/
${RK_AIQ_SOURCE_DIR}/include/algos/
${RK_AIQ_SOURCE_DIR}/include/common/
${RK_AIQ_SOURCE_DIR}/include/iq_parser/
${RK_AIQ_SOURCE_DIR}/common/
${RK_AIQ_SOURCE_DIR}/
)
SET(RK_AIQ_LIB rkaiq)
endif()

#head file path
include_directories(
${RK_AIQ_INCLUDE_DIR}
)

# set output binary path
if(WIN32)
    SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/exe)
    SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/all_lib/)
    SET(CMAKE_LIBRARY_PATH ${LIBRARY_OUTPUT_PATH}/${CMAKE_BUILD_TYPE})
else()
    SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/exe/${CMAKE_BUILD_TYPE})
    SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/all_lib/${CMAKE_BUILD_TYPE})
    SET(CMAKE_LIBRARY_PATH ${LIBRARY_OUTPUT_PATH})
endif()

#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
#set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
message(${CMAKE_LIBRARY_PATH})
message(${LIBRARY_OUTPUT_PATH})

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

#
link_directories("${LIBRARY_OUTPUT_PATH}" "${RK_AIQ_LIB_DIR}")

add_library(rkisp_api SHARED rkisp_api.cpp)
set_target_properties(rkisp_api PROPERTIES FOLDER "rk_aiq_api")
set_target_properties(rkisp_api PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(rkisp_api PROPERTIES C_VISIBILITY_PRESET default)
set_target_properties(rkisp_api PROPERTIES CXX_VISIBILITY_PRESET default)

target_link_libraries(rkisp_api ${RK_AIQ_LIB} pthread)

install(TARGETS rkisp_api
    LIBRARY DESTINATION lib
)
install(FILES rkisp_api.h DESTINATION include/rkaiq)
