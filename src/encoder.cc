#include "include/encoder.h"

Encoder::Encoder(int width, int heigth, AVPixelFormat pix_fmt, int bitrate, AVRational time_base, AVRational framerate) {
    const AVCodec* codec = avcodec_find_encoder_by_name("libx264");
    if (!codec) throw std::runtime_error("Encoder libx264 not found");

    ctx_ = avcodec_alloc_context3(codec);
    if (!ctx_) throw std::runtime_error("Error creating an AVCodecContext");
    ctx_->thread_count = 2;
    av_opt_set(ctx_->priv_data, "preset", "medium", 0);
    av_opt_set(ctx_->priv_data, "crf", "23", 0);
    // av_opt_set(ctx_->priv_data, "preset", "ultrafast", 0);

    ctx_->width = width;
    ctx_->height = heigth;
    ctx_->time_base = time_base;
    ctx_->framerate = framerate;
    ctx_->pix_fmt = pix_fmt;
    ctx_->bit_rate = bitrate;

    if (ctx_->flags & AVFMT_GLOBALHEADER) {
        ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (avcodec_open2(ctx_, codec, nullptr) < 0) {
        avcodec_free_context(&ctx_);
        throw std::runtime_error("Error creating encoder");
    }
}

Encoder::Encoder(const std::string& codec_name, int sample_rate, const AVChannelLayout& ch_layout, AVSampleFormat sample_fmt, int bitrate, AVRational time_base) {
    const AVCodec* codec = avcodec_find_encoder_by_name(codec_name.c_str());
    if (!codec) throw std::runtime_error("Encoder " + codec_name + " not found");

    ctx_ = avcodec_alloc_context3(codec);
    if (!ctx_) throw std::runtime_error("Error creating audio AVCodecContext");

    ctx_->sample_rate = sample_rate;
    ctx_->sample_fmt = sample_fmt;
    ctx_->bit_rate = bitrate;
    ctx_->time_base = time_base;

    av_channel_layout_copy(&ctx_->ch_layout, &ch_layout);

    if (ctx_->flags & AVFMT_GLOBALHEADER) {
        ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (avcodec_open2(ctx_, codec, nullptr) < 0) {
        avcodec_free_context(&ctx_);
        throw std::runtime_error("Error opening auido encoder");
    }
}

Encoder::~Encoder() {
    if (ctx_) avcodec_free_context(&ctx_);
}

bool Encoder::SendFrame(AVFrame* frame) {
    int ret = avcodec_send_frame(ctx_, frame);
    return ret >= 0;
}

bool Encoder::ReceivePacket(AVPacket* pkt) {
    int ret = avcodec_receive_packet(ctx_, pkt);
    return ret == 0;
}
