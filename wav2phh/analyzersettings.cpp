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
#define P_MIN_GLITCH_DEFAULT 2
#define P_MAX_GLITCH_DEFAULT 10
#define P_IPLN_FAC_DEFAULT 7
#define P_WINDOW_SIZE_DEFAULT 15
#define G_SOFT_GAIN_DEFAULT 1.0


enum USE_CASES {
    USE_6_SPP,
    USE_10_SPP
};


AnalyzerSettings::AnalyzerSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AnalyzerSettings)
{
    ui->setupUi(this);

    mBaseline = new BaseLine();
    mPulseEvent = new PulseEvent();

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

    /* set the initial setup to default after start up */
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

    ui->SpPcomboBox->addItem(QString("6 Samples per Pulse"), QVariant(USE_6_SPP));
    ui->SpPcomboBox->addItem(QString("10 Samples per Pulse"), QVariant(USE_10_SPP));

    connect(ui->SpPcomboBox, SIGNAL( currentIndexChanged(int) ), this, SLOT( onSpPcomboBoxIndexChanged(int) ) );
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

    haveSettings = true;
}

void AnalyzerSettings::on_buttonBox_rejected()
{
    haveSettings = false;
}

/* selecetion on samples per pulse combobox */
void AnalyzerSettings::onSpPcomboBoxIndexChanged(int index)
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
        break;
        case  USE_10_SPP:
                ui->BLdiffThreshSpinBox->setValue(B_DIFF_TRESH_DEFAULT);
                ui->BLabsThreshSpinBox->setValue(B_REL_THRESH_DEFAULT);
                ui->BLnumAvrgSpinBox->setValue(B_NUM_AVRG_DEFAULT);
                ui->PTrigThreshSpinBox->setValue(0.025);
                ui->PnumPastSpinBox->setValue(8);
                ui->PminGlitchSpinBox->setValue(P_MIN_GLITCH_DEFAULT);
                ui->PmaxGlitchSpinBox->setValue(25);
                ui->PIntrplntSpinBox->setValue(7);
                ui->PNumKernelSpinBox->setValue(22);
                ui->GenSoftGainSpinBox->setValue(2.5);
        break;
        default:
                /* poor mans fall through */
        break;
    }
}
