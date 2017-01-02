/** \file analyzersettings.cpp
 * \brief Core algorithm dialog box to setup the parameters
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

#include "analyzersettings.h"
#include "ui_analyzersettings.h"
#include <QDebug>

#define B_DIFF_TRESH_DEFAULT 0.005
#define B_REL_THRESH_DEFAULT 0.01
#define B_NUM_AVRG_DEFAULT 20
#define P_TRIG_THRESH_DEFAULT 0.015
#define P_NUM_PAST_DEFAULT 5
#define P_MIN_GLITCH_DEFAULT 1
#define P_MAX_GLITCH_DEFAULT 10
#define P_IPLN_FAC_DEFAULT 7
#define P_WINDOW_SIZE_DEFAULT 15
#define G_SOFT_GAIN_DEFAULT 1.0
#define G_NUM_BINS_HIST_DEFAULT 1024


enum USE_CASES {
    LOAD,
    USE_6_SPP,
    USE_6_SPP_HI_SUPR,
    USE_6_SPP_HI_GAIN,
    USE_10_SPP
};


AnalyzerSettings::AnalyzerSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AnalyzerSettings)
{
    ui->setupUi(this);

    mBaseline = new BaseLine();
    mPulseEvent = new PulseEvent();

    /* set the initial setup to default (USE_6_SPP) after start up */
    mBaseline->diffThresh = B_DIFF_TRESH_DEFAULT;
    mBaseline->relThresh = B_REL_THRESH_DEFAULT;
    mBaseline->numMAvrg = B_NUM_AVRG_DEFAULT;
    mPulseEvent->trigThresh = P_TRIG_THRESH_DEFAULT;
    mPulseEvent->numPast = P_NUM_PAST_DEFAULT;
    mPulseEvent->minGlitchFilter = P_MIN_GLITCH_DEFAULT;
    mPulseEvent->maxGlitchFilter = P_MAX_GLITCH_DEFAULT;
    mPulseEvent->iplnFactor = P_IPLN_FAC_DEFAULT;
    mPulseEvent->windowSize = P_WINDOW_SIZE_DEFAULT;
    mSoftGain = G_SOFT_GAIN_DEFAULT;
    mNumBinsHist = G_NUM_BINS_HIST_DEFAULT;

    /* and play back the internal variables to the gui */
    ui->BLdiffThreshSpinBox->setValue(mBaseline->diffThresh);
    ui->BLabsThreshSpinBox->setValue(mBaseline->relThresh);
    ui->BLnumAvrgSpinBox->setValue(mBaseline->numMAvrg);
    ui->PTrigThreshSpinBox->setValue(mPulseEvent->trigThresh);
    ui->PnumPastSpinBox->setValue(mPulseEvent->numPast);
    ui->PminGlitchSpinBox->setValue(mPulseEvent->minGlitchFilter);
    ui->PmaxGlitchSpinBox->setValue(mPulseEvent->maxGlitchFilter);
    ui->PIntrplntSpinBox->setValue(mPulseEvent->iplnFactor);
    ui->PNumKernelSpinBox->setValue(mPulseEvent->windowSize);
    ui->GenSoftGainSpinBox->setValue(mSoftGain);
    ui->GenNumBinsHistSpinBox->setValue(mNumBinsHist);

    ui->SpPcomboBox->addItem(QString("Load Config #"), QVariant(LOAD));
    ui->SpPcomboBox->addItem(QString("6 Samples/Pulse (default)"), QVariant(USE_6_SPP));
    ui->SpPcomboBox->addItem(QString("6 Samples/Pulse (high supression)"), QVariant(USE_6_SPP_HI_SUPR));
    ui->SpPcomboBox->addItem(QString("6 Samples/Pulse (high gain)"), QVariant(USE_6_SPP_HI_GAIN));
    ui->SpPcomboBox->addItem(QString("10 Samples/Pulse"), QVariant(USE_10_SPP));

    connect(ui->SpPcomboBox, SIGNAL( activated(int) ), this, SLOT( onSpPcomboBoxNewSettings(int) ) );
}

