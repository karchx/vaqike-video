#include "include/decoder.h"

Decoder::Decoder(AVCodecParameters* codecpar) {
    if (!codecpar) {
        throw std::invalid_argument("AVCodecParameters is null");
    }

    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
       throw std::runtime_error("Codec is not support"); 
    }

    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_) {
        throw std::runtime_error("Failed codec_ctx_ assign memory"); 
    }

    if (avcodec_parameters_to_context(codec_ctx_, codecpar) < 0) {
        avcodec_free_context(&codec_ctx_);
        throw std::runtime_error("Failed codec_ctx_ copy parameters"); 
    }

    if (avcodec_open2(codec_ctx_, codec, nullptr) < 0) {
        avcodec_free_context(&codec_ctx_);
        throw std::runtime_error("Failed init decoder"); 
    }
}

Decoder::~Decoder() {
    if (codec_ctx_) {
        avcodec_free_context(&codec_ctx_);
    }
}

bool Decoder::SendPacket(const AVPacket* packet) {
    int ret = avcodec_send_packet(codec_ctx_, packet);

    return ret >= 0;
}

bool Decoder::ReceiveFrame(AVFrame* frame) {
    int ret = avcodec_receive_frame(codec_ctx_, frame);

    return ret >= 0;
}

