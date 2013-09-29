#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLocale>
#include <QUdpSocket>
#include <QHostAddress>
#include <QFile>
#include "Communication.pb.h"
#include "qjoystick.h"
using namespace org::hummingdroid;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void setTelemetry(const TelemetryPacket & telemetry);
    
protected:
    void timerEvent(QTimerEvent *event);

private:
    Ui::MainWindow *ui;
    QUdpSocket udpSocket;
    QHostAddress birdAddress;
    QLocale locale;
    QJoystick joystick;

    // Command packet
    CommandPacket command;
    int command_timer_id;

    // Configuration packet
    CommandPacket config;
    CommandPacket emergency_config;
    int config_timer_id;
    bool emergency;
    float joy_lt; // Joystick left trigger
    float joy_rt; // Joystick right trigger
    QFile log;

public slots:
    void readPendingDatagrams();
    void onJoyAxisChanged(qint64 timestamp, int axis, float value);
    void onJoyButtonPressed(qint64 timestamp, int button);
    void onJoyButtonReleased(qint64 timestamp, int button);
    void restoreConfig();
    void saveConfig();
};

#endif // MAINWINDOW_H
