#include <iostream>
#include "fftest.h"

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

#include <thread>



using std::cout;


class DummySetSessionDescriptionObserver : public webrtc::SetSessionDescriptionObserver {
public:
    static rtc::scoped_refptr<DummySetSessionDescriptionObserver> Create() {
        return rtc::make_ref_counted<DummySetSessionDescriptionObserver>();
    }
    void OnSuccess() {
        cout << std::this_thread::get_id() << " set description callback\n";
    }
    void OnFailure(webrtc::RTCError error) {
        cout << "set description fail\n";
    }
};

class MyVideoTrackSource : public webrtc::VideoTrackSourceInterface {
public:
    rtc::VideoSinkInterface<webrtc::VideoFrame>* sink_to_write;
    // maybe mutex is needed here
    // call this method to send frame
    void writeFrameToSink(const webrtc::VideoFrame& frame) {
        if (!sink_to_write) return;
        sink_to_write->OnFrame(frame);
    }
    // VideoTrackSourceInterface implementation
    void AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink, const rtc::VideoSinkWants& wants) override {
        sink_to_write = sink;
        cout << std::this_thread::get_id() << " sink added to my video source\n";
    }
    void RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) override {
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

class VideoRenderer : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
public:
    rtc::scoped_refptr<webrtc::VideoTrackInterface> rendered_track;
    MyVideoTrackSource * destinationToWrite;
    VideoRecorder * recorder;
    VideoRenderer(webrtc::VideoTrackInterface* track_to_render, MyVideoTrackSource * destination, VideoRecorder * rec):
        rendered_track(track_to_render), destinationToWrite(destination), recorder(rec)
    {
        rendered_track->AddOrUpdateSink(this, rtc::VideoSinkWants());
        cout << "created video renderer\n";
    }
    ~VideoRenderer() {
        rendered_track->RemoveSink(this);
        cout << "destroyed video renderer\n";
    }
    // VideoSinkInterface implementation
    void OnFrame(const webrtc::VideoFrame& frame) override {
        // here i can write frame before sending it back

        recorder->recordFrame(frame);
        destinationToWrite->writeFrameToSink(frame);
    }
};

class AudioRenderer : public webrtc::AudioTrackSinkInterface {
public:
    rtc::scoped_refptr<webrtc::AudioTrackInterface> rendered_track;
    VideoRecorder * recorder;
    AudioRenderer(webrtc::AudioTrackInterface* track_to_render, VideoRecorder * rec):
        rendered_track(track_to_render), recorder(rec) {
        rendered_track->AddSink(this);
        cout << "created audio renderer\n";
    }
    ~AudioRenderer() {
        rendered_track->RemoveSink(this);
        cout << "destroyed audio renderer\n";
    }
    // AudioTrackSinkInterface implementation
    void OnData(const void* audio_data, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_of_frames) override {
        // cout << "on audio data \n";
        recorder->recordAFrame(audio_data, bits_per_sample, sample_rate, number_of_channels, number_of_frames);
    }
};

class MyWebrtcHolder : public webrtc::PeerConnectionObserver, webrtc::CreateSessionDescriptionObserver
{
public:
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory;
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection;
    rtc::scoped_refptr<MyVideoTrackSource> videoSource;
    ix::WebSocket socket;

    VideoRecorder videoRecorder;
    void run() {
        cout << std::this_thread::get_id() << " start...\n";
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
            nullptr /* default_adm */,
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

        // rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
        //     factory->CreateAudioTrack("audio_label", factory->CreateAudioSource(cricket::AudioOptions()).get()));
        // result_or_error = peer_connection->AddTrack(audio_track, {"stream_id"});
        // if (!result_or_error.ok()) {
        //     cout << "Failed to add audio track to PeerConnection\n";
        // }

        if (shouldMakeOffer == "y") {
            signaling_thread->PostDelayedTask([&]() -> void {
                cout << std::this_thread::get_id() << " delayed create offer task " << '\n';
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
                // need to check how it will handle invalid json or with wrong shape
                // cout << "message " << str << '\n';

                auto offer = jmessage.get("offer", "");
                if (offer.isObject()) {
                    std::string type_str = offer.get("type", "").asString();
                    if (type_str == "offer") {
                        std::string sdp = offer.get("sdp", "").asString();
                        cout << std::this_thread::get_id() << " received sdp offer " << '\n';
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
                        cout << std::this_thread::get_id() << " received sdp answer " << '\n';
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
                    // cout << "received " << sdp_mid << " " << sdp_mlineindex << "\n";
                    // cout << sdp << "\n";
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
                    cout << std::this_thread::get_id() << " Applied the received candidate\n";
                    return;
                }
            }
        });
        std::string url("ws://localhost:8080/");
        socket.setUrl(url);
        socket.run();
    }
    // CreateSessionDescriptionObserver
    void OnSuccess(webrtc::SessionDescriptionInterface* desc) override {
        cout << std::this_thread::get_id() << " description created\n";
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
    // PeerConnectionObserver
    void OnAddTrack(
        rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
        const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>> &
            streams) override {
        cout << std::this_thread::get_id() << " on add track\n";
        auto* track = receiver->track().release();
        if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
            auto* video_track = static_cast<webrtc::VideoTrackInterface*>(track);
            // just test, this needs to be deleted when is not necessary
            new VideoRenderer(video_track, videoSource.get(), &videoRecorder);
        }
        if (track->kind() == webrtc::MediaStreamTrackInterface::kAudioKind) {
            auto* audio_track = static_cast<webrtc::AudioTrackInterface*>(track);
            // just test, this needs to be deleted when is not necessary
            new AudioRenderer(audio_track, &videoRecorder);
        }
        track->Release();

        // or just send the track back
        // peer_connection->AddTrack(receiver->track(), {"stream_id"});
    }
    void OnIceCandidate(const webrtc::IceCandidateInterface *candidate) override {
        cout << std::this_thread::get_id() << " found candidate\n";
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
        if (new_state == webrtc::PeerConnectionInterface::PeerConnectionState::kConnected) {
            videoRecorder.start();
        }
        if (new_state == webrtc::PeerConnectionInterface::PeerConnectionState::kDisconnected) {
            videoRecorder.finish();
        }
    }
    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override {
        cout << "signaling change " << webrtc::PeerConnectionInterface::AsString(new_state) << "\n";
    }
    void OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) override {}
    void OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override {}
    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {}
    void OnRenegotiationNeeded() override {}
    void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override {}
    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override {}
    void OnIceConnectionReceivingChange(bool receiving) override {}
    void OnStandardizedIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override {
        cout << "ice connection change " << webrtc::PeerConnectionInterface::AsString(new_state) << "\n";
    }
    void OnIceCandidateError(const std::string& address, int port, const std::string& url, int error_code, const std::string& error_text) override {
        cout << "ice candidate error\n";
    }
};

int main(int argc, char *argv[]) {
    auto rtcHolder = rtc::make_ref_counted<MyWebrtcHolder>();
    rtcHolder->run();
    return 0;
}