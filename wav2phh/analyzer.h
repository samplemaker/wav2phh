/** \file analyzer.h
 * \brief Pulse to histogram core algorithm
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

#ifndef ANALYZER_H
#define ANALYZER_H

#include "interpolate.h"
#include <cstdlib>
#include <QObject>

class BaseLine
{
  public:
    double value;
    double diffThresh;
    double relThresh;
    int numMAvrg;
  private:
};

class PulseEvent
{
  public:
    double trigThresh;
    size_t numPast;
    size_t minGlitchFilter;
    size_t maxGlitchFilter;
    size_t iplnFactor;
    size_t windowSize;
  private:
};


/* we need the QObject to implement signals and slots */
class Analyzer : public QObject
{
    Q_OBJECT

public:
   explicit Analyzer(unsigned int histResolution, size_t extraSamples, size_t bufLen,  BaseLine * baseline, PulseEvent * pulseEvent, QObject *parent = 0);
   ~Analyzer();
   void reset(void);
   unsigned int * histogram;
   unsigned int histResolution;
   float percentOld;

signals:
   void histogramReady(unsigned int * histogram, const unsigned int numBins, float percent);

public slots:
   void doHistogram(const double *dataStream, size_t len, float percent);

private:
   double doBaseline (double n0, double n1);
   double baseLine;
   size_t numExtra;
   size_t bufLen;
   signed long lastPos;
   MovingAverage * mAvrg;
   BaseLine * mBaseline;
   PulseEvent * mPulseEvent;
   Interpolator * lti;
   FILE * fp;
};


#endif
