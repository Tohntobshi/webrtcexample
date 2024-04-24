#include <iostream>
#include <api/peer_connection_interface.h>
#include <api/media_stream_interface.h>
#include <api/create_peerconnection_factory.h>
#include "api/audio_codecs/audio_decoder_factory.h"
#include "api/audio_codecs/audio_encoder_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/video_codecs/video_decoder_factory.h"
#include "api/video_codecs/video_decoder_factory_template.h"
#include "api/video_codecs/video_decoder_factory_template_dav1d_adapter.h"
#include "api/video_codecs/video_decoder_factory_template_libvpx_vp8_adapter.h"
#include "api/video_codecs/video_decoder_factory_template_libvpx_vp9_adapter.h"
#include "api/video_codecs/video_decoder_factory_template_open_h264_adapter.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "api/video_codecs/video_encoder_factory_template.h"
#include "api/video_codecs/video_encoder_factory_template_libaom_av1_adapter.h"
#include "api/video_codecs/video_encoder_factory_template_libvpx_vp8_adapter.h"
#include "api/video_codecs/video_encoder_factory_template_libvpx_vp9_adapter.h"
#include "api/video_codecs/video_encoder_factory_template_open_h264_adapter.h"
#include "api/video/i420_buffer.h"
#include "api/video/video_frame_buffer.h"

#include "rtc_base/ssl_adapter.h"

#include <ixwebsocket/IXWebSocket.h>
#include <jsoncpp/json/json.h>

#include "third_party/libyuv/include/libyuv/convert.h"
#include "third_party/libyuv/include/libyuv/convert_from.h"

#include "dummy_audio_device.h"

using std::cout;



// This class does nothing but its instance is required when setting descriptions.
class DummySetSessionDescriptionObserver : public webrtc::SetSessionDescriptionObserver {
public:
    static rtc::scoped_refptr<DummySetSessionDescriptionObserver> Create() {
        return rtc::make_ref_counted<DummySetSessionDescriptionObserver>();
    }
    void OnSuccess() {}
    void OnFailure(webrtc::RTCError error) {
        cout << "set description fail\n";
    }
};

// This class takes frames and returns modified frames.
// This is an example of how to read, create and manipulate pixel data of each frame.
class FrameTransformer {
public:
    uint8_t argbdata[1920 * 1080 * 4];
    long counter = 0;
    webrtc::VideoFrame transformFrame(const webrtc::VideoFrame & frame) {
        rtc::scoped_refptr<webrtc::I420BufferInterface> buffer(frame.video_frame_buffer()->ToI420());
        int width = buffer->width();
        int height = buffer->height();
        if (width > 1920 || height > 1080) {
            // it won't fit to argbdata, just returning the frame without changes
            return frame;
        }

        libyuv::I420ToARGB(buffer->DataY(), buffer->StrideY(),
            buffer->DataU(), buffer->StrideU(),
            buffer->DataV(), buffer->StrideV(),
            argbdata, width * 4, width, height);

        // now "argbdata" has completely decoded array of pixels and we can do anything with it

        // just painting moving diagonally 100x100 square on top of the received image as a test
        int h_start = counter % (height - 100);
        int w_start = counter % (width - 100);
        for (int h = h_start; h < h_start + 100; h++) {
            for (int w = w_start; w < w_start + 100; w++) {
                int pixIndex = h * width * 4 + w * 4;
                argbdata[pixIndex] = 255; // b
                argbdata[pixIndex + 1] = 255; // g
                argbdata[pixIndex + 2] = 0; // r
                argbdata[pixIndex + 3] = 255; // a
            }
        }
        counter++;

        // putting it to a new frame
        rtc::scoped_refptr<webrtc::I420Buffer> new_buffer = webrtc::I420Buffer::Create(width, height);
        libyuv::ARGBToI420(argbdata, width * 4,
            new_buffer->MutableDataY(), buffer->StrideY(),
            new_buffer->MutableDataU(), buffer->StrideU(),
            new_buffer->MutableDataV(), buffer->StrideV(),
            width, height);
        
        webrtc::VideoFrame new_frame =
          webrtc::VideoFrame::Builder()
              .set_video_frame_buffer(new_buffer)
              .set_rotation(frame.rotation())
              .set_timestamp_us(frame.timestamp_us())
              .set_id(frame.id())
              .build();
        return new_frame;
    }
};


