#ifndef INCLUDED_FREI0R_MATH_H
#define INCLUDED_FREI0R_MATH_H

/*
  
  Code stripped from The Gimp:
  INT_MULT(a,b,t)
  INT_MULT3(a,b,c,t)
  INT_BLEND(a,b,alpha,tmp)
  CLAMP
  ROUND
  MAX255
  
  Code stripped from Drone:
  CLAMP0255
  SQR
*/

/* Clamps a int32-range int between 0 and 255 inclusive. */
#ifndef CLAMP0255
unsigned char CLAMP0255(int32_t a)
{
  return (unsigned char)
    ( (((-a) >> 31) & a)  // 0 if the number was negative
      | (255 - a) >> 31); // -1 if the number was greater than 255
}
#endif

/* Provided temporary int t, returns a * b / 255 */
#ifndef INT_MULT
#define INT_MULT(a,b,t)  ((t) = (a) * (b) + 0x80, ((((t) >> 8) + (t)) >> 8))
#endif

/* This version of INT_MULT3 is very fast, but suffers from some
   slight roundoff errors.  It returns the correct result 99.987
   percent of the time */
#ifndef INT_MULT3
#define INT_MULT3(a,b,c,t)  ((t) = (a) * (b) * (c) + 0x7F5B, \
                            ((((t) >> 7) + (t)) >> 16))
#endif

#ifndef INT_BLEND
#define INT_BLEND(a,b,alpha,tmp)  (INT_MULT((a) - (b), alpha, tmp) + (b))
#endif

#ifndef CLAMP
//! Clamp x at lower = l and upper = u.
#define CLAMP(x,l,u) ( x < l ? l : ( x > u ? u : x ) )
#endif

#ifndef ROUND
//! Round.
#define ROUND(x) ((int32_t)((x)+0.5))
#endif

#ifndef SQR
//! Square.
#define SQR(x) ((x) * (x))
#endif

#ifndef MAX255
//! Limit a (0->511) int to 255.
uint8_t MAX255(uint32_t a) { return (uint8_t) (a | ((a & 256) - ((a & 256) >> 8))); }
#endif

#ifndef MIN
#define MIN(x, y)  ((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x, y)  ((x) > (y) ? (x) : (y))
#endif

#endif
