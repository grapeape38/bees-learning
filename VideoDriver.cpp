#include <stdio.h>
#include "VideoFilter.h"
#include "SIFTFeatures.h"
#include "Clusters.h"
#include "PCA.h"
#include "getopt.h"
#include "sys/stat.h"

const char *vid_dir = "videos",
           *out_dir = "output",
           *vec_dir = "features";

void printUsage() {
    printf("Usage: -f <video file> -o <output dir> -s <start frame> -n <numframes> -t <type>\
          -d <data file> -tiles <number of tiles>");
}

int parseArgs(int argc, char **argv, char *video_path, char *out_path, 
    int &num_tiles, int &start_frame, int &num_frames, VectorToColor **v2c)
{
    __mode_t mode = S_IRUSR | S_IXUSR | S_IWUSR;
    static struct option long_options[] =
    {
        {"tiles", required_argument, 0, 0},
        {"file", required_argument, 0, 'f'},
        {"outfile", required_argument, 0, 'o'},
        {"datafile", required_argument, 0, 'd'},
        {"startframe", required_argument, 0, 's'},
        {"numframes", required_argument, 0, 'n'},
        {"type", required_argument, 0, 't'}
    };
    char c;
    char vid_name[60], type[60], fname[60];
    bool img, out, vd, tname;
    start_frame = 0;
    num_frames = 50;
    int opt_index = 0;
    while ((c = getopt_long_only(argc, argv, "f:o:d:s:n:t:", long_options, &opt_index)) != -1) {
        switch(c) {
            case 0:
                if (opt_index == 0)
                    num_tiles = atoi(optarg);
                else {
                    printUsage();
                    return -1;
                }
                break;
            case 'f':
                img = true;
                sprintf(vid_name, "%s", optarg);
                break;
            case 'o':
                out = true;
                sprintf(out_path, "%s", optarg);
                break;
            case 'd':
                vd = true;
                sprintf(fname, "%s", optarg);
                break;
            case 't':
                tname = true;
                sprintf(type, "%s", optarg);
                break;
            case 's':
                start_frame = atoi(optarg);
                break;
            case 'n':
                num_frames = atoi(optarg);
                break;
            case '?':
            default:
                printUsage();
                return -1;
        }
    }
    if (start_frame < 0 || num_frames < 0 || num_tiles < 0) {
        printUsage();
        return -1;
    }
    if (!img) {
        sprintf(vid_name, "%s", "bees2.mp4");
    }
    if (!out) {
        sprintf(out_path, "%s/%s_%u", out_dir, vid_name, (unsigned)time(NULL)); 
        if (mkdir(out_path, mode) < 0) {
            printf("Failed to create new directory.");
            return -1;
        }
    }
    if (!vd) {
        sprintf(fname, "%s/%s", vec_dir, "cluster_centers.txt");
    }
    if (!tname) {
        sprintf(type, "%s", "clusters");
    }
    if (!strcmp(type, "clusters")) {
        *v2c = new Clusters(fname);
    }
    else if (!strcmp(type, "pca")) {
        *v2c = new PCA(fname);
    }
    else {
        printf("Unknown option type.");
        return -1;
    }
    char desc_path[128];
    sprintf(desc_path, "%s/descriptors", out_path); 
    if (mkdir(desc_path, mode) < 0) {
        printf("Failed to create directory for descriptors.");
        return -1;
    }
    sprintf(video_path, "%s/%s", vid_dir, vid_name);
    return 0;
}

int main(int argc, char **argv) {
    char video_path[128], out_path[128];
    VectorToColor *v2c;
    int num_tiles = 1, start_frame = 0, num_frames = 50;
    int ret = parseArgs(argc, argv, video_path, out_path, num_tiles, start_frame, num_frames, &v2c);
    if (ret < 0) {
        return -1;
    }
    SIFTFeatures sft(v2c);
    VideoFilter vf(video_path, out_path, &sft, num_tiles);
    int err = vf.filter(num_frames);
    if (err) {
        printf("Video Filter returned an error.");
        return -1;
    }
    return 0;
}