// This class in an example of a source of video data, normally such class takes frames from camera for example, and writes them to the sink.
// But this example is device agnostic and when we want this class to send frame, we call sendFrame method explicitly with whatever data we want to send.
class MyVideoTrackSource : public webrtc::VideoTrackSourceInterface {
public:
    rtc::VideoSinkInterface<webrtc::VideoFrame>* sink_to_write = nullptr;
    // call this method to send frame
    void sendFrame(const webrtc::VideoFrame& frame) {
        if (!sink_to_write) return;
        sink_to_write->OnFrame(frame);
    }
    // VideoTrackSourceInterface implementation
    void AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink, const rtc::VideoSinkWants& wants) override {
        sink_to_write = sink;
        cout << "sink updated or added to my video source\n";
    }
    void RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) override {
        cout << "sink removed from my video source\n";
        sink_to_write = nullptr;
    }
    // just mock implementation of the rest of the methods
    webrtc::MediaSourceInterface::SourceState state() const override {
        return webrtc::MediaSourceInterface::SourceState::kLive;
    }
    bool remote() const override { return false; }
    bool is_screencast() const override { return false; }
    absl::optional<bool> needs_denoising() const override { return absl::nullopt; }
    bool GetStats(Stats* stats) override { return false; }
    bool SupportsEncodedOutput() const override { return false; }
    void GenerateKeyFrame() override {}
    void AddEncodedSink(rtc::VideoSinkInterface<webrtc::RecordableEncodedFrame>* sink) override {}
    void RemoveEncodedSink(rtc::VideoSinkInterface<webrtc::RecordableEncodedFrame>* sink) override {}
    // probably need to gather and notify these observers on state changes
    void RegisterObserver(webrtc::ObserverInterface* observer) override {}
    void UnregisterObserver(webrtc::ObserverInterface* observer) override {}
};

// This class is an example of audio data source. Normally it takes data from microphone and sends data to the sink but this example is device agnostic
// and when we want to send audio chunk, we call sendAudioData explicitly with whatever data we want to send
class MyAudioSource : public webrtc::AudioSourceInterface {
public:
    webrtc::AudioTrackSinkInterface* sink_to_write = nullptr;
    // call this method to send audio data
    void sendAudioData(const void* audio_data,
                      int bits_per_sample,
                      int sample_rate,
                      size_t number_of_channels,
                      size_t number_of_frames) {
        if (!sink_to_write) return;
        sink_to_write->OnData(audio_data, bits_per_sample, sample_rate, number_of_channels, number_of_frames);
    }
    // AudioSourceInterface implementation
    void AddSink(webrtc::AudioTrackSinkInterface* sink) override {
        sink_to_write = sink;
        cout << "sink added to my audio source\n";
    }
    void RemoveSink(webrtc::AudioTrackSinkInterface* sink) override {
        sink_to_write = nullptr;
        cout << "sink removed from my audio source\n";
    }
    const cricket::AudioOptions options() const override {
        return cricket::AudioOptions();
    }
    webrtc::MediaSourceInterface::SourceState state() const override {
        return webrtc::MediaSourceInterface::SourceState::kLive;
    }
    bool remote() const override { return false; }
    // probably need to gather and notify these observers on state changes
    void RegisterObserver(webrtc::ObserverInterface* observer) override {}
    void UnregisterObserver(webrtc::ObserverInterface* observer) override {}
};

// This class receives frames. Normally it should render them on the screen. But in this device agnostic example we just modify the frames and pass them back
class VideoReceiver : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
public:
    MyVideoTrackSource * destinationToWrite;
    FrameTransformer transformer;
    VideoReceiver(MyVideoTrackSource * destination): destinationToWrite(destination) {}
    // VideoSinkInterface implementation
    void OnFrame(const webrtc::VideoFrame& frame) override {
        // this is called on each received frame

        // check FrameTransformer class implementation to see how to access raw rgb data of a frame
    
        // at this point we can render these frames, record them and do anything we want with them

        // in this example we modify them and send them back
        destinationToWrite->sendFrame(transformer.transformFrame(frame));
    }
};

// This class receives audio samples. Normally it should play them. But in this device agnostic example we just pass them back
class AudioReceiver : public webrtc::AudioTrackSinkInterface {
public:
    MyAudioSource * destinationToWrite;
    AudioReceiver(MyAudioSource * destination): destinationToWrite(destination) {}
    // AudioTrackSinkInterface implementation
    void OnData(const void* audio_data, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_of_frames) override {
        // this is called every ~10 ms with audio data

        // at this point we have access to raw audio data from the counterpart

        // we are just sending back audio data, beware of echo
        destinationToWrite->sendAudioData(audio_data, bits_per_sample, sample_rate, number_of_channels, number_of_frames);
    }
};

