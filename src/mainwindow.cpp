#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>

#define PORT 49152

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    birdAddress("192.168.1.125"),
    locale(QLocale::system()),
    t0(QDateTime::currentMSecsSinceEpoch()),
    altitude_current_graph(QColor(0, 0, 0), &altitude_scene, t0),
    altitude_command_graph(QColor(0, 255, 0), &altitude_scene, t0),
    altitude_motor_graph(QColor(255, 0, 0), &altitude_scene, t0),
    roll_current_graph(QColor(0, 0, 0), &roll_scene, t0),
    roll_command_graph(QColor(0, 255, 0), &roll_scene, t0),
    roll_motor_graph(QColor(255, 0, 0), &roll_scene, t0),
    pitch_current_graph(QColor(0, 0, 0), &pitch_scene, t0),
    pitch_command_graph(QColor(0, 255, 0), &pitch_scene, t0),
    pitch_motor_graph(QColor(255, 0, 0), &pitch_scene, t0),
    yaw_rate_current_graph(QColor(0, 0, 0), &yaw_rate_scene, t0),
    yaw_rate_command_graph(QColor(0, 255, 0), &yaw_rate_scene, t0),
    yaw_rate_motor_graph(QColor(255, 0, 0), &yaw_rate_scene, t0)
{
    ui->setupUi(this);
    ui->altitude_graph->setScene(&altitude_scene);
    ui->altitude_graph->setT0(t0);
    ui->roll_graph->setScene(&roll_scene);
    ui->roll_graph->setT0(t0);
    ui->pitch_graph->setScene(&pitch_scene);
    ui->pitch_graph->setT0(t0);
    ui->yaw_rate_graph->setScene(&yaw_rate_scene);
    ui->yaw_rate_graph->setT0(t0);
    udpSocket.bind(PORT);
    joystick.open();
    connect(&udpSocket, SIGNAL(readyRead()),
                this, SLOT(readPendingDatagrams()));
    connect(&joystick, SIGNAL(onJoyAxisChanged(qint64, int,float)),
            this, SLOT(onJoyAxisChanged(qint64, int,float)));
    connect(&joystick, SIGNAL(onJoyButtonPressed(qint64, int)),
            this, SLOT(onJoyButtonPressed(qint64, int)));
    connect(&joystick, SIGNAL(onJoyButtonReleased(qint64, int)),
            this, SLOT(onJoyButtonReleased(qint64, int)));
    // Sending command every 20ms
    command_timer_id = startTimer(20);
    // Sending configuration every second
    config_timer_id = startTimer(1000);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setTelemetry(const TelemetryPacket &telemetry)
{
    if (telemetry.has_attitude()) {
        const Attitude & attitude = telemetry.attitude();
        ui->altitude_current->setText(locale.toString(attitude.altitude(), 'f', 2));
        altitude_current_graph.set(attitude.altitude());
        ui->roll_current->setText(locale.toString(attitude.roll(), 'f', 2));
        roll_current_graph.set(attitude.roll());
        ui->pitch_current->setText(locale.toString(attitude.pitch(), 'f', 2));
        pitch_current_graph.set(attitude.pitch());
        ui->yaw_rate_current->setText(locale.toString(attitude.yaw_rate(), 'f', 2));
        yaw_rate_current_graph.set(attitude.yaw_rate());
    }

    if (telemetry.has_control()) {
        const MotorsControl & control = telemetry.control();
        ui->altitude_motors->setText(locale.toString(control.altitude_throttle()));
        altitude_motor_graph.set(control.altitude_throttle());
        ui->roll_motors->setText(locale.toString(control.roll_throttle()));
        roll_motor_graph.set(control.roll_throttle());
        ui->pitch_motors->setText(locale.toString(control.pitch_throttle()));
        pitch_motor_graph.set(control.pitch_throttle());
        ui->yaw_rate_motors->setText(locale.toString(control.yaw_throttle()));
        yaw_rate_motor_graph.set(control.yaw_throttle());
    }
}

void MainWindow::timerEvent(QTimerEvent *event)
{
    int id = event->timerId();
    if (id == command_timer_id) {
        // Send a command packet
        int size = command.ByteSize();
        char buffer[size];
        assert(command.SerializeToArray(buffer, sizeof(buffer)));
        assert(udpSocket.writeDatagram(buffer, size, birdAddress, PORT) == size);
    } else if (id == config_timer_id) {
        // Send a config packet
        CommandPacket::ControllerConfig *cc =
                config.mutable_controller_config();
        {
            PID *pid = cc->mutable_altitude_pid();
            pid->set_kp(ui->altitude_kp->text().toFloat());
            pid->set_ki(ui->altitude_ki->text().toFloat());
            pid->set_kd(ui->altitude_kd->text().toFloat());
            pid->set_ko(ui->altitude_ko->text().toFloat());
        }
        {
            PID *pid = cc->mutable_roll_pid();
            pid->set_kp(ui->roll_kp->text().toFloat());
            pid->set_ki(ui->roll_ki->text().toFloat());
            pid->set_kd(ui->roll_kd->text().toFloat());
            pid->set_ko(ui->roll_ko->text().toFloat());
        }
        {
            PID *pid = cc->mutable_pitch_pid();
            pid->set_kp(ui->pitch_kp->text().toFloat());
            pid->set_ki(ui->pitch_ki->text().toFloat());
            pid->set_kd(ui->pitch_kd->text().toFloat());
            pid->set_ko(ui->pitch_ko->text().toFloat());
        }
        {
            PID *pid = cc->mutable_yaw_rate_pid();
            pid->set_kp(ui->yaw_rate_kp->text().toFloat());
            pid->set_ki(ui->yaw_rate_ki->text().toFloat());
            pid->set_kd(ui->yaw_rate_kd->text().toFloat());
            pid->set_ko(ui->yaw_rate_ko->text().toFloat());
        }

        if (ui->roll_max_enabled->isChecked()) {
            cc->set_max_inclinaison(ui->roll_max->text().toFloat());
        }
        if (ui->altitude_max_enabled->isChecked()) {
            cc->set_max_altitude(ui->altitude_max->text().toFloat());
        }
        if (ui->yaw_rate_max_enabled->isChecked()) {
            cc->set_max_yaw_rate(ui->yaw_rate_max->text().toFloat());
        }

        // Telemetry config
        {
            CommandPacket::TelemetryConfig *tc =
                    config.mutable_telemetry_config();
            tc->mutable_host()->assign("192.168.1.132");
            tc->set_port(PORT);
            tc->set_commandenabled(true);
            tc->set_attitudeenabled(true);
            tc->set_controlenabled(true);
        }

        int size = config.ByteSize();
        char buffer[size];
        assert(config.SerializeToArray(buffer, sizeof(buffer)));
        assert(udpSocket.writeDatagram(buffer, size, birdAddress, PORT) == size);
    }
}

void MainWindow::readPendingDatagrams()
{
    while (udpSocket.hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udpSocket.pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        udpSocket.readDatagram(datagram.data(), datagram.size(),
                                &sender, &senderPort);

        TelemetryPacket packet;
        packet.ParseFromArray(datagram.data(), datagram.size());
        setTelemetry(packet);
    }
}

void MainWindow::onJoyAxisChanged(qint64 timestamp, int axis, float value)
{
    (void)timestamp;

    Attitude *attitude = command.mutable_command();
    switch(axis) {
    case 5:
    {
        float altitude = attitude->altitude() + (value + 1.f) / 100;
        if (altitude > 1.f) {
            altitude = 1.f;
        }
        ui->altitude_command->setText(QString::number(altitude));
        altitude_command_graph.set(altitude);
        attitude->set_altitude(altitude);
        break;
    }
    case 2:
    {
        float altitude = attitude->altitude() - (value + 1.f) / 100;
        if (altitude < -1.f) {
            altitude = -1.f;
        }
        ui->altitude_command->setText(QString::number(altitude));
        altitude_command_graph.set(altitude);
        attitude->set_altitude(altitude);
        break;
    }
    case 0:
        ui->roll_command->setText(QString::number(value));
        roll_command_graph.set(value);
        attitude->set_roll(value);
        break;
    case 1:
        ui->pitch_command->setText(QString::number(value));
        pitch_command_graph.set(value);
        attitude->set_pitch(value);
        break;
    case 3:
        ui->yaw_rate_command->setText(QString::number(value));
        yaw_rate_command_graph.set(value);
        attitude->set_yaw_rate(value);
        break;
    }
}

void MainWindow::onJoyButtonPressed(qint64 timestamp, int button)
{
    (void)timestamp;
    (void)button;
}

void MainWindow::onJoyButtonReleased(qint64 timestamp, int button)
{
    (void)timestamp;
    (void)button;
}
