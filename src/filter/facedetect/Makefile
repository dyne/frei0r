# Adapt this to the installation prefix of opencv
OPENCV_PREFIX = /usr/local

CC=g++
#CCFLAGS=-g -Wall -fPIC
CCFLAGS  = -I../../../spec -I$(OPENCV_PREFIX)/include/opencv
CCFLAGS += -DOPENCV_PREFIX=$(OPENCV_PREFIX) -c

LDFLAGS = -Wl -shared -L$(OPENCV_PREFIX)/lib/ -lcv -lhighgui 

all: objects
	$(CC) -o facedetect.so facedetect.o $(LDFLAGS)

objects: facedetect.c
	$(CC) $(CCFLAGS) -o facedetect.o facedetect.c

clean:
	- rm -f facedetect.o facedetect.dll *~
