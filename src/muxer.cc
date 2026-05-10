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

void Muxer::AddStream(AVCodecContext* enc_ctx) {
    stream_ = avformat_new_stream(fmt_ctx_, nullptr);
    if (!stream_) throw std::runtime_error("Failed create enconder stream");

    stream_->id = fmt_ctx_->nb_streams - 1;
    if (avcodec_parameters_from_context(stream_->codecpar, enc_ctx) < 0) {
        throw std::runtime_error("Error copying encoder parameters to the stream");
    }
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

void Muxer::WritePacket(AVPacket* pkt, AVRational encoder_time_base) {
    av_packet_rescale_ts(pkt, encoder_time_base, stream_->time_base);
    pkt->stream_index = stream_->index;

    av_interleaved_write_frame(fmt_ctx_, pkt);
}

void Muxer::WriteTrailer() {
    av_write_trailer(fmt_ctx_);
}

