/** \file qdrawboxwidget.cpp
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

#include <QPainter>

#include "qdrawboxwidget.h"

QDrawBoxWidget::QDrawBoxWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(maxx, maxy);
    pixmap = new QPixmap(maxx, maxy);
    QPainter paintToMap(pixmap);
    paintToMap.setPen(QColor("#ffffff"));
    paintToMap.setBrush(QBrush("#ffffff"));
    paintToMap.drawRect(0, 0, maxx, maxy);
}

//a paint event happens if repaint() or update() was invoked
void QDrawBoxWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.drawPixmap(QPoint(0,0), *pixmap);
}

void QDrawBoxWidget::drawLine(int x1, int y1, int x2, int y2)
{
    QPainter paintToMap(pixmap);
    paintToMap.setPen(QPen(Qt::red, 1));
    paintToMap.drawLine(x1, maxy - y1, x2, maxy - y2);
}


void QDrawBoxWidget::drawHistogram(unsigned int * histogram, const unsigned int numBins, float percent)
{
    const int xMargin = 20;
    const int yMargin = 25;

    QPainter paintToMap(pixmap);
    /*paintToMap.setPen(QColor("#ffffff"));
    paintToMap.setBrush(QBrush("#ffffff"));*/
    paintToMap.setBrush(Qt::darkBlue);
    paintToMap.drawRect(0, 0, maxx, maxy);

    paintToMap.setPen(QPen(Qt::red, 1));
    unsigned int binMaxY = 0;
    unsigned int binMaxX = 0;
    for (unsigned int i = 0; i < numBins; i ++){
        if (histogram[i] > binMaxY) { binMaxY = histogram[i]; binMaxX = i;}
    }

    /* \TODO: Fixme: Draw the last point */
    for (unsigned int i = 0; i < numBins - 1; i ++){
        if (numBins < maxx){
            /* histogram resolution less than display resolution */
            unsigned int y1 = (unsigned int)((double)(maxy - 2*yMargin) * (double)(histogram[i]) / (double)(binMaxY) );
            unsigned int x1 = (unsigned int)((double)(maxx - 3*xMargin) * (double)(i) / (double)(numBins) );
            unsigned int x2 = (unsigned int)((double)(maxx - 3*xMargin) * (double)(i+1) / (double)(numBins) );
            paintToMap.fillRect(x1 + xMargin, maxy - y1 - yMargin , x2 - x1, y1, Qt::red);
        }
        else{
            /* histogram higher than display resolution */
            unsigned int y = (unsigned int)((double)(maxy - 2*yMargin) * (double)(histogram[i]) / (double)(binMaxY) );
            unsigned int x = (unsigned int)((double)(maxx - 3*xMargin) * (double)(i) / (double)(numBins) );
            paintToMap.drawLine(x + xMargin, maxy - y - yMargin , x + xMargin, maxy - yMargin);
        }
    }

    /* draw some graticule */
    paintToMap.setPen(QPen(Qt::white, 2));
    /* x-Axis */
    paintToMap.drawLine(xMargin/2, maxy - yMargin , maxx - xMargin/2, maxy - yMargin);
    /* y-Axis */
    paintToMap.drawLine(xMargin, maxy - yMargin + yMargin/4, xMargin, yMargin/2);
    /* draw some ticks */
    const unsigned int xBinMax = maxx - 2*xMargin;
    paintToMap.drawLine(xBinMax, maxy - yMargin + yMargin/4, xBinMax, maxy - yMargin - yMargin/4);

    paintToMap.drawText(xBinMax - 15, maxy - 5, QString::number(numBins));
    paintToMap.drawText(15, maxy - 5, "0");

    if (percent == 100.0){
        paintToMap.drawText(maxx/2 + maxx/4, 50, "Progress: Stopped");
    }
    else{
        QString displayStr = QString::number(percent, 'f', 0);
        paintToMap.drawText(maxx/2 + maxx/4, 50, "Progress: " + displayStr + '%');
    }

    /* peak stats */
    QString displayStr1 = QString::number(binMaxY);
    QString displayStr2 = QString::number(binMaxX);
    unsigned int xptr = (unsigned int)((double)(maxx - 3*xMargin) * (double)(binMaxX) / (double)(numBins) ) + + xMargin;
    paintToMap.setFont(QFont("times",10,QFont::Bold));
    paintToMap.drawText(xptr, yMargin, displayStr2 + '/' + displayStr1);

    repaint();
}
