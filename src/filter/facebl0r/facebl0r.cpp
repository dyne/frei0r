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


#include <opencv2/core/version.hpp>
#define CV_VERSION_NUM (CV_MAJOR_VERSION * 10000 \
                      + CV_MINOR_VERSION * 100 \
                      + CV_VERSION_REVISION)
#include <stdio.h>
#include <stdlib.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#if CV_VERSION_NUM > 30401
#include <opencv2/imgproc.hpp>
#endif

#include <frei0r.hpp>
#include <frei0r_math.h>

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

    void update(double time,
                uint32_t* out,
                const uint32_t* in);

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

    // plugin parameters
    std::string classifier;
    f0r_param_bool ellipse;
    f0r_param_double recheck;
    f0r_param_double threads;
    f0r_param_double search_scale;
    f0r_param_double neighbors;
    f0r_param_double smallest;
    f0r_param_double largest;

    std::string old_classifier;
  

    unsigned int face_found;
    unsigned int face_notfound;
};


frei0r::construct<FaceBl0r> plugin("FaceBl0r",
				  "automatic face blur",
				  "ZioKernel, Biilly, Jilt, Jaromil, ddennedy",
				  1,1, F0R_COLOR_MODEL_PACKED32);

FaceBl0r::FaceBl0r(int wdt, int hgt) {

  face_rect = 0;
  image = 0;
  tracked_obj = 0;
  face_found = 0;
  
  cascade = 0;
  storage = 0;
  
  classifier = "/usr/share/opencv/haarcascades/haarcascade_frontalface_default.xml";
  register_param(classifier,
                 "Classifier",
                 "Full path to the XML pattern model for recognition; look in /usr/share/opencv/haarcascades");
  ellipse = false;
  register_param(ellipse, "Ellipse", "Draw a red ellipse around the object");
  recheck = 0.025;
  face_notfound = cvRound(recheck * 1000);
  register_param(recheck, "Recheck", "How often to detect an object in number of frames, divided by 1000");
  threads = 0.01; //number of CPUs
  register_param(threads, "Threads", "How many threads to use divided by 100; 0 uses CPU count");
  search_scale = 0.12; // increase size of search window by 20% on each pass
  register_param(search_scale, "Search scale", "The search window scale factor, divided by 10");
  neighbors = 0.02; // require 2 neighbors
  register_param(neighbors, "Neighbors", "Minimum number of rectangles that makes up an object, divided by 100");
  smallest = 0.0; // smallest window size is trained default
  register_param(smallest, "Smallest", "Minimum window size in pixels, divided by 1000");
  largest = 0.0500; // largest object size shown is 500 px
  register_param(largest, "Largest", "Maximum object size in pixels, divided by 10000");
}

FaceBl0r::~FaceBl0r() {
    if(tracked_obj)
        destroy_tracked_object(tracked_obj);

    if(cascade) cvReleaseHaarClassifierCascade(&cascade);
    if(storage) cvReleaseMemStorage(&storage);
    
}

void FaceBl0r::update(double time,
                      uint32_t* out,
                          const uint32_t* in) {

  if (!cascade) {
      cvSetNumThreads(cvRound(threads * 100));
      if (classifier.length() > 0) {
	if (classifier == old_classifier) {
	  // same as before, avoid repeating error messages
	  memcpy(out, in, size * 4); // of course assuming we are RGBA only
	  return;
	} else old_classifier = classifier;

	cascade = (CvHaarClassifierCascade*) cvLoad(classifier.c_str(), 0, 0, 0 );
	if (!cascade) {
	  fprintf(stderr, "ERROR in filter facebl0r, classifier cascade not found:\n");
	  fprintf(stderr, " %s\n", classifier.c_str());
	  memcpy(out, in, size * 4);
	  return;
	}
	storage = cvCreateMemStorage(0);
      }
      else {
	memcpy(out, in, size * 4);
	return;
      }
  }

  // sanitize parameters
  recheck = CLAMP(recheck, 0.001, 1.0);
  search_scale = CLAMP(search_scale, 0.11, 1.0);
  neighbors = CLAMP(neighbors, 0.01, 1.0);

  if( !image )
      image = cvCreateImage( cvSize(width,height), IPL_DEPTH_8U, 4 );

  memcpy(image->imageData, in, size * 4);

  /*
    no face*
     - look for (detect_face)
    yes face
     - track face
     - no more face
       no face*
   */
  if(face_notfound>0) {

      if(face_notfound % cvRound(recheck * 1000) == 0)
          face_rect = detect_face(image, cascade, storage);

      // if no face detected
      if (!face_rect) {
          face_notfound++;
      } else {
          //track detected face with camshift
          if(tracked_obj)
              destroy_tracked_object(tracked_obj);
          tracked_obj = create_tracked_object(image, face_rect);
          face_notfound = 0;
          face_found++;
      }

  }

  if(face_found>0) { 
      //track the face in the new frame
      face_box = camshift_track_face(image, tracked_obj);

      int min = cvRound(smallest * 1000);
          min = min? min : 10;
      int max = cvRound(largest * 10000);
      if( ( face_box.size.width < min )
          || (face_box.size.height < min )
          || (face_box.size.width > max )
          || (face_box.size.height > max )
          ) {
          face_found = 0;
          face_notfound++;
      }
      else {
////////////////////////////////////////////////////////////////////////
	      cvSetImageROI (image, tracked_obj->prev_rect);
//          cvSmooth (image, image, CV_BLUR, 22, 22, 0, 0);
		  cvSmooth (image, image, CV_BLUR, 23, 23, 0, 0);
//          cvSmooth (image, image, CV_GAUSSIAN, 11, 11, 0, 0);
		  cvResetImageROI (image);
////////////////////////////////////////////////////////////////////////
      
          //outline face ellipse
          if (ellipse)
              cvEllipseBox(image, face_box, CV_RGB(255,0,0), 2, CV_AA, 0);

          face_found++;
          if(face_found % cvRound(recheck * 1000) == 0)
              face_notfound = cvRound(recheck * 1000); // try recheck
      }
  }

  memcpy(out, image->imageData, size * 4);
  cvReleaseImage(&image);
}

/* Given an image and a classider, detect and return region. */
CvRect* FaceBl0r::detect_face (IplImage* image,
                               CvHaarClassifierCascade* cascade,
                               CvMemStorage* storage) {
    
  CvRect* rect = 0;
  
  if (cascade && storage) {
     //use an equalized gray image for better recognition
     IplImage* gray = cvCreateImage(cvSize(image->width, image->height), 8, 1);
     cvCvtColor(image, gray, CV_BGR2GRAY);
     cvEqualizeHist(gray, gray);
     cvClearMemStorage(storage);

      //get a sequence of faces in image
      int min = cvRound(smallest * 1000);
      CvSeq *faces = cvHaarDetectObjects(gray, cascade, storage,
         search_scale * 10.0,
         cvRound(neighbors * 100),
         CV_HAAR_FIND_BIGGEST_OBJECT|//since we track only the first, get the biggest
         CV_HAAR_DO_CANNY_PRUNING,  //skip regions unlikely to contain a face
         cvSize(min, min));
    
      //if one or more faces are detected, return the first one
      if(faces && faces->total)
        rect = (CvRect*) cvGetSeqElem(faces, 0);

      cvReleaseImage(&gray);
  }

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
