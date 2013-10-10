#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QSettings>

#define PORT 49152

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    birdAddress("192.168.43.1"),
    locale(QLocale::system()),
    emergency(false),
    joy_lt(-1.f),
    joy_rt(-1.f),
    log("/tmp/log.csv")
{
    showMaximized();
    command.mutable_command()->set_altitude(0.f);
    command.mutable_command()->set_pitch(0.f);
    command.mutable_command()->set_roll(0.f);
    command.mutable_command()->set_yaw_rate(0.f);
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
    connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(onTabChanged(int)));
    // Sending command every 20ms
    command_timer_id = startTimer(20);
    // Sending configuration every second
    config_timer_id = startTimer(1000);

    // Emergency config
    {
        CommandPacket::ControllerConfig *cc =
                emergency_config.mutable_controller_config();
        cc->mutable_altitude_pid()->set_kp(0.f);
        cc->mutable_altitude_pid()->set_ki(0.f);
        cc->mutable_altitude_pid()->set_kd(0.f);
        cc->mutable_altitude_pid()->set_td(0.f);
        cc->mutable_altitude_pid()->set_ko(0.f);
        cc->mutable_roll_pid()->set_kp(0.f);
        cc->mutable_roll_pid()->set_ki(0.f);
        cc->mutable_roll_pid()->set_kd(0.f);
        cc->mutable_roll_pid()->set_td(0.f);
        cc->mutable_roll_pid()->set_ko(0.f);
        cc->mutable_pitch_pid()->set_kp(0.f);
        cc->mutable_pitch_pid()->set_ki(0.f);
        cc->mutable_pitch_pid()->set_kd(0.f);
        cc->mutable_pitch_pid()->set_td(0.f);
        cc->mutable_pitch_pid()->set_ko(0.f);
        cc->mutable_yaw_rate_pid()->set_kp(0.f);
        cc->mutable_yaw_rate_pid()->set_ki(0.f);
        cc->mutable_yaw_rate_pid()->set_kd(0.f);
        cc->mutable_yaw_rate_pid()->set_td(0.f);
        cc->mutable_yaw_rate_pid()->set_ko(0.f);

        CommandPacket::TelemetryConfig *tc =
                emergency_config.mutable_telemetry_config();
        tc->mutable_host()->assign("192.168.43.101");
        tc->set_port(PORT);
        tc->set_commandenabled(true);
        tc->set_attitudeenabled(true);
        tc->set_controlenabled(true);
    }

    // Functional config
    {
        CommandPacket::TelemetryConfig *tc =
                config.mutable_telemetry_config();
        tc->mutable_host()->assign("192.168.43.101");
        tc->set_port(PORT);
        tc->set_commandenabled(true);
        tc->set_attitudeenabled(true);
        tc->set_controlenabled(true);
    }

    log.open(QIODevice::WriteOnly | QIODevice::Text);
    log.write("altitude,roll,pitch,yaw_rate,altitude,roll,pitch,yaw_rate\n");
}

MainWindow::~MainWindow()
{
    log.close();
    delete ui;
}

void MainWindow::setTelemetry(const TelemetryPacket &telemetry)
{
    if (telemetry.has_attitude()) {
        const Attitude & attitude = telemetry.attitude();
        ui->tab_altitude->setAttitude(attitude.altitude());
        ui->tab_roll->setAttitude(attitude.roll());
        ui->tab_pitch->setAttitude(attitude.pitch());
        ui->tab_yaw_rate->setAttitude(attitude.yaw_rate());

        log.write(QString("%1,%2,%3,%4,")
                  .arg(attitude.altitude())
                  .arg(attitude.roll())
                  .arg(attitude.pitch())
                  .arg(attitude.yaw_rate()).toUtf8());
    } else {
        log.write("0,0,0,0,");
    }

    if (telemetry.has_control()) {
        const MotorsControl & control = telemetry.control();
        ui->tab_altitude->setControl(control.altitude_throttle());
        ui->tab_roll->setControl(control.roll_throttle());
        ui->tab_pitch->setControl(control.pitch_throttle());
        ui->tab_yaw_rate->setControl(control.yaw_throttle());

        log.write(QString("%1,%2,%3,%4\n")
                  .arg(control.altitude_throttle())
                  .arg(control.roll_throttle())
                  .arg(control.pitch_throttle())
                  .arg(control.yaw_throttle()).toUtf8());
    } else {
        log.write("0,0,0,0\n");
    }
}

