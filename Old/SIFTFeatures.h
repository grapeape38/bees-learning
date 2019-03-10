#ifndef SIFT_FEAT_H
#define SIFT_FEAT_H

#include "FrameFilter.h"
#include "HalideBuffer.h"
#include "halide_image_io.h"
#include "stdio.h"
#include "find_maxima.h"
#include "find_maxima_inter.h"
#include "get_derivatives.h"
#include "Draw.h"

using namespace Halide;

class SIFTFeatures : public FrameFilter {
    public:
        SIFTFeatures(VectorToColor *v2c=NULL, bool inter=true, bool verb=false);
        int filter(Runtime::Buffer<uint8_t> &input, Runtime::Buffer<uint8_t> &output, const char* out_file);
        void getKeyPoints(Runtime::Buffer<uint8_t,3> input);   
        void plotKeyPoints(Runtime::Buffer<uint8_t> output);
        void writeDescriptorsToFile(const char *fname);
        bool isError() { return error != 0; }
    private:
        float scales[5];
        VectorToColor *vec2color;
        bool interleaved, verbose;
        int error;
        void log(const char *fmt, ...);
        std::vector<std::vector<int>> keypoints, kp_index; 
        std::vector<std::vector<float>> descriptors;
        void getDescriptors(Runtime::Buffer<float,3> mag, Runtime::Buffer<int16_t,3> theta);
};


#endif