class MyWebrtcApplication : public webrtc::PeerConnectionObserver, public webrtc::CreateSessionDescriptionObserver
{
public:
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory;
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection;
    rtc::scoped_refptr<MyVideoTrackSource> videoSource;
    rtc::scoped_refptr<MyAudioSource> audioSource;
    std::unique_ptr<VideoReceiver> videoReceiver;
    std::unique_ptr<AudioReceiver> audioReceiver;
    ix::WebSocket socket;
    void run() {
        rtc::InitializeSSL();
        std::string shouldMakeOffer;
        cout << "Should make offer? (y/n)\n";
        std::cin >> shouldMakeOffer;
        if (shouldMakeOffer == "y") {
            cout << "will make an offer in 5 seconds...\n";
        } else {
            cout << "will wait for an offer...\n";
        }
        auto signaling_thread = rtc::Thread::CreateWithSocketServer();
        signaling_thread->Start();

        factory = webrtc::CreatePeerConnectionFactory(
            nullptr /* network_thread */, nullptr /* worker_thread */, signaling_thread.get() /* signaling_thread*/,
            DummyAudioDeviceModule::Create(),
            webrtc::CreateBuiltinAudioEncoderFactory(),
            webrtc::CreateBuiltinAudioDecoderFactory(),
            std::make_unique<webrtc::VideoEncoderFactoryTemplate<
                webrtc::LibvpxVp8EncoderTemplateAdapter,
                webrtc::LibvpxVp9EncoderTemplateAdapter,
                webrtc::OpenH264EncoderTemplateAdapter,
                webrtc::LibaomAv1EncoderTemplateAdapter>>(),
            std::make_unique<webrtc::VideoDecoderFactoryTemplate<
                webrtc::LibvpxVp8DecoderTemplateAdapter,
                webrtc::LibvpxVp9DecoderTemplateAdapter,
                webrtc::OpenH264DecoderTemplateAdapter,
                webrtc::Dav1dDecoderTemplateAdapter>>(),
            nullptr /* audio_mixer */, nullptr /* audio_processing */);

        webrtc::PeerConnectionInterface::RTCConfiguration config;
        config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
        webrtc::PeerConnectionInterface::IceServer server;
        server.urls.push_back("stun:stun.l.google.com:19302");
        config.servers.push_back(server);

        webrtc::PeerConnectionDependencies pc_dependencies(this);
        auto error_or_peer_connection = factory->CreatePeerConnectionOrError(config, std::move(pc_dependencies));
        if (!error_or_peer_connection.ok())
        {
            cout << "couldn't create peerconnection\n";
            return;
        }
        peer_connection = std::move(error_or_peer_connection.value());

        videoSource = rtc::make_ref_counted<MyVideoTrackSource>();
        rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track(
            factory->CreateVideoTrack(videoSource, "video_label")
        );
        auto result_or_error = peer_connection->AddTrack(video_track, {"stream_id"});
        if (!result_or_error.ok()) {
            cout << "Failed to add video track to PeerConnection\n";
        }

        audioSource = rtc::make_ref_counted<MyAudioSource>();
        rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
            factory->CreateAudioTrack("audio_label", audioSource.get())
        );
        result_or_error = peer_connection->AddTrack(audio_track, {"stream_id"});
        if (!result_or_error.ok()) {
            cout << "Failed to add audio track to PeerConnection\n";
        }

        // we are passing source to renderer to send the received data back
        videoReceiver = std::make_unique<VideoReceiver>(videoSource.get());
        audioReceiver = std::make_unique<AudioReceiver>(audioSource.get());


