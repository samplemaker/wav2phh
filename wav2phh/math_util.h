/** \file math_util.h
 * \brief Provide mathematical functions
 *
 * \author Copyright (C) 2012 samplemaker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * \ingroup mathlib
 *
 * @{
 */


#ifndef MATH_UTIL_H
#define MATH_UTIL_H


/*-----------------------------------------------------------------------------
 * Includes
 *-----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>


#define d2i(x) ((x)<0?(int32_t)((x)-0.5):(int32_t)((x)+0.5))


/*-----------------------------------------------------------------------------
 * Types
 *-----------------------------------------------------------------------------
 */

/* buffer used for moving average */
typedef struct {
  /* do moving average over #elements */
  uint32_t elements;
  /* internals used for algorithm */
  uint32_t pos;
  double sum;
  double *buffer;
} ma_ring_buf_t;


typedef struct {
  /* interpolate k-1 points between two adjacent sample points */
  uint16_t k;
  /* invk=1/k */
  double invk;
  /* -windowsize/2 .. +windowsize/2 samples are used for interpolation */
  uint16_t window_size2;
} interpolation_t;


/*-----------------------------------------------------------------------------
 * Globals
 *-----------------------------------------------------------------------------
 */

static ma_ring_buf_t ma_ring_buf;
static interpolation_t interpolation;
/** Sinus cardinals ramtable */
static double *_ram_table;


/** Sinus cardinalis ramtable
 *
 *  Implements the sinc function within [0 .. window_size2] in k-steps
 */
inline static
void sinc_table_init(uint16_t k, const uint16_t  window_size2){

  interpolation.k = k;
  interpolation.invk = 1/(double)(k);
  interpolation.window_size2 = window_size2;

  /* \todo: confirm min and max range of the ramtable which is really used (buffer overflow) */
  _ram_table = (double *) malloc(sizeof(double) * (window_size2 + 1) * (k + 1));

  for (uint32_t m = 0; m < (uint32_t)((window_size2 + 1) * (k + 1)); m ++){
     const long double arg = M_PI * (long double)(m) / (long double)(k);
     if ( arg == 0.0 ){
       _ram_table[m] = 1.0;
     }
     else{
       _ram_table[m] = (double)(sinl(arg) / arg);
     }
  }
}


/** Free the sinus cardinalis ram table
 *
 */
inline static
void sinc_table_exit(){

  free(_ram_table);
}


/** Sinc function implementation
 *
 */
inline static
double t_sinc(int32_t m){
  if ( m < 0 ){
    return(_ram_table[-m]);
  }
  else{
    return(_ram_table[m]);
  }
}


/** Datastream upsampling
 *
 *  Interpolates k - 1 points between two adjacent sampling points (upsampling)
 *  by applying a windowed sinus cardinalis (cardinal series)
 *
 *  y[m]=\sum_n{x[n]*sinc(m*b-n)} where y[m]=x(b*m*Ta) with b=0..1 and x[n]=x(n*Ta)
 *
 *  Note: The sum is windowed from [-window_size2 .. +window_size2]
 */
inline static
void upsample(const double *sampleSrc, double *sampleDst, const uint32_t start,
               const uint32_t stop){

  const uint16_t  numSampleSrc = stop - start;
  const uint16_t  numSampleDst = interpolation.k * (numSampleSrc - 1);

  for (uint16_t m = 0; m < numSampleDst; m ++){
     const double n_shifts = (double)(m) * interpolation.invk;
     double tmp = 0.0;
     /* \todo: conversion from n_shifts double to integer via round ? */
     for (int16_t n = -interpolation.window_size2 + (uint16_t)(n_shifts);
          n <= interpolation.window_size2 + (uint16_t)(n_shifts); n ++){
       /* t_sinc(m - k * n) := sin(M_PI*(m / k - n)) / (M_PI*(m / k - n)) */
       tmp = tmp + sampleSrc[start + n] * t_sinc(m - interpolation.k * n);
/*
       if (((m - interpolation.k * n) >= ((interpolation.window_size2 + 1) * (interpolation.k + 1))) ||
           ((m - interpolation.k * n) <= -((interpolation.window_size2 + 1) * (interpolation.k + 1))))
       {
          printf("\n\r fault (math_util, upsample: table out of range \n\r");
       }
*/
     }
     sampleDst[m] = tmp;
  }
}


/** Moving average over ma_ring_buf->elements
 *
 */
inline static
double moving_average(double value){

  ma_ring_buf.buffer[ma_ring_buf.pos] = value;
  ma_ring_buf.pos ++;
  /* end of buffer reached */
  if (ma_ring_buf.pos == ma_ring_buf.elements){
     ma_ring_buf.sum += value - ma_ring_buf.buffer[0];
     ma_ring_buf.pos = 0;
   }
  else{
     ma_ring_buf.sum += value - ma_ring_buf.buffer[ma_ring_buf.pos];
  }
  return(ma_ring_buf.sum/(double)(ma_ring_buf.elements));
}


/**
 *
 */
inline static
void moving_average_init(uint16_t num_elements){

  ma_ring_buf.elements = num_elements;
  ma_ring_buf.pos = 0;
  ma_ring_buf.sum = 0.0;
  ma_ring_buf.buffer = (double *) malloc(sizeof(double) * ma_ring_buf.elements);
  if (ma_ring_buf.buffer == NULL){
    printf("error: cannot allocate memory for moving average !\n\r");
    exit(1);
  }
  memset(ma_ring_buf.buffer, 0, sizeof(double) * ma_ring_buf.elements);
}


/**
 *
 */
inline static
void moving_average_exit(){

  free(ma_ring_buf.buffer);
}


/** @} */

#endif /* !MATH_UTIL_H */
