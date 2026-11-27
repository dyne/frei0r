#include "frei0r.hpp"

#include <cstring>
#include <cstdint>

class scanline0r : public frei0r::filter
{
public:
  scanline0r(unsigned int width, unsigned int height)
  {
    (void)width;
    (void)height;
    //register_param(hsync,"HSync","the hsync offset");
  }
  
  virtual void update(double time,
                      uint32_t* out,
                      const uint32_t* in)
  {
    (void)time;
    const unsigned int w = width;
    const unsigned int h = height;
    const size_t row_bytes = (size_t)w * sizeof(uint32_t);

    for (unsigned int line = 0; line < h; line += 2)
      {
        /* Even line: min(v * 150 / 128, v) == v identically for all uint8_t */
        memcpy(out + (size_t)line * w, in + (size_t)line * w, row_bytes);

        /* Odd line: min(v * 64 / 128, v) == v >> 1 for each byte */
        const uint32_t* __restrict__ src = in + (size_t)(line + 1) * w;
        uint32_t* __restrict__ dst = out + (size_t)(line + 1) * w;
        for (unsigned int x = 0; x < w; ++x)
          {
            dst[x] = (src[x] >> 1) & 0x7F7F7F7Fu;
          }
      }
  }
  
private:
  //double hsync;
};


static frei0r::construct<scanline0r> plugin("scanline0r",
				     "interlaced dark lines",
				     "Martin Bayer",
				     0,3);
