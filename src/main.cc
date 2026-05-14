#include <iostream>
#include <thread>

#include "include/demuxer.h"
#include "include/decoder.h"
#include "include/queue.h"
#include "include/scaler.h"
#include "include/encoder.h"
#include "include/muxer.h"

extern "C" {
    #include <libswresample/swresample.h>
    #include <libavutil/audio_fifo.h>
}

void AudioWorker(Queue& input_queue, Encoder* encoder, Muxer* muxer, int stream_index, AVCodecContext* dec_ctx);

void VideoWorker(Queue& input_queue, Encoder* encoder, Muxer* muxer, int stream_index, int in_width,
                int in_height, AVPixelFormat in_fmt);


int main() {
    Demuxer demuxer("1.mp4");
    Decoder video_decoder(demuxer.GetVideoCodecParameters());

    AVRational src_time_base = demuxer.GetTimeBase();
    AVRational src_framerate = demuxer.GetFrameRate();

    Encoder video_encoder(640, 360, AV_PIX_FMT_YUV420P, 1000000, src_time_base, src_framerate);

    std::unique_ptr<Decoder> audio_decoder;
    std::unique_ptr<Encoder> audio_encoder;
    AVCodecContext* dec_ctx = nullptr;

    if (demuxer.GetAudioStreamIndex() >= 0) {
        audio_decoder = std::make_unique<Decoder>(demuxer.GetAudioCodecParameters());
        dec_ctx = audio_decoder->GetContext();
        
        AVRational audio_time_base = {1, dec_ctx->sample_rate};
        
        audio_encoder = std::make_unique<Encoder>(
            "aac",
            dec_ctx->sample_rate,
            dec_ctx->ch_layout,
            AV_SAMPLE_FMT_FLTP,
            128000,
            audio_time_base
        );
    }

    Muxer muxer("output_360p.mp4");
    int video_stream_idx = muxer.AddStream(video_encoder.GetContext());
    int audio_stream_idx = -1;

    if (audio_encoder) {
        audio_stream_idx = muxer.AddStream(audio_encoder->GetContext());
    }

    muxer.WriteHeader();

    Queue q_360p_video;
    Queue q_360p_audio;

    AVCodecContext* video_decoder_ctx = video_decoder.GetContext();

    std::thread t_360p(VideoWorker, std::ref(q_360p_video), &video_encoder, &muxer, video_stream_idx, 
                       video_decoder_ctx->width, video_decoder_ctx->height, video_decoder_ctx->pix_fmt);

    std::unique_ptr<std::thread> t_audio;
    if (audio_encoder) {
        t_audio = std::make_unique<std::thread>(AudioWorker, std::ref(q_360p_audio), 
                                                audio_encoder.get(), &muxer, audio_stream_idx, dec_ctx);
    }

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame =  av_frame_alloc();

    while (demuxer.ReadPacket(packet)) {
        if (packet->stream_index == demuxer.GetVideoStreamIndex()) {
            if (video_decoder.SendPacket(packet)) {
                while (video_decoder.ReceiveFrame(frame)) {
                    // av_frame_clone thread-safe and O(1) in memory
                    q_360p_video.Push(av_frame_clone(frame));

                    av_frame_unref(frame);
                }
            }
        } else if (audio_decoder && packet->stream_index == demuxer.GetAudioStreamIndex()) {
            if (audio_decoder->SendPacket(packet)) {
                while (audio_decoder->ReceiveFrame(frame)) {
                    q_360p_audio.Push(av_frame_clone(frame));
                    av_frame_unref(frame);
                }
            }
        }

        av_packet_unref(packet); // clean next iter
    }

    // flush video
    video_decoder.SendPacket(nullptr);
    while (video_decoder.ReceiveFrame(frame)) {
        // q_360p_video.Push(av_frame_clone(frame));
        av_frame_unref(frame);
    }

    // flush audio
    if (audio_decoder) {
        audio_decoder->SendPacket(nullptr);
        while (audio_decoder->ReceiveFrame(frame)) {
            // q_360p_audio.Push(av_frame_clone(frame));
            av_frame_unref(frame);
        }
    }

    q_360p_video.SignalEOF();
    q_360p_audio.SignalEOF();
    t_360p.join();
    if (t_audio) t_audio->join();

    muxer.WriteTrailer();

    av_frame_free(&frame);
    av_packet_free(&packet);
    return 0;
}

