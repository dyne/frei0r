/*
 * This source code  is free software; you can  redistribute it and/or
 * modify it under the terms of the GNU Public License as published by
 * the Free Software  Foundation; either version 3 of  the License, or
 * (at your option) any later version.
 *
 * This source code is distributed in the hope that it will be useful,
 * but  WITHOUT ANY  WARRANTY; without  even the  implied  warranty of
 * MERCHANTABILITY or FITNESS FOR  A PARTICULAR PURPOSE.  Please refer
 * to the GNU Public License for more details.
 *
 * You should  have received  a copy of  the GNU Public  License along
 * with this source code; if  not, write to: Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include <stdio.h>
#include <stdlib.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <frei0r.hpp>


typedef struct {
  IplImage* hsv;     //input image converted to HSV
  IplImage* hue;     //hue channel of HSV image
  IplImage* mask;    //image for masking pixels
  IplImage* prob;    //face probability estimates for each pixel

  CvHistogram* hist; //histogram of hue in original face image

  CvRect prev_rect;  //location of face in previous frame
  CvBox2D curr_box;  //current face location estimate
} TrackedObj;



class FaceBl0r: public frei0r::filter {

public:
    FaceBl0r(int wdt, int hgt);
    ~FaceBl0r();

    virtual void update();

private:
    
// camshift
    TrackedObj* create_tracked_object (IplImage* image, CvRect* face_rect);
    void destroy_tracked_object (TrackedObj* tracked_obj);
    CvBox2D camshift_track_face (IplImage* image, TrackedObj* imgs);
    void update_hue_image (const IplImage* image, TrackedObj* imgs);
    
//trackface
    CvRect* detect_face (IplImage*, CvHaarClassifierCascade*, CvMemStorage*);
    

    TrackedObj* tracked_obj;
    CvBox2D face_box; //area to draw
    CvRect* face_rect;
    
//used by capture_video_frame, so we don't have to keep creating.
    IplImage* image;

    CvHaarClassifierCascade* cascade;
    CvMemStorage* storage;

    char *classifier;

    bool face_found;
};


frei0r::construct<FaceBl0r> plugin("FaceBl0r",
				  "automatic face blur ",
				  "ZioKernel, Biilly, Jilt, Jaromil",
				  1,0);

FaceBl0r::FaceBl0r(int wdt, int hgt) {

  width = wdt;
  height = hgt;
  size = width*height*4;
  //initialize
  classifier = (const char*)"/usr/share/opencv/haarcascades/haarcascade_frontalface_default.xml";
  cascade = (CvHaarClassifierCascade*) cvLoad(classifier, 0, 0, 0 );
  storage = cvCreateMemStorage(0);
  //validate
  assert(cascade && storage);

  face_rect = 0;
  image = 0;
  tracked_obj = 0;
  face_found = false;

}

FaceBl0r::~FaceBl0r() {
    if(tracked_obj)
        destroy_tracked_object(tracked_obj);

//    if(cascade) cvReleaseHaarClassifierCascade(&cascade);
//    if(storage) cvReleaseMemStorage(&storage);
    
}

void FaceBl0r::update() {
  unsigned char *src = (unsigned char *)in;
  unsigned char *dst = (unsigned char *)out;

  if( !image )
      image = cvCreateImage( cvSize(width,height), IPL_DEPTH_8U, 4 );

  unsigned char* ipli = (unsigned char*)image->imageData;
  int step = image->widthStep;
  unsigned i, j;
  for (i = 0; (i < height); i++) {
    for (j = 0; (j < width); j++) {

//        ipli[(i*step)+j*4+0] = src[0];
        ipli[(i*step)+j*4+2] = src[2];
        ipli[(i*step)+j*4+1] = src[1];
        ipli[(i*step)+j*4+0] = src[0];
      //ipli += 4;
      src += 4;
    }
  }


  /*
    no face*
     - look for (detect_face)
    yes face
     - track face
     - no more face
       no face*
   */

  if(!face_found) {
      face_rect = detect_face(image, cascade, storage);
      // if face detected
      if (!face_rect) {
          memcpy(out,in,size);
          return;
      }
      //track detected face with camshift
      tracked_obj = create_tracked_object(image, face_rect);
      face_found = true;
  }

  //track the face in the new frame
  face_box = camshift_track_face(image, tracked_obj);

  if( ( face_box.size.width < 10 )
      || (face_box.size.height <10 )
      || (face_box.size.width > 500 )
      || (face_box.size.height > 500 )
      ) {

      face_found = false;
      return;
  }

////////////////////////////////////////////////////////////////////////
  cvSetImageROI (image, tracked_obj->prev_rect);
//  cvSmooth (image, image, CV_BLUR, 22, 22, 0, 0);
      cvSmooth (image, image, CV_BLUR, 11, 11, 0, 0);
//      cvSmooth (image, image, CV_GAUSSIAN, 11, 11, 0, 0);
  cvResetImageROI (image);
