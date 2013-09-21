#include "graph.h"
#include <QDateTime>

Graph::Graph(const QColor &color, QGraphicsScene *scene, qint64 t0) :
    scene(scene), t0(t0), t(0), y(0.), history(500)
{
    pen.setWidth(0);
    pen.setColor(color);
}

void Graph::set(qreal value)
{
    // Condition is to avoid NaN
    if (value == value) {
        qint64 now = QDateTime::currentMSecsSinceEpoch() - t0;
        QGraphicsLineItem* newline = scene->addLine(t, -y, now, -value, pen);
        lines.enqueue(newline);
        if (lines.size() > history) {
            QGraphicsLineItem* oldline = lines.dequeue();
            scene->removeItem(oldline);
            delete oldline;
        }
        t = now;
        y = value;
    }
}
