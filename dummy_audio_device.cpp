#include <iostream>
#include <thread>
#include <api/peer_connection_interface.h>
#include <api/media_stream_interface.h>
#include "rtc_base/time_utils.h"
#include "dummy_audio_device.h"

using std::cout;

rtc::scoped_refptr<DummyAudioDeviceModule> DummyAudioDeviceModule::Create() {
    return rtc::make_ref_counted<DummyAudioDeviceModule>();
}
int32_t DummyAudioDeviceModule::RegisterAudioCallback(webrtc::AudioTransport* _audioCallback) {
    bool just_created = false;
    {
        webrtc::MutexLock lock(&mutex);
        audioCallback = _audioCallback;

        if (!process_thread) {
            process_thread = rtc::Thread::Create();
            process_thread->Start();
            just_created = true;
        }
    }
    if (just_created) {
        process_thread->PostTask([this] {
            this->processFrame();
        });
    }
    return 0;
}
DummyAudioDeviceModule::~DummyAudioDeviceModule() {
    webrtc::MutexLock lock(&mutex);
    if (process_thread) {
        process_thread->Stop();
    }
}

void DummyAudioDeviceModule::processFrame() {
    int64_t time1 = rtc::TimeMillis();
    {
        webrtc::MutexLock lock(&mutex);
        size_t nSamplesOut = 0;
        int64_t elapsed_time_ms = 0;
        int64_t ntp_time_ms = 0;
        memset(data, 0, sizeof(data));
        if (this->audioCallback != nullptr) {
            this->audioCallback->NeedMorePlayData(AUDIO_SAMPLES_IN_WEBRTC_FRAME, 2, 1, AUDIO_SAMPLE_RATE, data, nSamplesOut, &elapsed_time_ms, &ntp_time_ms);
        }
    }
    int64_t time2 = rtc::TimeMillis();
    int64_t next_frame_time = time1 + frame_length;
    int64_t wait_time = next_frame_time - time2;
    process_thread->PostDelayedTask([this]() -> void {
        this->processFrame();
    }, webrtc::TimeDelta::Millis(wait_time));
}
int32_t DummyAudioDeviceModule::PlayoutDelay(uint16_t* delay_ms) const {
    *delay_ms = 0;
    return 0;
}
