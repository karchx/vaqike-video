#include <iostream>
#include "include/demuxer.h"


int main() {
    Demuxer demuxer("1.mp4");
    AVPacket* packet = av_packet_alloc();

    while (demuxer.ReadVideoPacket(packet)) {
        std::cout << "Size: " << packet->size << std::endl;
        av_packet_unref(packet); // clean next iter
    }

    av_packet_free(&packet);
    return 0;
}
