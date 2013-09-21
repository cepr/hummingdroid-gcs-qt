#ifndef GRAPH_H
#define GRAPH_H

#include <QColor>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QQueue>

class Graph
{
public:
    explicit Graph(const QColor &color, QGraphicsScene *scene, qint64 t0);
    void set(qreal value);

private:
    QGraphicsScene *scene;
    QQueue<QGraphicsLineItem*> lines;
    QPen pen;
    qint64 t0;
    qint64 t;
    qreal y;
    int history;
};

#endif // GRAPH_H
