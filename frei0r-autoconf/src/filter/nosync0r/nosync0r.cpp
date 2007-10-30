#include "frei0r.hpp"

#include <algorithm>
#include <cmath>

class nosync0r : public frei0r::filter
{
public:
  nosync0r(unsigned int width, unsigned int height)
  {
    register_param(hsync,"HSync","the hsync offset");
  }
  
  virtual void update()
  {
    unsigned int
      first_line=static_cast<unsigned int>(height*std::fmod(hsync,1.0));
    
    std::copy(in+width*first_line, in+width*height, out);
    std::copy(in, in+width*first_line, out+width*(height-first_line));
  }
  
private:
  f0r_param_double hsync;
};


frei0r::construct<nosync0r> plugin("nosync0r",
				   "broken tv",
				   "Martin Bayer",
				   0,1);

