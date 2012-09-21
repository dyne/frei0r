/* colgate.c
 * Copyright (C) 2012 Steinar H. Gunderson <sgunderson@bigfoot.com>
 *
 * Color correction in LMS color space.
 *
 * This plugin is intended for the same uses as the balanc0r plugin,
 * but differs from balanc0r in two important aspects:
 *
 *   1. It operates in the LMS color space, which is a better approximation
 *      to the human color sensation than RGB is. This allows for more
 *      natural color changes. (Note that this inevitably requires that we
 *      we work in linear color space, not the nonlinear space pixels are
 *      typically stored in.)
 *
 *   2. Its choice of neutral color is not limited by the Planckian locus
 *      (typically expressed in Kelvins) like balanc0r is; you can choose
 *      any color, even those that are not typically regarded as “white”.
 *      If you want a color cast (e.g. a warmer scene), this can be
 *      adjusted with a separate control for color temperature.
 *
 * Color is a very complex topic, and this plugin makes no claims to
 * being 100% correct in any way; however, the results appear visually
 * much more meaningful to me than the balanc0r plugin does, although it
 * also uses significantly more CPU time.
 *
 * frei0r plugins are not given any meaningful information about input or
 * output color space. We assume sRGB, since that typically matches people's
 * viewing devices pretty well. sRGB in turn very closely matches ITU-R Rec.
 * BT.709, the standard color space for HDTV; they share the same RGB
 * primaries, and have a similar gamma (2.35 or 2.4, versus sRGB's curve that
 * is approximately a gamma curve at 2.2). SDTV uses a different color space,
 * ITU-R Rec. BT.601, which has somewhat different primaries and a gamma of
 * 2.2, but in practice, sRGB should be an okay approximation for this as well.
 *
 * The color matrices used are typically from Wikipedia, and the inverses
 * are computed by Octave if nothing else is mentioned.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * We use fixed point, since conversion back and forth to floating-point is
 * slow. (This also enables us to use LUTs in an efficient way for lookup
 * to and from sRGB, as opposed to a pow()-based solution, which is very slow.)
 *
 * We need to think a bit about the range and precision of the different
 * elements to get the best results here, since linear RGB can span quite
 * some range. So:
 *
 *  - Input pixels (which are in linear RGB, always 0..1) are stored as 1.15.
 *  - Matrix elements are s4.10.
 *
 * Matrix elements up to +/- 16 should give us some headroom; extreme color
 * adjustments typically have elements of 5-6.
 *
 * Our standard operation is input_pixel * matrix_element, which becomes s5.25.
 * We add three of them, which would seem to be able to overflow, but doesn't,
 * since the largest pixel is 1.0 (represented as 2^15):
 *
 *   3 * 2^15 * (2^14 - 1) ~= 3 * 2^29 < 2^31.
 *
 * It _is_ larger than 2^30, though, so we don't have any bits to spare here.
 */
#define INPUT_PIXEL_BITS 15
#define MATRIX_ELEMENT_FRAC_BITS 10
#define MATRIX_ELEMENT_BITS (4 + MATRIX_ELEMENT_FRAC_BITS)

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>

#ifdef __SSE2__
#include <emmintrin.h>
#endif

#include "frei0r.h"
#include "frei0r_math.h"

enum ParamIndex {
	NEUTRAL_COLOR,
	COLOR_TEMPERATURE,
};

// Row major (opposite of OpenGL).
typedef float Matrix3x3[9];

typedef struct colgate_instance
{
	unsigned width;
	unsigned height;
	f0r_param_color_t neutral_color;
	double color_temperature;

#ifdef __SSE2__
	__m128i premult_r[256];
	__m128i premult_g[256];
	__m128i premult_b[256];
#else
	int premult_r[256][3];
	int premult_g[256][3];
	int premult_b[256][3];
#endif
} colgate_instance_t;

// Assumes input value in [0..255]; output value is normalized.
static float convert_srgb_to_linear_rgb(float x)
{
	if (x < 255.0f * 0.04045f) {
		return x * (1.0f / (255.0f * 12.92f));
	} else {
		return pow((x + 255.0f * 0.055) * (1.0 / (255.0f * 1.055f)), 2.4);
	}
}

// Assumes normalized input value; output value in [0..255].
static float convert_linear_rgb_to_srgb(float x)
{
	if (x < 0.0031308f) {
		return (255.0f * 12.92f) * x;
	} else {
		return ((255.0f * 1.055f) * pow(x, 1.0f / 2.4f)) - (0.055 * 255.0f);
	}
}

