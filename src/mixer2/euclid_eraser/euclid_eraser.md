# Euclid Eraser Notes #

## Overview ##
This is about the Euclid Eraser mixer.


## Description ##

This class is intended to operate as a mixer, taking two inputs and
yielding one output.

The first input is a reference input, such as a single image or a
stretched video of a single image.

The second input is the video stream to operate on.

The output is a clone of the RGB data of the second input, but with
the alpha channel modified.

This mixer takes the (first) reference input, such as a static
background, and removes it from every frame in the video stream of the
second input.

The alpha channel on the output is based on the euclidian distance of
the two input coordinates in 3-d RGB space. If the calculated distance
betwen the two inputs for a given pixel is less than a provided
(variable) threshold amount, that indicates the pixel in the
background (reference) image is the same or similar enough to the
operational (second) input that is part of the background to be
removed, and the transparency is set to fully transparent via the
alpha channel (set to 0).

If the calcuated distance exceeds the threshold, then that pixel is
part of the foreground image to be retained, and the transparency
of it is set to be fully opaque (alpha channel for that pixel set
to 255).

## Basic Algorithm ##
The basic comparison algorithm is:


x is reference image
y is comparison image to remove reference image from


```
float euclidDistance (int x_r, int x_g, int x_b, int y_r, int y_g, int y_b) 
{
   //calculating color channel differences for next steps
   float red_d = x_r - y_r;
   float green_d = x_g - y_g;
   float blue_d = x_b - y_b; 
  
   float sq_sum, dist;

   //calculating Euclidean distance
   sq_sum = pow(red_d, 2) + pow(green_d, 2) + pow (blue_d)
   dist = sqrt(sq_sum);    
   
   return dist;
}
```

## Note ##

Here's a handy reminder on how the bits map up.

```
      red_src1    = src1[0];
      green_src1  = src1[1];
      blue_src1   = src1[2];
      alpha_src1  = src1[3]
      
      red_src2    = src2[0];
      green_src2  = src2[1];
      blue_src2   = src2[2];
      alpha_src2  = src2[3]
```
## Further Work ##

Some potential improvements are:

- Faster performance (math calculations, others)
- Options beyond binary for alpha

   