AnalyzerSettings::~AnalyzerSettings()
{
    delete ui;
}

void AnalyzerSettings::on_buttonBox_accepted()
{

    mBaseline->diffThresh = ui->BLdiffThreshSpinBox->value(); /* differential threshold to supress volatile signals */
    mBaseline->relThresh = ui->BLabsThreshSpinBox->value();   /* absolute threshold */
    mBaseline->numMAvrg = ui->BLnumAvrgSpinBox->value();
    mPulseEvent->trigThresh = ui->PTrigThreshSpinBox->value();     /* trigger threshold */
    mPulseEvent->numPast = ui->PnumPastSpinBox->value();           /* samples taken from the past */
    mPulseEvent->minGlitchFilter = ui->PminGlitchSpinBox->value(); /* glitch filter: minimum samplepoints per pulse */
    mPulseEvent->maxGlitchFilter = ui->PmaxGlitchSpinBox->value(); /* glitch filter: max */
    mPulseEvent->iplnFactor = ui->PIntrplntSpinBox->value();       /* number - 1 of intermediate interpolation points */
    mPulseEvent->windowSize = ui->PNumKernelSpinBox->value();      /* half the window size / convolution length of low pass filter */
    mSoftGain = ui->GenSoftGainSpinBox->value();              /* amplification factor */
    mNumBinsHist = ui->GenNumBinsHistSpinBox->value();        /* number of bins in the Histogram */

    haveSettings = true;
}

void AnalyzerSettings::on_buttonBox_rejected()
{
    /* Cancel button pressed */
    /* But there may be changes in the gui settings - so load back the internal data into the gui */
    /* keep everything synchronized */
    ui->BLdiffThreshSpinBox->setValue(mBaseline->diffThresh);
    ui->BLabsThreshSpinBox->setValue(mBaseline->relThresh);
    ui->BLnumAvrgSpinBox->setValue(mBaseline->numMAvrg);
    ui->PTrigThreshSpinBox->setValue(mPulseEvent->trigThresh);
    ui->PnumPastSpinBox->setValue(mPulseEvent->numPast);
    ui->PminGlitchSpinBox->setValue(mPulseEvent->minGlitchFilter);
    ui->PmaxGlitchSpinBox->setValue(mPulseEvent->maxGlitchFilter);
    ui->PIntrplntSpinBox->setValue(mPulseEvent->iplnFactor);
    ui->PNumKernelSpinBox->setValue(mPulseEvent->windowSize);
    ui->GenSoftGainSpinBox->setValue(mSoftGain);
    ui->GenNumBinsHistSpinBox->setValue(mNumBinsHist);

    haveSettings = false;
}