/*
 * For linear -> sRGB, we need at least 13 bits to be able to distinguish all
 * input values; we go for 14 to get some extra accuracy. This results in an 16
 * kB LUT, but we generally don't need the L1 cache for a lot of other things
 * anyway, so hopefully the LUT can mostly stay in L1 cache.
 */
#define REVERSE_LUT_BITS 14
#define REVERSE_LUT_SIZE (1 << REVERSE_LUT_BITS)

static uint8_t linear_rgb_to_srgb_lut[REVERSE_LUT_SIZE];

static void fill_srgb_lut()
{
	int i;
	for (i = 0; i < REVERSE_LUT_SIZE; ++i) {
		// Subtract 0.5 to compensate for the fact that we don't round
		// (which, for our purposes, would entail _adding_ 0.5) at lookup time.
		float x = (i - 0.5) / (float)(REVERSE_LUT_SIZE);
		int srgb = lrintf(convert_linear_rgb_to_srgb(x));
		assert(srgb >= 0 && srgb <= 255);
		linear_rgb_to_srgb_lut[i] = srgb;
	}
}

// Multiply two 3x3 matrices.
static void multiply_3x3_matrices(const Matrix3x3 a, const Matrix3x3 b, Matrix3x3 result)
{
	result[0] = a[0] * b[0] + a[1] * b[3] + a[2] * b[6];
	result[3] = a[3] * b[0] + a[4] * b[3] + a[5] * b[6];
	result[6] = a[6] * b[0] + a[7] * b[3] + a[8] * b[6];

	result[1] = a[0] * b[1] + a[1] * b[4] + a[2] * b[7];
	result[4] = a[3] * b[1] + a[4] * b[4] + a[5] * b[7];
	result[7] = a[6] * b[1] + a[7] * b[4] + a[8] * b[7];

	result[2] = a[0] * b[2] + a[1] * b[5] + a[2] * b[8];
	result[5] = a[3] * b[2] + a[4] * b[5] + a[5] * b[8];
	result[8] = a[6] * b[2] + a[7] * b[5] + a[8] * b[8];
}

// Multiply a 3x3 matrix with a three-element float vector.
static void multiply_3x3_matrix_float3(const Matrix3x3 M, float x0, float x1, float x2, float *y0, float *y1, float *y2)
{
	*y0 = M[0] * x0 + M[1] * x1 + M[2] * x2;
	*y1 = M[3] * x0 + M[4] * x1 + M[5] * x2;
	*y2 = M[6] * x0 + M[7] * x1 + M[8] * x2;
}

// Convert a linear RGB value in s6.25 fixed-point to sRGB (between 0 to 255, inclusive).
static inline uint8_t convert_linear_rgb_to_srgb_fp(int x)
{
	if (x < 0) {
		return 0;
	}
	if (x >= (REVERSE_LUT_SIZE << (INPUT_PIXEL_BITS + MATRIX_ELEMENT_FRAC_BITS - REVERSE_LUT_BITS))) {
		return 255;
	}
	return linear_rgb_to_srgb_lut[((unsigned)x) >> (INPUT_PIXEL_BITS + MATRIX_ELEMENT_FRAC_BITS - REVERSE_LUT_BITS)];
}

// Temperature is in Kelvin. Formula from http://en.wikipedia.org/wiki/Planckian_locus#Approximation .
void convert_color_temperature_to_xyz(float T, float *x, float *y, float *z)
{
	double invT = 1.0 / T;
	double xc, yc;

	if (T <= 4000.0f) {
		xc = ((-0.2661239e9 * invT - 0.2343580e6) * invT + 0.8776956e3) * invT + 0.179910;
	} else {
		xc = ((-3.0258469e9 * invT + 2.1070379e6) * invT + 0.2226347e3) * invT + 0.240390;
	}

	if (T <= 2222.0f) {
		yc = ((-1.1063814 * xc - 1.34811020) * xc + 2.18555832) * xc - 0.20219683;
	} else if (T <= 4000.0f) {
		yc = ((-0.9549476 * xc - 1.37418593) * xc + 2.09137015) * xc - 0.16748867;
	} else {
		yc = (( 3.0817580 * xc - 5.87338670) * xc + 3.75112997) * xc - 0.37001483;
	}

	*x = xc;
	*y = yc;
	*z = 1.0 - xc - yc;
}

// sRGB primaries.
static const Matrix3x3 rgb_to_xyz_matrix = {
	0.4124, 0.3576, 0.1805,
	0.2126, 0.7152, 0.0722,
	0.0193, 0.1192, 0.9505,
};
static const Matrix3x3 xyz_to_rgb_matrix = {
	 3.240625, -1.537208, -0.498629,
	-0.968931,  1.875756,  0.041518,
	 0.055710, -0.204021,  1.056996,
};

