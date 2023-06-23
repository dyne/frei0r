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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <opencv2/opencv.hpp>
#include "frei0r.hpp"
#include "frei0r_math.h"

class TrackedObj {
public:
  void update_hist();
  void update_hue_image(const cv::Mat& image);
  cv::RotatedRect camshift_track_face();

  cv::Mat hsv;                      //input image converted to HSV
  cv::Mat hue;                      //hue channel of HSV image
  cv::Mat mask;                     //image for masking pixels
  cv::Mat prob;                     //face probability estimates for each pixel

  cv::Mat hist;                     //histogram of hue in original face image
  static const int hist_bins;       //number of histogram bins
  static const float hist_range[2]; //histogram range

  cv::Rect prev_rect;               //location of face in previous frame
  cv::RotatedRect curr_box;         //current face location estimate
};

const float TrackedObj::hist_range[2] = { 0, 180 };
const int   TrackedObj::hist_bins     = 30;

class FaceBl0r : public frei0r::filter {
public:
  FaceBl0r(int wdt, int hgt);
  ~FaceBl0r() = default;

  void update(double time,
              uint32_t *out,
              const uint32_t *in);

private:

//trackface
  std::vector <cv::Rect> detect_face();

  TrackedObj tracked_obj;

//used by capture_video_frame, so we don't have to keep creating.
  cv::Mat image;

  cv::CascadeClassifier cascade;

  // plugin parameters
  std::string classifier;
  bool ellipse;
  double recheck;
  double threads;
  double search_scale;
  double neighbors;
  double smallest;
  double largest;

  std::string old_classifier;

  unsigned int face_found;
  unsigned int face_notfound;
};


frei0r::construct <FaceBl0r> plugin("FaceBl0r",
                                    "automatic face blur",
                                    "ZioKernel, Biilly, Jilt, Jaromil, ddennedy",
                                    1, 1, F0R_COLOR_MODEL_BGRA8888);

FaceBl0r::FaceBl0r(int wdt, int hgt)
{
  face_found = 0;

  classifier = "/usr/share/opencv/haarcascades/haarcascade_frontalface_default.xml";
  register_param(classifier,
                 "Classifier",
                 "Full path to the XML pattern model for recognition; look in /usr/share/opencv/haarcascades");
  ellipse = false;
  register_param(ellipse, "Ellipse", "Draw a red ellipse around the object");
  recheck       = 0.025;
  face_notfound = cvRound(recheck * 1000);
  register_param(recheck, "Recheck", "How often to detect an object in number of frames, divided by 1000");
  threads = 0.01;      //number of CPUs
  register_param(threads, "Threads", "How many threads to use divided by 100; 0 uses CPU count");
  search_scale = 0.12; // increase size of search window by 20% on each pass
  register_param(search_scale, "Search scale", "The search window scale factor, divided by 10");
  neighbors = 0.02;    // require 2 neighbors
  register_param(neighbors, "Neighbors", "Minimum number of rectangles that makes up an object, divided by 100");
  smallest = 0.0;      // smallest window size is trained default
  register_param(smallest, "Smallest", "Minimum window size in pixels, divided by 1000");
  largest = 0.0500;    // largest object size shown is 500 px
  register_param(largest, "Largest", "Maximum object size in pixels, divided by 10000");
}

