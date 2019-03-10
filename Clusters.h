#ifndef CLUSTERS_H
#define CLUSTERS_H

#include <vector>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <cmath>

#include "Draw.h"
#include "MatrixHelper.h"

class Clusters : public VectorToColor {
    public:
        Clusters(const char* fname, int c_sz=128) : c_size(c_sz) {
            load(fname);
        }

        Color getColor(const std::vector<float> &v) {
            return colors[nearestCluster(v) % n_clusters];
        }

        void load(const char* fname, int c_sz=128) {
            c_size = c_sz;
            std::fstream f(fname, std::fstream::in);
            if (!f.is_open()) {
                std::cout << "Was unable to open the cluster file.";
                return;
            }
            std::string line;
            while (getline(f, line)) {
                std::stringstream(line);
                std::vector<float> cluster(c_size);
                for (int i = 0; i < c_size; i++)
                    f >> cluster[i];
                clusters.push_back(cluster);
            }
            n_clusters = clusters.size();
            f.close();
        }

        int n_clusters, c_size;

    private:
        int nearestCluster(const std::vector<float> &v) {
            if (!n_clusters) return 0;
            float min_dist = dist2(v, clusters[0]);
            int min_index = 0;
            for (int i = 1; i < n_clusters; i++) {
                float d = dist2(v, clusters[i]);
                if (d < min_dist) {
                    min_dist = d;
                    min_index = i;
                }
            }
            return min_index;
        }

        const Color colors[8] = {
            {0x40e0d0},
            {0xffd700},
            {0xe6e6fa},
            {0x00ffff},
            {0x666666},
            {0xFF0000},
            {0xd3ffce},
            {0xff7373}};

        std::vector<std::vector<float>> clusters;
};

#endif
