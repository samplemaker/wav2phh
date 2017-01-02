/** \file mainwindow.cpp
 * \brief Controls the logic of the main window and setup the audio interface
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

#include <QtMultimedia/QAudioDeviceInfo>
#include <QtMultimedia/QAudioInput>

#include <QMessageBox>
#include <QFileDialog>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "analyzer.h"
#include "audioinput.h"

// \Todo: put this into a Analyzer Objects which is given to AudioInfo - Ringbuffer
#define NUM_ELEMENTS_RINGBUF 4096
#define NUM_FUTUREPAST_RINGBUF 1024


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->actionOpenWavfile, SIGNAL(triggered()), this, SLOT(onActionOpenWavfile()) );
    connect(ui->actionConfigFilter , SIGNAL(triggered()), this, SLOT(actionConfigFilter()) );
    connect(ui->actionHelp , SIGNAL(triggered()), this, SLOT(onActionHelp()) );
    connect(ui->actionExit, SIGNAL(triggered()), qApp, SLOT(quit()) );
    connect(ui->actionSaveHistogram, SIGNAL(triggered()), this, SLOT(onActionSaveHistogram()) );
    connect(ui->actionAboutThis, SIGNAL(triggered()), this, SLOT(onActionAboutThis()) );


    mBaseline = new BaseLine();
    mPulseEvent = new PulseEvent();
    mBaseline = mAnalyzerSetting.mBaseline;
    mPulseEvent = mAnalyzerSetting.mPulseEvent;
    unsigned int mNumBinsHist = mAnalyzerSetting.mNumBinsHist;

    m_Analyzer  = new Analyzer(mNumBinsHist, NUM_FUTUREPAST_RINGBUF, mBaseline, mPulseEvent);

    /*
    Qt::DirectConnection 1
        When emitted, the signal is immediately delivered to the slot.
    Qt::QueuedConnection 2
        When emitted, the signal is queued until the event loop is able to deliver it to the slot.
    Qt::BlockingQueuedConnection 4
        Same as QueuedConnection, except that the current thread blocks until the slot has been delivered.
        This connection type should only be used for receivers in a different thread.
        Note that misuse of this type can lead to dead locks in your application.
    Qt::AutoConnection 0
        If the signal is emitted from the thread in which the receiving object lives, the slot is invoked directly,
        as with Qt::DirectConnection; otherwise the signal is queued, as with Qt::QueuedConnection.
    */

    QObject::connect(m_Analyzer,
                     SIGNAL( histogramReady(unsigned int *, const unsigned int, float) ),
                     ui->paintArea,
                     SLOT( drawHistogram(unsigned int *, const unsigned int, float) ),
                     Qt::BlockingQueuedConnection);

    /* before changes on the Analyzer are allowed a wav file has to be selected and a AudioInfo has to be created */
    ui->menu_Configure->setDisabled(true);
    ui->recordButton->setCheckable(false);
    ui->audioDeviceLabel->setText("No wav file loaded");
}


MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::onDecodeFinished(){
    qWarning() << "onDecodeFinished";
    ui->paintArea->drawHistogram(m_Analyzer->histogram, m_Analyzer->histResolution, 100.0);
    ui->menu_Configure->setEnabled(true);
    ui->menu_File->setEnabled(true);
    ui->recordButton->setChecked(false);
    /* redirect central button functionionality to "start wav export" */
    disconnect(ui->recordButton, SIGNAL(clicked()), this, SLOT(recordButtonStopRec()));
    connect(ui->recordButton, SIGNAL(clicked()), this, SLOT(recordButtonStartRec()));
}


void MainWindow::recordButtonStartRec()
{
    /* redirect central button to "stop export" */
    disconnect(ui->recordButton, SIGNAL(clicked()), this, SLOT(recordButtonStartRec()));
    /* menubar only accessible if stopped */
    ui->menu_Configure->setDisabled(true);
    ui->menu_File->setDisabled(true);
    /* reset baseline and other stuff from a previous export */
    m_Analyzer->reset();
    /* start a new export thread (decode) */
    m_audioInfo->start();
    connect(ui->recordButton, SIGNAL(clicked()), this, SLOT(recordButtonStopRec()));
}


