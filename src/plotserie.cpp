#include "plotserie.h"
#include "plotwidget.h"
#include <QPainter>

PlotSerie::PlotSerie(PlotWidget *parent, const QString &name, const QColor &color, LineType type) :
    parent(parent), name(name), type(type)
{
    pen.setColor(color);
    if (type == LINE) {
        pen.setWidth(2);
    } else {
        pen.setWidth(4);
    }
}

void PlotSerie::appendValue(qreal value)
{
    values.append(timevalue(parent->timer.elapsed(), value));
/*    while(values.size() > parent->hist_size) {
        values.removeFirst();
    }*/
}

void PlotSerie::paint(QPainter & painter)
{
    if (values.empty()) {
        return;
    }

    // Coordinate transformation
    qreal range = parent->range_max - parent->range_min;
    qreal range_ratio = (qreal)parent->height() / range;

    // Compute the first array index to display
    qint64 t = parent->timer.elapsed();
    qint64 t0 = t - ((qreal)parent->width()) / parent->pixels_per_ms;

    painter.setPen(pen);

    if (type == LINE) {
        QVector<struct timevalue>::iterator it = values.begin();
        struct timevalue a, b = values.first();
        while (it != values.end()) {
            a = b;
            b = *it;

            if (b.t > t0) {
                QPointF pa((a.t - t0) * parent->pixels_per_ms, (qreal) parent->height() - (a.v - parent->range_min) * range_ratio);
                QPointF pb((b.t - t0) * parent->pixels_per_ms, (qreal) parent->height() - (b.v - parent->range_min) * range_ratio);
                painter.drawLine(pa, pb);
            }

            ++it;
        }
    } else {
        QVector<struct timevalue>::iterator it = values.begin();
        while (it != values.end()) {
            const struct timevalue & a = *it;
            if (a.t > t0) {
                painter.drawPoint((a.t - t0) * parent->pixels_per_ms, (qreal) parent->height() - (a.v - parent->range_min) * range_ratio);
            }

            ++it;
        }
    }
}
