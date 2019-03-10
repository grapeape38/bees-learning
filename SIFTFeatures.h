#ifndef SIFT_FEAT_H
#define SIFT_FEAT_H

#include "FeatureExtractor.h"
#include "HalideBuffer.h"
#include "halide_image_io.h"
#include "find_maxima.h"
#include "find_maxima_inter.h"
#include "get_derivatives.h"

using namespace Halide;

class SIFTFeatures : public FeatureExtractor {
    public:
        SIFTFeatures(VectorToColor *v2c=NULL, bool inter=true, bool verb=false);
        void computeFeatures(Runtime::Buffer<uint8_t,3> input);   
    private:
        void getDescriptors(Runtime::Buffer<float,3> mag, Runtime::Buffer<int16_t,3> theta);
        float scales[5];
};

#endif