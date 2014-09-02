/** \file interpolate.h
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

#ifndef INTERPOLATE_H
#define INTERPOLATE_H

#include <cstdlib>


class Interpolator
{

  public:
    /* constructor */
    Interpolator (const unsigned int k, const size_t len);
    /* destructor */
    ~Interpolator ();
    void upsample (const double *sampleSrc,
                   double *sampleDst,
                   const size_t numSampleSrc,
                   double offset);
  private:
    int ipln_factor;
    int num_kernel;
    double * filter_lookup;
    double filter_kernel(int m);
};

class MovingAverage
{

  public:
    /* constructor */
    MovingAverage (int numElements);
    /* destructor */
    ~MovingAverage ();
    double doMovingAverage(double value);
  private:
    double * ringBufData;
    int headPos;
    int numRecords;
    int maxBufPos;
    double ringBufSum;

};


#endif

