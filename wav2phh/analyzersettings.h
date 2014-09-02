/** \file analyzersettings.h
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

#ifndef ANALYZERSETTINGS_H
#define ANALYZERSETTINGS_H

#include <QDialog>
#include "analyzer.h"

namespace Ui {
class AnalyzerSettings;
}

class AnalyzerSettings : public QDialog
{
    Q_OBJECT

public:
    explicit AnalyzerSettings(QWidget *parent = 0);
    ~AnalyzerSettings();
    BaseLine * mBaseline;
    PulseEvent * mPulseEvent;
    double mSoftGain;
    bool haveSettings;

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void onSpPcomboBoxIndexChanged(int index);

private:
    Ui::AnalyzerSettings *ui;
};

#endif // ANALYZERSETTINGS_H
