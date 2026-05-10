#ifndef DECODER_H
#define DECODER_H

#include <stdexcept>

extern "C" {
#include <libavcodec/avcodec.h>
}

class Decoder {
public:
    explicit Decoder(AVCodecParameters* codecpar);
    ~Decoder();

    Decoder(const Decoder&) = delete;
    Decoder& operator=(const Decoder&) = delete;

    bool SendPacket(const AVPacket* packet);
    bool ReceiveFrame(AVFrame* frame);

private:
    AVCodecContext* codec_ctx_ = nullptr;
};

#endif // DECODER_H