void MainWindow::recordButtonStopRec()
{
    disconnect(ui->recordButton, SIGNAL(clicked()), this, SLOT(recordButtonStopRec()));
    /* stop a running export thread */
    m_audioInfo->stopProcess();
    ui->menu_Configure->setEnabled(true);
    ui->menu_File->setEnabled(true);
    connect(ui->recordButton, SIGNAL(clicked()), this, SLOT(recordButtonStartRec()));
}


/******************************************************************************
 *
 * Callback slots if something happened in the menu bar
 *
 ******************************************************************************/


//never - i said NEVER name this on_xyz because you will get into hell trouble with qtcreator
void MainWindow::onActionOpenWavfile()
{
    QString fileName = QFileDialog::getOpenFileName(
                this,
                "Select wav file",
                "./",
                "Wav (*.wav);;All Files (*.*)");
    if (!fileName.isEmpty()){
        wavFile = fileName;
        /* if there is a previous incident delete it */
        if(m_audioInfo != NULL)
        {
            QObject::disconnect(m_audioInfo,
                              SIGNAL( audioDataReady(const double *, size_t, float) ),
                              m_Analyzer,
                              SLOT( doHistogram(const double *, size_t, float)) );

            QObject::disconnect(m_audioInfo,
                              SIGNAL( decodeFinished() ),
                              this,
                              SLOT( onDecodeFinished()) );
            delete m_audioInfo;
            m_audioInfo = NULL;

        }
        m_audioInfo  = new AudioInfo(NUM_ELEMENTS_RINGBUF, NUM_FUTUREPAST_RINGBUF, this);
        m_audioInfo->resetSoftGain(mAnalyzerSetting.mSoftGain);
        if ( m_audioInfo->open(wavFile) ){
            ui->audioDeviceLabel->setText("Wav file opened");
            connect(ui->recordButton, SIGNAL(clicked()), this, SLOT(recordButtonStartRec()));
            m_Analyzer->reset();
            ui->recordButton->setCheckable(true);
            ui->menu_Configure->setEnabled(true);

            QObject::connect(m_audioInfo,
                              SIGNAL( audioDataReady(const double *, size_t, float) ),
                              m_Analyzer,
                              SLOT( doHistogram(const double *, size_t, float)),
                             Qt::DirectConnection);

            QObject::connect(m_audioInfo,
                              SIGNAL( decodeFinished() ),
                              this,
                              SLOT( onDecodeFinished()) );
            ui->paintArea->drawReadyToGo();
        }
        else{
            QMessageBox msgBox;
            msgBox.setText("Unknown format: 16bit, 1 channel, SignedInt - WAV only!");
            msgBox.exec();
            ui->menu_Configure->setDisabled(true);
            ui->audioDeviceLabel->setText("No wav file loaded");
            disconnect(ui->recordButton, SIGNAL(clicked()), this, SLOT(recordButtonStartRec()));
            ui->recordButton->setCheckable(false);
            delete m_audioInfo;
            m_audioInfo = NULL;
        }
    }
}


