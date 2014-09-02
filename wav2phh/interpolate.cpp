/** \file interpolate.cpp
 * \brief Upsampling according to shannon nyquist
 *
 * \author Copyright (C) 2014 samplemaker
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
 * @{
 */

#include <cmath>
#include <cstdlib>
#include <QDebug>
#include "interpolate.h"

#define d2i(x) ((x)<0?(int)((x)-0.5):(int)((x)+0.5)

/**
 *  The Nyquist Shannon Law states the reconstruction of a bandwidth limited signal
 *  (x(t); x==0 for all t<0) from its sample values at the time stamps t:=nTa.
 *  The reconstruction can be done with a cardinal series employing the use of the sinc-function:
 *  x(t) & = & \sum_{n=0}^{+\infty}x(nTa)\frac{\sin(\pi(\frac{t}{Ta}-n))}{\pi(\frac{t}{Ta}-n)}
 *  Let k:=Ta/Tb the upsampling factor. Then resampling of x(t) at the points t:=mTb yields
 *  y(mTb) & = & \sum_{n=0}^{+\infty}x(nTa)f[m-kn]
 *  where 
 *  f[u]:=\frac{\sin(\frac{\pi}{k}u)}{\frac{\pi}{k}u}
 *  The filter kernel f[u] can be implemented as look-up tabel and can be stored in RAM 
 **/

/** FIR filter kernel constructor
 *
 *  Setup the filter kernel
 *  f[u]:=\frac{\sin(\frac{\pi}{k}u)}{\frac{\pi}{k}u}
 *  as look up table.
 *
 *  k=Tb/Ta is the upsampling factor. Must be an integer factor - no rational fraction.
 *  k-1 intermediate interpolation points are inserted.
 *
 *  N is the number of datapoints in the sample source vector to be used for
 *  interpolation. Note: N (=N_kernel) can be smaller than the actual length of x[n].
 *  in this case not the full record x is taken into consideration for low pass
 *  filtering. (N_kernel correlates with the window function)
 *
 *  Let N the number of datapoints in the sample source x[n]=x(nTa) then the
 *  interpolation sum boils down to:
 *  y(mTb) & = & \sum_{n=0}^{N-1}x(nTa)f[m-kn]
 *  if extrapolation into the future is forbidden then
 *  m will range from 0 to k*(N-1) (i.e. k*(N-1)+1 datapoints are in the destiny vector) 
 *  hence f[u] is to be implemented in the range [-k*(N-1) .. k*(N-1)]
 *
 *  Due to symmetrical reasons we implement only the positive part:
 *  u in [ 0 .. k*(N-1) ]
 * 
 * \todo: may be multiplied by a window function like hamming, hanning or kaiser
 *
 **/
Interpolator::Interpolator (const unsigned int k, const size_t N_kernel) {
  ipln_factor = k;
  num_kernel = N_kernel;

  filter_lookup = new double[1 + k * ( N_kernel - 1)];

  for (unsigned int m = 0; m <= k*(N_kernel-1); m ++){
     const long double arg = M_PI * (long double)(m) / (long double)(k);
     if ( arg == 0.0 ){
       filter_lookup[m] = 1.0;
     }
     else{
       filter_lookup[m] = (double)(sinl(arg) / arg);
     }
//qWarning() << filter_lookup[m];
  }
}

/* destructor */
Interpolator::~Interpolator () {
  delete [] filter_lookup;
}

double Interpolator::filter_kernel(int m){
 if ( m < 0 ){
   return(filter_lookup[-m]);
 }
 else{
   return(filter_lookup[m]);
 }
}

/** Sample rate conversion (upsampling)
 *
 *  Interpolates k - 1 points between two adjacent sampling points
 *  by applying a windowed sinus cardinalis (cardinal series)
 *
 *  y(mTb) & = & \sum_{n=0}^{N-1}x(nTa)f[m-kn]
 *  where 
 *  f[m-kn] := sin(M_PI*((m/k)-n))/(M_PI*((m/k)-n))
 *
 *  N is the number of datapoints in the sample source vector to be used for
 *  interpolation. Note: The user may call upsample with data length which may
 *  differ from length used in the kernel init call.
 *
 *  see the constructor for more details.
 *
 **/
void Interpolator::upsample (const double *sampleSrc,
                             double *sampleDst,
                             const size_t numSampleSrc,
                             double offset) {

  const int  numSampleDst =  ipln_factor*(numSampleSrc-1)+1;

  /* for each m in the destiny vector */
  for (int m = 0; m < numSampleDst; m ++){
     double tmp = 0.0;
     /* \sum_{n=0}^{N-1}x(nTa)f[m-kn] */
     for (unsigned int n = 0; n < numSampleSrc; n ++){
         /* the caller may call upsamle with a higher numSampleSrc argument
          * than the length of the sinc-ramtable function (N_kernel).
          * in this case -k*(N_kernel-1) <= m-k*n <= k*(N_kernel-1) must be
          * fullfilled otherwise the filter kernel is called out of range.
          * (the sinc is assumed to be 0)
          */
       const int tmp1 = ipln_factor*n;
       /*\todo: outsource to static variable */
       const int tmp2 = ipln_factor*(num_kernel - 1);
       if ((-tmp2 <= (m-tmp1)) && ((m-tmp1) <= tmp2)){
         tmp = tmp + (sampleSrc[n] - offset) * filter_kernel(m-tmp1);
//qWarning() << "m:n" << m << n << "kernel[" <<m-tmp1<< "]";
       }
     }
     sampleDst[m] = tmp;
  }
}


MovingAverage::MovingAverage (int numElements) {
  maxBufPos = numElements - 1;
  ringBufData = new double[maxBufPos + 1]();
  headPos = 0;
  numRecords = 0;
  ringBufSum = 0.0;
}

/* destructor */
MovingAverage::~MovingAverage () {
  delete[] ringBufData;
}


double MovingAverage::doMovingAverage(double value) {
  ringBufData[headPos] = value;
  numRecords++;
  headPos ++;
  /* end of buffer reached */
  if (headPos == maxBufPos){
     ringBufSum += value - ringBufData[0];
     headPos = 0;
   }
  else{
     ringBufSum += value - ringBufData[headPos];
  }
  return(ringBufSum/(double)(numRecords));
}
