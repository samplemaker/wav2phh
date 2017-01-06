/** \file audioinput.cpp
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

#include <stdlib.h>
#include <cmath>
#include <QDebug>
#include <QtCore/qendian.h>
#include <QMessageBox>
#include <QVector>

#include "audioinput.h"

//#define WRITEDATATOFILE 1

/* Gets audio info, puts it into a ringbuffer and organizes the output (see below)
 * numElements: The number of values sent per incident to the output signal
 * numPast: Number of extra values (which are repeated) in the past and future
 */
AudioInfo::AudioInfo(size_t numElements, size_t numPast, QObject *parent) :
     QThread(parent)
{   

    //it is striktly forbidden to write to these variables within other procedures
    maxBufPos = numElements - 1;
    numExtra = numPast;
    // todo: dont use heap allocated variables in a thread constructor. create it rather in run()
    ringBufData = new double[maxBufPos + 1]();
    softGain = 1.0;
}


AudioInfo::~AudioInfo()
{
    /* stop the thread on exit */
    mutex.lock();
    this->m_abort = true;
    mutex.unlock();
    wait();
}


struct chunk
{
    char        id[4];
    quint32     size;
};

struct RIFFHeader
{
    chunk       descriptor;     // "RIFF"
    char        type[4];        // "WAVE"
};

struct WAVEHeader
{
    chunk       descriptor;
    quint16     audioFormat;
    quint16     numChannels;
    quint32     sampleRate;
    quint32     byteRate;
    quint16     blockAlign;
    quint16     bitsPerSample;
};

struct DATAHeader
{
    chunk       descriptor;
};

struct CombinedHeader
{
    RIFFHeader  riff;
    WAVEHeader  wave;
};


const QAudioFormat &AudioInfo::fileFormat()
{
    return m_fileFormat;
}


qint64 AudioInfo::headerLength()
{
    return m_headerLength;
}


void AudioInfo::resetSoftGain(double gain){
    softGain = gain;
}

bool AudioInfo::readHeader()
{
    fileName.seek(0);
    CombinedHeader header;
    bool result = fileName.read(reinterpret_cast<char *>(&header), sizeof(CombinedHeader)) == sizeof(CombinedHeader);

    if (result) {
        if ((memcmp(&header.riff.descriptor.id, "RIFF", 4) == 0
            || memcmp(&header.riff.descriptor.id, "RIFX", 4) == 0)
            && memcmp(&header.riff.type, "WAVE", 4) == 0
            && memcmp(&header.wave.descriptor.id, "fmt ", 4) == 0
            && (header.wave.audioFormat == 1 || header.wave.audioFormat == 0)) {

            // Read off remaining header information
            DATAHeader dataHeader;

            if (qFromLittleEndian<quint32>(header.wave.descriptor.size) > sizeof(WAVEHeader)) {
                // Extended data available
                quint16 extraFormatBytes;
                if (fileName.peek((char*)&extraFormatBytes, sizeof(quint16)) != sizeof(quint16))
                    return false;
                const qint64 throwAwayBytes = sizeof(quint16) + qFromLittleEndian<quint16>(extraFormatBytes);
                if (fileName.read(throwAwayBytes).size() != throwAwayBytes)
                    return false;
            }

            if (fileName.read((char*)&dataHeader, sizeof(DATAHeader)) != sizeof(DATAHeader))
                return false;

            // Establish format
            if (memcmp(&header.riff.descriptor.id, "RIFF", 4) == 0)
                m_fileFormat.setByteOrder(QAudioFormat::LittleEndian);
            else
                m_fileFormat.setByteOrder(QAudioFormat::BigEndian);

            int bps = qFromLittleEndian<quint16>(header.wave.bitsPerSample);
            m_fileFormat.setChannelCount(qFromLittleEndian<quint16>(header.wave.numChannels));
            m_fileFormat.setCodec("audio/pcm");
            m_fileFormat.setSampleRate(qFromLittleEndian<quint32>(header.wave.sampleRate));
            m_fileFormat.setSampleSize(qFromLittleEndian<quint16>(header.wave.bitsPerSample));
            m_fileFormat.setSampleType(bps == 8 ? QAudioFormat::UnSignedInt : QAudioFormat::SignedInt);
        } else {
            result = false;
        }
    }
    m_headerLength = fileName.pos();
    /* stop if we dont have exactly what we want */
    qWarning() << "Wav Header:";
    qWarning() << "sampleSize" << m_fileFormat.sampleSize();
    qWarning() << "sampleType" << m_fileFormat.sampleType();
    qWarning() << "channels" << m_fileFormat.channelCount();
    qWarning() << "samplerate" << m_fileFormat.sampleRate();
    if (!( (m_fileFormat.sampleSize() == 16) &&
           (m_fileFormat.sampleType() == QAudioFormat::SignedInt) &&
           (m_fileFormat.channelCount() == 1 ))) {
        result = false;
    }
    return result;
}