void MainWindow::timerEvent(QTimerEvent *event)
{
    int id = event->timerId();

    if (id == config_timer_id || emergency) {
        // Send a config packet
        if (!emergency) {
            CommandPacket::ControllerConfig *cc =
                    config.mutable_controller_config();
            cc->mutable_altitude_pid()->CopyFrom(ui->tab_altitude->getPID());
            cc->mutable_roll_pid()->CopyFrom(ui->tab_roll->getPID());
            cc->mutable_pitch_pid()->CopyFrom(ui->tab_pitch->getPID());
            cc->mutable_yaw_rate_pid()->CopyFrom(ui->tab_yaw_rate->getPID());
            if (ui->inclinaison_max_enabled->isChecked()) {
                cc->set_max_inclinaison(ui->inclinaison_max->text().toFloat());
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

            CommandPacket::SensorsConfig *sc = config.mutable_sensors_config();
            sc->set_accel_lowpass_constant(ui->accel_low_pass->text().toFloat());

            int size = config.ByteSize();
            char buffer[size];
            assert(config.SerializeToArray(buffer, sizeof(buffer)));
            udpSocket.writeDatagram(buffer, size, birdAddress, PORT);
        } else {
            int size = emergency_config.ByteSize();
            char buffer[size];
            assert(emergency_config.SerializeToArray(buffer, sizeof(buffer)));
            udpSocket.writeDatagram(buffer, size, birdAddress, PORT);
        }

    } else if (id == command_timer_id) {
        // Compute the new Joystick command (integrator for the altitude)
        {
            Attitude *attitude = command.mutable_command();
            float altitude = attitude->altitude() + (joy_rt - joy_lt) / 100;
            if (altitude > 1.f) {
                altitude = 1.f;
            } else if (altitude < 0.f) {
                altitude = 0.f;
            }
            ui->tab_altitude->setCommand(altitude);
            attitude->set_altitude(altitude);
        }
        // Send a command packet
        int size = command.ByteSize();
        char buffer[size];
        assert(command.SerializeToArray(buffer, sizeof(buffer)));
        udpSocket.writeDatagram(buffer, size, birdAddress, PORT);
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
        ui->tab_roll->setCommand(value * .25);
        attitude->set_roll(value * .25);
        break;
    case 1:
        ui->tab_pitch->setCommand(value * .25);
        attitude->set_pitch(value * .25);
        break;
    case 3:
        ui->tab_yaw_rate->setCommand(value);
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
    }
    // Update UI
    ui->tab_altitude->setPID(config.controller_config().altitude_pid());
    ui->tab_roll->setPID(config.controller_config().roll_pid());
    ui->tab_pitch->setPID(config.controller_config().pitch_pid());
    ui->tab_yaw_rate->setPID(config.controller_config().yaw_rate_pid());
    if (config.controller_config().has_max_altitude()) {
        ui->altitude_max_enabled->setChecked(true);
        ui->altitude_max->setText(QString::number(config.controller_config().max_altitude()));
    } else {
        ui->altitude_max_enabled->setChecked(false);
        ui->altitude_max->setText("");
    }
    if (config.controller_config().has_max_inclinaison()) {
        ui->inclinaison_max_enabled->setChecked(true);
        ui->inclinaison_max->setText(QString::number(config.controller_config().max_inclinaison()));
    } else {
        ui->inclinaison_max_enabled->setChecked(false);
        ui->inclinaison_max->setText("");
    }
    if (config.controller_config().has_max_yaw_rate()) {
        ui->yaw_rate_max_enabled->setChecked(true);
        ui->yaw_rate_max->setText(QString::number(config.controller_config().max_yaw_rate()));
    } else {
        ui->yaw_rate_max_enabled->setChecked(false);
        ui->yaw_rate_max->setText("");
    }
    if (config.has_sensors_config()) {
        ui->accel_low_pass->setText(QString::number(config.sensors_config().accel_lowpass_constant()));
    }
}

void MainWindow::saveConfig()
{
    QSettings settings;
    QByteArray buffer(config.ByteSize(), 0);
    config.SerializeToArray(buffer.data(), buffer.size());
    settings.setValue("config", QVariant(buffer));
}

void MainWindow::onTabChanged(int tab)
{
    ui->tab_altitude->pause(tab != 0);
    ui->tab_roll->pause(tab != 1);
    ui->tab_pitch->pause(tab != 2);
    ui->tab_yaw_rate->pause(tab != 3);
}
