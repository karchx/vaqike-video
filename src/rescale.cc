#include "include/rescale.h"

ReScale::ReScale(AVCodecContext* encoder_ctx, AVCodecContext* decoder_ctx) : encoder_ctx_(encoder_ctx), decoder_ctx_(decoder_ctx) {
    swr_ctx_ = swr_alloc();
    av_opt_set_chlayout(swr_ctx_, "in_chlayout",    &decoder_ctx->ch_layout, 0);
    av_opt_set_int(swr_ctx_,      "in_sample_rate", decoder_ctx->sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx_, "in_sample_fmt",  decoder_ctx->sample_fmt, 0);

    av_opt_set_chlayout(swr_ctx_, "out_chlayout",   &encoder_ctx->ch_layout, 0);
    av_opt_set_int(swr_ctx_,      "out_sample_rate", encoder_ctx->sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx_, "out_sample_fmt", encoder_ctx->sample_fmt, 0);
    swr_init(swr_ctx_);

    fifo_ = av_audio_fifo_alloc(
        encoder_ctx->sample_fmt, 
        encoder_ctx->ch_layout.nb_channels, 
        1
    );
}

void ReScale::Push(const AVFrame* input_frame) {
    if (!input_frame) return;

    AVFrame* resampled_frame = av_frame_alloc();
    resampled_frame->sample_rate = encoder_ctx_->sample_rate;
    av_channel_layout_copy(&resampled_frame->ch_layout, &encoder_ctx_->ch_layout);
    resampled_frame->format = encoder_ctx_->sample_fmt;
    
    int out_samples = av_rescale_rnd(swr_get_delay(swr_ctx_, decoder_ctx_->sample_rate) + input_frame->nb_samples,
                                     encoder_ctx_->sample_rate, decoder_ctx_->sample_rate, AV_ROUND_UP);
    resampled_frame->nb_samples = out_samples;
    av_frame_get_buffer(resampled_frame, 0);
    
    out_samples = swr_convert(swr_ctx_, resampled_frame->data, out_samples,
                              (const uint8_t**)input_frame->data, input_frame->nb_samples);

    if (out_samples > 0) {
        av_audio_fifo_write(fifo_, (void**)resampled_frame->data, out_samples);
    }

    av_frame_free(&resampled_frame);
}

AVFrame* ReScale::Pop(int frame_size) {
    if (av_audio_fifo_size(fifo_) < frame_size) {
        return nullptr;
    }

    AVFrame* out_frame = av_frame_alloc();
    out_frame->nb_samples = encoder_ctx_->frame_size;
    out_frame->sample_rate = encoder_ctx_->sample_rate;
    av_channel_layout_copy(&out_frame->ch_layout, &encoder_ctx_->ch_layout);
    out_frame->format = encoder_ctx_->sample_fmt;
    av_frame_get_buffer(out_frame, 0);

    av_audio_fifo_read(fifo_, (void**)out_frame->data, encoder_ctx_->frame_size);
    return out_frame;
}

int ReScale::GetFifoSize() const {
    return av_audio_fifo_size(fifo_);
}

ReScale::~ReScale() {
    if (fifo_) {
        av_audio_fifo_free(fifo_);
    }

    if (swr_ctx_) {
        swr_free(&swr_ctx_);
    }
}
