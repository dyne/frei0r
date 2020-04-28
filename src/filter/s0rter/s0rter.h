#ifndef __S0RTER
#define __S0RTER 1;

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <frei0r.h>

typedef struct hsl {
    /** [0, 360] (degrees) */
    double_t hue;
    /** [0, 1] (unitless) */
    double_t saturation;
    /** [0, 1] (unitless) */
    double_t lightness;
} hsl_t;

typedef struct s0rter_instance {
    /** The width of the video */
    unsigned int width;
    /** The height of the video */
    unsigned int height;
    /** Temporary storage for hsl-formatted pixels. */
    hsl_t* hsl_buffer;
} s0rter_instance_t;

/**
 * Converts an rgb pixel to its hsl representation.
 * \param[in] rgb the RGB representation of a pixel
 * \param[out] hsl the HSL representation of a pixel
 *
 * \see https://en.wikipedia.org/wiki/HSL_and_HSV#From_RGB
 */
void rgb_to_hsl(const uint32_t* rgb, hsl_t* hsl);

/**
 * Converts an hsl pixel to its rgb representation.
 * \param[in] hsl the HSL representation of a pixel
 * \param[out] rgb the RGB representation of a pixel
 *
 * \see https://en.wikipedia.org/wiki/HSL_and_HSV#HSL_to_RGB
 */
void hsl_to_rgb(const hsl_t* hsl, uint32_t* rgb);

/**
 * Copies an entire rgb frame into its hsl representation.
 * \param[out] instance the s0rter instance that will store an hsl-formatted frame
 * \param[in] rgb_frame a pointer to the rgb-formatted frame to convert
 */
void hslify_frame(const s0rter_instance_t* instance, const uint32_t* rgb_frame);

/**
 * Copies an entire hsl frame into its rgb representation.
 * \param[in] instance the s0rter instance that contains the hsl-formatted frame to convert
 * \param[out] rgb_frame a pointer to the rgb-formatted frame after conversion
 */
void rgbify_frame(const s0rter_instance_t* instance, uint32_t* rgb_frame);
#endif