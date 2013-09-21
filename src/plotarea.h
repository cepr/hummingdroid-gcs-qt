#ifndef PLOTAREA_H
#define PLOTAREA_H

#include <QGraphicsView>
#include <QGraphicsScene>

class PlotArea : public QGraphicsView
{
    Q_OBJECT
public:
    explicit PlotArea(QWidget *parent = 0);
    void timerEvent(QTimerEvent *);
    void setT0(qint64 t0);

private:
    qint64 t0;
    qint64 history;

signals:

public slots:

};

#endif // PLOTAREA_H
