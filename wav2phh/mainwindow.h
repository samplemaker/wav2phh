/** \file mainwindow.h
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "audioinput.h"
#include "analyzer.h"

#include "analyzersettings.h"
#include <QMainWindow>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    BaseLine * mBaseline;
    PulseEvent * mPulseEvent;
    AnalyzerSettings mAnalyzerSetting;

private slots:
    void onDecodeFinished();
    void recordButtonStartRec();
    void recordButtonStopRec();
    /* slots to be entered if something in the menubar is selected */
    void onActionOpenWavfile();
    void actionConfigFilter();
    void onActionSaveHistogram();
    void onActionAboutThis();
    void onActionHelp();

private:
    Ui::MainWindow *ui;
    Analyzer *m_Analyzer;
    AudioInfo * m_audioInfo = NULL;
    QString fileToSave;
    QString wavFile;
    void saveFile();
};

#endif // MAINWINDOW_H
