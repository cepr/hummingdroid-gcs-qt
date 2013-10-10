#include "plotarea.h"
#include "ui_plotarea.h"
#include <QPixmap>
#include <QPainter>
#include <QResizeEvent>
#include <QtDebug>

PlotArea::PlotArea(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlotArea),
    pixmap_1(0),
    pixmap_2(0),
    step(2)
{
    ui->setupUi(this);

    pens[CURRENT].setColor(QColor(0, 0, 0));
    pens[CURRENT].setWidth(1);
    pens[COMMAND].setColor(QColor(0, 0, 255));
    pens[COMMAND].setWidth(1);
    pens[MOTOR].setColor(QColor(255, 0, 0));
    pens[MOTOR].setWidth(1);

    // Start the scrolling timer
    timerId = startTimer(20);
}

void PlotArea::timerEvent(QTimerEvent *)
{
    draw(pixmap_1, ui->label_1);
    draw(pixmap_2, ui->label_2);
    for (int i = 0; i < GRAPH_COUNT; i++) {
        values[0][i] = values[1][i];
    }
}

void PlotArea::set(int index, qreal value)
{
    values[1][index] = (value == value) ? value : 0.;
}

void PlotArea::pause(bool checked)
{
    if (!checked && timerId == -1) {
        timerId = startTimer(20);
    } else if (checked && timerId != -1) {
        killTimer(timerId);
        timerId = -1;
    }
}

void PlotArea::resizeEvent(QResizeEvent *)
{
    memset(values, 0, sizeof(values));

    ui->label_1->setFixedSize(size());
    ui->label_1->move(0, 0);
    delete pixmap_1;
    pixmap_1 = new QPixmap(size());
    pixmap_1->fill();
    ui->label_1->setPixmap(*pixmap_1);

    ui->label_2->setFixedSize(size());
    ui->label_2->move(-width(), 0);
    delete pixmap_2;
    pixmap_2 = new QPixmap(size());
    pixmap_2->fill();
    ui->label_2->setPixmap(*pixmap_2);
}

void PlotArea::draw(QPixmap *pixmap, QLabel *label)
{
    if (!pixmap) {
        return;
    }

    int w = width();
    int h = height();

    // Scroll the label
    int x = label->pos().x() - step;
    if (x < -w) {
        x += 2 * w;
    }
    label->move(x, 0);

    // Draw the new line
    x = w - label->pos().x() - 1;
    QPainter p(pixmap);
    p.fillRect(x - step + 1, 0, step, h, Qt::white);
    for (int i = 0; i < GRAPH_COUNT; i++) {
        p.setPen(pens[i]);
        p.drawLine(x - step, (1. - values[0][i]) * h / 2, x, (1. - values[1][i]) * h / 2);
    }
    label->setPixmap(*pixmap);
}
