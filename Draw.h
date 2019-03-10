#ifndef DRAW_H
#define DRAW_H

#include <stdint.h>
#include "Halide.h"

using namespace Halide;

struct Color {
    uint8_t R, G, B;
    Color(uint8_t r, uint8_t g, uint8_t b) : R(r), G(g), B(b) {}
    Color(const std::vector<uint8_t> &v) : R(v[0]), G(v[1]), B(v[2]) {}
    Color(int color) {
            R = (color & 0xFF0000) >> 16; 
            G = (color & 0x00FF00 ) >> 8; 
            B = (color & 0x0FF);
    }
};

inline void draw(Runtime::Buffer<uint8_t> buff, int x, int y, Color c) {
    buff(x,y,0) = c.R;
    buff(x,y,1) = c.G;
    buff(x,y,2) = c.B;
}

class VectorToColor {
    public:
        virtual Color getColor(const std::vector<float> &v) {
            return Color {0xFF00FF};
        }
};

#endif