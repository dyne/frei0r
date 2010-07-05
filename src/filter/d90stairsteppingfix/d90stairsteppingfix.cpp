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
            
            int sliceNumber = (sizeof slices)/(sizeof slices[0]);
            int newSize = height + sliceNumber;
            
            printf("%d slices, %d total new lines\n", sliceNumber, newSize);
            
            float withInsertedLines[newSize];
            
            int count = 0;
            int index = 0;
            for (int i = 0; i < sliceNumber; i++) {
                for (int j = 0; j < slices[i]; j++) {
                    
                    withInsertedLines[index] = count;
                    count++;
                    index++;
                    
                }
                if (count < newSize) {
                    withInsertedLines[index] = count - 0.5;
                    index++;
                }
            }
            for (int i = 0; i < newSize; i++) {
                printf("inserted Lines: %f at %d\n", withInsertedLines[i], i);
            }
            
            float interpolationWeights[height];
            float scaleFactor = (float) newSize/height;
            printf("scale factor: %f\n", scaleFactor);
            for (int i = 0; i < height; i++) {
                interpolationWeights[i] = i*scaleFactor;
            }
            for (int i = 0; i < height; i++) {
                printf("scaled: %f at %d\n", interpolationWeights[i], i);
            }
            
            float offset;
            for (int i = 0; i < height; i++) {
                
                index = floor(interpolationWeights[i]);
                offset = interpolationWeights[i] - index;
                
                if (i+1 < height) {
                    m_interpolationValues[i] = offset*interpolationWeights[i] + (1-offset)*interpolationWeights[i+1];
                } else {
                    m_interpolationValues[i] = interpolationWeights[i];
                    if (m_interpolationValues[i] > (newSize-1)) {
                        m_interpolationValues[i] = newSize-1;
                    }
                }
                printf("%f at %d with weights %f and %f\n", m_interpolationValues[i], 
                    i, offset*interpolationWeights[i], (1-offset)*interpolationWeights[i+1]);
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
        
        printf("Color value of first pixel: r=%d g=%d b=%d a=%d.\nuint value: %d.\n", col.r, col.g, col.b, col.a, in[0]);
        if (height == 720) {
            printf("Converting.\n");
            float factor;
            unsigned char r,g,b,a;
            
            for (int line = 0; line+1 < height; line++) {
                factor = (float) m_interpolationValues[line] - floor(m_interpolationValues[line]);
                //printf("Factor is %f.\n", factor);
                for (int index = 0; index < width; index++) {
                    
                    out[width*line+index] = factor*in[width*line+index] + (1-factor) * in[width*(line+1) + index];
                    //if (index % 2 == 0) { out[width*line+index] = 0; }
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
