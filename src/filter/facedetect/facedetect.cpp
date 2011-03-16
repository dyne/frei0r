/**
 * Copyright (C) 2007 binarymillenium
 * Copyright (C) 2011 Dan Dennedy <dan@dennedy.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>
#include <opencv/cv.h>
#include "frei0r.hpp"
#include "frei0r_math.h"

#define USE_ROI
#define PAD (40)

#define FACEBL0R_PARAM_CLASSIFIER (0)

class FaceDetect;
frei0r::construct<FaceDetect> plugin("opencvfacedetect",
				  "detect faces and draw shapes on them",
				  "binarymillenium, ddennedy",
				  2,0, F0R_COLOR_MODEL_PACKED32);

class FaceDetect: public frei0r::filter
{
private:
    IplImage *image;

    unsigned count;
    CvSeq *objects;
    CvRect roi;
    CvMemStorage *storage;
    CvHaarClassifierCascade *cascade;

    // plugin parameters
    f0r_param_double shape;
    f0r_param_double recheck;
    f0r_param_double threads;
    f0r_param_double search_scale;
    f0r_param_double neighbors;
    f0r_param_double smallest;
    f0r_param_double scale;
    f0r_param_double stroke;
    f0r_param_bool   antialias;
    f0r_param_double alpha;
    f0r_param_color  color[5];
	
public:
    FaceDetect(int width, int height)
        : image(0)
        , count(0)
        , objects(0)
        , storage(0)
        , cascade(0)
    {
        roi.width = roi.height = 0;
        register_param("/usr/share/opencv/haarcascades/haarcascade_frontalface_default.xml",
                       "Classifier",
                       "Full path to the XML pattern model for recognition; look in /usr/share/opencv/haarcascades"); 
        threads = 0.01; //number of CPUs
        register_param(threads, "Threads", "How many threads to use divided by 100; 0 uses CPU count");
        shape = 0.0;
        register_param(shape, "Shape", "The shape to draw: 0=circle, 0.1=ellipse, 0.2=rectangle, 1=random");
        recheck = 0.025;
        register_param(recheck, "Recheck", "How often to detect an object in number of frames, divided by 1000");
        search_scale = 0.12; // increase size of search window by 20% on each pass
        register_param(search_scale, "Search scale", "The search window scale factor, divided by 10");
        neighbors = 0.02; // require 2 neighbors
        register_param(neighbors, "Neighbors", "Minimum number of rectangles that makes up an object, divided by 100");
        smallest = 0.0; // smallest window size is trained default
        register_param(smallest, "Smallest", "Minimum window size in pixels, divided by 1000");
        scale = 1.0 / 1.5;
        register_param(scale, "Scale", "Down scale the image prior detection");
        stroke = CV_FILLED;
        register_param(stroke, "Stroke", "Line width, divided by 100, or fill if 0");
        antialias = false;
        register_param(antialias, "Antialias", "Draw with antialiasing");
        alpha = 1.0;
        register_param(alpha, "Alpha", "The alpha channel value for the shapes");
        f0r_param_color color0 = {1.0, 1.0, 1.0};
        color[0] = color0;
        register_param(color[0], "Color 1", "The color of the first object");
        f0r_param_color color1 = {0.0, 0.5, 1.0};
        color[1] = color1;
        register_param(color[0], "Color 2", "The color of the second object");
        f0r_param_color color2 = {0.0, 1.0, 1.0};
        color[2] = color2;
        register_param(color[0], "Color 3", "The color of the third object");
        f0r_param_color color3 = {0.0, 1.0, 0.0};
        color[3] = color3;
        register_param(color[0], "Color 4", "The color of the fourth object");
        f0r_param_color color4 = {1.0, 0.5, 0.0};
        color[4] = color4;
        register_param(color[0], "Color 5", "The color of the fifth object");
        srand(::time(NULL));
    }

    ~FaceDetect()
    {
        if (cascade) cvReleaseHaarClassifierCascade(&cascade);
        if (storage) cvReleaseMemStorage(&storage);        
    }

    void update()
    {
        if (!cascade) {
            cvSetNumThreads(cvRound(threads * 100));
            f0r_param_string classifier;
            get_param_value(&classifier, FACEBL0R_PARAM_CLASSIFIER);
            if (classifier && strcmp(classifier, "")) {
                cascade = (CvHaarClassifierCascade*) cvLoad(classifier, 0, 0, 0 );
                if (!cascade)
                    fprintf(stderr, "ERROR: Could not load classifier cascade %s\n", classifier);
                storage = cvCreateMemStorage(0);
            }
            else {
                memcpy(out, in, size * 4);
                return;
            }
        }

        // sanitize parameters
        search_scale = CLAMP(search_scale, 0.11, 1.0);
        neighbors = CLAMP(neighbors, 0.01, 1.0);

        // copy input image to OpenCV
        if( !image )
            image = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 4);
        memcpy(image->imageData, in, size * 4);
        
        // only re-detect periodically to control performance and reduce shape jitter
        int recheckInt = abs(cvRound(recheck * 1000));
        if ( recheckInt > 0 && count % recheckInt )
        {
            // skip detect
            count++;
//            fprintf(stderr, "draw-only counter %u\n", count);
        }
        else
        {
            count = 1;   // reset the recheck counter
            if (objects) // reset the list of objects
                cvClearSeq(objects);
            
            double elapsed = (double) cvGetTickCount();

            objects = detect();

            // use detection time to throttle frequency of re-detect vs. redraw (automatic recheck)
            elapsed = cvGetTickCount() - elapsed;
            elapsed = elapsed / ((double) cvGetTickFrequency() * 1000.0);

            // Automatic recheck uses an undocumented negative parameter value,
            // which is not compliant, but technically feasible.
            if (recheck < 0 && cvRound( elapsed / (1000.0 / (recheckInt + 1)) ) <= recheckInt)
                    count += recheckInt - cvRound( elapsed / (1000.0 / (recheckInt + 1)));
//            fprintf(stderr, "detection time = %gms counter %u\n", elapsed, count);
        }
        
        draw();
        
        // copy filtered OpenCV image to output
        memcpy(out, image->imageData, size * 4);
        cvReleaseImage(&image);
    }
    
private:
    CvSeq* detect()
    {
        if (!cascade) return 0;
        double scale = this->scale == 0? 1.0 : this->scale;
        IplImage* gray = cvCreateImage(cvSize(width, height ), 8, 1);
        IplImage* small = cvCreateImage(cvSize(cvRound(width * scale), cvRound(height * scale)), 8, 1);
        int min = cvRound(smallest * 1000);            
        CvSeq* faces = 0;
        
        // use a region of interest to improve performance
        // This idea comes from the More than Technical blog:
        // http://www.morethantechnical.com/2009/08/09/near-realtime-face-detection-on-the-iphone-w-opencv-port-wcodevideo/
        if ( roi.width > 0 && roi.height > 0)
        {
            cvSetImageROI(small, roi);
            CvRect scaled_roi = cvRect(roi.x / scale, roi.y / scale,
                                       roi.width / scale, roi.height / scale);
            cvSetImageROI(image, scaled_roi);
            cvSetImageROI(gray, scaled_roi);
        }
        
        // use an equalized grayscale to improve detection
        cvCvtColor(image, gray, CV_BGR2GRAY);
        // use a smaller image to improve performance
        cvResize(gray, small, CV_INTER_LINEAR);
        cvEqualizeHist(small, small);
        
        // detect with OpenCV
        cvClearMemStorage(storage);
        faces = cvHaarDetectObjects(small, cascade, storage,
                                    search_scale * 10.0,
                                    cvRound(neighbors * 100),
                                    CV_HAAR_DO_CANNY_PRUNING,
                                    cvSize(min, min));
        
#ifdef USE_ROI
        if (!faces || faces->total == 0)
        {
            // clear the region of interest
            roi.width = roi.height = 0;
        }
        else if (faces && faces->total > 0)
        {
            // determine the region of interest from the first detected object
            // XXX: based on the first object only?
            CvRect* r = (CvRect*) cvGetSeqElem(faces, 0);
            
            if (roi.width > 0 && roi.height > 0)
            {
                r->x += roi.x;
                r->y += roi.y;
            }
            int startX = MAX(r->x - PAD, 0);
            int startY = MAX(r->y - PAD, 0);
            int w = small->width - startX - r->width - PAD * 2;
            int h = small->height - startY - r->height - PAD * 2;
            int sw = r->x - PAD, sh = r->y - PAD;
            
            // store the region of interest
            roi.x = startX;
            roi.y = startY,
            roi.width = r->width + PAD * 2 + ((w < 0) ? w : 0) + ((sw < 0) ? sw : 0);
            roi.height = r->height + PAD * 2 + ((h < 0) ? h : 0) + ((sh < 0) ? sh : 0); 
        }
#endif
        cvReleaseImage(&gray);
        cvReleaseImage(&small);
        cvResetImageROI(image);
        return faces;
    }
    
    void draw()
    {
        double scale = this->scale == 0? 1.0 : this->scale;
        CvScalar colors[5] = {
            {{cvRound(color[0].r * 255), cvRound(color[0].g * 255), cvRound(color[0].b * 255), cvRound(alpha * 255)}},
            {{cvRound(color[1].r * 255), cvRound(color[1].g * 255), cvRound(color[1].b * 255), cvRound(alpha * 255)}},
            {{cvRound(color[2].r * 255), cvRound(color[2].g * 255), cvRound(color[2].b * 255), cvRound(alpha * 255)}},
            {{cvRound(color[3].r * 255), cvRound(color[3].g * 255), cvRound(color[3].b * 255), cvRound(alpha * 255)}},
            {{cvRound(color[4].r * 255), cvRound(color[4].g * 255), cvRound(color[4].b * 255), cvRound(alpha * 255)}},
        };
        
        for (int i = 0; i < (objects ? objects->total : 0); i++)
        {
            CvRect* r = (CvRect*) cvGetSeqElem(objects, i);
            CvPoint center;
            int thickness = stroke <= 0? CV_FILLED : cvRound(stroke * 100);
            int linetype = antialias? CV_AA : 8;
            
            center.x = cvRound((r->x + r->width * 0.5) / scale);
            center.y = cvRound((r->y + r->height * 0.5) / scale);
            
            switch (shape == 1.0? (rand() % 3) : cvRound(shape * 10))
            {
            default:
            case 0:
                {
                    int radius = cvRound((r->width + r->height) * 0.25 / scale);
                    cvCircle(image, center, radius, colors[i % 5], thickness, linetype);
                    break;
                }
            case 1:
                {
                    CvBox2D box = {{center.x, center.y}, {r->width / scale, (r->height / scale) * 1.2}, 90};
                    cvEllipseBox(image, box, colors[i % 5], thickness, linetype);
                    break;
                }
            case 2:
                {
                    CvPoint pt1 = {r->x / scale, r->y / scale};
                    CvPoint pt2 = {(r->x + r->width) / scale, (r->y + r->height) / scale};
                    cvRectangle(image, pt1, pt2, colors[i % 5], thickness, linetype);
                    break;
                }
            }
        }
    }
};
