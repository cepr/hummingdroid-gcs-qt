#include "controlleraxiswidget.h"
#include "ui_controlleraxiswidget.h"

using namespace org::hummingdroid;

#define COMMAND 0
#define CURRENT 1
#define MOTOR 2

ControllerAxisWidget::ControllerAxisWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ControllerAxisWidget)
{
    ui->setupUi(this);
    ui->plot_area->series.append(PlotSerie(ui->plot_area, "Target", QColor(0, 0, 255), PlotSerie::LINE));
    ui->plot_area->series.append(PlotSerie(ui->plot_area, "Sensor", QColor(0, 0, 0), PlotSerie::DOTS));
    ui->plot_area->series.append(PlotSerie(ui->plot_area, "Motor", QColor(255, 0, 0), PlotSerie::LINE));
    connect(ui->pause, SIGNAL(clicked(bool)), this, SLOT(pause(bool)));
}

ControllerAxisWidget::~ControllerAxisWidget()
{
    delete ui;
}

void ControllerAxisWidget::setRange(qreal min, qreal max)
{
    ui->plot_area->setRange(min, max);
}

void ControllerAxisWidget::setVerticalGrid(qreal value)
{
    ui->plot_area->setVerticalGrid(value);
}

void ControllerAxisWidget::setCommand(float value)
{
    ui->command->setText(locale.toString(value, 'f', 2));
    ui->plot_area->series[COMMAND].appendValue(value);
}

void ControllerAxisWidget::setAttitude(float value)
{
    ui->current->setText(locale.toString(value, 'f', 2));
    ui->plot_area->series[CURRENT].appendValue(value);
}

void ControllerAxisWidget::setControl(float value)
{
    ui->motors->setText(locale.toString(value));
    ui->plot_area->series[MOTOR].appendValue(value);
}

const PID &ControllerAxisWidget::getPID()
{
    static PID pid;
    pid.set_kp(ui->kp->text().toFloat());
    pid.set_ki(ui->ki->text().toFloat());
    pid.set_kd(ui->kd->text().toFloat());
    pid.set_td(ui->td->text().toFloat());
    pid.set_ko(ui->offset->text().toFloat());
    return pid;
}

void ControllerAxisWidget::setPID(const PID &from)
{
    ui->kp->setText(QString::number(from.kp()));
    ui->ki->setText(QString::number(from.ki()));
    ui->kd->setText(QString::number(from.kd()));
    ui->td->setText(QString::number(from.td()));
    ui->offset->setText(QString::number(from.ko()));
}

void ControllerAxisWidget::pause(bool checked)
{
    // TODO
    (void)checked;
}
