/** \file audioinput.h
 * \brief A complex audio input device which formats the audio data
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

#ifndef AUDIOINPUT_H
#define AUDIOINPUT_H

#include <QtMultimedia/QAudioDeviceInfo>
#include <QtMultimedia/QAudioInput>

#include <QtCore/qobject.h>
#include <QtCore/qfile.h>
#include <QtMultimedia/qaudioformat.h>
#include <QThread>
#include <QtCore>


class AudioInfo : public QThread
{
   Q_OBJECT
public:
   explicit AudioInfo(size_t numElements, size_t numExtras, QObject *parent = 0 );
   ~AudioInfo();
   bool open(const QString &name);
   bool readHeader();
   void decode();
   void run();
   const QAudioFormat &fileFormat();
   qint64 headerLength();
   void resetSoftGain(double gain);
   void stopProcess();

private:
   QFile fileName;
   QAudioFormat m_fileFormat;
   quint64 m_headerLength;
   size_t maxBufPos;
   size_t numExtra;
   double * ringBufData;
   double softGain;
   size_t numProcessed;
   bool m_abort;
   QMutex mutex;

signals:
   void audioDataReady(const double * data, size_t len, float percent);
   void decodeFinished();

};


#endif
