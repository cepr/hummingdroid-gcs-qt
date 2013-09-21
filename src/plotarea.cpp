#include "plotarea.h"
#include <QDateTime>

PlotArea::PlotArea(QWidget *parent) :
    QGraphicsView(parent), t0(0), history(30000)
{
    setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // Start the scrolling timer
    startTimer(50);
}

void PlotArea::timerEvent(QTimerEvent *)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch() - t0;
    //scene()->setSceneRect(now - history, -1, history, 2);
    fitInView(now - history, -1, history, 2);
}

void PlotArea::setT0(qint64 t0)
{
    this->t0 = t0;
}
