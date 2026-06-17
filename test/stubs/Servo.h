#pragma once

class Servo {
public:
    void attach(int pin)
    {
        attachedPin = pin;
        attached = true;
    }

    void detach()
    {
        attached = false;
    }

    void writeMicroseconds(int pulseUs)
    {
        lastPulseUs = pulseUs;
    }

    bool attached = false;
    int attachedPin = -1;
    int lastPulseUs = 1500;
};
