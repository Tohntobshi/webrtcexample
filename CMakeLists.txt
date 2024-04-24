cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_CXX_STANDARD 17)
project( Webrtcexample )

ADD_DEFINITIONS(-DWEBRTC_POSIX=1)
ADD_DEFINITIONS(-D__STDC_CONSTANT_MACROS=1)

SET(GCC_COVERAGE_COMPILE_FLAGS "-pthread")
SET(GCC_COVERAGE_LINK_FLAGS    "")

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")

include_directories(
    /home/tohntobshi/webrtc/src
    /home/tohntobshi/webrtc/src/third_party/abseil-cpp
    /home/tohntobshi/webrtc/src/third_party/libyuv/include

    /home/tohntobshi/ffmpeg_build/include
)

set(
  TARGETS
    dummy_audio_device.cpp
    frame_manipulation_example_main.cpp
)

add_executable( frame_manipulation_example ${TARGETS} )

target_link_libraries(frame_manipulation_example

    ixwebsocket
    z
    jsoncpp

    /home/tohntobshi/webrtc/src/out/Default2/obj/libwebrtc.a

    # X11
)