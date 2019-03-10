#ifndef FEATURE_EXTRACTOR_H
#define FEATURE_EXTRACTOR_H

#include <fstream>

#include "HalideBuffer.h"
#include "halide_image_io.h"
#include "FrameFilter.h"
#include "Draw.h"

using namespace Halide;
using Feature = std::vector<float>;

class FeatureExtractor : public FrameFilter {
    public:
        void log(const char *fmt, ...) {
            if (verbose) {
                va_list args;
                fprintf(stderr, "LOG: ");
                va_start(args, fmt);
                vfprintf(stderr, fmt, args);
                va_end(args);
                fprintf(stderr, "\n");
            }
        }
        int filter(Runtime::Buffer<uint8_t,3> input, Runtime::Buffer<uint8_t> &output) {
            computeFeatures(input);
            plotFeatures(input, output);
            return error;
        }
        bool isError() { return error; }
        void writeFeatures(const char *fname) {
            if (fname != NULL) {
                std::fstream f(fname, std::fstream::out);
                if (!f.is_open()) {
                    std::cout << "Unable to open file for writing.\n";
                }
                for (const auto &v : features) {
                    for (int i = 0; i < f_size; i++) {
                        f << v[i] << " ";
                    }
                    f << '\n';
                }
                f.close();
                log("%d features written to file.", features.size());
            }
        }
    protected:
        virtual void computeFeatures(Runtime::Buffer<uint8_t,3> input) = 0;
        int plotFeatures(const Runtime::Buffer<uint8_t> &input, Runtime::Buffer<uint8_t> &output) {
            output = std::move(gray(input));
            for (const auto &v : keypoints) {
                int x = v[1], y = v[2], scale = v[3];
                const Feature &ft = features[ft_index[x][y]];
                Color col = vec2color != NULL ? vec2color->getColor(ft) : Color {0xFF00FF};
                draw(output, v[1]*scale, v[2]*scale, col);
            }
        }
        int f_size;
        bool verbose, error;
        VectorToColor *vec2color;
        std::vector<Feature> features;
        std::vector<std::vector<int>> keypoints, ft_index;
};

#endif

