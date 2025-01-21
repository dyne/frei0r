#include "frei0r.hpp"

#include <algorithm>
#include <cmath>

class nosync0r : public frei0r::filter
{
public:
  nosync0r(unsigned int width, unsigned int height)
  {
    hsync = 0.0;
    register_param(hsync,"HSync","the hsync offset");
    vsync = 0.0;
    register_param(vsync,"VSync","the vsync offset");
  }
  
  virtual void update(double time,
                      uint32_t* out,
                      const uint32_t* in)
  {
    if (hsync == 0.0 && vsync == 0.0) {
        std::copy(in, in+width*height, out);
    } else if (vsync == 0.0) {
      unsigned int
        first_line=static_cast<unsigned int>(height*std::fmod(std::fabs(hsync),1.0));
    
      std::copy(in+width*first_line, in+width*height, out);
      std::copy(in, in+width*first_line, out+width*(height-first_line));
    } else {
        unsigned int hoffset=static_cast<unsigned int>(height*std::fmod(std::fabs(hsync),1.0));
        unsigned int voffset=static_cast<unsigned int>(width*std::fmod(std::fabs(vsync),1.0));
        for (unsigned int src_line = 0; src_line < height; src_line++) {
            unsigned int dst_line = (src_line + hoffset) % height;
            const uint32_t* src_pix = in + (width * src_line);
            uint32_t* dst_pix = out + (width * dst_line);
            std::copy(src_pix, src_pix + width - voffset, dst_pix + voffset);
            std::copy(src_pix + width - voffset, src_pix + width, dst_pix);
        }
    }
  }
  
private:
  double hsync;
  double vsync;
};


frei0r::construct<nosync0r> plugin("nosync0r",
				   "broken tv",
				   "Martin Bayer",
				   0,2);

