#include <iostream>

#include "fftest.h"
#include "third_party/libyuv/include/libyuv/convert.h"
#include "third_party/libyuv/include/libyuv/convert_from.h"

using std::cout;


void VideoRecorder::start() {
    std::unique_lock lock(myMtx);
    if (started) return;
    width = 640;
    height = 480;
    v_context = nullptr;
    oc = nullptr;
    v_codec = nullptr;
    v_packet = nullptr;
    v_stream = nullptr;
    a_codec = nullptr;
    a_packet = nullptr;
    a_stream = nullptr;
    opt = nullptr;
    last_v_pts = -1;
    total_written_a_samples = 0;
    channel_layout = AV_CHANNEL_LAYOUT_MONO;
    start_time = std::chrono::system_clock::now();

    std::string filename = std::to_string(start_time.time_since_epoch().count()) + ".mp4";
    int ret = 0;
    ret = avformat_alloc_output_context2(&oc, NULL, NULL, filename.c_str());

    v_codec = avcodec_find_encoder(oc->oformat->video_codec);
    v_packet = av_packet_alloc();
    v_stream = avformat_new_stream(oc, NULL);
    v_stream->id = oc->nb_streams-1;
    v_context = avcodec_alloc_context3(v_codec);

    a_codec = avcodec_find_encoder(oc->oformat->audio_codec);
    a_packet = av_packet_alloc();
    a_stream = avformat_new_stream(oc, NULL);
    a_stream->id = oc->nb_streams-1;
    a_context = avcodec_alloc_context3(a_codec);

    v_context->codec_id = oc->oformat->video_codec;
    v_context->bit_rate = 400000;
    v_context->width = width;
    v_context->height = height;
    v_stream->time_base = (AVRational){ 1, 30 };
    v_context->time_base = v_stream->time_base;
    v_context->gop_size = 12; /* emit one intra frame every twelve frames at most */
    v_context->pix_fmt = AV_PIX_FMT_YUV420P;
    if (v_context->codec_id == AV_CODEC_ID_MPEG2VIDEO) { v_context->max_b_frames = 2; }
    if (v_context->codec_id == AV_CODEC_ID_MPEG1VIDEO) { v_context->mb_decision = 2; }
    if (oc->oformat->flags & AVFMT_GLOBALHEADER) { v_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; }

    a_context->sample_fmt = a_codec->sample_fmts ? a_codec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;

    const AVSampleFormat * p = a_codec->sample_fmts;
    while (p) {
        if (*p == -1) break;
        cout << "supported format " << *p << "\n";
        p++;
    }
    a_context->bit_rate = 64000;
    a_context->sample_rate = 48000;
    if (a_codec->supported_samplerates) {
        a_context->sample_rate = a_codec->supported_samplerates[0];
        for (int i = 0; a_codec->supported_samplerates[i]; i++) {
            if (a_codec->supported_samplerates[i] == 48000) {
                a_context->sample_rate = 48000;
                cout << "codec supports 48khz\n";
            }
        }
    }
    av_channel_layout_copy(&a_context->ch_layout, &channel_layout);
    a_stream->time_base = (AVRational){ 1, a_context->sample_rate };

    if (oc->oformat->flags & AVFMT_GLOBALHEADER) {
        v_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        a_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    ret = avcodec_open2(v_context, v_codec, NULL);
    v_frame = av_frame_alloc();
    v_frame->format = v_context->pix_fmt;
    v_frame->width  = v_context->width;
    v_frame->height = v_context->height;
    ret = av_frame_get_buffer(v_frame, 0);
    ret = avcodec_parameters_from_context(v_stream->codecpar, v_context);

    ret = avcodec_open2(a_context, a_codec, NULL);
    // int nb_samples = a_context->frame_size;
    // if (a_context->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE) {
    //     cout << "audio codec supports variable frame size\n";
    //     nb_samples = 10000;
    // }
    
    a_frame = av_frame_alloc();
    a_frame->format = a_context->sample_fmt;
    cout << "frame format " << a_context->sample_fmt << "\n";
    av_channel_layout_copy(&(a_frame->ch_layout), &(a_context->ch_layout));
    a_frame->sample_rate = a_context->sample_rate;
    a_frame->nb_samples = a_context->frame_size;
    ret = av_frame_get_buffer(a_frame, 0);

    // a_t_frame = av_frame_alloc();
    // a_t_frame->format = AV_SAMPLE_FMT_S16;
    // av_channel_layout_copy(&(a_t_frame->ch_layout), &(a_context->ch_layout));
    // a_t_frame->sample_rate = a_context->sample_rate;
    // a_t_frame->nb_samples = nb_samples;
    // ret = av_frame_get_buffer(a_t_frame, 0);

    ret = avcodec_parameters_from_context(a_stream->codecpar, a_context);
    // swr_ctx = swr_alloc();
    // av_opt_set_chlayout(swr_ctx, "in_chlayout", &(a_context->ch_layout), 0);
    // av_opt_set_int(swr_ctx, "in_sample_rate", a_context->sample_rate, 0);
    // av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    // av_opt_set_chlayout(swr_ctx, "out_chlayout", &(a_context->ch_layout), 0);
    // av_opt_set_int(swr_ctx, "out_sample_rate", a_context->sample_rate, 0);
    // av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", a_context->sample_fmt, 0);
    // ret = swr_init(swr_ctx);

    ret = avio_open(&(oc->pb), filename.c_str(), AVIO_FLAG_WRITE);
    ret = avformat_write_header(oc, &opt);

    started = true;
}

// void fill_mock_a_frame() {
//     int j, i, v;
//     int16_t *q = (int16_t*)a_t_frame->data[0];
//     for (j = 0; j < a_t_frame->nb_samples; j++) {
//         v = (int)(sin(t) * 10000);
//         for (i = 0; i < a_context->ch_layout.nb_channels; i++)
//             *q++ = v;
//         t += tincr;
//         tincr += tincr2;
//     }
//     a_t_frame->pts = a_pts;
//     a_pts += a_t_frame->nb_samples;
//     int dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, a_context->sample_rate) + a_t_frame->nb_samples,
//                                     a_context->sample_rate, a_context->sample_rate, AV_ROUND_UP);
//     int ret = av_frame_make_writable(a_frame);
//     ret = swr_convert(swr_ctx, a_frame->data, dst_nb_samples, (const uint8_t **)a_t_frame->data, a_t_frame->nb_samples);
//     a_frame->pts = av_rescale_q(samples_count, (AVRational){1, a_context->sample_rate}, a_context->time_base);
//     samples_count += dst_nb_samples;
// }

void VideoRecorder::recordFrame(const webrtc::VideoFrame & frame) {
    std::unique_lock lock(myMtx);
    if (!started) return;
    // rtc::scoped_refptr<webrtc::I420BufferInterface> buffer(frame.video_frame_buffer()->ToI420());
    auto buffer = frame.video_frame_buffer()->Scale(width, height)->ToI420();

    // need to rescale frame only if necessary  before writing

    if (av_frame_make_writable(v_frame) < 0) { cout << "coudn't make writable\n"; }
    
    libyuv::I420Copy(
        buffer->DataY(), buffer->StrideY(),
        buffer->DataU(), buffer->StrideU(),
        buffer->DataV(), buffer->StrideV(),
        v_frame->data[0], v_frame->linesize[0],
        v_frame->data[1], v_frame->linesize[1],
        v_frame->data[2], v_frame->linesize[2],
        width, height
    );

    auto current_time = std::chrono::system_clock::now();
    std::chrono::duration<double> frame_time = current_time - start_time;
    double time = frame_time.count();

    v_frame->pts = (int64_t)(time * 30.0);

    int ret = 0;
    if (v_frame->pts != last_v_pts) {
        avcodec_send_frame(v_context, v_frame);
        while (ret >= 0) {
            ret = avcodec_receive_packet(v_context, v_packet);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) { break; }
            av_packet_rescale_ts(v_packet, v_context->time_base, v_stream->time_base);
            v_packet->stream_index = v_stream->index;
            av_interleaved_write_frame(oc, v_packet);
        }
        last_v_pts = v_frame->pts;
    }
}

