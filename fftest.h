#include <api/media_stream_interface.h>
#include <mutex>
#include <chrono>
extern "C"
{
// #include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
// #include <libavutil/mathematics.h>
// #include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
// #include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

class VideoRecorder {
    std::mutex myMtx;
    AVChannelLayout channel_layout;
    int width;
    int height;
    AVFormatContext *oc;
    // video
    AVCodecContext *v_context;
    const AVCodec *v_codec;
    AVPacket *v_packet;
    AVStream *v_stream;
    AVFrame * v_frame;
    // audio
    AVCodecContext *a_context;
    const AVCodec *a_codec;
    AVPacket *a_packet;
    AVStream *a_stream;
    AVFrame *a_frame;
    // AVFrame *a_t_frame;
    struct SwrContext *swr_ctx;
    // -------------------
    AVDictionary *opt;
    int a_pts;
    bool started = false;
    std::chrono::system_clock::time_point start_time;
    int last_v_pts;
    void writeASample(float sample);
    int total_written_a_samples;
public:
    void start();
    void recordFrame(const webrtc::VideoFrame & frame);
    void recordAFrame(const void* audio_data, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_of_frames);
    void finish();
};
