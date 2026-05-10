#include "include/scaler.h"

Scaler::Scaler(int in_width, int in_height, AVPixelFormat in_pix_fmt, 
               int out_width, int out_height, AVPixelFormat out_pix_fmt) 
    : out_width_(out_width), out_height_(out_height), out_pix_fmt_(out_pix_fmt) {

        sws_ctx_ = sws_getContext(
                in_width, in_height, in_pix_fmt,
                out_width, out_height, out_pix_fmt,
                SWS_LANCZOS, nullptr, nullptr, nullptr
        );

        if (!sws_ctx_) {
            throw std::runtime_error("Failed to initialize SwsContext");
        }
}

Scaler::~Scaler() {
    if (sws_ctx_) {
        sws_freeContext(sws_ctx_);
    }
}


AVFrame* Scaler::Scale(const AVFrame* input_frame) {
    if (!input_frame) return nullptr;

    AVFrame* output_frame = av_frame_alloc();
    if (!output_frame) return nullptr;

    output_frame->format = out_pix_fmt_;
    output_frame->width = out_width_;
    output_frame->height = out_height_;

    // color metadata
    output_frame->colorspace = input_frame->colorspace;
    output_frame->color_range = input_frame->color_range;
    output_frame->color_primaries = input_frame->color_primaries;
    output_frame->color_trc = input_frame->color_trc;
    output_frame->pts = input_frame->pts;

    if (av_frame_get_buffer(output_frame, 32) < 0) {
        av_frame_free(&output_frame);
        return nullptr;
    }

    int ret = sws_scale(sws_ctx_, input_frame->data, input_frame->linesize, 0, input_frame->height, output_frame->data, output_frame->linesize);

    if (ret <= 0) {
        av_frame_free(&output_frame);
        return nullptr;
    }
    
    return output_frame;
}

