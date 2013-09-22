#ifndef PLOTAREA_H
#define PLOTAREA_H

#include <QLabel>
#include <QPixmap>
#include <QPen>

#define GRAPH_COUNT 3

#define CURRENT 0
#define COMMAND 1
#define MOTOR 2

class PlotArea : public QLabel
{
    Q_OBJECT
public:
    explicit PlotArea(QWidget *parent = 0);
    void timerEvent(QTimerEvent *);
    void set(int index, qreal value);

protected:
    void resizeEvent(QResizeEvent *);
    qreal values[2][GRAPH_COUNT];
    QPen pens[GRAPH_COUNT];
    QPixmap* pixmap;

signals:

public slots:

};

#endif // PLOTAREA_H
