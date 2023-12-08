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
  }
  
  virtual void update(double time,
                      uint32_t* out,
                      const uint32_t* in)
  {
    unsigned int
      first_line=static_cast<unsigned int>(height*std::fmod(std::fabs(hsync),1.0));
    
    std::copy(in+width*first_line, in+width*height, out);
    std::copy(in, in+width*first_line, out+width*(height-first_line));
  }
  
private:
  double hsync;
};


frei0r::construct<nosync0r> plugin("nosync0r",
				   "broken tv",
				   "Martin Bayer",
				   0,2);