void MainWindow::actionConfigFilter()
{
    mAnalyzerSetting.exec();

    if (mAnalyzerSetting.haveSettings){
        qWarning() << "actionConfigFilter: have new settings";
        /* note that analyzer config is only accessable if an audioinfo object exists */
        m_audioInfo->resetSoftGain(mAnalyzerSetting.mSoftGain);
        mBaseline = mAnalyzerSetting.mBaseline;
        mPulseEvent = mAnalyzerSetting.mPulseEvent;
        unsigned int mNumBinsHist = mAnalyzerSetting.mNumBinsHist;

        /* create a new Analyzer object and update all Analyzer signals and slots */
        QObject::disconnect(m_Analyzer,
                         SIGNAL( histogramReady(unsigned int *, const unsigned int, float) ),
                         ui->paintArea,
                         SLOT( drawHistogram(unsigned int *, const unsigned int, float) ));

        QObject::disconnect(m_audioInfo,
                          SIGNAL( audioDataReady(const double *, size_t, float) ),
                          m_Analyzer,
                          SLOT( doHistogram(const double *, size_t, float)) );
        delete m_Analyzer;

        m_Analyzer  = new Analyzer(mNumBinsHist, NUM_FUTUREPAST_RINGBUF, mBaseline, mPulseEvent);
        QObject::connect(m_Analyzer,
                         SIGNAL( histogramReady(unsigned int *, const unsigned int, float) ),
                         ui->paintArea,
                         SLOT( drawHistogram(unsigned int *, const unsigned int, float) ),
                         Qt::BlockingQueuedConnection);

        QObject::connect(m_audioInfo,
                          SIGNAL( audioDataReady(const double *, size_t, float) ),
                          m_Analyzer,
                          SLOT( doHistogram(const double *, size_t, float)),
                         Qt::DirectConnection);
    }
}


void MainWindow::onActionAboutThis()
{
    QMessageBox::about(this, tr("About this application"),
                             tr("<p><b>Wav2phh</b> creates histograms from an " \
                             "audio wav file. <br> " \
                             "V0.2-alpha (2016-08-29) by samplemaker. <br> " \
                             "<a href=\"https://github.com/samplemaker/wav2phh/\">Visit at Github</a> </p>"));
}


void MainWindow::onActionHelp()
{
    QMessageBox::about(this, tr("Legend table"),
                       tr("<p><b>Differential Threshold</b>:<br>" \
                          "baseline: ignore samples if the difference of two adjacent samples is greater than this value (noise supression)<br>" \
                          "<b>Absolute Threshold</b>:<br>" \
                          "baseline: samples which are higher than this value are not recognized (noise supression)<br>" \
                          "<b>Num Average</b>:<br>" \
                          "baseline: total number of samples considered for baseline calculation (moving average)<br>" \
                          "<b>Trigger Threshold</b>:<br>" \
                          "pulse: samples with an excursion greater than this value from the baseline are recognized as a pulse<br>" \
                          "<b>Num Past</b>:<br>" \
                          "pulse: extra samples to the left and right of the pulse if a pulse event is cut out from the audio stream for futher processing <br>" \
                          "<b>Min Glitchfilter</b>:<br>" \
                          "events are only further processed if the number of samples per pulse lies within the glitch filter boundarys<br>" \
                          "<b>Max Glitchfilter</b>:<br>" \
                          "events are only further processed if the number of samples per pulse lies within the glitch filter boundarys<br>" \
                          "<b>Interpolation Factor</b>:<br>" \
                          "upsampling for peak detection: create 'number-1' of intermediate interpolation points<br>" \
                          "<b>Window Size</b>:<br>" \
                          "half the window size, convolution length of the low pass filter (sinus cardinalis with rectangular window)<br>" \
                          "<b>Soft Gain</b>:<br>" \
                          "factor to amplify or attenuate the audiostream before it is processed</p>"));
}


void MainWindow::onActionSaveHistogram()
{
    QString fileName = QFileDialog::getSaveFileName(
                this,
                "Save as",
                "./",
                "csv Files (*.csv);;All Files (*.*)");
    if (!fileName.isEmpty()){
        fileToSave = fileName + ".csv";
        saveFile();
    }
}


void MainWindow::saveFile()
{
    QFile file(fileToSave);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text)){
        QTextStream outPut(&file);
        for (size_t i = 0; i < m_Analyzer->histResolution; i++){
            qWarning() << i << "\t" << m_Analyzer->histogram[i];
            outPut << i << "\t" << m_Analyzer->histogram[i] << endl;
        }
        file.close();
    }else{
        QMessageBox::warning(
                    this,
                    "Save as",
                    tr("Cannot write file %1.\nError: %2")
                    .arg(fileToSave)
                    .arg(file.errorString()));
    }
}
