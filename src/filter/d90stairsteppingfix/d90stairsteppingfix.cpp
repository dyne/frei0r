#include "frei0r.hpp"

#include <stdio.h>
#include <math.h>

static int slices[] = {7,9,9,8,9,9,9,9,9,8,9,9,9,9,8,9,9,9,9,9,8,9,9,9,9,
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
        m_interpolationValues = new float[height];
        
        
        //printf("Struct size: %d. Pixel size: %d.\n", sizeof(rgbColor), sizeof(uint32_t));
        
        if (height == 720) {
            
            /** Number of newly inserted lines: always between two slices (so #slices-1) */
            int sliceLinesNumber = (sizeof slices)/(sizeof slices[0]) - 1;
            
            /** The height of the image after inserting the slice lines */
            int newHeight = height + sliceLinesNumber;
            
            printf("%d slice lines, %d total new lines\n", sliceLinesNumber, newHeight);
            
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
                for (int j = 0; j < slices[i]; j++) {
                    
                    filled[index] = count;
                    count++;
                    index++;
                    
                }
                if (count < newHeight) {
                    filled[index] = count - 0.5;
                    index++;
                }
            }
            for (int i = 0; i < newHeight; i++) {
                printf("inserted Lines: %f at %d\n", filled[i], i);
            }
            
            /**
             * Scaling numbers to scale the full height matrix with
             * the slice lines down to the original height (720p).
             */
            float downScaling[height];
            
            float scaleFactor = (float) newHeight/height;
            printf("scale factor: %f\n", scaleFactor);
            for (int i = 0; i < height; i++) {
                downScaling[i] = (float) (((2*i+1)*scaleFactor)-1)/2;
                printf("scaled: %f at %d\n", downScaling[i], i);
            }
            
            
            /**
             * Scale the full height vector down to 720p using the 
             * previous scaling vector
             */
            float offset;
            for (int i = 0; i < height; i++) {
                
                index = floor(downScaling[i]);
                offset = downScaling[i] - index;
                
                m_interpolationValues[i] = (1-offset)*filled[index] + offset*filled[index+1];
                printf("%f at %d with weights %f and %f\n", m_interpolationValues[i], i, (1-offset)*downScaling[i], offset*downScaling[i+1]);
            }
            
        } else {
            // Not a 720p file.
        }
    }
    
    ~D90StairsteppingFix()
    {
        delete m_interpolationValues;
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
        /*struct rgbColor* col = (struct rgbColor*) &in[0];*/
        /*struct rgbColor col = *(struct rgbColor*) &in[0];*/
        
        struct rgbColor col = toRGB(in[0]);
        
        struct rgbColor colA, colB;
        
        printf("Color value of first pixel: r=%d g=%d b=%d a=%d.\nuint value: %d.\n", col.r, col.g, col.b, col.a, in[0]);
        if (height == 720) {
            printf("Converting.\n");
            float factor;
            int index;
            unsigned char r,g,b,a;
            
            for (int line = 0; line < height; line++) {
                index = floor(m_interpolationValues[line]);
                factor = (float) m_interpolationValues[line] - index;
                printf("Factor %f on line %d, using index %d.\n", factor, line, index);
                
                for (int pixel = 0; pixel < width; pixel++) {
                
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
            printf("Just copying. Height is %d.\n", height);
            std::copy(in, in + width*height, out);
        }
    }
    
private:
    float *m_interpolationValues;

};



frei0r::construct<D90StairsteppingFix> plugin("Nikon D90 Stairstepping fix",
                "Removes the Stairstepping from Nikon D90 videos by interpolation",
                "Simon A. Eugster (Granjow)",
                0,1,
                F0R_COLOR_MODEL_RGBA8888);
