cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_CXX_STANDARD 17)
project( Webrtcexample )

ADD_DEFINITIONS(-DWEBRTC_POSIX=1)
ADD_DEFINITIONS(-D__STDC_CONSTANT_MACROS=1)

include_directories(
    ${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-linux/include
    ${CMAKE_SOURCE_DIR}/webrtc-checkout/src
    ${CMAKE_SOURCE_DIR}/webrtc-checkout/src/third_party/abseil-cpp
    ${CMAKE_SOURCE_DIR}/webrtc-checkout/src/third_party/libyuv/include
)

set(
  TARGETS
    dummy_audio_device.cpp
    frame_manipulation_example_main.cpp
)

add_executable( frame_manipulation_example ${TARGETS} )

target_link_libraries(frame_manipulation_example
    ${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-linux/lib/libixwebsocket.a
    ${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-linux/lib/libz.a
    ${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-linux/lib/libjsoncpp.a

    ${CMAKE_SOURCE_DIR}/webrtc-checkout/src/out/Default/obj/libwebrtc.a
)