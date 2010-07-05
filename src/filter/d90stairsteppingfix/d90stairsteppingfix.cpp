/* 
 * Copyright (C) 2010 Simon Andreas Eugster (simon.eu@gmail.com)
 * This file is a Frei0r plugin.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/**
 * Videos recorded with the Nikon D90 and Firmware 1.0 (which might never 
 * be updated) show nasty steps in slanting lines. 
 * 
 * Lee Wilson's post about how to fix this was the first solution I found
 * in the web:
 * http://www.dvxuser.com/V6/showthread.php?t=149663
 * Thank you very much.
 * 
 * Mike Martucci used some C code to remove the steps together with 
 * ffmpeg, to be found here: http://www.epsilonic.com/d90fix/.
 * 
 * The «magic numbers», called slices here, have been developed by 
 * buildyo and cgipro, also in the dvxuser forum.
 * 
 * 
 * This plugin «skips» the scaling step by using linear interpolation
 * on the whole image directly. 
 * 
 * More accurate: 
 * The PROBLEM is that Nikon did not scale the videos directly but 
 * simply used some pixel rows and skipped others, as it seems.
 * In regular distances the number of skipped rows is higher than 
 * usual, creating a gap that leads to steps on sloped lines. 
 * 
 * The SLICES describe rows belonging together, i.e. not having a 
 * big gap between.
 * 
 * The IDEA behind fixing the stair steps is to insert
 * a new line between two slices (there are a total of 82 slices),
 * filling the line by interpolation, and scaling the image
 * (which is now 720+81 pixels high) down to a height of 720 pixels 
 * again. (Only rows are affected, columns are not, mystically.)
 * 
 * 
 * This filling followed by scaling can also be done 
 * WITHOUT SCALING.
 * 
 * Terminology:
 * H original height
 * N number of slice lines (#slices-1)
 * 
 * 1 Create a MESH containing points representing the position of each
 *   row. Initially it is just [0 1 2 3 4 ...].
 *   Length: H
 * 
 * 2 Insert the SLICE LINES into this mesh after each slice. If e.g.
 *   the first slice is of size 3, then we get [0 1 2 2.5 3 4 ...].
 *   The new slice line is located between the 3rd and 4th line
 *   (i.e. line 2 and line 3). 2.5 will be interpolated using 
 *   LINEAR INTERPOLATION:
 *     0.5 * the values of line 2
 *     0.5 * the values of line 3
 *   Analogous would a value of 2.1 mean
 *     0.9 * the value of line 2
 *     0.1 * the value of line 3
 *   Length: H+N
 * 
 * 3 SCALE the mesh BACK to size H again.
 *   The mesh might now look like that:
 *     [0.08 1.25 2.2 2.8 3.75 4.9 ...]
 *   It gets denser around the inserted slice line and looser in the 
 *   middle of the slices.
 *   Length: H
 * 
 * This mesh can be reused now for every frame. What needs to be done
 * for each frame is
 * 4 Interpolate the color values using the mesh values. Continuing 
 *   the example above, this e.g. means for the pixels in the first 
 *   row (line) of the target frame:
 *     For each color, the new color is
 *     0.92 * color of line 0
 *     0.08 * color of line 1
 *   And, for the second row:
 *     For each color, the new color value is
 *     0.75 * color of line 1
 *     0.25 * color of line 2
 *   Note that for a mesh value a.b, 
 *     a      is the lower line number and
 *     (1-b)  is the factor for the lower line.
 * 
 */

#include "frei0r.hpp"

#include <stdio.h>
#include <math.h>

static int slices720p[] = {7,9,9,8,9,9,9,9,9,8,9,9,9,9,8,9,9,9,9,9,8,9,9,9,9,
                    9,8,9,9,9,9,9,8,9,9,9,9,9,8,9,9,9,9,8,9,9,9,9,9,8,
                    9,9,9,9,9,8,9,9,9,9,9,8,9,9,9,9,9,8,9,9,9,9,8,9,9,
                    9,9,9,8,9,9,7};

struct rgbColor
{
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
} test;

