#include <iostream>
#include <thread>
#include <api/peer_connection_interface.h>
#include <api/media_stream_interface.h>
#include <api/create_peerconnection_factory.h>
#include "rtc_base/time_utils.h"

class DummyAudioDeviceModule : public webrtc::AudioDeviceModule {
 public:
    bool playing = false;
    webrtc::AudioTransport* audioCallback = nullptr;    
    static rtc::scoped_refptr<AudioDeviceModule> Create() {
        return rtc::make_ref_counted<DummyAudioDeviceModule>();
    }
    int32_t ActiveAudioLayer(AudioLayer* audioLayer) const override { return 0; }
    int32_t RegisterAudioCallback(webrtc::AudioTransport* _audioCallback) override {
        audioCallback = _audioCallback;
        std::cout << "audio callback\n";
        return 0;
    }
    int32_t Init() override { return 0; }
    int32_t Terminate() override { return 0; }
    bool Initialized() const override { return true; }
    int16_t PlayoutDevices() override { return 0; }
    int16_t RecordingDevices() override { return 0; }
    int32_t PlayoutDeviceName(uint16_t index,
                                    char name[webrtc::kAdmMaxDeviceNameSize],
                                    char guid[webrtc::kAdmMaxGuidSize]) override { return 0; }
    int32_t RecordingDeviceName(uint16_t index,
                                      char name[webrtc::kAdmMaxDeviceNameSize],
                                      char guid[webrtc::kAdmMaxGuidSize]) override { return 0; }
    int32_t SetPlayoutDevice(uint16_t index) override { return 0; }
    int32_t SetPlayoutDevice(WindowsDeviceType device) override { return 0; }
    int32_t SetRecordingDevice(uint16_t index) override { return 0; }
    int32_t SetRecordingDevice(WindowsDeviceType device) override { return 0; }
    int32_t PlayoutIsAvailable(bool* available) override {
        std::cout << "is available call\n";
        return 0;
    }
    int32_t InitPlayout() override { return 0; }
    bool PlayoutIsInitialized() const override { return true; }
    int32_t RecordingIsAvailable(bool* available) override { return 0; }
    int32_t InitRecording() override { return 0; }
    bool RecordingIsInitialized() const override { return true; }
    int32_t StartPlayout() override {
        std::cout << "start playout\n";
        playing = true;
        std::thread tr([this]() -> void {
            uint8_t data[16 * 480];
            while (this->playing)
            {
                size_t nSamplesOut = 0;
                int64_t elapsed_time_ms = 0;
                int64_t ntp_time_ms = 0;
                int64_t time1 = rtc::TimeMillis();
                this->audioCallback->NeedMorePlayData(480, 2, 1, 48000, data, nSamplesOut, &elapsed_time_ms, &ntp_time_ms);
                int64_t time2 = rtc::TimeMillis();
                int64_t next_frame_time = time1 + 10;
                if (next_frame_time > time2) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(next_frame_time - time2));
                }
            }
            std::cout << "not playing anymore\n";
        });
        tr.detach();
        return 0;
    }
    int32_t StopPlayout() override {
        std::cout << "stop playout\n";
        playing = false;
        return 0;
    }
    bool Playing() const override { return playing; }
    int32_t StartRecording() override { return 0; }
    int32_t StopRecording() override { return 0; }
    bool Recording() const override { return 0; }
    int32_t InitSpeaker() override { return 0; }
    bool SpeakerIsInitialized() const override { return true; }
    int32_t InitMicrophone() override { return 0; }
    bool MicrophoneIsInitialized() const override { return true; }
    int32_t SpeakerVolumeIsAvailable(bool* available) override { return 0; }
    int32_t SetSpeakerVolume(uint32_t volume) override { return 0; }
    int32_t SpeakerVolume(uint32_t* volume) const override { return 0; }
    int32_t MaxSpeakerVolume(uint32_t* maxVolume) const override { return 0; }
    int32_t MinSpeakerVolume(uint32_t* minVolume) const override { return 0; }
    int32_t MicrophoneVolumeIsAvailable(bool* available) override { return 0; }
    int32_t SetMicrophoneVolume(uint32_t volume) override { return 0; }
    int32_t MicrophoneVolume(uint32_t* volume) const override { return 0; }
    int32_t MaxMicrophoneVolume(uint32_t* maxVolume) const override { return 0; }
    int32_t MinMicrophoneVolume(uint32_t* minVolume) const override { return 0; }
    int32_t SpeakerMuteIsAvailable(bool* available) override { return 0; }
    int32_t SetSpeakerMute(bool enable) override { return 0; }
    int32_t SpeakerMute(bool* enabled) const override { return 0; }
    int32_t MicrophoneMuteIsAvailable(bool* available) override { return 0; }
    int32_t SetMicrophoneMute(bool enable) override { return 0; }
    int32_t MicrophoneMute(bool* enabled) const override { return 0; }
    int32_t StereoPlayoutIsAvailable(bool* available) const override { return 0; }
    int32_t SetStereoPlayout(bool enable) override { return 0; }
    int32_t StereoPlayout(bool* enabled) const override { return 0; }
    int32_t StereoRecordingIsAvailable(bool* available) const override { return 0; }
    int32_t SetStereoRecording(bool enable) override { return 0; }
    int32_t StereoRecording(bool* enabled) const override { return 0; }
    int32_t PlayoutDelay(uint16_t* delayMS) const override { return 0; }
    bool BuiltInAECIsAvailable() const override { return 0; }
    bool BuiltInAGCIsAvailable() const override { return 0; }
    bool BuiltInNSIsAvailable() const override { return 0; }
    int32_t EnableBuiltInAEC(bool enable) override { return -1; }
    int32_t EnableBuiltInAGC(bool enable) override { return -1; }
    int32_t EnableBuiltInNS(bool enable) override { return -1; }

// Only supported on iOS.
#if defined(WEBRTC_IOS)
    int GetPlayoutAudioParameters(AudioParameters* params) const override { return 0; }
    int GetRecordAudioParameters(AudioParameters* params) const override { return 0; }
#endif  // WEBRTC_IOS

protected:
    ~DummyAudioDeviceModule() override {}
};