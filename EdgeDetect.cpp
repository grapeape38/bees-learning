#include "Halide.h"
#include <stdio.h>

using namespace Halide;

class EdgeDetect : public Halide::Generator<EdgeDetect> {
    public:
        GeneratorParam<bool> interleaved{"interleaved", true};

        Input<Buffer<uint8_t>> input{"input",3};
        Output<Buffer<uint8_t>> output{"output", 3};

        Var x,y,c;

        void generate() {
            if (interleaved) {
                input.dim(0).set_stride(3);
                input.dim(2).set_stride(1);
            }

            Expr clamped_x = clamp(x, 0, input.width()-1);
            Expr clamped_y = clamp(y, 0, input.height()-1);

            Func gray;
            gray(x, y) = 0.299f * input(x, y, 0) + 0.587f * input(x, y, 1) + 0.114f * input(x, y, 2);

            Func clamped, gaussian_x, gaussian_y;
            clamped(x,y) = gray(clamped_x, clamped_y);

            gaussian_x(x,y) =  (-1 * clamped(x-2,y) +
                                -4 * clamped(x-1,y) +
                                 6 * clamped(x,y) + 
                                 4 * clamped(x+1,y) +
                                 1 * clamped(x+2,y))/16;

            gaussian_y(x,y) =  (-1 * gaussian_x(x,y-2) +  
                                -4 * gaussian_x(x,y-1) +
                                 6 * gaussian_x(x,y)  + 
                                 4 * gaussian_x(x,y+1) +
                                 1 * gaussian_x(x,y+2))/16;

            Func edges_x;
            edges_x(x,y) = (-2*gaussian_y(x-2,y) - gaussian_y(x-1,y) + gaussian_y(x+1,y) + 2*gaussian_y(x+2,y)) / 6; 
            output(x,y,c) = cast<uint8_t>((-2*edges_x(x,y-2) - edges_x(x,y-1) + edges_x(x,y+1) + 2*edges_x(x,y+2)) / 6);
            if (interleaved) {
                output.dim(0).set_stride(3);
                output.dim(2).set_stride(1);
            }
        }
};


HALIDE_REGISTER_GENERATOR(EdgeDetect, edge_detect);

