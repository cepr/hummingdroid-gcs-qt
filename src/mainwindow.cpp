#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QSettings>

#define PORT 49152

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    birdAddress("192.168.1.125"),
    locale(QLocale::system()),
    emergency(false),
    joy_lt(-1.f),
    joy_rt(-1.f)
{
    ui->setupUi(this);
    restoreConfig();
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
    connect(ui->action_Restore, SIGNAL(triggered()),
            this, SLOT(restoreConfig()));
    connect(ui->action_Save, SIGNAL(triggered()),
            this, SLOT(saveConfig()));
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
        ui->altitude_graph->set(CURRENT, attitude.altitude());
        ui->roll_current->setText(locale.toString(attitude.roll(), 'f', 2));
        ui->roll_graph->set(CURRENT, attitude.roll());
        ui->pitch_current->setText(locale.toString(attitude.pitch(), 'f', 2));
        ui->pitch_graph->set(CURRENT, attitude.pitch());
        ui->yaw_rate_current->setText(locale.toString(attitude.yaw_rate(), 'f', 2));
        ui->yaw_rate_graph->set(CURRENT, attitude.yaw_rate());
    }

    if (telemetry.has_control()) {
        const MotorsControl & control = telemetry.control();
        ui->altitude_motors->setText(locale.toString(control.altitude_throttle()));
        ui->altitude_graph->set(MOTOR, control.altitude_throttle());
        ui->roll_motors->setText(locale.toString(control.roll_throttle()));
        ui->roll_graph->set(MOTOR, control.roll_throttle());
        ui->pitch_motors->setText(locale.toString(control.pitch_throttle()));
        ui->pitch_graph->set(MOTOR, control.pitch_throttle());
        ui->yaw_rate_motors->setText(locale.toString(control.yaw_throttle()));
        ui->yaw_rate_graph->set(MOTOR, control.yaw_throttle());
    }
}

