/** \file analyzer.cpp
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


#include <stdlib.h>
#include <cmath>
#include <QDebug>
#include <QtCore/qendian.h>

#include "analyzer.h"

#define d2i(x) ((x)<0?(int)((x)-0.5):(int)((x)+0.5))

//#define WRITEDATATOFILE 1

/* we need the QObject to implement signals and slots */
/* extraSamples (repeated) accessed in the future and the past */
Analyzer::Analyzer(unsigned int numBinsHist, size_t extraSamples,size_t bufLen_, BaseLine * baseline, PulseEvent * pulseEvent, QObject *parent) : QObject(parent){
   mBaseline = baseline;
   mPulseEvent = pulseEvent;
   histResolution = numBinsHist;
   mAvrg = new MovingAverage(mBaseline->numMAvrg);
   /* k-1 intermediate interpolation points with windowsize2 = 15 extra points used for interpolation */
   lti = new Interpolator(mPulseEvent->iplnFactor, mPulseEvent->windowSize);
   histogram = new unsigned int [histResolution + 1]();
   /* redundant extra samples in past & future as per configuration of the ringbuffer
    * you have to ensure that numExtra is larger than numPast and future samples which
    * may occur due to a pulse event */
   bufLen = bufLen_;
   numExtra = extraSamples;
   lastPos = bufLen - numExtra;
   mBaseline->value = 0;
   percentOld = 0;
#ifdef WRITEDATATOFILE
   fp = fopen ("analyzer.txt", "w");
#endif
}

Analyzer::~Analyzer()
{
 delete[] histogram;
 #ifdef WRITEDATATOFILE 
   fclose(fp);
 #endif
}

void Analyzer::doHistogram(const double *dataStream, size_t len, float percent)
{
  //qWarning() << "Thread calling sequence 2 (slot) (has to be DirectConnection)";
  //in the last sequence we ended at m = lastPos ind the dataStream
  //so in the next cycle we have to adjust our pointer to

  // \TODO: can this get less than zero?????
  size_t m = lastPos - (bufLen-numExtra);

  /*qWarning() << "lastpos " << lastPos;
  qWarning() << "len " << len;
  qWarning() << "numExtra " << numExtra;
  qWarning() << "m " << m;
  printf("press <enter> to continue ...\n\r");
  getchar();*/


  while (m < bufLen - numExtra){

#ifdef WRITEDATATOFILE
  fprintf(fp, "%f\n", dataStream[m]);
#endif

       const double n0 = dataStream[m];
       const double n1 = dataStream[m + 1];
       const double baseline = doBaseline(n0, n1);
       /* rising edge above trigger threshold is found */
       if ((n0 < n1) && ((n1 - baseline) > mPulseEvent->trigThresh)){
         /* get the pulse start position plus some extra samples in the past */
         const size_t start = m - mPulseEvent->numPast;
         /* until peak is reached */
         /* \todo: if the program segfaults here we could implement a checke against
          * mPulseEvent->maxGlitchFilter  */
         while ((dataStream[m] < dataStream[m + 1])){
            m ++;
#ifdef WRITEDATATOFILE
  fprintf(fp, "%f\n", dataStream[m]);
#endif
            if (m >= bufLen - 1){
              qWarning() << "input buffer to small due to search peak " << m;
              //exit(1);
            }
         }
         /* get stop position := start+2*(peakpos-start) */
// \todo increase stop + 1
         const size_t stop = m + m - start;
         if (stop >= bufLen - 1){
           qWarning() << "input buffer to small due to stop pos" << stop;
           //exit(1);
         }
         const size_t numSrc = stop - start;
         /* the pulse width without extra samples (past & future) is */
         size_t pulseWidth = numSrc - mPulseEvent->numPast - mPulseEvent->numPast;

         /* skip glitches */
         if ((pulseWidth > mPulseEvent->minGlitchFilter) &&
            (pulseWidth < mPulseEvent->maxGlitchFilter)) {
 //            m = stop;
             unsigned int numDst = mPulseEvent->iplnFactor * (numSrc - 1) + 1;
             double * peakBuffer = new double[numDst + 1];
             lti->upsample(dataStream + start, peakBuffer, numSrc, 0);
             //lti->upsample(dataStream + start, peakBuffer, numSrc, baseline);

             /* get the peak maximum and minimum */
             double searchMax = -1.0;
             double searchMin = 1.0;
             for (unsigned int n = 0; n < numDst; n ++){
                if (searchMax < peakBuffer[n]) {
                   searchMax = peakBuffer[n];
                }
                if (searchMin > peakBuffer[n]) {
                   searchMin = peakBuffer[n];
                }
             }
             /* cancel pile up: output max - min
              * note: in noisy environments it might be better to trust in
              * the baseline: search_max = search_max - baseline->act_value;
              * 31.Jul.2014: Call Upsample with baseline as offset
              */
             searchMax = searchMax - searchMin;
             /* count the peak value into a pulse height histogram */
             /* if we compare linux vs. windows (mingw) histogram results
              * they are somewhat different due to rounding issues. an extra
              * float cast is spent to get the results identical */
             const int index = (int)(d2i((float)(histResolution * searchMax)));
             if ((index < (int)(histResolution)) && (index >= 0)){
                histogram[index] ++;
             }
             //qWarning() << "height:" << searchMax << "baseLine:" << baseline;
             //#define PRINT_VERBOSE 1
             #ifdef PRINT_VERBOSE
                qWarning() << "start:" << start << "stop:" << stop << "width:" << pulseWidth;
                qWarning() << "height:" << searchMax << "baseLine:" << baseline;
                qWarning() << "press <enter> to print data dump";
                getchar();
                qWarning() << "source:";
                for (int a = start; a < stop; a++){
                   qWarning() << "m:" << a << "\t raw:" << dataStream[a];
                }
                qWarning() << "interpolation:";
                for (unsigned int a = 0; a < numDst;a++){
                   qWarning() << "\t" << peakBuffer[a];
                }
                printf("press <enter> to continue ...\n\r");
                getchar();
             #endif
             delete[] peakBuffer;
         }
         else{
           m ++;
         }
       }    
       else{
         m ++;
       }
    }
  lastPos = m;
    /* only update on each percent */
    if (percent - percentOld > 1.0){
        percentOld = percent;
        emit histogramReady(histogram, histResolution, percent);
    }
}


double Analyzer::doBaseline (double n0, double n1) {

  /* calculate moving average and extract baseline */
  const double delta = n0 - n1;
  if ((fabs(delta) < mBaseline->diffThresh) && (n0 < mBaseline->relThresh)){
     mBaseline->value = mAvrg->doMovingAverage(n0);
  }
  return(mBaseline->value);
}

void Analyzer::reset(void) {
  delete (mAvrg);
  mAvrg = new MovingAverage(mBaseline->numMAvrg);
  delete (lti);
  lti = new Interpolator(mPulseEvent->iplnFactor, mPulseEvent->windowSize);
  memset (histogram, 0, sizeof(histogram[0])*(histResolution + 1) );
  mBaseline->value = 0;
  percentOld = 0;
}
