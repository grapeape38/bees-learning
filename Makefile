CXX=g++
IMGFLAGS=`libpng-config --cflags --ldflags` -ljpeg

CPPFLAGS=-g -std=c++11 -I ../Halide/include -I ../Halide/tools 

LDLIBS=-L ../Halide/bin
LDFLAGS=-lHalide -lpthread -ldl $(IMGFLAGS)

PIPE_H=find_maxima.h find_maxima_inter.h get_derivatives.h edge_detect_inter.h
PIPE_LIB=find_maxima.a find_maxima_inter.a get_derivatives.a edge_detect_inter.a
PIPE_GEN=EdgePipeline SIFTGenerator

IMG_OBJS=ImageDriver.o ImageFilter.o SIFTFeatures.o find_maxima.a edge_pipeline.a
SIFT_OBJS=SIFTDriver.o SIFTFeatures.o $(PIPE_LIB) 
VID_OBJS=VideoDriver.o VideoFilter.o SIFTFeatures.o $(PIPE_LIB) 

ML_H=Clusters.h PCA.h

FFMPEG_LIBS=libavformat libavfilter libavcodec libavutil libswscale

CPPFLAGS := $(shell pkg-config --cflags $(FFMPEG_LIBS)) $(CPPFLAGS) 

LDLIBS := $(shell pkg-config --libs $(FFMPEG_LIBS)) $(LDLIBS) 

video: $(VID_OBJS) 
	$(CXX) $(CPPFLAGS) $^ $(LDLIBS) $(LDFLAGS) -o VideoDriver

sift: $(SIFT_OBJS)
	$(CXX) $(CPPFLAGS) $^ $(LDLIBS) $(LDFLAGS) -o SIFTDriver

image: $(IMG_OBJS) 
	$(CXX) $(CPPFLAGS) $^ $(LDLIBS) $(LDFLAGS) -o ImageDriver

SIFTDriver.o: SIFTDriver.cpp $(ML_H) $(PIPE_H) SIFTFeatures.h  

ImageDriver.o: ImageDriver.cpp SIFTFeatures.h  

VideoDriver.o: VideoDriver.cpp VideoFilter.h $(ML_H) $(PIPE_H) FrameFilter.h SIFTFeatures.h

ImageFilter.o: ImageFilter.cpp ImageFilter.h 

VideoFilter.o: VideoFilter.cpp VideoFilter.h 

SIFTFeatures.o: SIFTFeatures.cpp Clusters.h $(PIPE_H) SIFTFeatures.h Draw.h FeatureExtractor.h

pipe: $(PIPE_H)

edge_pipeline.h: EdgePipeline
	./gen_pipeline.sh EdgePipeline 

edge_pipeline_inter.h: EdgePipeline
	./gen_pipeline.sh EdgePipeline 

EdgePipeline: EdgePipeline.cpp
	g++ EdgePipeline.cpp -g -std=c++11 -I ../Halide/include -L ../Halide/bin -lHalide -lpthread -ldl -o EdgePipeline

find_maxima_inter.a find_maxima_inter.h: SIFTGenerator 
	./SIFTGenerator -g maxima_gen -f find_maxima_inter -o . target=host

find_maxima.a find_maxima.h: SIFTGenerator
	./SIFTGenerator -g maxima_gen -f find_maxima -o . target=host interleaved=false

get_derivatives.a get_derivatives.h: SIFTGenerator
	./SIFTGenerator -g deriv_gen -f get_derivatives -o . target=host 

edge_detect_inter.a edge_detect_inter.h: EdgeDetect
	./EdgeDetect -g edge_detect -f edge_detect_inter -o . target=host 

EdgeDetect: EdgeDetect.cpp
	g++ EdgeDetect.cpp ../Halide/tools/GenGen.cpp -g -std=c++11 -fno-rtti -I ../Halide/include -L ../Halide/bin -lHalide -lpthread -ldl -o EdgeDetect

SIFTGenerator: SIFTGenerator.cpp
	g++ SIFTGenerator.cpp ../Halide/tools/GenGen.cpp -g -std=c++11 -fno-rtti -I ../Halide/include -L ../Halide/bin -lHalide -lpthread -ldl -o SIFTGenerator

clean:
	rm -rf *.o VideoDriver ImageDriver SIFTDriver EdgeDetect $(IMG_OBJS) $(VID_OBJS) $(SIFT_OBJS) *.a *.registration.* $(PIPE_GEN) $(PIPE_H) $(PIPE_LIB) 