////////////////////////////////////////////////////////////////////////
      
  //outline face ellipse
  cvEllipseBox(image, face_box, CV_RGB(255,0,0), 3, CV_AA, 0);

//  memcpy(out,image->imageData,size);
  src = (unsigned char *)in;

  for (i = 0; (i < height); i++) {
    for (j = 0; (j < width); j++) {

//        ipli[(i*step)+j*4+0] = src[0];
        dst[3] = src[3];
        dst[2] = ipli[(i*step)+j*4+2];
        dst[1] = ipli[(i*step)+j*4+1];
        dst[0] = ipli[(i*step)+j*4+0];
      //ipli += 4;
        dst += 4;
        src += 4;
    }
  }

}

/* Given an image and a classider, detect and return region. */
CvRect* FaceBl0r::detect_face (IplImage* image,
                               CvHaarClassifierCascade* cascade,
                               CvMemStorage* storage) {
    
  CvRect* rect = 0;
  
  //get a sequence of faces in image
  CvSeq *faces = cvHaarDetectObjects(image, cascade, storage,
     1.1,                       //increase search scale by 10% each pass
     6,                         //require 6 neighbors
     CV_HAAR_DO_CANNY_PRUNING,  //skip regions unlikely to contain a face
     cvSize(0, 0));             //use default face size from xml

  //if one or more faces are detected, return the first one
  if(faces && faces->total)
    rect = (CvRect*) cvGetSeqElem(faces, 0);

  return rect;
}

/* Create a camshift tracked object from a region in image. */
TrackedObj* FaceBl0r::create_tracked_object (IplImage* image, CvRect* region) {
  TrackedObj* obj;
  
  //allocate memory for tracked object struct
  if((obj = (TrackedObj *) malloc(sizeof *obj)) != NULL) {
    //create-image: size(w,h), bit depth, channels
    obj->hsv  = cvCreateImage(cvGetSize(image), 8, 3);
    obj->mask = cvCreateImage(cvGetSize(image), 8, 1);
    obj->hue  = cvCreateImage(cvGetSize(image), 8, 1);
    obj->prob = cvCreateImage(cvGetSize(image), 8, 1);

    int hist_bins = 30;           //number of histogram bins
    float hist_range[] = {0,180}; //histogram range
    float* range = hist_range;
    obj->hist = cvCreateHist(1,             //number of hist dimensions
                             &hist_bins,    //array of dimension sizes
                             CV_HIST_ARRAY, //representation format
                             &range,        //array of ranges for bins
                             1);            //uniformity flag
  }
  
  //create a new hue image
  update_hue_image(image, obj);

  float max_val = 0.f;
  
  //create a histogram representation for the face
  cvSetImageROI(obj->hue, *region);
  cvSetImageROI(obj->mask, *region);
  cvCalcHist(&obj->hue, obj->hist, 0, obj->mask);
  cvGetMinMaxHistValue(obj->hist, 0, &max_val, 0, 0 );
  cvConvertScale(obj->hist->bins, obj->hist->bins,
                 max_val ? 255.0/max_val : 0, 0);
  cvResetImageROI(obj->hue);
  cvResetImageROI(obj->mask);
  
  //store the previous face location
  obj->prev_rect = *region;

  return obj;
}

/* Release resources from tracked object. */
void FaceBl0r::destroy_tracked_object (TrackedObj* obj) {
  cvReleaseImage(&obj->hsv);
  cvReleaseImage(&obj->hue);
  cvReleaseImage(&obj->mask);
  cvReleaseImage(&obj->prob);
  cvReleaseHist(&obj->hist);

  free(obj);
}

/* Given an image and tracked object, return box position. */
CvBox2D FaceBl0r::camshift_track_face (IplImage* image, TrackedObj* obj) {
  CvConnectedComp components;

  //create a new hue image
  update_hue_image(image, obj);

  //create a probability image based on the face histogram
  cvCalcBackProject(&obj->hue, obj->prob, obj->hist);
  cvAnd(obj->prob, obj->mask, obj->prob, 0);

  //use CamShift to find the center of the new face probability
  cvCamShift(obj->prob, obj->prev_rect,
             cvTermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1),
             &components, &obj->curr_box);

  //update face location and angle
  obj->prev_rect = components.rect;
  obj->curr_box.angle = -obj->curr_box.angle;

  return obj->curr_box;
}

void FaceBl0r::update_hue_image (const IplImage* image, TrackedObj* obj) {
  //limits for calculating hue
  int vmin = 65, vmax = 256, smin = 55;
  
  //convert to HSV color model
  cvCvtColor(image, obj->hsv, CV_BGR2HSV);
  
  //mask out-of-range values
  cvInRangeS(obj->hsv,                               //source
             cvScalar(0, smin, MIN(vmin, vmax), 0),  //lower bound
             cvScalar(180, 256, MAX(vmin, vmax) ,0), //upper bound
             obj->mask);                             //destination
  
  //extract the hue channel, split: src, dest channels
  cvSplit(obj->hsv, obj->hue, 0, 0, 0 );
}
