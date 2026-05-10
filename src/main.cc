#include <iostream>
#include <thread>

#include "include/demuxer.h"
#include "include/decoder.h"
#include "include/queue.h"
#include "include/scaler.h"
#include "include/encoder.h"
#include "include/muxer.h"

void ScalerWorker(Queue& input_queue, int out_width, int out_height, AVPixelFormat out_fmt,
                  AVRational src_time_base, AVRational src_framerate);

int main() {
    Demuxer demuxer("1.mp4");
    Decoder decoder(demuxer.GetVideoCodecParameters());

    Queue q_360p;

    AVRational src_time_base = demuxer.GetTimeBase();
    AVRational src_framerate = demuxer.GetFrameRate();

    std::thread t_360p(ScalerWorker, std::ref(q_360p), 640, 360, AV_PIX_FMT_YUV420P, src_time_base, src_framerate);

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame =  av_frame_alloc();

    while (demuxer.ReadVideoPacket(packet)) {
        if (decoder.SendPacket(packet)) {
            while (decoder.ReceiveFrame(frame)) {
                // av_frame_clone thread-safe and O(1) in memory
                q_360p.Push(av_frame_clone(frame));

                av_frame_unref(frame);
            }
        }
        av_packet_unref(packet); // clean next iter
    }
    decoder.SendPacket(nullptr);
    
    while (decoder.ReceiveFrame(frame)) {
        q_360p.Push(av_frame_clone(frame));
        av_frame_unref(frame);
    }

    q_360p.SignalEOF();
    
    t_360p.join();

    av_frame_free(&frame);
    av_packet_free(&packet);
    return 0;
}

// TODO: add pipeline manager class
void ScalerWorker(Queue& input_queue, int out_width, int out_height, AVPixelFormat out_fmt,
                  AVRational src_time_base, AVRational src_framerate) {
    Scaler* scaler = nullptr;
    Encoder* encoder = nullptr;
    Muxer* muxer = nullptr;
    AVPacket* pkt = av_packet_alloc();

    while(true) {
        AVFrame *frame = input_queue.Pop();
        if (!frame) break; // EOF signal

        if (!scaler) {
            scaler = new Scaler(frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
                                out_width, out_height, out_fmt);

            encoder = new Encoder(out_width, out_height, out_fmt, 1000000, src_time_base, src_framerate);

            muxer = new Muxer("output_360p.mp4");
            muxer->AddStream(encoder->GetContext());
            muxer->WriteHeader();
        }

        AVFrame* scaled_frame = scaler->Scale(frame);
        if (scaled_frame) {
            scaled_frame->pts = frame->pts;
            
            if (encoder->SendFrame(scaled_frame)) {
                while (encoder->ReceivePacket(pkt)) {
                    muxer->WritePacket(pkt, encoder->GetContext()->time_base);
                    av_packet_unref(pkt);
                }
            }

            av_frame_free(&scaled_frame);
        }

        av_frame_free(&frame);
    }

    if (encoder && muxer) {
        encoder->SendFrame(nullptr);
        while (encoder->ReceivePacket(pkt)) {
            muxer->WritePacket(pkt, encoder->GetContext()->time_base);
            av_packet_unref(pkt);
        }
        muxer->WriteTrailer();
    }

    delete scaler;
    delete encoder;
    delete muxer;
}
