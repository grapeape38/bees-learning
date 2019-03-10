#include "SIFTFeatures.h"
#include "MatrixHelper.h"
#include <iostream>
#include <cmath>
#include <fstream>

SIFTFeatures::SIFTFeatures(VectorToColor *v2c, bool inter, bool verb)
    : vec2color(v2c), interleaved(inter), error(-1), verbose(verb) { }

void SIFTFeatures::log(const char *fmt, ...) {
    if (verbose) {
        va_list args;
        fprintf(stderr, "LOG: ");
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
    }
}

inline Runtime::Buffer<uint8_t> gray(Runtime::Buffer<uint8_t> &buf, bool interleaved=false) {
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

int SIFTFeatures::filter(Runtime::Buffer<uint8_t> &input, Runtime::Buffer<uint8_t> &output, const char* out_file) {
    getKeyPoints(input);
    output = std::move(gray(input, interleaved));
    plotKeyPoints(output);
    if (out_file != NULL) {
        writeDescriptorsToFile(out_file);
    }
    return error;
}

void SIFTFeatures::plotKeyPoints(Runtime::Buffer<uint8_t> output) {
    /*Color white {0xFFFFFF};
    for (int i = 0; i < output.width(); i++) {
        for (int j = 0; j < output.height(); j++) {
            draw(output, i, j, white);
        }
    }*/
    for (const auto &v : keypoints) {
        int x = v[1], y = v[2], scale = v[3];
        const std::vector<float> &descript = descriptors[kp_index[x][y]];
        Color col = vec2color != NULL ? vec2color->getColor(descript) : Color {0xFF00FF};
        draw(output, v[1]*scale, v[2]*scale, col);
    }
}

inline std::vector<float> getXHat(Runtime::Buffer<float,3> D2DX2[6], Runtime::Buffer<float,3> D1[3],
    int k, int x, int y) {
            std::vector<float> inv =
                 invsym3x3(D2DX2[0](k,x,y), D2DX2[1](k,x,y), D2DX2[2](k,x,y),
                                            D2DX2[3](k,x,y), D2DX2[4](k,x,y),
                                                             D2DX2[5](k,x,y));
            std::vector<float> md = mat_dot(inv, D1[0](k,x,y), D1[1](k,x,y), D1[2](k,x,y));
            mult(md, -1.0f);
            return md;
}

void SIFTFeatures::getKeyPoints(Runtime::Buffer<uint8_t,3> input) {   
    keypoints.clear();
    descriptors.clear();
    const int s = 5;
    //int num_scales = 2;
    //for(int i = 0; i < num_scales; i++) {
        int scale = 1;
        int width = input.width() / scale, height = input.height() / scale;
        std::vector<int> sizes {s, width, height};

        Runtime::Buffer<uint8_t, 3> scaled_inp;
        Runtime::Buffer<float> gauss(sizes), maxDiff(s-3, width, height), DoG(s-1, width, height),
            mag(sizes);
        Runtime::Buffer<int16_t> theta(sizes);

        kp_index.assign(width, std::vector<int>(height, 0));

        float kernels[5][5] = {
            {0.00135f, 0.157305f, 0.68269f, 0.157305f, 0.00135f},
            {0.016751f, 0.222893f, 0.520712f, 0.222893f, 0.016751f},
            {0.130598f, 0.230293f, 0.278216f, 0.230293f, 0.130598f},
            {0.162952f, 0.217391f, 0.239314f, 0.217391f, 0.162952f},
            {0.153388f, 0.221461f, 0.250301f, 0.221461f, 0.153388f},
        };

        Runtime::Buffer<float> k_buf{kernels};

        //ASSIGN SIGMA SCALE VALUE TO EACH LEVEL
        const float sc_mid = 1.6, sc_k = sqrtf(2.0f);
        scales[s/2] = sc_mid;
        for (int i = s/2 - 1; i >= 0; i--) {
            scales[i] = scales[i+1] / sc_k;
        }
        for (int i = s/2 + 1; i < s; i++) {
            scales[i] = scales[i-1] * sc_k;
        }

        if (interleaved) {
            error = find_maxima_inter(input, k_buf, gauss, maxDiff, DoG, mag, theta);
        }
        else {
            error = find_maxima(input, k_buf, gauss, maxDiff, DoG, mag, theta);
        }
        if (error) {
            return;
        }

        sizes = {s-1, width, height};
        Runtime::Buffer<float, 3> dx(sizes), dy(sizes), dk(sizes),
                                  dkk(sizes), dxk(sizes), dyk(sizes),
                                  dxx(sizes), dxy(sizes), dyy(sizes);

        error = get_derivatives(DoG, dx, dy, dk, dkk, dxk, dyk, dxx, dxy, dyy);
        if (error) {
            return;
        }

        Runtime::Buffer<float, 3> D2DX2[6] = {dkk,dxk,dyk,dxx,dxy,dyy}, D1[3] = {dk,dx,dy};

        std::vector<std::vector<int>> kp1;
        for (int k = 0; k < s - 3; k++) {
            for (int x = 0; x < width; x++) {
                for (int y = 0; y < height; y++) {
                    if (maxDiff(k,x,y) > 0.0f) {
                        kp1.push_back({k+1,x,y});
                    }
                }
            }
        }
        log("# KP before applying thresholding and interpolation: %d", kp1.size());

        const float maxT = 10.0f, minC = 0.03f;
        int kMax = s - 2, xMax = width - 1, yMax = height - 1;
        auto clmp = [](double d, double mn, double mx) { return std::max(std::min(d, mx), mn); };
        auto update = [](float f) { return f + f / fabsf(f); };
        int num_interp = 0;
        for (const auto &v : kp1) {
            int k = v[0], x = v[1], y = v[2];
            std::vector<float> xhat = getXHat(D2DX2, D1, k, x, y);
            //PERFORM INTERPOLATION
            int num_its = 20;
            while (num_its-- > 0  && (fabsf(xhat[0]) > 0.5f || fabsf(xhat[1]) > 0.5f || fabsf(xhat[2]) > 0.5f)) {
                if (fabsf(xhat[0]) > 0.5f) {
                    k = clmp(update(k), 0, kMax);
                }
                if (fabsf(xhat[1]) > 0.5f) {
                    x = clmp(update(x), 0, xMax);
                }
                if (fabsf(xhat[2]) > 0.5f) {
                    y = clmp(update(y), 0, yMax);
                }
                xhat = getXHat(D2DX2, D1, k, x, y);
            }
            if (num_its >= 0) num_interp++;
            //MAXIMUM EIGENV RATIO AND CONTRAST THRESHOLDING STEPS
            float ER = edge_response(dxx(k,x,y), dxy(k,x,y), dyy(k,x,y)),
                contrast = fabsf(dk(k,x,y) * xhat[0] + dx(k,x,y) * xhat[1] * dy(k,x,y) * xhat[2] + DoG(k,x,y));
            if (ER < maxT && contrast > minC) {
                keypoints.push_back({k, x, y, scale});
            }
        }

        log("%0.2f \% keypoints successfully interpolated: ", 100*(double)num_interp / kp1.size());

        log("# KP after: %d", keypoints.size());
        getDescriptors(mag, theta);
}

constexpr float gauss_weight(int x, int y, float sc) {
    return 1 / (2 * (float)M_PI) * exp(-0.5f * (x * x + y * y) / (sc * sc));
}

inline void normalize(std::vector<float> &v) {
    float s = sqrtf(std::inner_product(v.begin(), v.end(), v.begin(), 0.0f));
    std::transform(v.begin(), v.end(), v.begin(), [&](float x) { return x / s; });
}

inline void threshold(std::vector<float> &v) {
    std::transform(v.begin(), v.end(), v.begin(), [&](float x) { return std::min(x, 0.2f); });
}

inline void rotCoord(int &x, int &y, float rad) {
    int x2 = x*cos(rad) - y*sin(rad);
    int y2 = x*sin(rad) + y*cos(rad);
    x = x2;
    y = y2;
}

inline std::vector<float> computeDescript(int k, int x, int y, int maxX, int maxY, int th, float sc,
    Runtime::Buffer<float,3> mag, Runtime::Buffer<int16_t,3> theta) {
    std::vector<float> descript(128);
    float rad = M_PI * th / 180.0f;
    int x_corner = x - 7, y_corner = y - 7;
    for (int i = 0; i < 16; i++) {
        int ux = x_corner + (i % 4) * 4, uy = y_corner + (i / 4) * 4;
        for (int j = 0; j < 16; j++) {
            int ux2 = ux + j % 4, uy2 = uy + j / 4;
            int dx = ux2 - x, dy = y - uy2;
            rotCoord(dx, dy, rad);
            ux2 = x + dx;
            uy2 = y - dy;
            if (ux2 >= 0 && ux2 < maxX && uy2 >= 0 && uy2 < maxY) {
                int th2 = theta(k, ux2, uy2) + th;
                if (th2 < 0) th2 += 360;
                th2 /= 45;
                descript[i * 8 + th2] += mag(k, ux2, uy2) * gauss_weight(dx, dy, sc);
            }
        }
    }
    normalize(descript);
    threshold(descript);
    normalize(descript);
    return descript;
}

void SIFTFeatures::getDescriptors(Runtime::Buffer<float,3> mag, Runtime::Buffer<int16_t,3> theta) {
    //ASSIGN ORIENTATION TO EACH KEYPOINT
    int num_orient = 0;
    int maxX = theta.dim(1).extent(), maxY = theta.dim(2).extent();
    for (const auto &v : keypoints) {
        std::vector<float> orient_hist(36);
        int k = v[0], x = v[1], y = v[2];
        float sc = scales[k]*1.5f, maxTh = -1.0f;
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                int x2 = x + dx, y2 = y + dy;
                if (x2 >= 0 && x2 < maxX && y2 >= 0 && y2 < maxY) {
                    int16_t th = theta(k,x2,y2);
                    if (th < 0) th += 360;
                    th /= 10;
                    orient_hist[th] += mag(k,x2,y2) * gauss_weight(dx,dy,sc);
                    maxTh = std::max(orient_hist[th], maxTh);
                }
            }
        }
        int kp = 0;
        sc = scales[k]*0.5f;
        for (int i = 0; i < 36; i++) {
            if (maxTh - orient_hist[i] < 0.2f * maxTh) {
                int th = i * 10;
                if (th > 180) th -= 360;
                kp++;
                descriptors.push_back(computeDescript(k,x,y,maxX,maxY,th,sc,mag,theta));
                if (maxTh == orient_hist[i])
                    kp_index[x][y] = descriptors.size() - 1;
            }
        }
        if (kp > 1) {
            num_orient++;
        }
    }
    log("%0.3f\% keypoints with mult orientations", (double)num_orient * 100 / keypoints.size());
}

void SIFTFeatures::writeDescriptorsToFile(const char *fname) {
    std::fstream f(fname, std::fstream::out);
    if (!f.is_open()) {
        std::cout << "Unable to open file for writing.\n";
    }
    for (const auto &v : descriptors) {
        for (int i = 0; i < 128; i++) {
            f << v[i] << " ";
        }
        f << '\n';
    }
    f.close();
    log("%d descriptors written to file", descriptors.size());
}

