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
unsigned char CLAMP0255(int32_t a)
{
  return (unsigned char)
    ( (((-a) >> 31) & a)  // 0 if the number was negative
      | (255 - a) >> 31); // -1 if the number was greater than 255
}

/* Provided temporary int t, returns a * b / 255 */
#define INT_MULT(a,b,t)  ((t) = (a) * (b) + 0x80, ((((t) >> 8) + (t)) >> 8))

/* This version of INT_MULT3 is very fast, but suffers from some
   slight roundoff errors.  It returns the correct result 99.987
   percent of the time */
#define INT_MULT3(a,b,c,t)  ((t) = (a) * (b) * (c) + 0x7F5B, \
                            ((((t) >> 7) + (t)) >> 16))

#define INT_BLEND(a,b,alpha,tmp)  (INT_MULT((a) - (b), alpha, tmp) + (b))

//! Clamp x at lower = l and upper = u.
#define CLAMP(x,l,u) ( x < l ? l : ( x > u ? u : x ) )

//! Round.
#define ROUND(x) ((int32_t)((x)+0.5))

//! Square.
#define SQR(x) ((x) * (x))

//! Limit a (0->511) int to 255.
uint8_t MAX255(uint32_t a) { return (uint8_t) (a | ((a & 256) - ((a & 256) >> 8))); }

#define MIN(x, y)  ((x) < (y) ? (x) : (y));

#define MAX(x, y)  ((x) > (y) ? (x) : (y));

#endif