static void convert_linear_rgb_to_linear_xyz(float r, float g, float b, float *x, float *y, float *z)
{
	multiply_3x3_matrix_float3(rgb_to_xyz_matrix, r, g, b, x, y, z);
}

static void convert_linear_xyz_to_linear_rgb(float x, float y, float z, float *r, float *g, float *b)
{
	multiply_3x3_matrix_float3(xyz_to_rgb_matrix, x, y, z, r, g, b);
}

/*
 * There are several different LMS spaces, at least according to Wikipedia.
 * Through practical testing, I've found most of them (like the CIECAM02 model)
 * to yield a result that is too reddish in practice. This is the RLAB space,
 * normalized to D65, which means that the standard D65 illuminant
 * (x=0.31271, y=0.32902, z=1-y-x) gives L=M=S under this transformation.
 * This makes sense because sRGB (which is used to derive those XYZ values
 * in the first place) assumes the D65 illuminant, and so the D65 illuminant
 * also gives R=G=B in sRGB.
 */
static const Matrix3x3 xyz_to_lms_matrix = {
	 0.4002, 0.7076, -0.0808,
	-0.2263, 1.1653,  0.0457,
	    0.0,    0.0,  0.9182,
};
static const Matrix3x3 lms_to_xyz_matrix = {
	1.86007, -1.12948,  0.21990,
	0.36122,  0.63880, -0.00001,
	0.00000,  0.00000,  1.08909,
};

static void convert_linear_xyz_to_linear_lms(float x, float y, float z, float *l, float *m, float *s)
{
	multiply_3x3_matrix_float3(xyz_to_lms_matrix, x, y, z, l, m, s);
}

static void convert_linear_lms_to_linear_xyz(float l, float m, float s, float *x, float *y, float *z)
{
	multiply_3x3_matrix_float3(lms_to_xyz_matrix, l, m, s, x, y, z);
}

/*
 * For a given reference color (given in XYZ space),
 * compute scaling factors for L, M and S. What we want at the output is equal L, M and S
 * for the reference color (making it a neutral illuminant), or sL ref_L = sM ref_M = sS ref_S.
 * This removes two degrees of freedom for our system, and we only need to find fL.
 *
 * A reasonable last constraint would be to preserve Y, approximately the brightness,
 * for the reference color. Since L'=M'=S' and the Y row of the LMS-to-XYZ matrix
 * sums to unity, we know that Y'=L', and it's easy to find the fL that sets Y'=Y.
 */
static void compute_lms_scaling_factors(float x, float y, float z, float *scale_l, float *scale_m, float *scale_s)
{
	float l, m, s;
	convert_linear_xyz_to_linear_lms(x, y, z, &l, &m, &s);

	*scale_l = y / l;
	*scale_m = *scale_l * (l / m);
	*scale_s = *scale_l * (l / s);
}

