# Makefile
TARGETS = Template.so
OBJS = $(SRCS:.cpp=.o)
CC = g++

SSP_LIB = -L ../../../config/sensor/lib
SSP_INC = -I include

MMAL_LIB = -L/opt/vc/lib -lmmal -lmmal_core -lmmal_util -lvcos -lbcm_host -lwiringPi -lpthread

CFLAGS = -shared -fPIC -Wall -rdynamic
SRCS = $(wildcard *.cpp)
BASE_SRCS = ${wildcard ../../base/*.cpp}
BASE_INC = -I ../../base
#OPT = -lm -std=c++11
OPT = -lm -O3
OPENCV_LIB = -L/usr/lib/arm-linux-gnueabihf -lopencv_calib3d -lopencv_contrib -lopencv_core -lopencv_features2d -lopencv_flann -lopencv_gpu -lopencv_highgui -lopencv_imgproc -lopencv_legacy -lopencv_ml -lopencv_objdetect -lopencv_video -lopencv_core
OBJ_PATH = $(wildcard *.o)




$(TARGETS): $(OBJS)
	$(CC) $(CFLAGS) -g -fopenmp -o $(TARGETS) $(BASE_SRCS) $(SRCS) $(BASE_INC) $(OPT) $(OPENCV_LIB) $(SSP_LIB) $(SSP_INC)  $(MMAL_LIB) -lssp -lsspprof `wx-config --cxxflags` `wx-config --libs`

.cpp.o:
	$(CC) $(CFLAGS) -fopenmp -c $(BASE_INC) $(OPT)  $(SSP_LIB) $(SSP_INC)  $(MMAL_LIB) -lssp -lsspprof $< `wx-config --cppflags`

.PHONY: clean
clean:
	$(RM) *~ $(OBJ_PATH) $(TARGETS)
