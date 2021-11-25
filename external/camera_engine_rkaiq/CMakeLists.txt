
if(NOT CMAKE_BUILD_TYPE)
    # default to Release build for GCC builds
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif()

if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-DBUILD_TYPE_DEBUG)
endif()

cmake_minimum_required(VERSION 2.8)

#project name
PROJECT(rk_aiq)

# ----------------------------------------------------------------------------
# set property to classify library kinds
# ----------------------------------------------------------------------------
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")

#set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

#set(ENABLE_ASAN true)

#set RK_SIMULATOR_HW
#set(RK_SIMULATOR_HW true)

if(NOT RK_SIMULATOR_HW)
set(GENMESH_ONLINE true)
set(MOTION_DETECT true)
endif()

# ----------------------------------------------------------------------------
# SET THE COMPILER PATH
# ----------------------------------------------------------------------------
if(RK_SIMULATOR_HW)
	add_compile_options(-DRK_SIMULATOR_HW)
elseif(BUILDROOT_BUILD_PROJECT)
	# Buildroot will set compiler when building
else()
	SET(CMAKE_C_COMPILER "${PROJECT_SOURCE_DIR}/../../../../../prebuilts/gcc/linux-x86/arm/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc")
	SET(CMAKE_CXX_COMPILER "${PROJECT_SOURCE_DIR}/../../../../../prebuilts/gcc/linux-x86/arm/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++")
endif()

# ----------------------------------------------------------------------------
# ADD SYMBOL TABLE FOR GDB
# ----------------------------------------------------------------------------
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -std=c++11 -Wno-reorder -Werror")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -std=c++11 -Werror -Wno-reorder")
# SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

#head file path
include_directories(
xcore
xcore/base
aiq_core
algos
common
common/linux
hwi
hwi/isp20
ipc
iq_parser
uAPI
ipc_server
#algos/awb
#../core/inc/luma
#../core/inc/stat_3a_ae
#../core/inc/stat_3a_af
#../core/inc/orb
#../core/inc/common
./
include
include/common
include/common/mediactl
include/iq_parser
include/uAPI
include/xcore
include/xcore/base
include/algos
include/algos/a3dlut
include/algos/ablc
include/algos/accm
include/algos/acgc
include/algos/acp
include/algos/adebayer
include/algos/adehaze
include/algos/adpcc
include/algos/ae
include/algos/af
include/algos/afd
include/algos/afec
include/algos/agamma
include/algos/adegamma
include/algos/agic
include/algos/ahdr
include/algos/aie
include/algos/aldch
include/algos/alsc
include/algos/anr
include/algos/anr/tnr_md
include/algos/aorb
include/algos/ar2y
include/algos/asd
include/algos/asharp
include/algos/awb
include/algos/awdr
include/common/gen_mesh
include/ipc_server
)
#source directory
#AUX_SOURCE_DIRECTORY(demo/src DIR_SRCS)
set (AIQ_CORE_SRC
	aiq_core/RkAiqHandle.cpp
	aiq_core/RkAiqHandleInt.cpp
	aiq_core/RkAiqCore.cpp
	aiq_core/RkLumaCore.cpp
	)
set (ALGO_COMMON_SRC
	algos/interpolation.cpp
	)
if(GENMESH_ONLINE)
set (COMMON_SRC
    common/mediactl/mediactl.c
    common/gen_mesh/genMesh.cpp
    )
else()
set (COMMON_SRC
	common/mediactl/mediactl.c
	)
endif()
if(RK_SIMULATOR_HW)
set (HWI_SRC
        hwi/CamHwBase.cpp
        hwi/simulator/CamHwSimulator.cpp
        )
else()
set (HWI_SRC
        hwi/CamHwBase.cpp
        hwi/isp20/CamHwIsp20.cpp
        hwi/isp20/Isp20Params.cpp
        hwi/isp20/Isp20PollThread.cpp
        hwi/isp20/Isp20StatsBuffer.cpp
        hwi/isp20/Isp20_module_dbg.cpp
        hwi/isp20/Isp20SpThread.cpp
        hwi/SensorHw.cpp
        hwi/LensHw.cpp
        hwi/FlashLight.cpp
        hwi/fakecamera/FakeSensorHw.cpp
        hwi/fakecamera/FakeCamHwIsp20.cpp
        )
endif()
set (IPC_SRC
	)
set (IQ_PARSER_SRC
	iq_parser/RkAiqCalibParser.cpp
	iq_parser/RkAiqCalibDb.cpp
	iq_parser/RkAiqCalibTag.cpp
	iq_parser/tinyxml2.cpp
	iq_parser/xmltags.cpp
	)
set (UAPI_SRC
	uAPI/rk_aiq_user_api_sysctl.cpp
	uAPI/rk_aiq_user_api_debug.cpp
	)
set (XCORE_SRC
	xcore/xcam_common.cpp
	xcore/xcam_log.cpp
	xcore/xcam_thread.cpp
	xcore/xcam_buffer.cpp
	xcore/video_buffer.cpp
	xcore/v4l2_buffer_proxy.cpp
	xcore/v4l2_device.cpp
	xcore/fake_v4l2_device.cpp
	xcore/buffer_pool.cpp
	xcore/poll_thread.cpp
	)