void MainWindow::timerEvent(QTimerEvent *event)
{
    int id = event->timerId();
    if (id == command_timer_id) {
        // Compute the new Joystick command (integrator for the altitude)
        {
            Attitude *attitude = command.mutable_command();
            float altitude = attitude->altitude() + (joy_rt - joy_lt) / 100;
            if (altitude > 1.f) {
                altitude = 1.f;
            } else if (altitude < -1.f) {
                altitude = -1.f;
            }
            ui->altitude_command->setText(QString::number(altitude));
            ui->altitude_graph->set(COMMAND, altitude);
            attitude->set_altitude(altitude);
        }
        // Send a command packet
        int size = command.ByteSize();
        char buffer[size];
        assert(command.SerializeToArray(buffer, sizeof(buffer)));
        assert(udpSocket.writeDatagram(buffer, size, birdAddress, PORT) == size);
    } else if (id == config_timer_id) {
        // Send a config packet
        CommandPacket::ControllerConfig *cc =
                config.mutable_controller_config();
        if (!emergency) {
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
        } else {
            {
                PID *pid = cc->mutable_altitude_pid();
                pid->set_kp(0.f);
                pid->set_ki(0.f);
                pid->set_kd(0.f);
                pid->set_ko(0.f);
            }
            {
                PID *pid = cc->mutable_roll_pid();
                pid->set_kp(0.f);
                pid->set_ki(0.f);
                pid->set_kd(0.f);
                pid->set_ko(0.f);
            }
            {
                PID *pid = cc->mutable_pitch_pid();
                pid->set_kp(0.f);
                pid->set_ki(0.f);
                pid->set_kd(0.f);
                pid->set_ko(0.f);
            }
            {
                PID *pid = cc->mutable_yaw_rate_pid();
                pid->set_kp(0.f);
                pid->set_ki(0.f);
                pid->set_kd(0.f);
                pid->set_ko(0.f);
            }
        }
        if (ui->roll_max_enabled->isChecked()) {
            cc->set_max_inclinaison(ui->roll_max->text().toFloat());
        } else {
            cc->clear_max_inclinaison();
        }
        if (ui->altitude_max_enabled->isChecked()) {
            cc->set_max_altitude(ui->altitude_max->text().toFloat());
        } else {
            cc->clear_max_altitude();
        }
        if (ui->yaw_rate_max_enabled->isChecked()) {
            cc->set_max_yaw_rate(ui->yaw_rate_max->text().toFloat());
        } else {
            cc->clear_max_yaw_rate();
        }

        // Telemetry config
        {
            CommandPacket::TelemetryConfig *tc =
                    config.mutable_telemetry_config();
            tc->mutable_host()->assign("192.168.1.96");
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
    case 5: // Right Trigger
        joy_rt = value;
        break;
    case 2: // Left Trigger
        joy_lt = value;
        break;
    case 0:
        ui->roll_command->setText(QString::number(value * .25));
        ui->roll_graph->set(COMMAND, value * .25);
        attitude->set_roll(value * .25);
        break;
    case 1:
        ui->pitch_command->setText(QString::number(value * .25));
        ui->pitch_graph->set(COMMAND, value * .25);
        attitude->set_pitch(value * .25);
        break;
    case 3:
        ui->yaw_rate_command->setText(QString::number(value));
        ui->yaw_rate_graph->set(COMMAND, value);
        attitude->set_yaw_rate(value);
        break;
    }
}

void MainWindow::onJoyButtonPressed(qint64 timestamp, int button)
{
    if (button == 8) {
        // Emergency stop
        emergency = true;
    } else if (button == 7) {
        // Release emergency
        emergency = false;
    }
    (void)timestamp;
}

void MainWindow::onJoyButtonReleased(qint64 timestamp, int button)
{
    (void)timestamp;
    (void)button;
}

void MainWindow::restoreConfig()
{
    QSettings settings;
    QVariant v = settings.value("config");
    if (v.type() == QVariant::ByteArray) {
        const QByteArray & buffer = v.toByteArray();
        config.ParseFromArray(buffer.data(), buffer.size());
    } else {
        // Apply default config
        config.mutable_controller_config()->mutable_altitude_pid()->set_kp(0.f);
        config.mutable_controller_config()->mutable_altitude_pid()->set_ki(0.f);
        config.mutable_controller_config()->mutable_altitude_pid()->set_kd(0.f);
        config.mutable_controller_config()->mutable_altitude_pid()->set_ko(0.f);
        config.mutable_controller_config()->mutable_roll_pid()->set_kp(0.f);
        config.mutable_controller_config()->mutable_roll_pid()->set_ki(0.f);
        config.mutable_controller_config()->mutable_roll_pid()->set_kd(0.f);
        config.mutable_controller_config()->mutable_roll_pid()->set_ko(0.f);
        config.mutable_controller_config()->mutable_pitch_pid()->set_kp(0.f);
        config.mutable_controller_config()->mutable_pitch_pid()->set_ki(0.f);
        config.mutable_controller_config()->mutable_pitch_pid()->set_kd(0.f);
        config.mutable_controller_config()->mutable_pitch_pid()->set_ko(0.f);
        config.mutable_controller_config()->mutable_yaw_rate_pid()->set_kp(0.f);
        config.mutable_controller_config()->mutable_yaw_rate_pid()->set_ki(0.f);
        config.mutable_controller_config()->mutable_yaw_rate_pid()->set_kd(0.f);
        config.mutable_controller_config()->mutable_yaw_rate_pid()->set_ko(0.f);
    }
    // Update UI
    ui->altitude_kp->setText(QString::number(config.controller_config().altitude_pid().kp()));
    ui->altitude_ki->setText(QString::number(config.controller_config().altitude_pid().ki()));
    ui->altitude_kd->setText(QString::number(config.controller_config().altitude_pid().kd()));
    ui->altitude_ko->setText(QString::number(config.controller_config().altitude_pid().ko()));
    ui->roll_kp->setText(QString::number(config.controller_config().roll_pid().kp()));
    ui->roll_ki->setText(QString::number(config.controller_config().roll_pid().ki()));
    ui->roll_kd->setText(QString::number(config.controller_config().roll_pid().kd()));
    ui->roll_ko->setText(QString::number(config.controller_config().roll_pid().ko()));
    ui->pitch_kp->setText(QString::number(config.controller_config().pitch_pid().kp()));
    ui->pitch_ki->setText(QString::number(config.controller_config().pitch_pid().ki()));
    ui->pitch_kd->setText(QString::number(config.controller_config().pitch_pid().kd()));
    ui->pitch_ko->setText(QString::number(config.controller_config().pitch_pid().ko()));
    ui->yaw_rate_kp->setText(QString::number(config.controller_config().yaw_rate_pid().kp()));
    ui->yaw_rate_ki->setText(QString::number(config.controller_config().yaw_rate_pid().ki()));
    ui->yaw_rate_kd->setText(QString::number(config.controller_config().yaw_rate_pid().kd()));
    ui->yaw_rate_ko->setText(QString::number(config.controller_config().yaw_rate_pid().ko()));
    if (config.controller_config().has_max_altitude()) {
        ui->altitude_max_enabled->setChecked(true);
        ui->altitude_max->setText(QString::number(config.controller_config().max_altitude()));
    } else {
        ui->altitude_max_enabled->setChecked(false);
        ui->altitude_max->setText("");
    }
    if (config.controller_config().has_max_inclinaison()) {
        ui->roll_max_enabled->setChecked(true);
        ui->roll_max->setText(QString::number(config.controller_config().max_inclinaison()));
    } else {
        ui->roll_max_enabled->setChecked(false);
        ui->roll_max->setText("");
    }
    if (config.controller_config().has_max_yaw_rate()) {
        ui->yaw_rate_max_enabled->setChecked(true);
        ui->yaw_rate_max->setText(QString::number(config.controller_config().max_yaw_rate()));
    } else {
        ui->yaw_rate_max_enabled->setChecked(false);
        ui->yaw_rate_max->setText("");
    }
}

void MainWindow::saveConfig()
{
    QSettings settings;
    QByteArray buffer(config.ByteSize(), 0);
    config.SerializeToArray(buffer.data(), buffer.size());
    settings.setValue("config", QVariant(buffer));
}
