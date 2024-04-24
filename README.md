# Libwebrtc usage example

Simple libwebrtc usage example with minimum unrelated logic. It uses ixwebsocket for sending and receiving offers, answers and candidates. It just receives frames from the browser and sends them back.


## Libwebrtc build parameters

    gn gen out/Default --args='is_debug=false is_component_build=false rtc_include_tests=false use_custom_libcxx=false treat_warnings_as_errors=false use_ozone=true rtc_use_x11=false use_rtti=true'

Start signaling server

    npm run server

Start browser client

    npm run client

Open browser client on

    http://127.0.0.1:8000/ (or whatever address you see in console when you start the client)