set (IPC_SERVER_SRC
    ipc_server/socket_server.cpp
    ipc_server/rkaiq_tool_ae.cpp
    ipc_server/rkaiq_tool_anr.cpp
    ipc_server/rkaiq_tool_imgproc.cpp
    ipc_server/rkaiq_tool_sharp.cpp
    ipc_server/rkaiq_tool_sysctl.cpp
    ipc_server/command_process.cpp
    )
set (RK_AIQ_SRC
	${ALGO_COMMON_SRC}
	${COMMON_SRC}
	${HWI_SRC}
	${IPC_SRC}
	${IQ_PARSER_SRC}
	${UAPI_SRC}
	${XCORE_SRC}
	${AIQ_CORE_SRC}
    ${IPC_SERVER_SRC}
	RkAiqManager.cpp
    )
if(OPENCV_SUPPORT)
set (OPENCV_LIBS
    opencv_calib3d
    opencv_core
    opencv_dnn
    opencv_features2d
    opencv_flann
    opencv_gapi
    opencv_highgui
    opencv_imgcodecs
    opencv_imgproc
    opencv_ml
    opencv_objdetect
    opencv_photo
    opencv_stitching
    opencv_video
    opencv_videoio
    )
set (OPENCV_3RDPARTY_LIBS
    dl
    ade
    IlmImf
    ippicv
    ippiw
    ittnotify
    libprotobuf
    libwebp
    quirc
    )
set (OPENCV_LIBS_DIR
    ${PROJECT_SOURCE_DIR}/common/opencv2/lib
    ${PROJECT_SOURCE_DIR}/common/opencv2/lib/3rdparty
    )
add_compile_options(-DOPENCV_SUPPORT)
endif()

if(GENMESH_ONLINE)
    set (GENMESH_LIBS genMeshLib)
    set (GENMESH_LIBS_DIR
        ${PROJECT_SOURCE_DIR}/common/gen_mesh/linux/genMesh_static_32bit/
        )
    add_compile_options(-DGENMESH_ONLINE)
endif()

if(MOTION_DETECT)
    set (MOTION_DETECT_LIBS rkaiq_tnr_md)
endif()

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

if(ENABLE_ASAN)
add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
SET(LINK_ASAN asan)
endif()

# add C++11 option
#add_compile_options(-std=c++11)
add_compile_options(-fPIC -Wall -Wno-error -Os -g)
add_compile_options(-Wno-unused-variable
					-Wno-unused-but-set-variable
					-Wno-unused-function
					-Wno-misleading-indentation
					-Wno-unused-label
					-fno-strict-aliasing)
# add_compile_options(-v)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

# ----------------------------------------------------------------------------
# add algos implement
# ----------------------------------------------------------------------------
add_subdirectory(algos)

# surpress link_directories warning
cmake_policy(SET CMP0015 NEW)

#
link_directories("${LIBRARY_OUTPUT_PATH}" "${OPENCV_LIBS_DIR}" "${GENMESH_LIBS_DIR}")

add_library(rkaiq SHARED ${RK_AIQ_SRC})
set_target_properties(rkaiq PROPERTIES FOLDER "rk_aiq")
set_target_properties(rkaiq PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(rkaiq PROPERTIES C_VISIBILITY_PRESET default)
set_target_properties(rkaiq PROPERTIES CXX_VISIBILITY_PRESET default)

#
link_directories("${LIBRARY_OUTPUT_PATH}")

target_link_libraries(rkaiq rkaiq_ae rkaiq_awb rkaiq_af rkaiq_anr rkaiq_asd rkaiq_ahdr
	rkaiq_acp rkaiq_asharp rkaiq_adhaz rkaiq_a3dlut rkaiq_ablc rkaiq_accm rkaiq_acgc
	rkaiq_adebayer rkaiq_adpcc rkaiq_afec rkaiq_agamma rkaiq_adegamma rkaiq_agic rkaiq_aie rkaiq_aldch
	rkaiq_alsc rkaiq_aorb rkaiq_ar2y rkaiq_awdr rkaiq_afd ${OPENCV_LIBS}
	${OPENCV_3RDPARTY_LIBS} ${GENMESH_LIBS} ${LINK_ASAN} pthread ${MOTION_DETECT_LIBS})

set (VERSIONFILEPATH ${PROJECT_SOURCE_DIR}/)
execute_process(COMMAND sh
	${PROJECT_SOURCE_DIR}/tools/check-version-with-tuningtools.sh
	${VERSIONFILEPATH})

# ----------------------------------------------------------------------------
# add rkisp_api implement
# ----------------------------------------------------------------------------
add_subdirectory(rkisp_api)

if(ENABLE_RKISP_DEMO)
add_subdirectory(rkisp_demo)
endif()

install(TARGETS rkaiq
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(DIRECTORY include/ DESTINATION include/rkaiq
        FILES_MATCHING PATTERN "*.h")
install(DIRECTORY common/ DESTINATION include/rkaiq/common
        FILES_MATCHING PATTERN "*.h")

