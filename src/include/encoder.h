#ifndef ENCODER_H
#define ENCODER_H

#include <stdexcept>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavutil/opt.h>
    #include <libavformat/avformat.h>
}

class Encoder {
public:
    // Video Constructor
    Encoder(int width, int heigth, AVPixelFormat pix_fmt, int bitrate, AVRational time_base, AVRational framerate);

    // Audio Constructor
    Encoder(const std::string& codec_name, int sample_rate, const AVChannelLayout& ch_layout, 
            AVSampleFormat sample_fmt, int bitrate, AVRational time_base);
    ~Encoder();

    bool SendFrame(AVFrame* frame);
    bool ReceivePacket(AVPacket* pkt);

    AVCodecContext* GetContext() const { return ctx_; }

private:
    AVCodecContext* ctx_ = nullptr;
};

#endif // ENCODER_H
