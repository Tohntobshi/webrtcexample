# Libwebrtc usage example

Simple libwebrtc usage example with minimum unrelated logic. It just receives frames from the browser, manipulates some pixels and sends them back.

## How to build

### vcpkg

You need to have vcpkg utility installed to download dependencies. Run in the project's root

    vcpkg install


### libwebrtc

Then you need to download and compile webrtc library.

Download depot-tools, instruction is here: https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up (basically you need to do 2 steps: clone repository and set path to depot tools)

Download and build webrtc. Create webrtc-checkout directory in the root of the project and follow the rest of the instructions here: https://webrtc.github.io/webrtc-org/native-code/development/

on linux after `gclient sync` you need to execute `./build/install-build-deps.sh` in src directory of webrtc-checkout

optimal flags for building webrtc:

    gn gen out/Default --args='is_debug=false is_component_build=false rtc_include_tests=false use_custom_libcxx=false treat_warnings_as_errors=false use_ozone=true rtc_use_x11=false use_rtti=true rtc_build_examples=false is_clang=false'

To see all possible flags run `gn args <build_dir> --list`

After that you should have static library here <project_root>/webrtc-checkout/src/out/Default/obj/libwebrtc.a

### build the project

You need to have cmake installed. In project's root execute

    mkdir build
    cd build
    cmake ..
    make


### signaling server

Go to project's root

Download js dependencies

    npm install

Start signaling server

    npm run server

### start clients

Start browser client

    npm run client

Run the app and enter 'n' when asked to send offer

    ./build/frame_manipulation_example

Open browser client on http://127.0.0.1:8000/ (or whatever address you see in console when you start the client) and click 'send offer'

At this point you should see original webcam image on the left and the same image with floating square which was sent and returned back from webrtc app on the right




