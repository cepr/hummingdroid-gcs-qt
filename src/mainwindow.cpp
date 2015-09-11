#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QSettings>
#include <qmath.h>

#define COMMAND_PORT 49152
#define TELEM_PORT 49153

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    locale(QLocale::system()),
    midi_mixer("/dev/snd/midiC1D0"),
    command_timer_id(-1),
    config_timer_id(-1),
    emergency(false),
    calibrating(0),
    calibration_finished_timer_id(-1),
    sum_gyro_yaw(0.),
    sum_accel_roll(0.),
    sum_accel_pitch(0.),
    calibration_measures(0),
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

    ui->tab_altitude->setRange(-0.1, 1.1);
    ui->tab_altitude->setVerticalGrid(.1);
    ui->tab_pitch->setRange(-3.15, 3.15);
    ui->tab_pitch->setVerticalGrid(3.14 * 45 / 180);
    ui->tab_roll->setRange(-3.15, 3.15);
    ui->tab_roll->setVerticalGrid(3.14 * 45 / 180);
    ui->tab_yaw_rate->setRange(-5, 5);
    ui->tab_yaw_rate->setVerticalGrid(1.);

    restoreConfig();
    udpSocket.bind(TELEM_PORT);
    //joystick.open();
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
    connect(ui->calibrate_gyro_bias, SIGNAL(clicked()), this, SLOT(onCalibrateGyroBias()));
    connect(ui->calibrate_gyro_gain, SIGNAL(clicked()), this, SLOT(onCalibrateGyroGain()));
    connect(ui->calibrate_accel, SIGNAL(clicked()), this, SLOT(onCalibrateAccel()));
    connect(&midi_mixer, SIGNAL(onSliderValueChanged(int,float)), this, SLOT(setSliderValue(int,float)));

    // Resolve the bird host name
    QHostInfo::lookupHost("192.168.1.63", this, SLOT(onHostnameAddressResolved(QHostInfo)));

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
        tc->mutable_host()->assign("cpriscal-UX32VD.local");
        tc->set_port(TELEM_PORT);
        tc->set_commandenabled(true);
        tc->set_attitudeenabled(true);
        tc->set_controlenabled(true);
        tc->set_switchesenabled(false);

        CommandPacket::MotorsConfig *mc =
                emergency_config.mutable_motors_config();
        mc->set_max_pwm(0.);
        mc->set_min_pwm(0.);
    }

    // Functional config
    {
        CommandPacket::TelemetryConfig *tc =
                config.mutable_telemetry_config();
        tc->mutable_host()->assign("cpriscal-UX32VD.local");
        tc->set_port(TELEM_PORT);
        tc->set_commandenabled(true);
        tc->set_attitudeenabled(true);
        tc->set_controlenabled(true);
        tc->set_switchesenabled(false);
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
    latest_telem = telemetry;
    if (telemetry.has_attitude()) {
        const Attitude & attitude = telemetry.attitude();
        if (calibrating) {
            if (!calibration_measures) {
                first_telem = telemetry;
            }
            sum_gyro_yaw += attitude.yaw_rate();
            sum_accel_roll += attitude.roll();
            sum_accel_pitch += attitude.pitch();
            calibration_measures++;
        }
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

void MainWindow::sendConfig()
{
    if (!emergency) {
        CommandPacket::ControllerConfig *cc =
                config.mutable_controller_config();
        cc->mutable_altitude_pid()->CopyFrom(ui->tab_altitude->getPID());
        cc->mutable_roll_pid()->CopyFrom(ui->tab_roll->getPID());
        cc->mutable_pitch_pid()->CopyFrom(ui->tab_pitch->getPID());
        cc->mutable_yaw_rate_pid()->CopyFrom(ui->tab_yaw_rate->getPID());

        // Security
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

        // Sensor settings
        CommandPacket::SensorsConfig *sc = config.mutable_sensors_config();
        if (!calibrating) {
            sc->set_accel_lowpass_constant(ui->accel_low_pass->text().toFloat());
        }
        sc->set_gyro_roll_bias(ui->gyro_roll_bias->text().toFloat());
        sc->set_gyro_roll_gain(ui->gyro_roll_gain->text().toFloat());
        sc->set_gyro_pitch_bias(ui->gyro_pitch_bias->text().toFloat());
        sc->set_gyro_pitch_gain(ui->gyro_pitch_gain->text().toFloat());
        sc->set_gyro_yaw_bias(ui->gyro_yaw_bias->text().toFloat());
        sc->set_gyro_yaw_gain(ui->gyro_yaw_gain->text().toFloat());
        sc->set_accel_roll_bias(ui->accel_roll_bias->text().toFloat());
        sc->set_accel_pitch_bias(ui->accel_pitch_bias->text().toFloat());
        sc->set_apply_modulo(ui->apply_modulo->isChecked());

        // Motors settings
        CommandPacket::MotorsConfig *mc = config.mutable_motors_config();
        mc->set_min_pwm(ui->min_pwm->text().toFloat());
        mc->set_max_pwm(ui->max_pwm->text().toFloat());

        int size = config.ByteSize();
        char buffer[size];
        assert(config.SerializeToArray(buffer, sizeof(buffer)));
        udpSocket.writeDatagram(buffer, size, birdAddress, COMMAND_PORT);
    } else {
        int size = emergency_config.ByteSize();
        char buffer[size];
        assert(emergency_config.SerializeToArray(buffer, sizeof(buffer)));
        udpSocket.writeDatagram(buffer, size, birdAddress, COMMAND_PORT);
    }

}

void MainWindow::timerEvent(QTimerEvent *event)
{
    int id = event->timerId();

    if (id == config_timer_id || emergency) {
        // Send a config packet
        sendConfig();
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
            ui->tab_pitch->setCommand(attitude->pitch());
            ui->tab_roll->setCommand(attitude->roll());
            ui->tab_yaw_rate->setCommand(attitude->yaw_rate());
        }
        // Send a command packet
        int size = command.ByteSize();
        char buffer[size];
        assert(command.SerializeToArray(buffer, sizeof(buffer)));
        udpSocket.writeDatagram(buffer, size, birdAddress, COMMAND_PORT);
        // Refresh the GUI
        ui->tabWidget->currentWidget()->update();
    } else if (id == calibration_finished_timer_id) {
        killTimer(calibration_finished_timer_id);
        calibration_finished_timer_id = -1;
        CommandPacket::SensorsConfig* sc = config.mutable_sensors_config();
        if (calibrating == 1) {
            // gyro
            sc->set_gyro_roll_bias(
                        (latest_telem.attitude().roll() - first_telem.attitude().roll()) /
                        (latest_telem.attitude().timestamp() - first_telem.attitude().timestamp()));
            sc->set_gyro_pitch_bias(
                        (latest_telem.attitude().pitch() - first_telem.attitude().pitch()) /
                        (latest_telem.attitude().timestamp() - first_telem.attitude().timestamp()));
            sc->set_gyro_yaw_bias(sum_gyro_yaw / calibration_measures);
            ui->gyro_roll_bias->setText(QString::number(sc->gyro_roll_bias()));
            ui->gyro_pitch_bias->setText(QString::number(sc->gyro_pitch_bias()));
            ui->gyro_yaw_bias->setText(QString::number(sc->gyro_yaw_bias()));
        } else if (calibrating == 2) {
            // accel
            sc->set_accel_roll_bias(sum_accel_roll / calibration_measures);
            sc->set_accel_pitch_bias(sum_accel_pitch / calibration_measures);
            ui->accel_roll_bias->setText(QString::number(sc->accel_roll_bias()));
            ui->accel_pitch_bias->setText(QString::number(sc->accel_pitch_bias()));
        }
        sc->set_accel_lowpass_constant(ui->accel_low_pass->text().toFloat()); // restore the accelerometer
        calibrating = 0;
        ui->calibrate_gyro_bias->setEnabled(true);
        ui->calibrate_gyro_gain->setEnabled(true);
        ui->calibrate_accel->setEnabled(true);
        ui->accel_low_pass->setEnabled(true);
    }
}

void MainWindow::onHostnameAddressResolved(QHostInfo info)
{
    if (info.addresses().length() > 0) {
        birdAddress = info.addresses().at(0);
    }
    // TODO else: retry to resolve later
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
    ui->tabWidget->currentWidget()->update();
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
        ui->gyro_roll_bias->setText(QString::number(config.sensors_config().gyro_roll_bias()));
        ui->gyro_pitch_bias->setText(QString::number(config.sensors_config().gyro_pitch_bias()));
        ui->gyro_yaw_bias->setText(QString::number(config.sensors_config().gyro_yaw_bias()));
        ui->gyro_roll_gain->setText(QString::number(config.sensors_config().gyro_roll_gain()));
        ui->gyro_pitch_gain->setText(QString::number(config.sensors_config().gyro_pitch_gain()));
        ui->gyro_yaw_gain->setText(QString::number(config.sensors_config().gyro_yaw_gain()));
        ui->accel_roll_bias->setText(QString::number(config.sensors_config().accel_roll_bias()));
        ui->accel_pitch_bias->setText(QString::number(config.sensors_config().accel_pitch_bias()));
        ui->apply_modulo->setChecked(config.sensors_config().apply_modulo());
    }
    if (config.has_motors_config()) {
        ui->min_pwm->setText(QString::number(config.motors_config().min_pwm()));
        ui->max_pwm->setText(QString::number(config.motors_config().max_pwm()));
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

void MainWindow::onCalibrateGyroBias()
{
    ui->calibrate_gyro_bias->setEnabled(false);
    ui->calibrate_gyro_gain->setEnabled(false);
    ui->calibrate_accel->setEnabled(false);
    sum_gyro_yaw = 0.;
    calibration_measures = 0;
    config.mutable_sensors_config()->set_accel_lowpass_constant(1.e9); // we use a huge value to remove the accelerometer correction
    ui->gyro_roll_bias->setText("0");
    ui->gyro_pitch_bias->setText("0");
    ui->gyro_yaw_bias->setText("0");
    ui->accel_low_pass->setEnabled(false);
    calibrating = 1;
    sendConfig();
    calibration_finished_timer_id = startTimer(10000);
}

void MainWindow::onCalibrateGyroGain()
{
    // The user must rotate the vehicule one full positive rotation on the roll and on the pitch axis
    ui->gyro_roll_gain->setText(QString::number(M_PI * 2 / latest_telem.attitude().roll()));
    ui->gyro_pitch_gain->setText(QString::number(M_PI * 2 / latest_telem.attitude().pitch()));
}

void MainWindow::onCalibrateAccel()
{
    ui->calibrate_gyro_bias->setEnabled(false);
    ui->calibrate_gyro_gain->setEnabled(false);
    ui->calibrate_accel->setEnabled(false);
    sum_accel_roll = 0.;
    sum_accel_pitch = 0.;
    calibration_measures = 0;
    config.mutable_sensors_config()->set_accel_lowpass_constant(0); // do not use the gyro
    ui->accel_roll_bias->setText("0");
    ui->accel_pitch_bias->setText("0");
    ui->accel_low_pass->setEnabled(false);
    calibrating = 2;
    sendConfig();
    calibration_finished_timer_id = startTimer(10000);
}

void MainWindow::setSliderValue(int index, float value)
{
    switch(index) {
    case 1:
    {
        Attitude *attitude = command.mutable_command();
        attitude->set_altitude(value);
        break;
    }
    default:
        // ignore
        ;
    }
}
