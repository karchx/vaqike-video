#include "include/encoder.h"

Encoder::Encoder(int width, int heigth, AVPixelFormat pix_fmt, int bitrate, AVRational time_base, AVRational framerate) {
    const AVCodec* codec = avcodec_find_encoder_by_name("libx264");
    if (!codec) throw std::runtime_error("Encoder libx264 not found");

    ctx_ = avcodec_alloc_context3(codec);
    if (!ctx_) throw std::runtime_error("Error creating an AVCodecContext");

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
