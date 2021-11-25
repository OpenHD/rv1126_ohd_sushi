cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(rkadk_examples)

add_definitions(-Wall -g -O0 -ggdb -gdwarf -funwind-tables -rdynamic -Wno-unused-parameter -D_GNU_SOURCE)

aux_source_directory(common/isp ISP_SRC)
aux_source_directory(common/mp3_header MP3_SRC)

if(USE_RKAIQ)
    find_package(RkAiq REQUIRED)
    include_directories(${RKAIQ_INCLUDE_DIRS})
    add_definitions(-DRKAIQ)
endif()

#-----------------------------------
# rkadk_setting_test
#-----------------------------------
add_executable(rkadk_setting_test rkadk_setting_test.c)
add_dependencies(rkadk_setting_test rkadk)
target_link_libraries(rkadk_setting_test rkadk)
target_include_directories(rkadk_setting_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkadk_setting_test DESTINATION "bin")

#-----------------------------------
# rkadk_media_test: record photo stream
#-----------------------------------
add_executable(rkadk_media_test ${ISP_SRC} ${MP3_SRC} rkadk_media_test.c)
add_dependencies(rkadk_media_test rkadk)
target_link_libraries(rkadk_media_test easymedia rkadk rkaiq pthread)
target_include_directories(rkadk_media_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(rkadk_media_test PRIVATE ${CMAKE_SOURCE_DIR}/examples/common)
install(TARGETS rkadk_media_test DESTINATION "bin")

#--------------------------
# rkadk_record_test
#--------------------------
add_executable(rkadk_record_test rkadk_record_test.c ${ISP_SRC})
add_dependencies(rkadk_record_test rkadk)
target_link_libraries(rkadk_record_test easymedia rkadk rkaiq)
target_include_directories(rkadk_record_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(rkadk_record_test PRIVATE ${CMAKE_SOURCE_DIR}/examples/common)
install(TARGETS rkadk_record_test DESTINATION "bin")

#--------------------------
# rkadk_photo_test
#--------------------------
add_executable(rkadk_photo_test rkadk_photo_test.c ${ISP_SRC})
add_dependencies(rkadk_photo_test rkadk)
target_link_libraries(rkadk_photo_test easymedia rkadk rkaiq)
target_include_directories(rkadk_photo_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(rkadk_photo_test PRIVATE ${CMAKE_SOURCE_DIR}/examples/common)
install(TARGETS rkadk_photo_test DESTINATION "bin")

#--------------------------
# rkadk_stream_test
#--------------------------
add_executable(rkadk_stream_test rkadk_stream_test.c ${ISP_SRC} ${MP3_SRC})
add_dependencies(rkadk_stream_test rkadk)
target_link_libraries(rkadk_stream_test easymedia rkadk rkaiq)
target_include_directories(rkadk_stream_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(rkadk_stream_test PRIVATE ${CMAKE_SOURCE_DIR}/examples/common)
install(TARGETS rkadk_stream_test DESTINATION "bin")

#--------------------------
# rkadk_player_test
#--------------------------
add_executable(rkadk_player_test rkadk_player_test.c)
add_dependencies(rkadk_player_test rkadk)
target_link_libraries(rkadk_player_test rockit rkadk pthread)
target_include_directories(rkadk_player_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkadk_player_test DESTINATION "bin")

#--------------------------
# rkadk_thumb_test
#--------------------------
add_executable(rkadk_thumb_test rkadk_thumb_test.c)
add_dependencies(rkadk_thumb_test rkadk)
target_link_libraries(rkadk_thumb_test rkadk easymedia rockit)
target_include_directories(rkadk_thumb_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS rkadk_thumb_test DESTINATION "bin")

#--------------------------
# rkadk_rtmp_test
#--------------------------
add_executable(rkadk_rtmp_test rkadk_rtmp_test.c ${ISP_SRC})
add_dependencies(rkadk_rtmp_test rkadk)
target_link_libraries(rkadk_rtmp_test easymedia rkadk rkaiq)
target_include_directories(rkadk_rtmp_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(rkadk_rtmp_test PRIVATE ${CMAKE_SOURCE_DIR}/examples/common)
install(TARGETS rkadk_rtmp_test DESTINATION "bin")

#--------------------------
# rkadk_rtsp_test
#--------------------------
add_executable(rkadk_rtsp_test rkadk_rtsp_test.c ${ISP_SRC})
add_dependencies(rkadk_rtsp_test rkadk)
target_link_libraries(rkadk_rtsp_test easymedia rkadk rkaiq)
target_include_directories(rkadk_rtsp_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(rkadk_rtsp_test PRIVATE ${CMAKE_SOURCE_DIR}/examples/common)
install(TARGETS rkadk_rtsp_test DESTINATION "bin")

#--------------------------
# rkadk_disp_test
#--------------------------
add_executable(rkadk_disp_test rkadk_disp_test.c ${ISP_SRC})
add_dependencies(rkadk_disp_test rkadk)
target_link_libraries(rkadk_disp_test easymedia rkadk rkaiq)
target_include_directories(rkadk_disp_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(rkadk_disp_test PRIVATE ${CMAKE_SOURCE_DIR}/examples/common)
install(TARGETS rkadk_disp_test DESTINATION "bin")