bool AudioInfo::open(const QString &name)
{
    fileName.setFileName(name);
    return fileName.open(QIODevice::ReadOnly) && readHeader();
}


void AudioInfo::stopProcess(){
    qWarning() << "Sending stop signal to thread ...";
    mutex.lock();
    m_abort = true;
    mutex.unlock();
}


void AudioInfo::run(){
    m_abort = false;
    qWarning() << "Starting decode-thread ...";
    decode();
    emit decodeFinished();
}


/**
 *
 * write into the ringbuffer until numElements - numExtra elements are written.
 * then create an outputstream.
 *
 * calling RingBuf(7,2) an audio stream
 * {1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0, 2.1}
 * will be converted into:
 * 0.0, 0.0, 1.0, 1.1, 1.2, 1.3, 1.4
 * 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9
 * 1.8, 1.9, 2.0, 2.1, 2.2, 2.3, 2.4
 * ...
 *
 **/
void AudioInfo::decode()
{

#ifdef WRITEDATATOFILE
    FILE * fp;
    fp = fopen ("audio.txt", "w");
#endif

    //these are locals which have to be reset not only during object initilizazion but also
    //each time decode is called (e.g. the button is pressed twice)
    size_t headPos = 0;
    size_t numRecords = numExtra; //presume zeros for initialization
    size_t popPos = 1;

    qRegisterMetaType<size_t>("size_t");

    const int channelBytes = m_fileFormat.sampleSize() / 8;
    const int sampleBytes = m_fileFormat.channelCount() * channelBytes;

    //size_t totalLen = fileName.size() - m_headerLength;
    const size_t totalSamples = (fileName.size() - m_headerLength) / sampleBytes;
    qWarning() << "have total samples:" << totalSamples;

    size_t numBlockSamples = 1;
    char *blckPtr = new char[channelBytes * numBlockSamples];

    size_t accuCounts = 0;
    fileName.seek(m_headerLength);
    while(!fileName.atEnd()){
        const double fact_16Bit_INT = 1.0 / (double)(32767);
        double rawValue = 0.0;

        fileName.read(blckPtr, channelBytes * numBlockSamples);
        char *ptr = blckPtr;
        for (size_t i = 0; i < numBlockSamples; i++){
            accuCounts++;
            //if (m_fileFormat.sampleSize() == 16) {
            rawValue =  fact_16Bit_INT * (double)(qFromLittleEndian<qint16>((uchar*)ptr));
#ifdef WRITEDATATOFILE
   fprintf(fp, "%f\n", rawValue);
#endif
            ptr += channelBytes;
            double value = softGain * rawValue;
            //printf("%f,\n", rawValue);
            /* write the sample into the ring buffer (insertValue) */
            headPos++;
            if (headPos > maxBufPos){
                headPos = 0;
                ringBufData[0] = value;
            }
            else{
                ringBufData[headPos] = value;
            }
            numRecords++;
            //qWarning() << "pos:" << i ;
            /* buffer is full */
            if (numRecords > maxBufPos){
                //qWarning() << "buffer full at pos:" << i ;
                /* rearrange the buffer (copyArray) to have a linear output array */
                int physPos = popPos - numExtra;
                int startPos;
                if (physPos < 0){
                   startPos = maxBufPos + physPos + 1;
                }
                else{
                   startPos = physPos;
                }
                const size_t numElements = maxBufPos + 1;
                double * outBuffer = new double[numElements];
                for (size_t j = 0; j < numElements; j++){
                   outBuffer[j] = ringBufData[(j + startPos) % (maxBufPos + 1)];
                }

                //qWarning() << "Thread calling sequence 1 (has to be DirectConnection)";
                const float percentAct = 100.0 * (float)(accuCounts)/(float)(totalSamples);
                emit audioDataReady(outBuffer, numElements, percentAct);

                //qWarning() << "Thread calling sequence 3 (has to be DirectConnection)";
                delete [] outBuffer;

                /* data now processed. empty the ringbuffer. keep numExtra elements for
                   the next cycle (numElements - numExtra to be deleted) */
                numRecords = numExtra ;
                /* adjust to the correct position */
                const size_t newPos = popPos + numElements - numExtra;
                if (newPos > maxBufPos){
                    popPos = newPos - (maxBufPos + 1);
                }
                else{
                    popPos =  newPos;
                }
            }
        }
        /* stop thread if requested */
        if(this->m_abort) break;
    }
    delete [] blckPtr;
#ifdef WRITEDATATOFILE
 fclose(fp);
#endif
}
