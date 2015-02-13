#ifndef PLOTSERIE_H
#define PLOTSERIE_H

#include <QString>
#include <QColor>
#include <QPen>
#include <QVector>

class PlotWidget;

class PlotSerie
{
public:
    enum LineType {
        LINE,
        DOTS
    };

    PlotSerie(PlotWidget *parent, const QString & name, const QColor & color, LineType type);
    void appendValue(qreal value);
    void paint(QPainter &painter);
    struct timevalue {
        timevalue() : t(0), v(0) {}
        timevalue(qint64 t, qreal v) : t(t), v(v) {}
        timevalue(const timevalue & that) : t(that.t), v(that.v) {}
        qint64 t;
        qreal v;
    };
    QVector<struct timevalue> values;

protected:
    PlotWidget *parent;
    QString name;
    QPen pen;
    LineType type;
};

#endif // PLOTSERIE_H
