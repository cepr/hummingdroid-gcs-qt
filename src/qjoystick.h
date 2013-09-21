#ifndef QJOYSTICK_H
#define QJOYSTICK_H

#include <QFile>
#include <QThread>

class QJoystick : public QThread
{
    Q_OBJECT
public:
    explicit QJoystick(QObject *parent = 0, int joystick_number = 0);
    bool open();
    void run();

signals:
    void onJoyAxisChanged(qint64 timestamp, int axis, float value);
    void onJoyButtonPressed(qint64 timestamp, int button);
    void onJoyButtonReleased(qint64 timestamp, int button);

private:
    int joystick_number;
    int fd;
};

#endif // QJOYSTICK_H
