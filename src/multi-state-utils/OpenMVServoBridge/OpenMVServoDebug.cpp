#include "OpenMVServoDebug.h"

#include "../../config.h"
#include "../ScrewDrive/ScrewDriveInterface.h"

static constexpr uint16_t DEBUG_MIN_PULSE_US = 1000;
static constexpr uint16_t DEBUG_MAX_PULSE_US = 2000;
static constexpr uint32_t DEBUG_MIN_CAMERA_BAUD = 9600;
static constexpr uint32_t DEBUG_MAX_CAMERA_BAUD = 2000000;
static uint32_t currentCameraBaud = CAMERA_SERIAL_BAUD;

static bool parsePulsePair(const String& input, uint16_t& leftPulseUs, uint16_t& rightPulseUs) {
    String args = input.substring(String("openmv_servo").length());
    args.trim();

    int separator = args.indexOf(' ');
    if (separator < 0) {
        return false;
    }

    String leftToken = args.substring(0, separator);
    String rightToken = args.substring(separator + 1);
    leftToken.trim();
    rightToken.trim();

    int leftPulse = leftToken.toInt();
    int rightPulse = rightToken.toInt();

    if (leftPulse < DEBUG_MIN_PULSE_US || leftPulse > DEBUG_MAX_PULSE_US ||
        rightPulse < DEBUG_MIN_PULSE_US || rightPulse > DEBUG_MAX_PULSE_US) {
        return false;
    }

    leftPulseUs = static_cast<uint16_t>(leftPulse);
    rightPulseUs = static_cast<uint16_t>(rightPulse);
    return true;
}

static void printOpenMVServoDebugTx(Stream& debugOutput, const char* command) {
    debugOutput.print("DBG_OPENMV_SERVO_TX ");
    debugOutput.println(command);
}

static bool parseCameraBaud(const String& input, uint32_t& baud) {
    String args = input.substring(String("openmv_baud").length());
    args.trim();

    if (args.length() == 0) {
        return false;
    }

    long parsedBaud = args.toInt();
    if (parsedBaud < static_cast<long>(DEBUG_MIN_CAMERA_BAUD) ||
        parsedBaud > static_cast<long>(DEBUG_MAX_CAMERA_BAUD)) {
        return false;
    }

    baud = static_cast<uint32_t>(parsedBaud);
    return true;
}

static void setCameraSerialBaud(HardwareSerial& cameraSerial, uint32_t baud, Stream& debugOutput) {
    cameraSerial.flush();
    cameraSerial.end();
    delay(10);
    cameraSerial.begin(baud);
    currentCameraBaud = baud;

    debugOutput.print("DBG_OPENMV_BAUD_SET baud=");
    debugOutput.println(currentCameraBaud);
}

bool handleOpenMVServoDebugCommand(const String& input,
                                   HardwareSerial& cameraSerial,
                                   ScrewDriveInterface& screwDrive,
                                   Stream& debugOutput) {
#if !ENABLE_OPENMV_SERVO_UART_DEBUG
    (void)input;
    (void)cameraSerial;
    (void)screwDrive;
    (void)debugOutput;
    return false;
#else
    if (input.length() == 0) {
        return false;
    }

    if (input == "openmv_ping" || input.startsWith("openmv_ping ")) {
        String token = "";
        if (input.length() > String("openmv_ping").length()) {
            token = input.substring(String("openmv_ping").length());
            token.trim();
        }

        if (token.length() == 0) {
            token = String(millis());
        }

        cameraSerial.print("PING_SERVO ");
        cameraSerial.println(token);

        debugOutput.print("DBG_OPENMV_SERVO_TX PING_SERVO token=");
        debugOutput.println(token);
        return true;
    }

    if (input.startsWith("openmv_servo")) {
        uint16_t leftPulseUs = 0;
        uint16_t rightPulseUs = 0;

        if (!parsePulsePair(input, leftPulseUs, rightPulseUs)) {
            debugOutput.println("DBG_OPENMV_SERVO_TX_ERR reason=bad_openmv_servo expected=openmv_servo_<leftUs>_<rightUs>");
            return true;
        }

        cameraSerial.print("SERVO L ");
        cameraSerial.print(leftPulseUs);
        cameraSerial.print(" R ");
        cameraSerial.println(rightPulseUs);

        debugOutput.print("DBG_OPENMV_SERVO_TX SERVO L=");
        debugOutput.print(leftPulseUs);
        debugOutput.print(" R=");
        debugOutput.println(rightPulseUs);
        return true;
    }

    if (input == "openmv_get_servo") {
        cameraSerial.println("GET_SERVO");
        printOpenMVServoDebugTx(debugOutput, "GET_SERVO");

        debugOutput.print("DBG_OPENMV_SERVO_LOCAL leftPulseUs=");
        debugOutput.print(screwDrive.getLastLeftPulseUs());
        debugOutput.print(" rightPulseUs=");
        debugOutput.print(screwDrive.getLastRightPulseUs());
        debugOutput.print(" backend=");
        debugOutput.println(screwDrive.isUsingOpenMVUartOutput() ? "openmv_uart" : "direct_servo");
        return true;
    }

    if (input.startsWith("openmv_baud")) {
        uint32_t baud = 0;
        if (!parseCameraBaud(input, baud)) {
            debugOutput.print("DBG_OPENMV_BAUD_ERR reason=bad_baud min=");
            debugOutput.print(DEBUG_MIN_CAMERA_BAUD);
            debugOutput.print(" max=");
            debugOutput.print(DEBUG_MAX_CAMERA_BAUD);
            debugOutput.println(" expected=openmv_baud_<baud>");
            return true;
        }

        setCameraSerialBaud(cameraSerial, baud, debugOutput);
        return true;
    }

    if (input == "openmv_get_baud") {
        debugOutput.print("DBG_OPENMV_BAUD baud=");
        debugOutput.println(currentCameraBaud);
        return true;
    }

    return false;
#endif
}
