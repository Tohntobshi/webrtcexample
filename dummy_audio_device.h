#pragma once
#include <api/peer_connection_interface.h>
#include <api/media_stream_interface.h>
#include "rtc_base/synchronization/mutex.h"

#define AUDIO_SAMPLES_IN_WEBRTC_FRAME 480
#define AUDIO_SAMPLE_RATE 48000

class DummyAudioDeviceModule : public webrtc::AudioDeviceModule {
    webrtc::Mutex mutex;
    webrtc::AudioTransport* audioCallback = nullptr;
    std::unique_ptr<rtc::Thread> process_thread;
    uint8_t data[2 * AUDIO_SAMPLES_IN_WEBRTC_FRAME];
    int frame_length = (AUDIO_SAMPLES_IN_WEBRTC_FRAME * 1000) / AUDIO_SAMPLE_RATE;
    void processFrame();
public:
    static rtc::scoped_refptr<DummyAudioDeviceModule> Create();
    int32_t RegisterAudioCallback(webrtc::AudioTransport* _audioCallback) override;
    int32_t ActiveAudioLayer(AudioLayer* audioLayer) const override { return 0; };
    int32_t Init() override { return 0; };
    int32_t Terminate() override{ return 0; };
    bool Initialized() const override { return 0; };
    int16_t PlayoutDevices() override { return 0; };
    int16_t RecordingDevices() override { return 0; };
    int32_t PlayoutDeviceName(uint16_t index,
                                    char name[webrtc::kAdmMaxDeviceNameSize],
                                    char guid[webrtc::kAdmMaxGuidSize]) override { return 0; };
    int32_t RecordingDeviceName(uint16_t index,
                                      char name[webrtc::kAdmMaxDeviceNameSize],
                                      char guid[webrtc::kAdmMaxGuidSize]) override { return 0; };
    int32_t SetPlayoutDevice(uint16_t index) override { return 0; };
    int32_t SetPlayoutDevice(WindowsDeviceType device) override { return 0; };
    int32_t SetRecordingDevice(uint16_t index) override { return 0; };
    int32_t SetRecordingDevice(WindowsDeviceType device) override { return 0; };
    int32_t PlayoutIsAvailable(bool* available) override { return 0; };
    int32_t InitPlayout() override { return 0; };
    bool PlayoutIsInitialized() const override { return 0; };
    int32_t RecordingIsAvailable(bool* available) override { return 0; };
    int32_t InitRecording() override { return 0; };
    bool RecordingIsInitialized() const override { return 0; };
    int32_t StartPlayout() override { return 0; };
    int32_t StopPlayout() override { return 0; };
    bool Playing() const override { return 0; };
    int32_t StartRecording() override { return 0; };
    int32_t StopRecording() override { return 0; };
    bool Recording() const override { return 0; };
    int32_t InitSpeaker() override { return 0; };
    bool SpeakerIsInitialized() const override { return 0; };
    int32_t InitMicrophone() override { return 0; };
    bool MicrophoneIsInitialized() const override { return 0; };
    int32_t SpeakerVolumeIsAvailable(bool* available) override { return 0; };
    int32_t SetSpeakerVolume(uint32_t volume) override { return 0; };
    int32_t SpeakerVolume(uint32_t* volume) const override { return 0; };
    int32_t MaxSpeakerVolume(uint32_t* maxVolume) const override { return 0; };
    int32_t MinSpeakerVolume(uint32_t* minVolume) const override { return 0; };
    int32_t MicrophoneVolumeIsAvailable(bool* available) override { return 0; };
    int32_t SetMicrophoneVolume(uint32_t volume) override { return 0; };
    int32_t MicrophoneVolume(uint32_t* volume) const override { return 0; };
    int32_t MaxMicrophoneVolume(uint32_t* maxVolume) const override { return 0; };
    int32_t MinMicrophoneVolume(uint32_t* minVolume) const override { return 0; };
    int32_t SpeakerMuteIsAvailable(bool* available) override { return 0; };
    int32_t SetSpeakerMute(bool enable) override { return 0; };
    int32_t SpeakerMute(bool* enabled) const override { return 0; };
    int32_t MicrophoneMuteIsAvailable(bool* available) override { return 0; };
    int32_t SetMicrophoneMute(bool enable) override { return 0; };
    int32_t MicrophoneMute(bool* enabled) const override { return 0; };
    int32_t StereoPlayoutIsAvailable(bool* available) const override { return 0; };
    int32_t SetStereoPlayout(bool enable) override { return 0; };
    int32_t StereoPlayout(bool* enabled) const override { return 0; };
    int32_t StereoRecordingIsAvailable(bool* available) const override { return 0; };
    int32_t SetStereoRecording(bool enable) override { return 0; };
    int32_t StereoRecording(bool* enabled) const override { return 0; };
    int32_t PlayoutDelay(uint16_t* delay_ms) const override;
    bool BuiltInAECIsAvailable() const override { return 0; };
    bool BuiltInAGCIsAvailable() const override { return 0; };
    bool BuiltInNSIsAvailable() const override { return 0; };
    int32_t EnableBuiltInAEC(bool enable) override { return -1; };
    int32_t EnableBuiltInAGC(bool enable) override { return -1; };
    int32_t EnableBuiltInNS(bool enable) override { return -1; };

// Only supported on iOS.
#if defined(WEBRTC_IOS)
    int GetPlayoutAudioParameters(AudioParameters* params) const override { return 0; };
    int GetRecordAudioParameters(AudioParameters* params) const override { return 0; };
#endif  // WEBRTC_IOS

protected:
    ~DummyAudioDeviceModule() override;
};