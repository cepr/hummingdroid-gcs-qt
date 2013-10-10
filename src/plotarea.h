#ifndef PLOTAREA_H
#define PLOTAREA_H

#include <QWidget>
#include <QPixmap>
#include <QLabel>
#include <QPen>

#define GRAPH_COUNT 3

#define CURRENT 0
#define COMMAND 1
#define MOTOR 2

namespace Ui {
class PlotArea;
}

class PlotArea : public QWidget
{
    Q_OBJECT
public:
    explicit PlotArea(QWidget *parent = 0);
    void timerEvent(QTimerEvent *);
    void set(int index, qreal value);
    void pause(bool checked);

protected:
    Ui::PlotArea *ui;
    int timerId;
    void resizeEvent(QResizeEvent *);
    qreal values[2][GRAPH_COUNT];
    QPen pens[GRAPH_COUNT];
    QPixmap* pixmap_1;
    QPixmap* pixmap_2;
    int step;

private:
    void draw(QPixmap* pixmap, QLabel* label);

signals:

public slots:

};

#endif // PLOTAREA_H
