#ifndef SCALER_H
#define SCALER_H

#include <stdexcept>
extern "C" {
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/frame.h>
}

class Scaler {
public:
    explicit Scaler(int in_width, int in_height, AVPixelFormat in_pix_fmt, int out_width, int out_height, AVPixelFormat out_pix_fmt);
    ~Scaler();

    Scaler(const Scaler&) = delete;
    Scaler& operator=(const Scaler&) = delete;

    AVFrame* Scale(const AVFrame* input_frame);

private:
    SwsContext* sws_ctx_ = nullptr;
    int out_width_;
    int out_height_;
    AVPixelFormat out_pix_fmt_;
};

#endif // SCALE_H
