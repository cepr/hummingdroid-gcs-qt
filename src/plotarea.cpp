#include "plotarea.h"
#include <QPixmap>
#include <QPainter>

PlotArea::PlotArea(QWidget *parent) :
    QLabel(parent)
{
    pens[CURRENT].setColor(QColor(0, 0, 0));
    pens[CURRENT].setWidth(1);
    pens[COMMAND].setColor(QColor(0, 0, 255));
    pens[COMMAND].setWidth(1);
    pens[MOTOR].setColor(QColor(255, 0, 0));
    pens[MOTOR].setWidth(1);

    memset(values, 0, sizeof(values));
    pixmap = new QPixmap(width(), height());
    pixmap->fill();
    QPainter p(pixmap);
    p.drawLine(0, height() / 2, width(), height() / 2);
    setPixmap(*pixmap);

    // Start the scrolling timer
    startTimer(25);
}

void PlotArea::timerEvent(QTimerEvent *)
{
    int w = pixmap->width();
    int h = pixmap->height();
    pixmap->scroll(-1, 0, pixmap->rect());
    QPainter p(pixmap);
    p.fillRect(w - 1, 0, 1, h, Qt::white);
    for (int i = 0; i < GRAPH_COUNT; i++) {
        p.setPen(pens[i]);
        p.drawLine(w - 2, (1. - values[0][i]) * h / 2, w - 1, (1. - values[1][i]) * h / 2);
        values[0][i] = values[1][i];
    }
    setPixmap(*pixmap);
}

void PlotArea::set(int index, qreal value)
{
    values[1][index] = (value == value) ? value : 0.;
}

void PlotArea::resizeEvent(QResizeEvent *)
{
    delete pixmap;
    pixmap = new QPixmap(width(), height());
    pixmap->fill();
    QPainter p(pixmap);
    p.drawLine(0, height() / 2, width(), height() / 2);
    setPixmap(*pixmap);
}
