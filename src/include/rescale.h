#ifndef RESCALE_H
#define RESCALE_H

#include <stdexcept>
extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavutil/frame.h>
    #include <libswresample/swresample.h>
    #include <libavutil/audio_fifo.h>
    #include <libavutil/opt.h>
}

class ReScale {
public:
    explicit ReScale(AVCodecContext* encoder_ctx, AVCodecContext* decoder_ctx);
    ~ReScale();

    ReScale(const ReScale&) = delete;
    ReScale& operator=(const ReScale&) = delete;

    void Push(const AVFrame* input_frame);
    AVFrame* Pop(int frame_size);
    int GetFifoSize() const;

private:
    SwrContext* swr_ctx_ = nullptr;
    AVAudioFifo* fifo_ = nullptr;
    AVCodecContext* encoder_ctx_;
    AVCodecContext* decoder_ctx_;
};

#endif // RESCALE_H