/* selecetion on samples per pulse combobox */
void AnalyzerSettings::onSpPcomboBoxNewSettings(int index)
{
    qWarning() << "selected configuration" << index;
    switch( index ) {
        /* default */
        case  USE_6_SPP:
                ui->BLdiffThreshSpinBox->setValue(B_DIFF_TRESH_DEFAULT);
                ui->BLabsThreshSpinBox->setValue(B_REL_THRESH_DEFAULT);
                ui->BLnumAvrgSpinBox->setValue(B_NUM_AVRG_DEFAULT);
                ui->PTrigThreshSpinBox->setValue(P_TRIG_THRESH_DEFAULT);
                ui->PnumPastSpinBox->setValue(P_NUM_PAST_DEFAULT);
                ui->PminGlitchSpinBox->setValue(P_MIN_GLITCH_DEFAULT);
                ui->PmaxGlitchSpinBox->setValue(P_MAX_GLITCH_DEFAULT);
                ui->PIntrplntSpinBox->setValue(P_IPLN_FAC_DEFAULT);
                ui->PNumKernelSpinBox->setValue(P_WINDOW_SIZE_DEFAULT);
                ui->GenSoftGainSpinBox->setValue(G_SOFT_GAIN_DEFAULT);
                ui->GenNumBinsHistSpinBox->setValue(G_NUM_BINS_HIST_DEFAULT);
        break;
        case  USE_6_SPP_HI_SUPR:
                ui->BLdiffThreshSpinBox->setValue(B_DIFF_TRESH_DEFAULT);
                ui->BLabsThreshSpinBox->setValue(B_REL_THRESH_DEFAULT);
                ui->BLnumAvrgSpinBox->setValue(B_NUM_AVRG_DEFAULT);
                ui->PTrigThreshSpinBox->setValue(P_TRIG_THRESH_DEFAULT);
                ui->PnumPastSpinBox->setValue(P_NUM_PAST_DEFAULT);
                ui->PminGlitchSpinBox->setValue(2);
                ui->PmaxGlitchSpinBox->setValue(P_MAX_GLITCH_DEFAULT);
                ui->PIntrplntSpinBox->setValue(P_IPLN_FAC_DEFAULT);
                ui->PNumKernelSpinBox->setValue(P_WINDOW_SIZE_DEFAULT);
                ui->GenSoftGainSpinBox->setValue(G_SOFT_GAIN_DEFAULT);
                ui->GenNumBinsHistSpinBox->setValue(G_NUM_BINS_HIST_DEFAULT);
        break;
        case  USE_6_SPP_HI_GAIN:
                #define SPPHIGAIN 3.0
                ui->BLdiffThreshSpinBox->setValue(SPPHIGAIN*B_DIFF_TRESH_DEFAULT);
                ui->BLabsThreshSpinBox->setValue(SPPHIGAIN*B_REL_THRESH_DEFAULT);
                ui->BLnumAvrgSpinBox->setValue(B_NUM_AVRG_DEFAULT);
                ui->PTrigThreshSpinBox->setValue(SPPHIGAIN*P_TRIG_THRESH_DEFAULT);
                ui->PnumPastSpinBox->setValue(P_NUM_PAST_DEFAULT);
                ui->PminGlitchSpinBox->setValue(P_MIN_GLITCH_DEFAULT);
                ui->PmaxGlitchSpinBox->setValue(P_MAX_GLITCH_DEFAULT);
                ui->PIntrplntSpinBox->setValue(P_IPLN_FAC_DEFAULT);
                ui->PNumKernelSpinBox->setValue(P_WINDOW_SIZE_DEFAULT);
                ui->GenSoftGainSpinBox->setValue(SPPHIGAIN);
                ui->GenNumBinsHistSpinBox->setValue(G_NUM_BINS_HIST_DEFAULT);
        break;
        case  USE_10_SPP:
                ui->BLdiffThreshSpinBox->setValue(B_DIFF_TRESH_DEFAULT);
                ui->BLabsThreshSpinBox->setValue(B_REL_THRESH_DEFAULT);
                ui->BLnumAvrgSpinBox->setValue(B_NUM_AVRG_DEFAULT);
                ui->PTrigThreshSpinBox->setValue(P_TRIG_THRESH_DEFAULT);
                ui->PnumPastSpinBox->setValue(8);
                ui->PminGlitchSpinBox->setValue(P_MIN_GLITCH_DEFAULT);
                ui->PmaxGlitchSpinBox->setValue(25);
                ui->PIntrplntSpinBox->setValue(7);
                ui->PNumKernelSpinBox->setValue(22);
                ui->GenSoftGainSpinBox->setValue(G_SOFT_GAIN_DEFAULT);
                ui->GenNumBinsHistSpinBox->setValue(G_NUM_BINS_HIST_DEFAULT);
        break;
        default:
                /* poor mans fall through */
        break;
    }
    /* reset the combobox to its default state "load" */
    ui->SpPcomboBox->setCurrentIndex(LOAD);
}
