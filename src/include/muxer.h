#ifndef MUXER_H
#define MUXER_H

#include <string>
#include <stdexcept>

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
}

class Muxer {
public:
    Muxer(const std::string& filename);
    ~Muxer();

    void AddStream(AVCodecContext* enc_ctx);
    void WriteHeader();

    void WritePacket(AVPacket* pkt, AVRational encoder_time_base);
    void WriteTrailer();

private:
    AVFormatContext* fmt_ctx_ = nullptr;
    AVStream* stream_ = nullptr;
};

#endif // MUXER_H
