#include "frei0r.hpp"

#include <algorithm>

class xfade0r : public frei0r::mixer2
{
public:
  xfade0r(unsigned int width, unsigned int height)
  {
    fader = 0.0;
    register_param(fader,"fader","the fader position");
  }

  struct fade_fun
  {
    fade_fun(double pos)
    {
      fader_pos=uint8_t(std::max(0.,std::min(255.,pos*255.)));
    }

    uint8_t operator()(uint8_t in1,uint8_t in2)
    {
      return ((255-fader_pos)*in1 + fader_pos*in2) / 256;
    }
    
    uint8_t fader_pos;
  };
  
  void update(double time,
              uint32_t* out,
              const uint32_t* in1,
              const uint32_t* in2,
              const uint32_t* in3)
  {
    std::transform(reinterpret_cast<const uint8_t*>(in1),
		   reinterpret_cast<const uint8_t*>(in1)+(width*height*4),
		   reinterpret_cast<const uint8_t*>(in2),
		   reinterpret_cast<uint8_t*>(out),
		   fade_fun(fader));
  }
  
private:
  f0r_param_double fader;
};


frei0r::construct<xfade0r> plugin("xfade0r",
				  "a simple xfader",
				  "Martin Bayer",
				  0,2);

