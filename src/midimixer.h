#ifndef MIDIMIXER_H
#define MIDIMIXER_H

#include <QObject>

class MidiMixer : public QObject
{
    Q_OBJECT
public:
    explicit MidiMixer(const char *device, QObject *parent = 0);

signals:
    void onSliderValueChanged(int index, float value);
    void onPotarValueChanged(int index, float value);
    void onButtonStateChanged(int index, bool clicked);

public slots:
    void onNewMidiEvent();

private:
    int dev_fd;
};

#endif // MIDIMIXER_H
