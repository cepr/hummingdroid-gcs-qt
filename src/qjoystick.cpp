#include "qjoystick.h"
#include <QFile>
#include <linux/joystick.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <QDateTime>

QJoystick::QJoystick(QObject *parent, int joystick_number) :
    QThread(parent), joystick_number(joystick_number)
{
}

bool QJoystick::open()
{
    QFile dev;

    // Search for the device
    dev.setFileName(QString("/dev/js%1").arg(joystick_number));
    if (!dev.exists()) {
        dev.setFileName(QString("/dev/input/js%1").arg(joystick_number));
        if (!dev.exists()) {
            return false;
        }
    }

    // Open the joystick
    fd = ::open(dev.fileName().toStdString().c_str(), O_RDONLY);
    if (fd == -1) {
        return false;
    }

    // Listen to joystick events
    start();

    return true;
}

void QJoystick::run()
{
    while(true) {
        struct js_event event;
        if (::read(fd, &event, sizeof(js_event)) != sizeof(js_event)) {
            // Problem while reading the file
            return;
        }
        qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
        if (event.type & JS_EVENT_BUTTON) {
            if (event.value) {
                emit onJoyButtonPressed(timestamp, event.number);
            } else {
                emit onJoyButtonReleased(timestamp, event.number);
            }
        } else if (event.type & JS_EVENT_AXIS) {
            emit onJoyAxisChanged(timestamp, event.number, (float)event.value / 32768);
        }
    }
}
