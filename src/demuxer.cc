#include "include/demuxer.h"
#include <libavcodec/codec_par.h>
#include <libavformat/avformat.h>

Demuxer::Demuxer(const std::string& filepath) {
    format_ctx_ = avformat_alloc_context();
    if (!format_ctx_) {
        throw std::runtime_error("Failed assign memory AVFormatContext");
    }

    if (avformat_open_input(&format_ctx_, filepath.c_str(), nullptr, nullptr) < 0) {
        avformat_free_context(format_ctx_);
        throw std::runtime_error("Failed open file: " + filepath);
    }

    if (avformat_find_stream_info(format_ctx_, nullptr) < 0) {
        avformat_close_input(&format_ctx_);
        throw std::runtime_error("Failed get data stream");
    }

    video_stream_index_ = av_find_best_stream(format_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);

    if (video_stream_index_ < 0) {
        avformat_close_input(&format_ctx_);
        throw std::runtime_error("Not found video in file");
    }
};

Demuxer::~Demuxer() {
    if (format_ctx_) {
        avformat_close_input(&format_ctx_);
    }
}

bool Demuxer::ReadVideoPacket(AVPacket* packet) {
    if (!packet) return false;

    while (av_read_frame(format_ctx_, packet) >= 0) {
        if (packet->stream_index == video_stream_index_) {
            return true;
        }

        // TODO: validate this packet is audio
        av_packet_unref(packet);
    }

    return false;
}

int Demuxer::GetVideoStreamIndex() const {
    return video_stream_index_;
}

AVCodecParameters* Demuxer::GetVideoCodecParameters() const {
    if (video_stream_index_ >= 0 && format_ctx_) {
        return format_ctx_->streams[video_stream_index_]->codecpar;
    }

    return nullptr;
}
