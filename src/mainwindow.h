#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLocale>
#include <QUdpSocket>
#include <QHostAddress>
#include "Communication.pb.h"
#include "qjoystick.h"
#include "graph.h"
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
    qint64 t0;
    QGraphicsScene altitude_scene;
    Graph altitude_current_graph;
    Graph altitude_command_graph;
    Graph altitude_motor_graph;
    QGraphicsScene roll_scene;
    Graph roll_current_graph;
    Graph roll_command_graph;
    Graph roll_motor_graph;
    QGraphicsScene pitch_scene;
    Graph pitch_current_graph;
    Graph pitch_command_graph;
    Graph pitch_motor_graph;
    QGraphicsScene yaw_rate_scene;
    Graph yaw_rate_current_graph;
    Graph yaw_rate_command_graph;
    Graph yaw_rate_motor_graph;

    // Command packet
    CommandPacket command;
    int command_timer_id;

    // Configuration packet
    CommandPacket config;
    int config_timer_id;

public slots:
    void readPendingDatagrams();
    void onJoyAxisChanged(qint64 timestamp, int axis, float value);
    void onJoyButtonPressed(qint64 timestamp, int button);
    void onJoyButtonReleased(qint64 timestamp, int button);
};

#endif // MAINWINDOW_H
