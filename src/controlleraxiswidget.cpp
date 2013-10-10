#include "controlleraxiswidget.h"
#include "ui_controlleraxiswidget.h"

using namespace org::hummingdroid;

ControllerAxisWidget::ControllerAxisWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ControllerAxisWidget)
{
    ui->setupUi(this);
    connect(ui->pause, SIGNAL(clicked(bool)), this, SLOT(pause(bool)));
}

ControllerAxisWidget::~ControllerAxisWidget()
{
    delete ui;
}

void ControllerAxisWidget::setCommand(float value)
{
    ui->command->setText(locale.toString(value, 'f', 2));
    ui->plot_area->set(COMMAND, value);
}

void ControllerAxisWidget::setAttitude(float value)
{
    ui->current->setText(locale.toString(value, 'f', 2));
    ui->plot_area->set(CURRENT, value);
}

void ControllerAxisWidget::setControl(float value)
{
    ui->motors->setText(locale.toString(value));
    ui->plot_area->set(MOTOR, value);
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
    ui->plot_area->pause(checked);
    if (ui->pause->isChecked() != checked) {
        ui->pause->setChecked(checked);
    }
}
