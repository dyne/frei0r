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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <opencv2/opencv.hpp>
#include "frei0r.hpp"
#include "frei0r/math.h"

#define USE_ROI
#define PAD (40)

class FaceDetect;
frei0r::construct<FaceDetect> plugin("opencvfacedetect",
				  "detect faces and draw shapes on them",
				  "binarymillenium, ddennedy",
				  2,0, F0R_COLOR_MODEL_PACKED32);

class FaceDetect: public frei0r::filter
{

private:
    cv::Mat image;
    unsigned count;
    std::vector<cv::Rect> objects;
    cv::Rect roi;
    cv::CascadeClassifier cascade;

    // plugin parameters
    std::string classifier;
    double shape;
    double recheck;
    double threads;
    double search_scale;
    double neighbors;
    double smallest;
    double scale;
    double stroke;
    bool   antialias;
    double alpha;
    f0r_param_color  color[5];

    std::string old_classifier;

public:
    FaceDetect(int width, int height)
        : count(0)
    {
        roi.width = roi.height = 0;
        roi.x = roi.y = 0;
        classifier = "/usr/share/opencv/haarcascades/haarcascade_frontalface_default.xml";
        register_param(classifier,
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
        stroke = 0.0;
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
        register_param(color[1], "Color 2", "The color of the second object");
        f0r_param_color color2 = {0.0, 1.0, 1.0};
        color[2] = color2;
        register_param(color[2], "Color 3", "The color of the third object");
        f0r_param_color color3 = {0.0, 1.0, 0.0};
        color[3] = color3;
        register_param(color[3], "Color 4", "The color of the fourth object");
        f0r_param_color color4 = {1.0, 0.5, 0.0};
        color[4] = color4;
        register_param(color[4], "Color 5", "The color of the fifth object");
        srand(::time(NULL));
    }

    ~FaceDetect()
    {
    }

    void update(double time,
                uint32_t* out,
                const uint32_t* in)
    {
        if (cascade.empty()) {
            cv::setNumThreads(cvRound(threads * 100));
            if (classifier.length() > 0 && classifier != old_classifier) {
                if (!cascade.load(classifier.c_str()))
                    fprintf(stderr, "ERROR: Could not load classifier cascade %s\n", classifier.c_str());
		old_classifier = classifier;
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
        image = cv::Mat(height, width, CV_8UC4, (void*)in);

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
            if (objects.size() > 0) // reset the list of objects
                objects.clear();
            double elapsed = (double) cv::getTickCount();

            objects = detect();

            // use detection time to throttle frequency of re-detect vs. redraw (automatic recheck)
            elapsed = cv::getTickCount() - elapsed;
            elapsed = elapsed / ((double) cv::getTickFrequency() * 1000.0);

            // Automatic recheck uses an undocumented negative parameter value,
            // which is not compliant, but technically feasible.
            if (recheck < 0 && cvRound( elapsed / (1000.0 / (recheckInt + 1)) ) <= recheckInt)
                    count += recheckInt - cvRound( elapsed / (1000.0 / (recheckInt + 1)));
//            fprintf(stderr, "detection time = %gms counter %u\n", elapsed, count);
        }
        
        draw();

        // copy filtered OpenCV image to output
        memcpy(out, image.data, size * 4);
    }
    
private:
    std::vector<cv::Rect> detect()
    {
        std::vector<cv::Rect> faces;
        if (cascade.empty()) return faces;
        double scale = this->scale == 0? 1.0 : this->scale;
        cv::Mat image_roi = image;
        cv::Mat gray, small;
        int min = cvRound(smallest * 1000. * scale);
        
        // use a region of interest to improve performance
        // This idea comes from the More than Technical blog:
        // http://www.morethantechnical.com/2009/08/09/near-realtime-face-detection-on-the-iphone-w-opencv-port-wcodevideo/
        if ( roi.width > 0 && roi.height > 0)
        {
            image_roi = image(roi);
        }

        // use an equalized grayscale to improve detection
        cv::cvtColor(image_roi, gray, cv::COLOR_BGR2GRAY);

        // use a smaller image to improve performance
        cv::resize(gray, small, cv::Size(cvRound(gray.cols * scale), cvRound(gray.rows * scale)));
        cv::equalizeHist(small, small);
        
        // detect with OpenCV
        cascade.detectMultiScale(small, faces, 1.1, 2, 0, cv::Size(min, min));
        
#ifdef USE_ROI
        if (faces.size() == 0)
        {
            // clear the region of interest
            roi.width = roi.height = 0;
            roi.x = roi.y = 0;
        }
        else if (faces.size() > 0)
        {
            // determine the region of interest from the detected objects
            int minx = width * scale;
            int miny = height * scale;
            int maxx, maxy = 0;
            for (size_t i = 0; i < faces.size(); i++)
            {
                faces[i].x+= roi.x * scale;
                faces[i].y+= roi.y * scale;
                minx = MIN(faces[i].x, minx);
                miny = MIN(faces[i].y, miny);
                maxx = MAX(faces[i].x + faces[i].width, maxx);
                maxy= MAX(faces[i].y + faces[i].height, maxy);
            }
            minx= MAX(minx - PAD, 0);
            miny= MAX(miny - PAD, 0);
            maxx = MIN(maxx + PAD, width * scale);
            maxy = MIN(maxy + PAD, height * scale);

            // store the region of interest
            roi.x = minx / scale;
            roi.y = miny / scale;
            roi.width = (maxx - minx) / scale;
            roi.height = (maxy - miny) / scale; 
        }
#endif
        return faces;
    }
    
    void draw()
    {
        double scale = this->scale == 0? 1.0 : this->scale;
        cv::Scalar colors[5] = {
            cv::Scalar(cvRound(color[0].r * 255), cvRound(color[0].g * 255), cvRound(color[0].b * 255), cvRound(alpha * 255)),
            cv::Scalar(cvRound(color[1].r * 255), cvRound(color[1].g * 255), cvRound(color[1].b * 255), cvRound(alpha * 255)),
            cv::Scalar(cvRound(color[2].r * 255), cvRound(color[2].g * 255), cvRound(color[2].b * 255), cvRound(alpha * 255)),
            cv::Scalar(cvRound(color[3].r * 255), cvRound(color[3].g * 255), cvRound(color[3].b * 255), cvRound(alpha * 255)),
            cv::Scalar(cvRound(color[4].r * 255), cvRound(color[4].g * 255), cvRound(color[4].b * 255), cvRound(alpha * 255)),
        }; 
        
        for (size_t i = 0; i < objects.size(); i++)
        {
            cv::Rect* r = (cv::Rect*) &objects[i];
            cv::Point center;
            int thickness = stroke <= 0? cv::FILLED : cvRound(stroke * 100);
            int linetype = antialias? cv::LINE_AA : 8;
            
            center.x = cvRound((r->x + r->width * 0.5) / scale);
            center.y = cvRound((r->y + r->height * 0.5) / scale);
            
            switch (shape == 1.0? (rand() % 3) : cvRound(shape * 10))
            {
            default:
            case 0:
                {
                    int radius = cvRound((r->width + r->height) * 0.25 / scale);
                    cv::circle(image, center, radius, colors[i % 5], thickness, linetype);
                    break;
                }
            case 1:
                {
                    cv::ellipse(image, center, cv::Size(r->width / scale, (r->height / scale) * 1.2), 90, 0, 360, colors[i % 5], thickness, linetype);
                    break;
                }
            case 2:
                {
                    cv::Point pt1 = cv::Point(r->x / scale, r->y / scale);
                    cv::Point pt2 = cv::Point((r->x + r->width) / scale, (r->y + r->height) / scale);
                    cv::rectangle(image, pt1, pt2, colors[i % 5], thickness, linetype);
                    break;
                }
            }
        }
    }
};
