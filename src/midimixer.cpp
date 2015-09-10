
#include "midimixer.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <QSocketNotifier>

MidiMixer::MidiMixer(const char* device, QObject *parent) :
    QObject(parent)
{
    dev_fd = ::open(device, O_RDONLY);
    Q_ASSERT(dev_fd >= 0);
    QSocketNotifier *notifier = new QSocketNotifier(dev_fd, QSocketNotifier::Read, this);
    connect(notifier, SIGNAL(activated(int)), SLOT(onNewMidiEvent()));
}

void MidiMixer::onNewMidiEvent()
{
    // Called when new data can be read on the MIDI device
    unsigned char buffer[3];
    if (::read(dev_fd, buffer, 3) != 3) {
        return;
    }
    if (buffer[0] != 0xb7) {
        return;
    }
    if (buffer[1] > 0x20 && buffer[1] < 0x2a) {
        emit onSliderValueChanged(buffer[1] - 0x20, (float)buffer[2] / 127.);
    } else if (buffer[1] > 0x10 && buffer[1] < 0x1a) {
        emit onPotarValueChanged(buffer[1] - 0x10, (float)buffer[2] / 127.);
    } else {
        emit onButtonStateChanged(buffer[1], (bool)buffer[2]);
    }
}
