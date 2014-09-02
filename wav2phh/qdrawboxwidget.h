/** \file qdrawboxwidget.h
 * \brief Very crude graphic histogram output
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

#ifndef QDRAWBOXWIDGET_H
#define QDRAWBOXWIDGET_H

#include <QResizeEvent>
#include <QColor>
#include <QDebug>
#include <QWidget>

class QDrawBoxWidget : public QWidget
{
    Q_OBJECT

    public:
        QDrawBoxWidget(QWidget *parent);
        void drawLine(int x1,int y1,int x2,int y2);
        // \todo: get the sizes from qtcreator over this->size() QSize
        // YOU HAVE TO ADJUST THESE SETTINGS IDENTICAL TO THOSE FROM QTCREATOR
        const static int maxx = 600;
        const static int maxy = 320;

    public slots:
        void drawHistogram(unsigned int * histogram, const unsigned int numBins, float percent);

    protected:
        virtual void paintEvent (QPaintEvent *event);

    private:
        QPixmap *pixmap;

};

#endif // QDRAWBOXWIDGET_H
