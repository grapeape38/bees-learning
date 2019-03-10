#ifndef PCA_H
#define PCA_H

#include "Draw.h"
#include <fstream>
#include <sstream>
#include "MatrixHelper.h"

class PCA : public VectorToColor {
    public:
        PCA(const char *fname, int sz=128) : c_size(sz) {
            load(fname);
        }
        std::vector<float> transform(const std::vector<float> &v) {
            return mat_dot(components, v);
        }
        Color getColor(const std::vector<float> &v) {
            std::vector<float> c_vec = transform(v);
            std::vector<uint8_t> res(3);
            std::transform(c_vec.begin(), c_vec.end(), res.begin(), [](float f) {
                return 128 * (f + 1);
            });
            Color c(res);
            return c;
        }
        void load(const char* fname, int sz=128) {
            c_size = sz;
            std::fstream f(fname, std::fstream::in);
            if (!f.is_open()) {
                std::cout << "Was unable to open the components file.";
                return;
            }
            std::string line;
            while (getline(f, line)) {
                std::stringstream ss(line);
                std::vector<float> comp(c_size);
                for (int i = 0; i < c_size; i++)
                    ss >> comp[i];
                components.push_back(comp);
            }
        }
    private:
        std::vector<std::vector<float>> components;
        int c_size;
};

#endif