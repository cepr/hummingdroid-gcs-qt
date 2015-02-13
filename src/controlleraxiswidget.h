#ifndef CONTROLLERAXISWIDGET_H
#define CONTROLLERAXISWIDGET_H

#include <QWidget>
#include <QLocale>
#include "Communication.pb.h"

namespace Ui {
class ControllerAxisWidget;
}

class ControllerAxisWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit ControllerAxisWidget(QWidget *parent = 0);
    ~ControllerAxisWidget();
    void setRange(qreal min, qreal max);
    void setVerticalGrid(qreal value);
    void setCommand(float value);
    void setAttitude(float value);
    void setControl(float value);

    const org::hummingdroid::PID & getPID();
    void setPID(const org::hummingdroid::PID & from);

public slots:
    void pause(bool checked);

private:
    Ui::ControllerAxisWidget *ui;
    QLocale locale;
};

#endif // CONTROLLERAXISWIDGET_H