void VideoRecorder::writeASample(float value) {
    float * q = (float*)a_frame->data[0];
    int samples_written_in_frame = total_written_a_samples % a_frame->nb_samples;
    q[samples_written_in_frame] = value;
    total_written_a_samples++;
    samples_written_in_frame++;

    if (samples_written_in_frame == a_frame->nb_samples) {
        a_frame->pts = total_written_a_samples - a_frame->nb_samples;
        avcodec_send_frame(a_context, a_frame);
        int ret = 0;
        while (ret >= 0) {
            ret = avcodec_receive_packet(a_context, a_packet);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) { break; }
            av_packet_rescale_ts(a_packet, a_context->time_base, a_stream->time_base);
            a_packet->stream_index = a_stream->index;
            av_interleaved_write_frame(oc, a_packet);
        }
    }
}

void VideoRecorder::recordAFrame(const void* audio_data, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_of_frames) {
    std::unique_lock lock(myMtx);
    if (!started) return;
    if (bits_per_sample != 16) {
        cout << "unexpected audio sample size\n";
        return;
    }
    if (sample_rate != 48000) {
        cout << "unexpected audio sample rate\n";
        return;
    }
    if (number_of_channels != 1) {
        cout << "unexpected number of audio channels\n";
        return;
    }

    for (int i = 0; i < number_of_frames; i++) {
        int16_t * data = (int16_t *)audio_data;
        int16_t sample = data[i];
        float fl_sample = (float)sample / 32767.0F;
        writeASample(fl_sample);
    }
}

void VideoRecorder::finish() {
    std::unique_lock lock(myMtx);
    if (!started) return;
    av_write_trailer(oc);

    avcodec_free_context(&v_context);
    av_frame_free(&v_frame);
    av_packet_free(&v_packet);

    avcodec_free_context(&a_context);
    av_frame_free(&a_frame);
    // av_frame_free(&a_t_frame);
    av_packet_free(&a_packet);
    // swr_free(&swr_ctx);

    avio_closep(&oc->pb);
    avformat_free_context(oc);
    started = false;
}
