#include <stdio.h>
#include "SIFTFeatures.h"
#include "Clusters.h"
#include "PCA.h"
#include "unistd.h"

using namespace Halide;

static const char* image_dir = "images", *out_dir = "output", *vec_dir = "features";

void printUsage() {
    printf("Usage: -f <filename> -o <output name> -d <vector color data> -t <vector color alg (clusters/pca)\n");
}

int parseArgs(int argc, char** argv,
    char *image_path, char *out_path, char *fname, char *write_path, VectorToColor **v2c, bool &write, bool &verbose)
{
    if (argc < 2) {
        printUsage();
        return -1;
    }
    char c;
    char img_file[60], type[60];
    bool img = false, out_p = false, tname = false, vcd = false;
    while ((c = getopt(argc, argv, "f:o:d:t:vw")) != -1) {
        switch(c) {
            case 'f':
                img = true;
                sprintf(img_file, "%s", optarg);
                break;
            case 'o':
                out_p = true;
                sprintf(out_path, "%s/%s", out_dir, optarg);
                break;
            case 'd':
                vcd = true;
                sprintf(fname, "%s/%s", vec_dir, optarg);
                break;
            case 't':
                tname = true;
                sprintf(type, "%s", optarg);
                break;
            case 'v':
                verbose = true;
                break;
            case 'w':
                write = true;
                break;
            case '?':
                printUsage();
                return -1;
            default:
                printUsage();
                return -1;
        }
    }
    if (!img) {
        sprintf(img_file, "bees.jpg");
    }
    if (!out_p) {
        sprintf(out_path, "%s/%s", out_dir, img_file);
    }
    if (!vcd) {
        sprintf(fname, "%s/cluster_centers.txt", vec_dir);
    }
    if (tname) {
        if (!strcmp(type, "clusters")) {
            *v2c = new Clusters(fname);
        }
        else if (!strcmp(type, "pca")) {
            *v2c = new PCA(fname);
        }
        else {
            printf("Unknown type.\n");
            return -1;
        }
    }
    else {
        *v2c = new Clusters(fname);
    }
    if (write) {
        sprintf(write_path, "%s/%s_descriptors.txt", out_dir, img_file);
    }
    sprintf(image_path, "%s/%s", image_dir, img_file);
    return 0;
}

void loadImage(Runtime::Buffer<uint8_t,3> &input, const char * fname) {
    input = Tools::load_image(fname);
}

void saveImage(Runtime::Buffer<uint8_t> &output, const char* fname) {
    Tools::save_image(output, fname);
}

int main(int argc, char **argv) {
        char image_path[50], out_path[60], fname[60], write_path[60];
    bool verbose = false, write = false;
    VectorToColor *v2c = NULL;
    int ret = parseArgs(argc, argv, image_path, out_path, fname, write_path, &v2c, write, verbose);
    if (ret < 0) {
        return -1;
    }
    char *wp = write ? write_path : NULL;
    SIFTFeatures sft(false, verbose);
    Runtime::Buffer<uint8_t,3> input;
    Runtime::Buffer<uint8_t> output;
    loadImage(input, image_path);
    sft.filter(input, output);
    if (sft.isError()) {
        printf("Halide returned an error.");
        return -1;
    }
    sft.writeFeatures(wp);
    saveImage(output, out_path);
    delete v2c;
    return 0;
}
