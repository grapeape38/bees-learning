#include "Halide.h"
#include <stdio.h>

using namespace Halide;

class MaximaGen : public Halide::Generator<MaximaGen> {
    public:
        GeneratorParam<bool> interleaved{"interleaved", true};

        Input<Buffer<uint8_t>> input{"input",3};
        Input<Buffer<float>> k_buf{"kernels",2};

        Output<Buffer<float>> gauss{"gauss", 3}, maxDiff{"maxDiff", 3}, DoG{"DoG", 3}, mag{"mag", 3};

        Output<Buffer<int16_t>> theta{"theta",3};
        Var x,y,c,k;

        void generate() {
            if (interleaved) {
                input.dim(0).set_stride(3);
                input.dim(2).set_stride(1);
            }

            Expr clamped_k = clamp(k, 0, 4);
            Expr clamped_x = clamp(x, 0, input.width()-1);
            Expr clamped_y = clamp(y, 0, input.height()-1);

            Func gray;
            gray(x, y) = 0.299f * input(x, y, 0) + 0.587f * input(x, y, 1) + 0.114f * input(x, y, 2);

            Func clamped, gaussian_x;
            clamped(x, y) = gray(clamped_x, clamped_y);

            gaussian_x(k,x,y) = k_buf(0,k) * clamped(x-2,y) +
                                k_buf(1,k) * clamped(x-1,y) +
                                k_buf(2,k) * clamped(x,y) + 
                                k_buf(3,k) * clamped(x+1,y) +
                                k_buf(4,k) * clamped(x+2,y);

            gauss(k,x,y) =      k_buf(0,k) * gaussian_x(k,x,y-2) +  
                                k_buf(1,k) * gaussian_x(k,x,y-1) +
                                k_buf(2,k) * gaussian_x(k,x,y)  + 
                                k_buf(3,k) * gaussian_x(k,x,y+1) +
                                k_buf(4,k) * gaussian_x(k,x,y+2);

            Func clamp_gauss;
            clamp_gauss(k,x,y) = gauss(clamped_k, clamped_x, clamped_y);

            Expr xdiff = clamp_gauss(k,x+1,y) - clamp_gauss(k,x-1,y);
            Expr ydiff = clamp_gauss(k,x,y+1) - clamp_gauss(k,x,y-1);
            mag(k,x,y) = sqrt(xdiff*xdiff + ydiff*ydiff);
            float rad2deg = 180.0f / (float)M_PI;
            theta(k,x,y) = cast<int16_t>(round(atan2(ydiff, xdiff) * rad2deg));

            Func dog;
            DoG(k,x,y) = clamp_gauss(k+1,x,y) - clamp_gauss(k,x,y);
            Func clamp_dog;
            clamp_dog(k,x,y) = DoG(clamped_k, clamped_x, clamped_y);

            RDom box(-1, 3, -1, 3, -1, 3);
            box.where(box.x != 0 || box.y != 0 || box.z != 0);
            Expr getMax = maximum(clamp_dog(k + 1 + box.x, x + box.y, y + box.z));
            maxDiff(k,x,y) = DoG(k+1,x,y) - getMax;
        }
};

class Derivatives : public Halide::Generator<Derivatives> {
    public:
        Var k,x,y;
        Input<Buffer<float>> DoG{"DoG", 3};
        Output<Func> D1 {"D1",{Float(32), Float(32), Float(32)}, 3},
                     D2DX2 {"D2DX2", {Float(32), Float(32), Float(32), Float(32), Float(32), Float(32)}, 3};
        void generate() {
            Func clamp_dog;
            Expr clamped_k = clamp(k, 0, 3);
            Expr clamped_x = clamp(x, 0, DoG.dim(1).extent()-1);
            Expr clamped_y = clamp(y, 0, DoG.dim(2).extent()-1);
            clamp_dog(k,x,y) = DoG(clamped_k, clamped_x, clamped_y);

            Func dx, dy, dk, dxx, dyy, dxy, dxk, dyk, dkk;
            dx = DX(clamp_dog);
            dy = DY(clamp_dog);
            dk = DK(clamp_dog);
            dxx = DX(dx);
            dyy = DY(dy);
            dxy = DY(dx);
            dxk = DX(dk);
            dyk = DY(dk);
            dkk = DK(dk);
            D2DX2(k,x,y) = {dkk(k,x,y), dxk(k,x,y), dyk(k,x,y), dxx(k,x,y), dxy(k,x,y), dyy(k,x,y)};
            D1(k,x,y) = {dk(k,x,y), dx(k,x,y), dy(k,x,y)};
        }
    private:
        Func DX(Func f) {
            Func out;
            //out(k,x,y) = (-2*f(k,x-2,y) - 1*f(k,x-1,y) + f(k,x+1,y) + 2*f(k,x+2,y)) / 6;
            out(k,x,y) = (-1*f(k,x-1,y) + f(k,x+1,y)) / 2;
            return out;
        }

        Func DY(Func f) {
            Func out;
            //out(k,x,y) = (-2*f(k,x,y-2) - 1*f(k,x,y-1) + f(k,x,y+1) + 2*f(k,x,y+2)) / 6;
            out(k,x,y) = (-1*f(k,x,y-1) + f(k,x,y+1)) / 2;
            return out;
        }

        Func DK(Func f) {
            Func out;
            //out(k,x,y) = (-2*f(k-2,x,y) - 1*f(k-1,x,y) + f(k+1,x,y) + 2*f(k+2,x,y)) / 6;
            out(k,x,y) = (-1*f(k-1,x,y) + f(k+1,x,y)) / 2;
            return out;
        }
};

HALIDE_REGISTER_GENERATOR(MaximaGen, maxima_gen);
HALIDE_REGISTER_GENERATOR(Derivatives, deriv_gen);