        if (shouldMakeOffer == "y") {
            signaling_thread->PostDelayedTask([&]() -> void {
                cout << "delayed create offer task " << '\n';
                peer_connection->CreateOffer(this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
            }, webrtc::TimeDelta::Seconds(5));
        }

        socket.setOnMessageCallback([&](const ix::WebSocketMessagePtr& msg) {
            if (msg->type == ix::WebSocketMessageType::Message)
            {
                Json::CharReaderBuilder rbuilder;
                std::unique_ptr<Json::CharReader> const reader(rbuilder.newCharReader());
                auto & str = msg->str;
                Json::Value jmessage;
                if (!reader->parse(str.data(), str.data() + str.length(), &jmessage, nullptr)) {
                    cout << "couldn't parse message\n";
                    return;
                }

                auto offer = jmessage.get("offer", "");
                if (offer.isObject()) {
                    std::string type_str = offer.get("type", "").asString();
                    if (type_str == "offer") {
                        std::string sdp = offer.get("sdp", "").asString();
                        absl::optional<webrtc::SdpType> type_maybe = webrtc::SdpTypeFromString(type_str);
                        webrtc::SdpParseError error;
                        std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
                            webrtc::CreateSessionDescription(*type_maybe, sdp, &error);
                        if (!session_description) {
                            cout << "couldn't parse sdp offer\n";
                            return;
                        }
                        peer_connection->SetRemoteDescription(
                            DummySetSessionDescriptionObserver::Create().get(),
                            session_description.release());
                        peer_connection->CreateAnswer(this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
                    }
                    return;
                }
                auto answer = jmessage.get("answer", "");
                if (answer.isObject()) {
                    std::string type_str = answer.get("type", "").asString();
                    if (type_str == "answer") {
                        std::string sdp = answer.get("sdp", "").asString();
                        absl::optional<webrtc::SdpType> type_maybe = webrtc::SdpTypeFromString(type_str);
                        webrtc::SdpParseError error;
                        std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
                            webrtc::CreateSessionDescription(*type_maybe, sdp, &error);
                        if (!session_description) {
                            cout << "couldn't parse sdp answer\n";
                            return;
                        }
                        peer_connection->SetRemoteDescription(
                            DummySetSessionDescriptionObserver::Create().get(),
                            session_description.release());
                    }
                    return;
                }
                auto candidateObj = jmessage.get("candidate", "");
                if (candidateObj.isObject()) {
                    std::string sdp_mid = candidateObj.get("sdpMid", "").asString();
                    int sdp_mlineindex = candidateObj.get("sdpMLineIndex", "").asInt();
                    std::string sdp = candidateObj.get("candidate", "").asString();
                    webrtc::SdpParseError error;
                    std::unique_ptr<webrtc::IceCandidateInterface> candidate(
                        webrtc::CreateIceCandidate(sdp_mid, sdp_mlineindex, sdp, &error));
                    if (!candidate.get()) {
                        cout << "Can't parse received candidate message.\n";
                        return;
                    }
                    if (!peer_connection->AddIceCandidate(candidate.get())) {
                        cout << "Failed to apply the received candidate\n";
                        return;
                    }
                    // cout << "Applied the received candidate\n";
                    return;
                }
            }
        });
        std::string url("ws://localhost:8080/");
        socket.setUrl(url);
        socket.run();
    }



    // CreateSessionDescriptionObserver implementation
    void OnSuccess(webrtc::SessionDescriptionInterface* desc) override {
        // this method is called when session description is created
        // cout << "description created\n";
        peer_connection->SetLocalDescription(
            DummySetSessionDescriptionObserver::Create().get(), desc);

        std::string sdp;
        desc->ToString(&sdp);

        Json::Value jmessage;
        Json::Value descriptionObj;
        std::string typeStr = webrtc::SdpTypeToString(desc->GetType());
        descriptionObj["type"] = typeStr;
        descriptionObj["sdp"] = sdp;
        jmessage[typeStr] = descriptionObj;
        Json::StreamWriterBuilder factory;
        socket.send(Json::writeString(factory, jmessage));
    }
    void OnFailure(webrtc::RTCError error) override {
        cout << "failed to create description\n";
    }




    // PeerConnectionObserver implementation
    void OnAddTrack(
        rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
        const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>> &streams
    ) override {
        // this method is called when we receive track data from counterpart
        cout << "on add track\n";
        auto* track = receiver->track().release();
        if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
            auto* video_track = static_cast<webrtc::VideoTrackInterface*>(track);
            video_track->AddOrUpdateSink(videoReceiver.get(), rtc::VideoSinkWants());
        }
        if (track->kind() == webrtc::MediaStreamTrackInterface::kAudioKind) {
            auto* audio_track = static_cast<webrtc::AudioTrackInterface*>(track);
            audio_track->AddSink(audioReceiver.get());
        }
        track->Release();
    }
    void OnIceCandidate(const webrtc::IceCandidateInterface *candidate) override {
        // this method is called when we find ice candidate
        // cout << "found candidate\n";
        Json::Value candidateObj;
        candidateObj["sdpMid"] = candidate->sdp_mid();
        candidateObj["sdpMLineIndex"] = candidate->sdp_mline_index();
        std::string sdp;
        if (!candidate->ToString(&sdp)) {
            cout << "Failed to serialize candidate";
            return;
        }
        candidateObj["candidate"] = sdp;
        Json::Value jmessage;
        jmessage["candidate"] = candidateObj;
        Json::StreamWriterBuilder factory;
        socket.send(Json::writeString(factory, jmessage));
    }
    void OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState new_state) override {
        cout << "connection change " << webrtc::PeerConnectionInterface::AsString(new_state) << "\n";
    }
    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override {}
    void OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) override {}
    void OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override {}
    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {}
    void OnRenegotiationNeeded() override {}
    void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override {}
    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override {}
    void OnIceConnectionReceivingChange(bool receiving) override {}
    void OnStandardizedIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override {}
    void OnIceCandidateError(const std::string& address, int port, const std::string& url, int error_code, const std::string& error_text) override {}
};

int main(int argc, char *argv[]) {
    auto app = rtc::make_ref_counted<MyWebrtcApplication>();
    app->run();
    return 0;
}