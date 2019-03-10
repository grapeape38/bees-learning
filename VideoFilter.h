#ifndef VIDEO_FILTER_H
#define VIDEO_FILTER_H

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/pixfmt.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/opt.h>
}

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "FrameFilter.h"

class VideoFilter {
    private:
        AVFormatContext *pFormatContext;
        AVCodecContext *dec_ctx; 
        AVCodecContext **tiles_ctx;
        AVFrame *inFrame, *wholeFrame;

        int frame_width, frame_height, tile_width, tile_height, video_stream_index, num_tiles;

        const char* out_dir;

        FILE **out_file;

        struct SwsContext *rgb_ctx, *yuv_ctx;

        FrameFilter *ff;

        int encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt, FILE *f);
        int apply_filter(uint8_t **buf, Halide::Runtime::Buffer<uint8_t> &out_buf);
        int decode_packet(AVPacket *pPacket, AVFrame *outFrame, uint8_t **rgb, Halide::Runtime::Buffer<uint8_t> &out_buf);
        int rgb_to_yuv(uint8_t **rgb, uint8_t **yuv);
        int yuv_to_rgb(uint8_t **yuv, uint8_t **rgb);
    public:
        VideoFilter(const char *fname, const char *out_dir_, FrameFilter *ff_, int n_tiles=1);
        ~VideoFilter();
        int filter(int n_frames);
};


#endif