class D90StairsteppingFix : public frei0r::filter
{

public:
    D90StairsteppingFix(unsigned int width, unsigned int height)
    {
        m_mesh = new float[height];
        
        
//      printf("Struct size: %d. Pixel size: %d.\n", sizeof(rgbColor), sizeof(uint32_t));
        
        if (height == 720) {
            
            /** Number of newly inserted lines: always between two slices (so #slices-1) */
            int sliceLinesNumber = (sizeof slices720p)/(sizeof slices720p[0]) - 1;
            
            /** The height of the image after inserting the slice lines */
            int newHeight = height + sliceLinesNumber;
            
//          printf("%d slice lines, %d total new lines\n", sliceLinesNumber, newHeight);
            
            /** 
             * The position of each line including slice lines. 
             * Slice lines are inserted between two lines (e.g. between 6 and 7) 
             * and therefore get the number (line1+line2)/2, here 6.5.
             * This positions will later be used for interpolation.
             */
            float filled[newHeight];
            
            int count = 0;
            int index = 0;
            for (int i = 0; i < sliceLinesNumber+1; i++) {
                for (int j = 0; j < slices720p[i]; j++) {
                    
                    filled[index] = count;
                    count++;
                    index++;
                    
                }
                if (count < newHeight) {
                    filled[index] = count - 0.5;
                    index++;
                }
            }
//          for (int i = 0; i < newHeight; i++) {
//            printf("inserted Lines: %f at %d\n", filled[i], i);
//          }
            
            /**
             * Calculate scaling numbers to scale the full height matrix
             * with the slice lines down to the original height (720p).
             */
            float downScaling[height];
            
            float scaleFactor = (float) newHeight/height;
//          printf("scale factor: %f\n", scaleFactor);
            for (int i = 0; i < height; i++) {
                downScaling[i] = (float) (((2*i+1)*scaleFactor)-1)/2;
//              printf("scaled: %f at %d\n", downScaling[i], i);
            }
            
            
            /**
             * Finish the mesh by scaling the H+N sized mesh to size H,
             * using linear interpolation and the previously 
             * calculated scaling numbers.
             */
            float offset;
            for (int i = 0; i < height; i++) {
                
                index = floor(downScaling[i]);
                offset = downScaling[i] - index;
                
                m_mesh[i] = (1-offset)*filled[index] + offset*filled[index+1];
//              printf("%f at %d with weights %f and %f\n", m_mesh[i], i, (1-offset)*downScaling[i], offset*downScaling[i+1]);
            }
            
        } else {
            // Not a 720p file.
        }
    }
    
    ~D90StairsteppingFix()
    {
        delete m_mesh;
    }
    
    struct rgbColor toRGB(uint32_t input)
    {
        struct rgbColor rgbCol;
        rgbCol.r = (input & 0x000000FF) >> 0;
        rgbCol.g = (input & 0x0000FF00) >> 8;
        rgbCol.b = (input & 0x00FF0000) >> 16;
        rgbCol.a = (input & 0xFF000000) >> 24;
        return rgbCol;
    }
    uint32_t toUint(struct rgbColor rgbCol)
    {
        uint32_t pixel = 0;
        pixel = pixel | (rgbCol.r << 0);
        pixel = pixel | (rgbCol.g << 8);
        pixel = pixel | (rgbCol.b << 16);
        pixel = pixel | (rgbCol.a << 24);
        return pixel;
    }

    virtual void update()
    {
        // This does not need to work, it just may. 
        // So use the toRGB/toUint from above.
        /*struct rgbColor* col = (struct rgbColor*) &in[0];*/
        /*struct rgbColor col = *(struct rgbColor*) &in[0];*/
        
        struct rgbColor col, colA, colB;
        
//      col = toRGB(in[0]);
//      printf("Color value of first pixel: r=%d g=%d b=%d a=%d.\nuint value: %d.\n", col.r, col.g, col.b, col.a, in[0]);

        if (height == 720) {
//          printf("Converting.\n");

            float factor;
            int index;
            unsigned char r,g,b,a;
            
            for (int line = 0; line < height; line++) {
                index = floor(m_mesh[line]);
                factor = (float) m_mesh[line] - index;
                
                for (int pixel = 0; pixel < width; pixel++) {
                    // Use linear interpolation on the colours
                
                    colA = toRGB(in[width*index+pixel]);
                    colB = toRGB(in[width*(index+1) + pixel]);
                    
                    col.r = floor((float)(1-factor)*colA.r + (factor)*colB.r);
                    col.g = floor((float)(1-factor)*colA.g + (factor)*colB.g);
                    col.b = floor((float)(1-factor)*colA.b + (factor)*colB.b);
                    col.a = floor((float)(1-factor)*colA.a + (factor)*colB.a);
                    
                    out[width*line+pixel] = toUint(col);
                }
            }
            std::copy(in + width*(height-1), in+width*height, out + width*(height-1));
        } else {
//          printf("Just copying. Height is %d.\n", height);
            std::copy(in, in + width*height, out);
        }
    }
    
private:
    float *m_mesh;

};



frei0r::construct<D90StairsteppingFix> plugin("Nikon D90 Stairstepping fix",
                "Removes the Stairstepping from Nikon D90 videos by interpolation",
                "Simon A. Eugster (Granjow)",
                0,1,
                F0R_COLOR_MODEL_RGBA8888);
