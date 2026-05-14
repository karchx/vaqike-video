#ifndef MUXER_H
#define MUXER_H

#include <string>
#include <mutex>
#include <vector>
#include <stdexcept>

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
}

class Muxer {
public:
    Muxer(const std::string& filename);
    ~Muxer();

    int AddStream(AVCodecContext* enc_ctx);
    void WriteHeader();

    void WritePacket(AVPacket* pkt, AVRational encoder_time_base, int stream_index);
    void WriteTrailer();

private:
    AVFormatContext* fmt_ctx_ = nullptr;
    std::mutex write_mutex_;
};

#endif // MUXER_H
