cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_CXX_STANDARD 17)
project( Webrtcexample )

ADD_DEFINITIONS(-DWEBRTC_POSIX=1)

include_directories(
    /home/tohntobshi/projects/webrtc-checkout/src
    /home/tohntobshi/projects/webrtc-checkout/src/third_party/abseil-cpp
)

set(
  TARGETS
    main.cpp
)

add_executable( webrtcexample ${TARGETS} )

target_link_libraries(webrtcexample
    pthread

    ixwebsocket
    z
    # ssl

    jsoncpp

    /home/tohntobshi/projects/webrtc-checkout/src/out/Default/obj/libwebrtc.a

    # X11
)