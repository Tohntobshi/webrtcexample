# Libwebrtc usage example

Simple libwebrtc usage example with minimum unrelated logic. It just receives frames from the browser, manipulates some pixels and sends them back.

## How to build

### vcpkg

You need to have vcpkg utility installed to download dependencies. Run in the project's root

    vcpkg install


### libwebrtc

Then you need to download and compile webrtc library.

Download depot-tools, instruction is here: https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up (basically you need to do 2 steps: clone repository and set path to depot tools)

Download and build webrtc. In root of the project run

    mkdir webrtc-checkout

    cd webrtc-checkout

    fetch --nohooks webrtc

    cd src

    git checkout branch-heads/6030

    gclient sync

    ./build/install-build-deps.sh

    gn gen out/Default --args='is_debug=false is_component_build=false rtc_include_tests=false use_custom_libcxx=false treat_warnings_as_errors=false use_ozone=true rtc_use_x11=false use_rtti=true rtc_build_examples=false'

    ninja -C out/Default

After that you should have static library here <project_root>/webrtc-checkout/src/out/Default/obj/libwebrtc.a

If something does not compile try to play with flags or different branches

To see all possible flags run `gn args <build_dir> --list`

Some info can be found here https://webrtc.github.io/webrtc-org/native-code/

### build the project

You need to have cmake and other tools for c++ compilation installed. In project's root execute

    mkdir build
    cd build
    cmake ..
    make


### js dependencies

You need to have nodejs installed. Go to project's root and

Download js dependencies

    npm install

## How to run

Start signaling server

    npm run server

Start browser client

    npm run client

Run the app

    ./build/frame_manipulation_example

And answer 'n', when asked if you want to send an offer

Open browser client on http://127.0.0.1:8000/ (or whatever address you see in console when you start the client) and click 'send offer'

At this point you should see original webcam image on the left and the same image with floating square which was sent and returned back from webrtc app on the right




