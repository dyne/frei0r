#ifdef __SSE2__
#include <emmintrin.h>
#endif

static float find_variance(float *hist, int thresh)
{
    float w_0 = 0.0, mu_0 = 0.0, w_1 = 0.0, mu_1 = 0.0;

    for (int i = 0; i < thresh; ++i)
    {
        w_0 += hist[i];
        mu_0 += i * hist[i];
    }
    for (int i = thresh; i < 256; ++i)
    {
        w_1 += hist[i];
        mu_1 += i * hist[i];
    }

    float mu_diff = (mu_0/w_0 - mu_1/w_1);
    return w_0*w_1*mu_diff*mu_diff;
}

#ifdef __SSE2__
static float _sse1_hadd_ps(__m128 v)
{
    // Based on a StackOverflow answer.
    __m128 shuf = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 3, 0, 1));
    __m128 sums = _mm_add_ps(v, shuf);
    shuf        = _mm_movehl_ps(shuf, sums);
    sums        = _mm_add_ss(sums, shuf);
    return _mm_cvtss_f32(sums);
}

static float find_variance_sse2(float *hist, int thresh)
{
    __m128 vw_0 = _mm_setzero_ps(), vmu_0 = _mm_setzero_ps(),
           vw_1 = _mm_setzero_ps(), vmu_1 = _mm_setzero_ps();
    __m128 vcnt = _mm_set_ps(0, 1, 2, 3);

    int thresh_low = (thresh / 4 + 0) * 4,
        thresh_up  = (thresh / 4 + 1) * 4;

    for (int i = 0; i < thresh_low; i+=4)
    {
        __m128 vhist = _mm_castsi128_ps(_mm_loadu_si128((void*)&hist[i]));

        vw_0  = _mm_add_ps(vhist, vw_0);
        vmu_0 = _mm_add_ps(_mm_mul_ps(vcnt, vhist), vw_0);
        vcnt  = _mm_add_ps(vcnt, _mm_set1_ps(4));
    }

    // This is skipped, and handled as an edge case by non-SSE code.
    vcnt = _mm_add_ps(vcnt, _mm_set1_ps(4));

    for (int i = thresh_up; i < 256; i+=4)
    {
        __m128 vhist = _mm_castsi128_ps(_mm_loadu_si128((void*)&hist[i]));

        vw_1  = _mm_add_ps(vhist, vw_1);
        vmu_1 = _mm_add_ps(_mm_mul_ps(vcnt, vhist), vw_1);
        vcnt  = _mm_add_ps(vcnt, _mm_set1_ps(4));
    }

    float w_0 = _sse1_hadd_ps(vw_0), mu_0 = _sse1_hadd_ps(vmu_0),
          w_1 = _sse1_hadd_ps(vw_1), mu_1 = _sse1_hadd_ps(vmu_1);

    // This edge case is here, because thresh may not be a multiple of 4.
    for (int i = thresh_low; i < thresh; ++i)
    {
        w_0 += hist[i];
        mu_0 += i * hist[i];
    }

    for (int i = thresh; i < thresh_up; ++i)
    {
        w_1 += hist[i];
        mu_1 += i * hist[i];
    }

    float mu_diff = (mu_0/w_0 - mu_1/w_1);
    return w_0*w_1*mu_diff*mu_diff;
}
#endif