static void compute_correction_matrix(colgate_instance_t *o)
{
	int i;

	/*
	 * Find out what the given neutral color would be in LMS space,
	 * and use that value to build a correction factor for each component
	 * so that the neutral color really becomes gray (in LMS).
	 */
	float ref_r = o->neutral_color.r * 255.0f;
	float ref_g = o->neutral_color.g * 255.0f;
	float ref_b = o->neutral_color.b * 255.0f;

	float linear_r = convert_srgb_to_linear_rgb(ref_r);
	float linear_g = convert_srgb_to_linear_rgb(ref_g);
	float linear_b = convert_srgb_to_linear_rgb(ref_b);

	float x, y, z;
	convert_linear_rgb_to_linear_xyz(linear_r, linear_g, linear_b, &x, &y, &z);

	float l, m, s;
	convert_linear_xyz_to_linear_lms(x, y, z, &l, &m, &s);

	float l_scale, m_scale, s_scale;
	compute_lms_scaling_factors(x, y, z, &l_scale, &m_scale, &s_scale);

	/*
	 * Now apply the color balance. Simply put, we find the chromacity point
	 * for the desired white temperature, see what LMS scaling factors they
	 * would have given us, and then reverse that transform. For T=6500K,
	 * the default, this gives us nearly an identity transform (but only nearly,
	 * since the D65 illuminant does not exactly match the results of T=6500K);
	 * we normalize so that T=6500K really is a no-op.
	 */
	float white_x, white_y, white_z, l_scale_white, m_scale_white, s_scale_white;
	convert_color_temperature_to_xyz(o->color_temperature, &white_x, &white_y, &white_z);
	compute_lms_scaling_factors(white_x, white_y, white_z, &l_scale_white, &m_scale_white, &s_scale_white);

	float ref_x, ref_y, ref_z, l_scale_ref, m_scale_ref, s_scale_ref;
	convert_color_temperature_to_xyz(6500.0f, &ref_x, &ref_y, &ref_z);
	compute_lms_scaling_factors(ref_x, ref_y, ref_z, &l_scale_ref, &m_scale_ref, &s_scale_ref);

	l_scale *= l_scale_ref / l_scale_white;
	m_scale *= m_scale_ref / m_scale_white;
	s_scale *= s_scale_ref / s_scale_white;

	/*
	 * Concatenate all the different linear operations into a single 3x3 matrix.
	 * Note that since we postmultiply our vectors, the order of the matrices
	 * has to be the opposite of the execution order.
	 */
	Matrix3x3 temp, temp2, corr_matrix;
	Matrix3x3 lms_scale_matrix = {
		l_scale,    0.0f,    0.0f,
		   0.0f, m_scale,    0.0f,
		   0.0f,    0.0f, s_scale,
	};
	multiply_3x3_matrices(xyz_to_rgb_matrix, lms_to_xyz_matrix, temp);
	multiply_3x3_matrices(temp, lms_scale_matrix, temp2);
	multiply_3x3_matrices(temp2, xyz_to_lms_matrix, temp);
	multiply_3x3_matrices(temp, rgb_to_xyz_matrix, corr_matrix);

	// Scale for fixed-point, and clamp. We clamp the matrix elements
	// instead of the actual fixed-point numbers below, to make sure
	// we get consistent results over the entire range.
	for (i = 0; i < 9; ++i) {
		corr_matrix[i] *= (float)(1 << MATRIX_ELEMENT_FRAC_BITS);
		if (corr_matrix[i] < -(1 << MATRIX_ELEMENT_BITS)) {
			corr_matrix[i] = -(1 << MATRIX_ELEMENT_BITS);
		}
		if (corr_matrix[i] > (1 << MATRIX_ELEMENT_BITS) - 1) {
			corr_matrix[i] = (1 << MATRIX_ELEMENT_BITS) - 1;
		}
	}

	// Precompute some of the multiplications (after conversion from sRGB)
	// to save some time per-pixel later. Each of these contain the given color
	// converted to linear space and then multiplied by three different factors,
	// given by the matrix.
	for (i = 0; i < 256; ++i) {
		int x = convert_srgb_to_linear_rgb(i) * (float)(1 << INPUT_PIXEL_BITS);

		int r0 = lrintf(x * corr_matrix[0]);
		int r1 = lrintf(x * corr_matrix[3]);
		int r2 = lrintf(x * corr_matrix[6]);

		int g0 = lrintf(x * corr_matrix[1]);
		int g1 = lrintf(x * corr_matrix[4]);
		int g2 = lrintf(x * corr_matrix[7]);

		int b0 = lrintf(x * corr_matrix[2]);
		int b1 = lrintf(x * corr_matrix[5]);
		int b2 = lrintf(x * corr_matrix[8]);

#if __SSE2__
		o->premult_r[i] = _mm_setr_epi32(r0, r1, r2, 0);
		o->premult_g[i] = _mm_setr_epi32(g0, g1, g2, 0);
		o->premult_b[i] = _mm_setr_epi32(b0, b1, b2, 0);
#else
		o->premult_r[i][0] = r0;
		o->premult_r[i][1] = r1;
		o->premult_r[i][2] = r2;

		o->premult_g[i][0] = g0;
		o->premult_g[i][1] = g1;
		o->premult_g[i][2] = g2;

		o->premult_b[i][0] = b0;
		o->premult_b[i][1] = b1;
		o->premult_b[i][2] = b2;
#endif
	}
}

int f0r_init()
{
	fill_srgb_lut();
	return 1;
}

void f0r_deinit()
{
}

void f0r_get_plugin_info(f0r_plugin_info_t *colordistance_info)
{
	colordistance_info->name = "White Balance (LMS space)";
	colordistance_info->author = "Steinar H. Gunderson";
	colordistance_info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
	colordistance_info->color_model = F0R_COLOR_MODEL_RGBA8888;
	colordistance_info->frei0r_version = FREI0R_MAJOR_VERSION;
	colordistance_info->major_version = 0;
	colordistance_info->minor_version = 1;
	colordistance_info->num_params = 2;
	colordistance_info->explanation = "Do simple color correction, in a physically meaningful way";
}