void VideoWorker(Queue& input_queue, Encoder* encoder, Muxer* muxer, int stream_index, 
                int in_width, int in_height, AVPixelFormat in_fmt) {

    AVCodecContext* encoder_ctx = encoder->GetContext();

    Scaler scaler(in_width, in_height, in_fmt,
                  encoder_ctx->width, encoder_ctx->height, encoder_ctx->pix_fmt);

    AVPacket* pkt = av_packet_alloc();

    while(true) {
        AVFrame *frame = input_queue.Pop();
        if (!frame) break; // EOF signal
        
        AVFrame* scaled_frame = scaler.Scale(frame);
        if (scaled_frame) {
            scaled_frame->pts = frame->pts;
            
            if (encoder->SendFrame(scaled_frame)) {
                while (encoder->ReceivePacket(pkt)) {
                    encoder_ctx = encoder->GetContext();
                    muxer->WritePacket(pkt, encoder_ctx->time_base, stream_index);
                    av_packet_unref(pkt);
                }
            }

            av_frame_free(&scaled_frame);
        }

        av_frame_free(&frame);
    }

    // Flush encoder
    encoder->SendFrame(nullptr);
    while (encoder->ReceivePacket(pkt)) {
        muxer->WritePacket(pkt, encoder_ctx->time_base, stream_index);
        av_packet_unref(pkt);
    }

    av_packet_free(&pkt);
}

void AudioWorker(Queue& input_queue, Encoder* encoder, Muxer* muxer, int stream_index, AVCodecContext* dec_ctx) {
    AVCodecContext* enc_ctx = encoder->GetContext();

    SwrContext* swr_ctx = swr_alloc();
    av_opt_set_chlayout(swr_ctx, "in_chlayout",    &dec_ctx->ch_layout, 0);
    av_opt_set_int(swr_ctx,      "in_sample_rate", dec_ctx->sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt",  dec_ctx->sample_fmt, 0);

    av_opt_set_chlayout(swr_ctx, "out_chlayout",   &enc_ctx->ch_layout, 0);
    av_opt_set_int(swr_ctx,      "out_sample_rate", enc_ctx->sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", enc_ctx->sample_fmt, 0);
    swr_init(swr_ctx);

    AVAudioFifo* fifo = av_audio_fifo_alloc(enc_ctx->sample_fmt, enc_ctx->ch_layout.nb_channels, 1);
    int64_t current_pts = 0;
    AVPacket* pkt = av_packet_alloc();

    while (true) {
        AVFrame* frame = input_queue.Pop();
        if (!frame) break;
        // Rescale input frame
        AVFrame* resampled_frame = av_frame_alloc();
        resampled_frame->sample_rate = enc_ctx->sample_rate;
        av_channel_layout_copy(&resampled_frame->ch_layout, &enc_ctx->ch_layout);
        resampled_frame->format = enc_ctx->sample_fmt;

        int out_samples = av_rescale_rnd(swr_get_delay(swr_ctx, dec_ctx->sample_rate) + frame->nb_samples,
                                         enc_ctx->sample_rate, dec_ctx->sample_rate, AV_ROUND_UP);
        resampled_frame->nb_samples = out_samples;
        av_frame_get_buffer(resampled_frame, 0);

        out_samples = swr_convert(swr_ctx, resampled_frame->data, out_samples,
                                  (const uint8_t**)frame->data, frame->nb_samples);

        av_audio_fifo_write(fifo, (void**)resampled_frame->data, out_samples);
        av_frame_free(&resampled_frame);
        av_frame_free(&frame);

        while (av_audio_fifo_size(fifo) >= enc_ctx->frame_size) {
            AVFrame* out_frame = av_frame_alloc();
            out_frame->nb_samples = enc_ctx->frame_size;
            out_frame->sample_rate = enc_ctx->sample_rate;
            av_channel_layout_copy(&out_frame->ch_layout, &enc_ctx->ch_layout);
            out_frame->format = enc_ctx->sample_fmt;
            av_frame_get_buffer(out_frame, 0);

            av_audio_fifo_read(fifo, (void**)out_frame->data, enc_ctx->frame_size);

            out_frame->pts = current_pts;
            current_pts += out_frame->nb_samples;

            if (encoder->SendFrame(out_frame)) {
                while (encoder->ReceivePacket(pkt)) {
                    muxer->WritePacket(pkt, enc_ctx->time_base, stream_index);
                    av_packet_unref(pkt);
                }
            }
            av_frame_free(&out_frame);
        }
    }

    if (av_audio_fifo_size(fifo) > 0) {
        AVFrame* out_frame = av_frame_alloc();
        out_frame->nb_samples = av_audio_fifo_size(fifo);
        out_frame->sample_rate = enc_ctx->sample_rate;
        av_channel_layout_copy(&out_frame->ch_layout, &enc_ctx->ch_layout);
        out_frame->format = enc_ctx->sample_fmt;
        av_frame_get_buffer(out_frame, 0);

        av_audio_fifo_read(fifo, (void**)out_frame->data, out_frame->nb_samples);
        out_frame->pts = current_pts;

        if (encoder->SendFrame(out_frame)) {
            while (encoder->ReceivePacket(pkt)) {
                muxer->WritePacket(pkt, enc_ctx->time_base, stream_index);
                av_packet_unref(pkt);
            }
        }
        av_frame_free(&out_frame);
    }

    encoder->SendFrame(nullptr);
    while (encoder->ReceivePacket(pkt)) {
        muxer->WritePacket(pkt, enc_ctx->time_base, stream_index);
        av_packet_unref(pkt);
    }

    av_packet_free(&pkt);
    av_audio_fifo_free(fifo);
    swr_free(&swr_ctx);
}

