#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLocale>
#include <QUdpSocket>
#include <QHostAddress>
#include <QHostInfo>
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
    void sendConfig();

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

    // Latest received telemetry packet
    TelemetryPacket first_telem; // telemetry packet when calibration started
    TelemetryPacket latest_telem;

    // Configuration packet
    CommandPacket config;
    CommandPacket emergency_config;
    int config_timer_id;
    bool emergency;
    int calibrating; // 0: not calibrating, 1: calibrating gyro, 2: calibrating accel
    int calibration_finished_timer_id;
    float sum_gyro_yaw;
    float sum_accel_roll;
    float sum_accel_pitch;
    int calibration_measures;
    float joy_lt; // Joystick left trigger
    float joy_rt; // Joystick right trigger
    QFile log;

public slots:
    void onHostnameAddressResolved(QHostInfo info);
    void readPendingDatagrams();
    void onJoyAxisChanged(qint64 timestamp, int axis, float value);
    void onJoyButtonPressed(qint64 timestamp, int button);
    void onJoyButtonReleased(qint64 timestamp, int button);
    void restoreConfig();
    void saveConfig();
    void onTabChanged(int tab);
    void onCalibrateGyro();
    void onCalibrateAccel();
};

#endif // MAINWINDOW_H
