#include <vector>
#include <cassert>
#include <algorithm>
#include <numeric>
#include <cmath>

#ifndef MATRIX_H
#define MATRIX_H

inline float det2(float a, float b, float c, float d) {
    return a * d - b * c;
}

inline std::vector<float> invsym3x3(float a, float b, float c, float d, float e, float f) {
    /*a b c
      b d e
      c e f*/
    float D =  a * (f * d - e * e) 
            - b * (f * b - c * e)
            + c * (b * e - d * c);
    float a11 = det2(d, e, e, f) / D,
         a12 = det2(c, f, b, e) / D,
         a13 = det2(b, c, d, e) / D,
         a22 = det2(f, c, c, a) / D,
         a23 = det2(b, a, e, c) / D,
         a33 = det2(a, b, b, d) / D; 
    return {a11, a12, a13, a22, a23, a33};
}

inline std::vector<float> mat_dot(std::vector<float> &M, float v1, float v2, float v3) {
    return {M[0] * v1 + M[1] * v2  + M[2] * v3,
            M[1] * v1 + M[3] * v2 + M[4] * v3,
            M[2] * v1 + M[4] * v2 + M[5] * v3};
}

inline void mult(std::vector<float> &M, float s) {
    std::transform(M.begin(), M.end(), M.begin(), [=](float x) { return x * s; });
}

inline std::vector<float> mat_dot(const std::vector<std::vector<float>> &M, const std::vector<float> &v) {
    assert(M[0].size() == v.size());
    std::vector<float> res(M.size());
    std::transform(M.begin(), M.end(), res.begin(), [&](const std::vector<float> &r) {
        return std::inner_product(v.begin(), v.end(), r.begin(), 0.0f);
    });
    return res;
}

inline float dist2(const std::vector<float> &v1, const std::vector<float> &v2) {
    int n = v1.size();
    float ans = 0.0f;
    for (int i = 0; i < n; i++) {
        float diff = fabs(v1[i] - v2[i]);
        ans += diff * diff; 
    }
    return ans;
}

inline float edge_response(float a, float b, float c) {
    float tr = a + c, det = det2(a,b,b,c);
    return tr*tr/det;
}

#endif