#include "frei0r.hpp"

#include <algorithm>

static inline uint8_t
scale(uint8_t value, uint8_t factor)
{
  return std::min((uint16_t)((uint16_t)value * (uint16_t)factor / 128), (uint16_t)value);
}

static inline void
scale_scanline(uint32_t *dst_begin, const uint32_t *src_begin, const uint32_t *src_end, uint8_t factor)
{
  union { uint32_t u32; uint8_t u8[4]; } v;

  while (src_begin < src_end) {
    v.u32 = *src_begin++;
    v.u8[0] = scale(v.u8[0], factor);
    v.u8[1] = scale(v.u8[1], factor);
    v.u8[2] = scale(v.u8[2], factor);
    v.u8[3] = scale(v.u8[3], factor);
    *dst_begin++ = v.u32;
  }
}

class scanline0r : public frei0r::filter
{
public:
  scanline0r(unsigned int width, unsigned int height)
  {
    //register_param(hsync,"HSync","the hsync offset");
  }
  
  virtual void update(double time,
                      uint32_t* out,
                      const uint32_t* in)
  {
    for (unsigned int line=0; line < height; line+=2)
      {
        scale_scanline(out+line*width, in+line*width, in+(line+1)*width, 150);
        scale_scanline(out+(line+1)*width, in+(line+1)*width, in+(line+2)*width, 64);
      }
  }
  
private:
  //double hsync;
};


frei0r::construct<scanline0r> plugin("scanline0r",
				     "interlaced dark lines",
				     "Martin Bayer",
				     0,3);

