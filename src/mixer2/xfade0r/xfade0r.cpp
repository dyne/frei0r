#include "frei0r.hpp"

#include <algorithm>

#if defined(__SSE2__)
#include <emmintrin.h>
#endif

class xfade0r : public frei0r::mixer2
{
public:
  xfade0r(unsigned int width, unsigned int height)
  {
    fader = 0.0;
    register_param(fader,"fader","the fader position");
  }

  void update(double time,
              uint32_t* out,
              const uint32_t* in1,
              const uint32_t* in2)
  {
    const uint8_t* __restrict__ src1 = reinterpret_cast<const uint8_t*>(in1);
    const uint8_t* __restrict__ src2 = reinterpret_cast<const uint8_t*>(in2);
    uint8_t* __restrict__ dst = reinterpret_cast<uint8_t*>(out);
    const uint8_t fader_pos = uint8_t(std::max(0., std::min(255., fader * 255.)));
    const uint8_t inv_pos = static_cast<uint8_t>(255 - fader_pos);
    const size_t total_bytes = static_cast<size_t>(width) * height * 4;
    size_t i = 0;

#if defined(__SSE2__)
    const __m128i zero = _mm_setzero_si128();
    const __m128i v_f = _mm_set1_epi16(fader_pos);
    const __m128i v_inv = _mm_set1_epi16(inv_pos);

    for (; i + 16 <= total_bytes; i += 16)
      {
        const __m128i va = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src1 + i));
        const __m128i vb = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src2 + i));

        const __m128i a_lo = _mm_unpacklo_epi8(va, zero);
        const __m128i a_hi = _mm_unpackhi_epi8(va, zero);
        const __m128i b_lo = _mm_unpacklo_epi8(vb, zero);
        const __m128i b_hi = _mm_unpackhi_epi8(vb, zero);

        const __m128i res_lo = _mm_srli_epi16(
            _mm_add_epi16(_mm_mullo_epi16(a_lo, v_inv), _mm_mullo_epi16(b_lo, v_f)), 8);
        const __m128i res_hi = _mm_srli_epi16(
            _mm_add_epi16(_mm_mullo_epi16(a_hi, v_inv), _mm_mullo_epi16(b_hi, v_f)), 8);

        _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + i),
                         _mm_packus_epi16(res_lo, res_hi));
      }
#endif

    for (; i < total_bytes; i++)
      {
        dst[i] = static_cast<uint8_t>((inv_pos * src1[i] + fader_pos * src2[i]) >> 8);
      }
  }
  
private:
  double fader;
};


static frei0r::construct<xfade0r> plugin("xfade0r",
				  "a simple xfader",
				  "Martin Bayer",
				  0,2);
