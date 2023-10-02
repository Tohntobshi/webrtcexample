# Libwebrtc usage example

Simple libwebrtc usage example with minimum unrelated logic. It uses ixwebsocket for sending and receiving offers, answers and candidates. It just receives frames from the browser and sends them back.


## Libwebrtc build parameters

    gn gen out/Default --args='is_debug=false is_component_build=false rtc_include_tests=false use_custom_libcxx=false treat_warnings_as_errors=false use_ozone=true rtc_use_h264=true rtc_use_x11=false'

## How to build debug version

    cmake -DCMAKE_BUILD_TYPE=Debug ..

## How to debug segfault (different versions of dependencies may cause that)

    gdb webrtcexample
    (gdb) run