void f0r_get_param_info(f0r_param_info_t *info, int param_index)
{
	switch (param_index) {
	case NEUTRAL_COLOR:
		info->name = "Neutral Color";
		info->type = F0R_PARAM_COLOR;
		info->explanation = "Choose a color from the source image that should be white.";
		break;

	case COLOR_TEMPERATURE:
		info->name = "Color Temperature";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Choose an output color temperature, if different from 6500 K.";
		break;
	}

}

f0r_instance_t f0r_construct(unsigned width, unsigned height)
{
	colgate_instance_t *inst = (colgate_instance_t *)calloc(1, sizeof(*inst));
	inst->width = width;
	inst->height = height;
	inst->neutral_color.r = 0.5;
	inst->neutral_color.g = 0.5;
	inst->neutral_color.b = 0.5;
	inst->color_temperature = 6500.0;
	compute_correction_matrix(inst);
	return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
	free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
	assert(instance);
	colgate_instance_t *inst = (colgate_instance_t *)instance;

	switch (param_index) {
	case NEUTRAL_COLOR:
		inst->neutral_color = *((f0r_param_color_t *)param);
		compute_correction_matrix(inst);
		break;

	case COLOR_TEMPERATURE:
		// Map frei0r range [0, 1] to temperature range [0, 15000].
		inst->color_temperature = *((double *)param) * 15000.0;
		if (inst->color_temperature < 1000.0 || inst->color_temperature > 15000.0) {
			inst->color_temperature = 6500.0;
		}
		compute_correction_matrix(inst);
		break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
	assert(instance);
	colgate_instance_t *inst = (colgate_instance_t*)instance;

	switch (param_index) {
	case NEUTRAL_COLOR:
		*((f0r_param_color_t *)param) = inst->neutral_color;
		break;

	case COLOR_TEMPERATURE:
		// Map temperature range [0, 15000] to frei0r range [0, 1].
		*((double *)param) = inst->color_temperature / 15000.0;
		break;
	}
}

void f0r_update(f0r_instance_t instance, double time, const uint32_t *inframe, uint32_t *outframe)
{
	assert(instance);
	colgate_instance_t *inst = (colgate_instance_t *)instance;
	unsigned len = inst->width * inst->height;
	unsigned char *dst = (unsigned char *)outframe;
	const unsigned char *src = (unsigned char *)inframe;
	unsigned i;

#ifdef __SSE2__
	__m128i zero = _mm_setzero_si128();
	__m128i max = _mm_set1_epi16(REVERSE_LUT_SIZE - 1);
	for (i = 0; i < len; ++i) {
		__m128i l1 = inst->premult_r[*src++];
		__m128i l2 = inst->premult_g[*src++];
		__m128i l3 = inst->premult_b[*src++];
		__m128i result = _mm_add_epi32(l3, _mm_add_epi32(l1, l2));

		// Shift into the right range, and then clamp to [min, max].
		// We convert to 16-bit values since we have min/max instructions
		// there (without needing SSE4), and because it allows us
		// to extract the values with one less SSE shift/move.
		result = _mm_srai_epi32(result, INPUT_PIXEL_BITS + MATRIX_ELEMENT_FRAC_BITS - REVERSE_LUT_BITS);
		result = _mm_packs_epi32(result, result);
		result = _mm_max_epi16(result, zero);
		result = _mm_min_epi16(result, max);

		unsigned new_rg = _mm_cvtsi128_si32(result);
		result = _mm_srli_si128(result, 4);
		unsigned new_b = _mm_cvtsi128_si32(result);

		*dst++ = linear_rgb_to_srgb_lut[new_rg & 0xffff];
		*dst++ = linear_rgb_to_srgb_lut[new_rg >> 16];
		*dst++ = linear_rgb_to_srgb_lut[new_b];
		*dst++ = *src++;  // Copy alpha.
	}
#else
	for (i = 0; i < len; ++i) {
		unsigned old_r = *src++;
		unsigned old_g = *src++;
		unsigned old_b = *src++;

		int new_r = inst->premult_r[old_r][0] + inst->premult_g[old_g][0] + inst->premult_b[old_b][0];
		int new_g = inst->premult_r[old_r][1] + inst->premult_g[old_g][1] + inst->premult_b[old_b][1];
		int new_b = inst->premult_r[old_r][2] + inst->premult_g[old_g][2] + inst->premult_b[old_b][2];

		*dst++ = convert_linear_rgb_to_srgb_fp(new_r);
		*dst++ = convert_linear_rgb_to_srgb_fp(new_g);
		*dst++ = convert_linear_rgb_to_srgb_fp(new_b);
		*dst++ = *src++;  // Copy alpha.
	}
#endif
}
