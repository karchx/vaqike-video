#include <iostream>
#include "include/demuxer.h"
#include "include/decoder.h"


int main() {
    Demuxer demuxer("1.mp4");
    Decoder decoder(demuxer.GetVideoCodecParameters());

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame =  av_frame_alloc();

    while (demuxer.ReadVideoPacket(packet)) {
        if (decoder.SendPacket(packet)) {
            while (decoder.ReceiveFrame(frame)) {
                av_frame_unref(frame);
            }
        }
        av_packet_unref(packet); // clean next iter
    }
    decoder.SendPacket(nullptr);
    
    while (decoder.ReceiveFrame(frame)) {
        av_frame_unref(frame);
    }
    av_frame_free(&frame);
    av_packet_free(&packet);
    return 0;
}
