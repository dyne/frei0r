/**
 * binarymillenium 2007
 *
 * This code is released under the GPL
 *
 * *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>


#include "cv.h"
//#include "highgui.h"

#include "frei0r.h"

#ifdef _EiC
#define WIN32
#endif


CvSeq* detect_and_draw( IplImage* img, CvMemStorage* storage,
                        CvHaarClassifierCascade* cascade);

#ifndef OPENCV_PREFIX
#error OPENCV_PREFIX must contain the installation prefix of OpenCV
#endif

#define STR(x) #x
#define TOSTR(x) STR(x)

static const char* const cascade_name =
   TOSTR(OPENCV_PREFIX)"/share/opencv/haarcascades/haarcascade_frontalface_alt2.xml";
/*    "haarcascade_frontalface_alt.xml";*/
/*    "haarcascade_profileface.xml";*/


typedef struct facedetect_instance{

  IplImage *frame, *frame_copy;

  int width;
  int height;

  CvMemStorage* storage;
  CvHaarClassifierCascade* cascade;

} facedetect_instance_t;

int f0r_init()
{
  return 1;
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  facedetect_instance_t* inst =
    (facedetect_instance_t*)malloc(sizeof(facedetect_instance_t));

  inst->width = width;
  inst->height = height;

  /// tbd - put this in init instead?
  inst->storage = 0;
  inst->cascade = 0;

  inst->frame = 0;
  inst->frame_copy = 0;

  inst->cascade = (CvHaarClassifierCascade*)cvLoad( cascade_name, 0, 0, 0 );
    
  if( !inst->cascade )
    {
      fprintf(stderr, "ERROR: Could not load classifier cascade %s\n",
              cascade_name);
      free(inst);
      return (f0r_instance_t)0;
    }
  else
    {
      inst->storage = cvCreateMemStorage(0);

      //cvNamedWindow( "result", 1 );

      return (f0r_instance_t)inst;
    }
}


void f0r_deinit()
{
}

void f0r_destruct(f0r_instance_t instance)
{
  free(instance);
  //cvDestroyWindow("result");
}

void f0r_set_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  facedetect_instance_t* inst = (facedetect_instance_t*)instance;

}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  facedetect_instance_t* inst = (facedetect_instance_t*)instance;

}

void f0r_get_plugin_info(f0r_plugin_info_t* facedetectInfo)
{
  facedetectInfo->name = "opencvfacedetect";
  facedetectInfo->author = "binarymillenium";
  facedetectInfo->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  facedetectInfo->color_model = F0R_COLOR_MODEL_BGRA8888;
  facedetectInfo->frei0r_version = FREI0R_MAJOR_VERSION;
  facedetectInfo->major_version = 0;
  facedetectInfo->minor_version = 1;
  facedetectInfo->num_params =  1;
  facedetectInfo->explanation = "detect faces";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
    {
    case 0:
      info->name = "test";
      info->type = F0R_PARAM_DOUBLE;
      info->explanation = "test";
      break;
    }

}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);

  facedetect_instance_t* inst = (facedetect_instance_t*)instance;

  unsigned char* dst = (unsigned char*)outframe;
  const unsigned char* src = (unsigned char*)inframe;


  if( !inst->frame_copy )
    inst->frame_copy = cvCreateImage( cvSize(inst->width,inst->height),
                                      IPL_DEPTH_8U, 4 );

  unsigned char* ipli = (unsigned char*)inst->frame_copy->imageData;
  int step = inst->frame_copy->widthStep;
  unsigned i, j;
  for (i = 0; (i < inst->height); i++) {
    for (j = 0; (j < inst->width); j++) {
      ipli[i*step+j*4+2] = src[2];
      ipli[i*step+j*4+1] = src[1];
      ipli[i*step+j*4+0] = src[0];

      //ipli += 4;
      src += 4;

    }

  }

  CvSeq* faces = detect_and_draw( inst->frame_copy,
                                  inst->storage,
                                  inst->cascade );

  ipli = (unsigned char*)inst->frame_copy->imageData;

  for (i = 0; (i < inst->height); i++) {
    for (j = 0; (j < inst->width); j++) {
      dst[2] = ipli[2];
      dst[1] = ipli[1];
      dst[0] = ipli[0];

      ipli += 4;
      dst += 4;
    }
  }

  cvReleaseImage( &(inst->frame_copy) );

}

CvSeq* detect_and_draw( IplImage* img, CvMemStorage* storage,
                        CvHaarClassifierCascade* cascade)
{
  static CvScalar colors[] =
    {
      {{255,255,255}},
      {{0,128,255}},
      {{0,255,255}},
      {{0,255,0}},
      {{255,128,0}},
      {{255,255,0}},
      {{255,0,0}},
      {{255,0,255}},
      {{0,0,0}}
    };

  double scale = 1.3;
  IplImage* gray = cvCreateImage( cvSize(img->width,img->height), 8, 1 );
  IplImage* small_img = cvCreateImage( cvSize( cvRound (img->width/scale),
                                               cvRound (img->height/scale)),
                                       8, 1 );
  int i;

  cvCvtColor( img, gray, CV_BGR2GRAY );
  cvResize( gray, small_img, CV_INTER_LINEAR );
  cvEqualizeHist( small_img, small_img );
  //cvClearMemStorage( storage );

  CvSeq* faces = 0;

  if( cascade )
    {
      double t = (double)cvGetTickCount();
      faces = cvHaarDetectObjects( small_img, cascade, storage,
                                   1.1, 2, 0/*CV_HAAR_DO_CANNY_PRUNING*/,
                                   cvSize(30, 30) );
      t = (double)cvGetTickCount() - t;
      //printf( "detection time = %gms\n", t/((double)cvGetTickFrequency()*1000.) );

      CvPoint pt1, pt2;
      pt1.x = 0;
      pt1.y = 0;
      pt2.x = img->width;
      pt2.y = img->height;
      cvRectangle( img, pt1, pt2, colors[8],CV_FILLED, 8, 0 );

      for( i = 0; i < (faces ? faces->total : 0); i++ )
        {
          CvRect* r = (CvRect*)cvGetSeqElem( faces, i );
          CvPoint center;
          int radius;
          center.x = cvRound((r->x + r->width*0.5)*scale);
          center.y = cvRound((r->y + r->height*0.5)*scale);
          radius = cvRound((r->width + r->height)*0.25*scale);

          pt1.x = r->x;// - r->width*0.5; 
          pt1.y = r->y;// - r->height*0.5; 
          pt2.x = r->x + r->width;
          pt2.y = r->y + r->height;
          //printf( " faces %d %d \n",  center.x, center.y);
          cvCircle( img, center, radius, colors[i%8],CV_FILLED, 8, 0); // 3, 8, 0 );
          //cvRectangle( img, pt1, pt2, colors[i%8], CV_FILLED );
        }
    }

  //cvShowImage( "result", img );
  cvReleaseImage( &gray );
  cvReleaseImage( &small_img );

  return faces;
}
