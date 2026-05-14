#include "include/muxer.h"

Muxer::Muxer(const std::string& filename) {
    avformat_alloc_output_context2(&fmt_ctx_, nullptr, nullptr, filename.c_str());
    if (!fmt_ctx_) throw std::runtime_error("Failed create AVFormatContext");
}

Muxer::~Muxer() {
    if (fmt_ctx_) {
        if(!(fmt_ctx_->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&fmt_ctx_->pb);
        }

        avformat_free_context(fmt_ctx_);
    }
}

int Muxer::AddStream(AVCodecContext* enc_ctx) {
    AVStream* stream = avformat_new_stream(fmt_ctx_, nullptr);
    if (!stream) throw std::runtime_error("Failed create enconder stream");

    stream->id = fmt_ctx_->nb_streams - 1;
    if (avcodec_parameters_from_context(stream->codecpar, enc_ctx) < 0) {
        throw std::runtime_error("Error copying encoder parameters to the stream");
    }
    // get time base original
    stream->time_base = enc_ctx->time_base;
    return stream->index;
}

void Muxer::WriteHeader() {
    if (!(fmt_ctx_->oformat->flags & AVFMT_NOFILE)) {
        if(avio_open(&fmt_ctx_->pb, fmt_ctx_->url, AVIO_FLAG_WRITE) < 0) {
            throw std::runtime_error("Error opening output file");
        }
    }
    
    if (avformat_write_header(fmt_ctx_, nullptr) < 0) {
        throw std::runtime_error("Error writing MP4 header");
    }
}

void Muxer::WritePacket(AVPacket* pkt, AVRational encoder_time_base, int stream_index) {
    std::lock_guard<std::mutex> lock(write_mutex_);
    
    AVStream* stream = fmt_ctx_->streams[stream_index];
    av_packet_rescale_ts(pkt, encoder_time_base, stream->time_base);
    pkt->stream_index = stream_index;

    if(av_interleaved_write_frame(fmt_ctx_, pkt) < 0) {
        throw std::runtime_error("Write error frame \n");
    }
}

void Muxer::WriteTrailer() {
    av_write_trailer(fmt_ctx_);
}

