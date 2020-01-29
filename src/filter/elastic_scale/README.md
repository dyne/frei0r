# Elastic Scale

Elastic scale allows one to apply non linear scale to video footage.

Written by Matthias Schnöll,  Aug 2018  and released under GNU GPL


## RELEASE NOTES

** Aug 2018
initial release of plugin



## Description of the parameters:

"Scale Center":<br/>
Sets the horizontal center where the scaling orgins from. range: [0,1]


"Linear Scale Area":<br/>
Width of the section that should only be scaled linearly. range: [0,1]


"Linear Scale Factor":<br/>
Scale factor by how much the linear scale are is scaled. range: [0,1]


"Non Linear Scale Factor":<br/>
Amount how much the outer left and outer right area besides the linear scale area are scaled non linearly. range: [0,1]



## Sample Images:


The included images show how the effect modifies the respective footage.
img1.jpg: original image, which shows a grid of equally sized squares


img2.jpg: elastic_scale (parameters: 0.5|0|0|0.7125) applied to img1.jpg


img3.jpg: elastic_scale and 16:9 linear scale applied to img1.jpg


### How to use with ffmpeg:

Transform img1.jpg to img2.jpg:<br/>
```ffmpeg -i img1.jpg -vf "frei0r=elastic_scale:0.5|0|0|0.7125" img2.jpg```

Transform img1.jpg to img3.jpg:<br/>
```ffmpeg -i img1.jpg -vf "frei0r=elastic_scale:0.5|0|0|0.7125,scale=1920:1080,setsar=1:1" img3.jpg```
