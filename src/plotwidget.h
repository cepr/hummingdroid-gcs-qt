#ifndef PLOTWIDGET_H
#define PLOTWIDGET_H

#include <QWidget>
#include <QList>
#include <QPen>
#include "plotserie.h"
#include <QElapsedTimer>

class PlotWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlotWidget(QWidget *parent = 0);
    void setPixelsPerMillis(qreal pixels_per_ms);
    void setRange(qreal min, qreal max);
    void setVerticalGrid(qreal interval);
    QList<PlotSerie> series;
    qreal pixels_per_ms;
    qreal range_min;
    qreal range_max;
    QElapsedTimer timer;

protected:
    void paintEvent(QPaintEvent *);

private:
    qreal vertical_grid_interval;
    QPen grid_line_pen;
    QPen axis_pen;
    QPen grid_label_pen;

signals:

public slots:

};

#endif // PLOTWIDGET_H
