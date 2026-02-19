#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <cerrno>

#include "telemetry-2026/EKF.capnp"
#include "telemetry-2026/PayloadTelemetryPacket.capnp"
#include "telemetry-2026/RemoteControl.capnp"
#include "telemetry-2026/Rocket2StageTelemetryPacket.capnp"
#include "telemetry-2026/Rocket30KTelemetryPacket.capnp"
#include "telemetry-2026/RocketCanardsTelemetryPacket.capnp"
#include "telemetry-2026/Sensors.capnp"
#include "telemetry-2026/Shared.capnp"

static int openFile(const char* path) {
    int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd < 0) {
        std::cerr << "open failed: " << strerror(errno) << "\n";
    }
    return fd;
}

static void resetFd(int fd) {
    lseek(fd, 0, SEEK_SET);
}

//////////////////////////////////////////////////////////////
// COMMON FILLERS (Shared, Sensors, EKF)
//////////////////////////////////////////////////////////////

void fillShared(Shared::Builder shared) {
    shared.setCallsign("ROCKET_ALPHA");
    shared.setTimeFromBoot(1000);
    shared.setLoopCount(10);
    shared.setSdFileNo(1);
    shared.setBatteryVoltage(12.2f);
}

void fillSensors(Sensors::Builder s) {
    s.setAccX(0.01f);
    s.setAccY(0.02f);
    s.setAccZ(0.98f);

    s.setHighAccX(0.1f);
    s.setHighAccY(0.2f);
    s.setHighAccZ(0.3f);

    s.setGyroX(0.001f);
    s.setGyroY(0.002f);
    s.setGyroZ(0.003f);

    s.setMagX(10.0f);
    s.setMagY(20.0f);
    s.setMagZ(30.0f);

    s.setPressure(101325.0f);
    s.setTemperature(22.0f);
}

void fillEKF(EKF::Builder ekf) {
    ekf.setW(1.0f);
    ekf.setI(0.0f);
    ekf.setJ(0.0f);
    ekf.setK(0.0f);

    ekf.setPosX(100.0f);
    ekf.setPosY(200.0f);
    ekf.setPosZ(300.0f);

    ekf.setVelX(1.0f);
    ekf.setVelY(0.0f);
    ekf.setVelZ(-1.0f);
}

//////////////////////////////////////////////////////////////
// 1. PayloadTelemetryPacket
//////////////////////////////////////////////////////////////

void writePayload(int fd) {
    ::capnp::MallocMessageBuilder message;
    auto pkt = message.initRoot<PayloadTelemetryPacket>();

    fillShared(pkt.initShared());
    pkt.setState(States::Boost);
    fillSensors(pkt.initSensorValues());
    fillEKF(pkt.initEkfValues());

    pkt.initSelfRighting1Servo().setCommanded(0.5f);
    pkt.initSelfRighting2Servo().setCommanded(0.6f);
    pkt.initLatchServo().setCommanded(0.0f);
    pkt.initAntennaServo().setCommanded(1.0f);

    pkt.initMotorLeft().setCommanded(0.8f);
    pkt.initMotorRight().setCommanded(0.9f);
    pkt.setMotorLeftTemp(55);
    pkt.setMotorRightTemp(56);
    pkt.setMotorLeftCurrent(200);
    pkt.setMotorRightCurrent(210);

    writePackedMessageToFd(fd, message);
}

//////////////////////////////////////////////////////////////
// 2. Rocket2StageTelemetryPacket
//////////////////////////////////////////////////////////////

void writeRocket2Stage(int fd) {
    ::capnp::MallocMessageBuilder message;
    auto pkt = message.initRoot<Rocket2StageTelemetryPacket>();

    fillShared(pkt.initShared());
    pkt.setState(States::Stage1Boost);
    fillSensors(pkt.initSensorValues());
    fillEKF(pkt.initEkfValues());

    auto airbrakes = pkt.initAirbrakes();
    airbrakes.setCommanded(0.7f);
    airbrakes.setActual(0.65f);

    writePackedMessageToFd(fd, message);
}

//////////////////////////////////////////////////////////////
// 3. Rocket30KTelemetryPacket
//////////////////////////////////////////////////////////////

void writeRocket30K(int fd) {
    ::capnp::MallocMessageBuilder message;
    auto pkt = message.initRoot<Rocket30KTelemetryPacket>();

    fillShared(pkt.initShared());
    pkt.setState(States::Boost);
    fillSensors(pkt.initSensorValues());
    fillEKF(pkt.initEkfValues());

    writePackedMessageToFd(fd, message);
}

//////////////////////////////////////////////////////////////
// 4. RocketCanardsTelemetryPacket
//////////////////////////////////////////////////////////////

void writeRocketCanards(int fd) {
    ::capnp::MallocMessageBuilder message;
    auto pkt = message.initRoot<RocketCanardsTelemetryPacket>();

    fillShared(pkt.initShared());
    pkt.setState(States::Canards);
    fillSensors(pkt.initSensorValues());
    fillEKF(pkt.initEkfValues());

    auto cov = pkt.initCovarianceDiagonal(3);
    cov.set(0, 0.1f);
    cov.set(1, 0.2f);
    cov.set(2, 0.3f);

    pkt.initCanard1().setCommanded(0.2f);
    pkt.getCanard1().setActual(0.18f);

    pkt.initCanard2().setCommanded(0.3f);
    pkt.getCanard2().setActual(0.29f);

    pkt.initCanard3().setCommanded(0.4f);
    pkt.getCanard3().setActual(0.39f);

    pkt.initCanard4().setCommanded(0.5f);
    pkt.getCanard4().setActual(0.48f);

    writePackedMessageToFd(fd, message);
}

//////////////////////////////////////////////////////////////
// 5. RemoteControlCommand
//////////////////////////////////////////////////////////////

void writeRemoteCommand(int fd) {
    ::capnp::MallocMessageBuilder message;
    auto cmd = message.initRoot<RemoteControlCommand>();

    cmd.setCallsign("GROUND_STATION");
    cmd.setCommandNumber(42);
    cmd.setCommand(RemoteControlCommand::Command::ArmFlight);

    writePackedMessageToFd(fd, message);
}

//////////////////////////////////////////////////////////////
// MAIN
//////////////////////////////////////////////////////////////

int main() {
    int fd;

    fd = openFile("/tmp/payload.bin");
    writePayload(fd);
    close(fd);

    fd = openFile("/tmp/rocket2stage.bin");
    writeRocket2Stage(fd);
    close(fd);

    fd = openFile("/tmp/rocket30k.bin");
    writeRocket30K(fd);
    close(fd);

    fd = openFile("/tmp/canards.bin");
    writeRocketCanards(fd);
    close(fd);

    fd = openFile("/tmp/remote_command.bin");
    writeRemoteCommand(fd);
    close(fd);

    std::cout << "All packets serialized successfully.\n";
    return 0;
}
