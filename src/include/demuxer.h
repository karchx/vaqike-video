#ifndef DEMUXER_H
#define DEMUXER_H

#include <string>
#include <stdexcept>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
    #include <libavutil/opt.h>
}

class Demuxer {
public:
    explicit Demuxer(const std::string& filepath);
    ~Demuxer();

    // Delete constructors copy and assign
    Demuxer(const Demuxer&) = delete;
    Demuxer& operator=(const Demuxer&) = delete;

    bool ReadVideoPacket(AVPacket* packet);

    int GetVideoStreamIndex() const;
    AVCodecParameters* GetVideoCodecParameters() const;

private:
    AVFormatContext* format_ctx_ = nullptr;
    int video_stream_index_ = -1;
};

#endif