void FaceBl0r::update(double time,
                      uint32_t *out,
                      const uint32_t *in)
{
  if (cascade.empty())
  {
    cv::setNumThreads(cvRound(threads * 100));
    if (classifier.length() == 0 || classifier == old_classifier)
    {
      // same as before, avoid repeating error messages
      memcpy(out, in, size * 4);       // of course assuming we are RGBA only
      return;
    }
    old_classifier = classifier;
  }

  if (!cascade.load(classifier.c_str()))
  {
    fprintf(stderr, "ERROR in filter facebl0r, classifier cascade not found:\n");
    fprintf(stderr, " %s\n", classifier.c_str());
    memcpy(out, in, size * 4);
    return;
  }

  // sanitize parameters
  recheck      = CLAMP(recheck, 0.001, 1.0);
  search_scale = CLAMP(search_scale, 0.11, 1.0);
  neighbors    = CLAMP(neighbors, 0.01, 1.0);

  // copy input image to OpenCV
  image = cv::Mat(height, width, CV_8UC4, (void *)in);
  tracked_obj.update_hue_image(image);

  /*
   * no face*
   * - look for (detect_face)
   * yes face
   * - track face
   * - no more face
   *   no face*
   */
  if (face_notfound > 0)
  {
    std::vector <cv::Rect> faces;
    if (face_notfound % cvRound(recheck * 1000) == 0)
    {
      faces = detect_face();
    }

    // if no face detected
    if (faces.empty())
    {
      face_notfound++;
    }
    else
    {
      tracked_obj.prev_rect = faces[0];
      tracked_obj.update_hist();
      face_notfound = 0;
      face_found++;
    }
  }

  if (face_found > 0)
  {
    //track the face in the new frame
    cv::RotatedRect face_box = tracked_obj.camshift_track_face();

    int min = cvRound(smallest * 1000);
    min = min? min : 10;
    int max = cvRound(largest * 10000);
    if ((face_box.size.width < min) ||
        (face_box.size.height < min) ||
        (face_box.size.width > max) ||
        (face_box.size.height > max)
        )
    {
      face_found = 0;
      face_notfound++;
    }
    else
    {
      cv::Rect blur_region = tracked_obj.prev_rect & cv::Rect({ 0, 0 }, image.size());
      cv::Mat  blur(image, blur_region);
      cv::blur(blur, blur, { 23, 23 }, cv::Point(-1, -1));

      //outline face ellipse
      if (ellipse)
      {
        cv::ellipse(image, face_box, CV_RGB(255, 0, 0), 2, cv::LINE_AA);
      }

      face_found++;
      if (face_found % cvRound(recheck * 1000) == 0)
      {
        face_notfound = cvRound(recheck * 1000);       // try recheck
      }
    }
  }

  memcpy(out, image.data, size * 4);
}

/* Given an image and a classider, detect and return region. */
std::vector <cv::Rect> FaceBl0r::detect_face()
{
  if (cascade.empty())
  {
    return (std::vector <cv::Rect>());
  }

  //use an equalized gray image for better recognition
  cv::Mat gray;
  cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
  cv::equalizeHist(gray, gray);

  //get a sequence of faces in image
  int min = cvRound(smallest * 1000);
  std::vector <cv::Rect> faces;
  cascade.detectMultiScale(gray, faces,
                           search_scale * 10.0,
                           cvRound(neighbors * 100),
                           cv::CASCADE_FIND_BIGGEST_OBJECT | //since we track
                                                             // only the first,
                                                             // get the biggest
                           cv::CASCADE_DO_CANNY_PRUNING,     //skip regions
                                                             // unlikely to
                                                             // contain a face
                           cv::Size(min, min));

  return (faces);
}

void TrackedObj::update_hist()
{
  //create a histogram representation for the face
  cv::Mat hue_roi(hue, prev_rect);
  cv::Mat mask_roi(mask, prev_rect);

  const float *range = hist_range;

  cv::calcHist(&hue_roi, 1, nullptr, mask_roi, hist, 1, &hist_bins, &range);
  normalize(hist, hist, 0, 255, cv::NORM_MINMAX);
}

/* Given an image and tracked object, return box position. */
cv::RotatedRect TrackedObj::camshift_track_face()
{
  //create a probability image based on the face histogram
  const float *range = hist_range;

  cv::calcBackProject(&hue, 1, nullptr, hist, prob, &range);
  prob &= mask;

  //use CamShift to find the center of the new face probability
  cv::RotatedRect curr_box = CamShift(prob, prev_rect,
                                      cv::TermCriteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 10, 1));

  //update face location
  prev_rect = curr_box.boundingRect();

  return (curr_box);
}

void TrackedObj::update_hue_image(const cv::Mat& image)
{
  //limits for calculating hue
  int vmin = 65, vmax = 256, smin = 55;

  //convert to HSV color model
  cv::cvtColor(image, hsv, cv::COLOR_BGR2HSV);

  //mask out-of-range values
  cv::inRange(hsv,                                   //source
              cv::Scalar(0, smin, MIN(vmin, vmax)),  //lower bound
              cv::Scalar(180, 256, MAX(vmin, vmax)), //upper bound
              mask);                                 //destination

  //extract the hue channel, split: src, dest channels
  cv::extractChannel(hsv, hue, 0);
}
