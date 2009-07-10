/*
 *  x_mirror.h
 *  Flip or mirror objects
 *  AYM 1999-05-01
 */

#include <math.h>


void flip_mirror (SelPtr list, int obj_type, char op);

void centre_of_objects (int obj_type, SelPtr list, int *x, int *y);

int exchange_objects_numbers (int obj_type, SelPtr list, bool adjust);


inline void RotateAndScaleCoords (int *x, int *y, double angle, double scale)
{
  double r, theta;

  r = hypot ((double) *x, (double) *y);
  theta = atan2 ((double) *y, (double) *x);
  *x = (int) (r * scale * cos (theta + angle) + 0.5);
  *y = (int) (r * scale * sin (theta + angle) + 0.5);
}

void RotateAndScaleObjects (int, SelPtr, double, double);


