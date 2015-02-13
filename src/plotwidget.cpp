#include "plotwidget.h"
#include <QPainter>
#include <QResizeEvent>
#include <math.h>

PlotWidget::PlotWidget(QWidget *parent) :
    QWidget(parent),
    pixels_per_ms(.25)
{
    range_min = 0;
    range_max = height();
    vertical_grid_interval = 0.;

    grid_line_pen.setStyle(Qt::DotLine);
    grid_line_pen.setColor(QColor(Qt::gray));
    axis_pen.setColor(QColor(Qt::gray));

    timer.start();
}

void PlotWidget::setPixelsPerMillis(qreal pixels_per_ms)
{
    this->pixels_per_ms = pixels_per_ms;
    update();
}

void PlotWidget::setRange(qreal min, qreal max)
{
    range_min = min;
    range_max = max;
    update();
}

void PlotWidget::setVerticalGrid(qreal interval)
{
    vertical_grid_interval = interval;
}

void PlotWidget::paintEvent(QPaintEvent *)
{
    if (width() <= 0 || height() <= 0 || (range_max <= range_min)) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Coordinate transformation
    qreal range = range_max - range_min;
    qreal range_ratio = (qreal)height() / range;

    // Draw the grid
    if (vertical_grid_interval > 0.) {
        int first_grid_line = ceil(range_min / vertical_grid_interval);
        int last_grid_line = floor(range_max / vertical_grid_interval);
        for (int i = first_grid_line; i <= last_grid_line; i++) {
            qreal y = (qreal)height() - ((vertical_grid_interval * i) - range_min) * range_ratio;
            if (i) {
                painter.setPen(grid_line_pen);
            } else {
                painter.setPen(axis_pen);
            }
            QRect grid_label(width()-30, y - 10, 30, 20);
            painter.drawLine(0, y, width() - 30, y);
            painter.setPen(grid_label_pen);
            painter.drawText(grid_label, Qt::AlignRight | Qt::AlignLeft, QString::number(vertical_grid_interval * i));
        }
    }

    // Draw the lines
    QList<PlotSerie>::iterator it;
    for (it = series.begin(); it != series.end(); ++it) {
        it->paint(painter);
    }

}
