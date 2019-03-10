#ifndef FRAME_FILTER_H
#define FRAME_FILTER_H
#include "HalideBuffer.h"
#include "halide_image_io.h"

using namespace Halide;

class FrameFilter {
    public:
        virtual int filter(Runtime::Buffer<uint8_t,3> input, Runtime::Buffer<uint8_t> &output) = 0; 
        int filterImage(const char* image_path, Runtime::Buffer<uint8_t> &output) {
            Runtime::Buffer<uint8_t,3> input = Tools::load_image(image_path);
            return filter(input, output);
        }
    protected:
        bool interleaved;
        inline Runtime::Buffer<uint8_t> gray(const Runtime::Buffer<uint8_t> &buf) {
            Runtime::Buffer<uint8_t> new_buf = interleaved ? 
                Runtime::Buffer<uint8_t>::make_interleaved(buf.width(), buf.height(), 3) :
                Runtime::Buffer<uint8_t>(buf.width(), buf.height(), 3);
            for (int x = 0; x < buf.width(); x++) {
                for (int y = 0; y < buf.height(); y++) {
                    for (int c = 0; c < 3; c++) {
                        new_buf(x,y,c) = 0.299f * buf(x, y, 0) + 0.587f * buf(x, y, 1) + 0.114f * buf(x, y, 2);
                    }
                }
            }
            return new_buf;
        }
};

#endif

